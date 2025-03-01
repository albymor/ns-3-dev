/*
 * Copyright (c) 2021 DERONNE SOFTWARE ENGINEERING
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Sébastien Deronne <sebastien.deronne@gmail.com>
 */

#include "eht-ppdu.h"

#include "eht-phy.h"

#include "ns3/log.h"
#include "ns3/wifi-phy-operating-channel.h"
#include "ns3/wifi-psdu.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("EhtPpdu");

EhtPpdu::EhtPpdu(const WifiConstPsduMap& psdus,
                 const WifiTxVector& txVector,
                 const WifiPhyOperatingChannel& channel,
                 Time ppduDuration,
                 uint64_t uid,
                 TxPsdFlag flag)
    : HePpdu(psdus, txVector, channel, ppduDuration, uid, flag)
{
    NS_LOG_FUNCTION(this << psdus << txVector << channel << ppduDuration << uid << flag);

    // For EHT SU transmissions (carried in EHT MU PPDUs), we have to create a HE SU PHY header
    // because this is not done by the parent class.
    // This is a workaround needed until we properly implement 11be PHY headers.
    if (!txVector.IsMu())
    {
        m_heSig.emplace<HeSuSigHeader>(HeSuSigHeader{
            .m_bssColor = txVector.GetBssColor(),
            .m_mcs = txVector.GetMode().GetMcsValue(),
            .m_bandwidth = GetChannelWidthEncodingFromMhz(txVector.GetChannelWidth()),
            .m_giLtfSize = GetGuardIntervalAndNltfEncoding(txVector.GetGuardInterval(),
                                                           2 /*NLTF currently unused*/),
            .m_nsts = GetNstsEncodingFromNss(txVector.GetNss())});
    }
    m_ehtPpduType = txVector.GetEhtPpduType();
}

WifiPpduType
EhtPpdu::GetType() const
{
    if (m_psdus.count(SU_STA_ID) > 0)
    {
        return WIFI_PPDU_TYPE_SU;
    }
    switch (m_preamble)
    {
    case WIFI_PREAMBLE_EHT_MU:
        return WIFI_PPDU_TYPE_DL_MU;
    case WIFI_PREAMBLE_EHT_TB:
        return WIFI_PPDU_TYPE_UL_MU;
    default:
        NS_ASSERT_MSG(false, "invalid preamble " << m_preamble);
        return WIFI_PPDU_TYPE_SU;
    }
}

bool
EhtPpdu::IsDlMu() const
{
    return (m_preamble == WIFI_PREAMBLE_EHT_MU) && (m_psdus.count(SU_STA_ID) == 0);
}

bool
EhtPpdu::IsUlMu() const
{
    return (m_preamble == WIFI_PREAMBLE_EHT_TB) && (m_psdus.count(SU_STA_ID) == 0);
}

void
EhtPpdu::SetTxVectorFromPhyHeaders(WifiTxVector& txVector) const
{
    txVector.SetLength(m_lSig.GetLength());
    txVector.SetAggregation(m_psdus.size() > 1 || m_psdus.begin()->second->IsAggregate());
    txVector.SetEhtPpduType(m_ehtPpduType); // FIXME: PPDU type should be read from U-SIG
    if (!txVector.IsMu())
    {
        auto heSigHeader = std::get_if<HeSuSigHeader>(&m_heSig);
        NS_ASSERT(heSigHeader);
        txVector.SetMode(EhtPhy::GetEhtMcs(heSigHeader->m_mcs));
        txVector.SetNss(GetNssFromNstsEncoding(heSigHeader->m_nsts));
        txVector.SetChannelWidth(GetChannelWidthMhzFromEncoding(heSigHeader->m_bandwidth));
        txVector.SetGuardInterval(GetGuardIntervalFromEncoding(heSigHeader->m_giLtfSize));
        txVector.SetBssColor(heSigHeader->m_bssColor);
    }
    else if (ns3::IsUlMu(m_preamble))
    {
        auto heSigHeader = std::get_if<HeTbSigHeader>(&m_heSig);
        NS_ASSERT(heSigHeader);
        txVector.SetChannelWidth(GetChannelWidthMhzFromEncoding(heSigHeader->m_bandwidth));
        txVector.SetBssColor(heSigHeader->m_bssColor);
    }
    else if (ns3::IsDlMu(m_preamble))
    {
        auto heSigHeader = std::get_if<HeMuSigHeader>(&m_heSig);
        NS_ASSERT(heSigHeader);
        txVector.SetChannelWidth(GetChannelWidthMhzFromEncoding(heSigHeader->m_bandwidth));
        txVector.SetGuardInterval(GetGuardIntervalFromEncoding(heSigHeader->m_giLtfSize));
        txVector.SetBssColor(heSigHeader->m_bssColor);
        txVector.SetSigBMode(HePhy::GetVhtMcs(heSigHeader->m_sigBMcs));
        const auto p20Index = m_operatingChannel.GetPrimaryChannelIndex(20);
        txVector.SetRuAllocation(heSigHeader->m_ruAllocation, p20Index);
    }
    if (txVector.IsDlMu())
    {
        auto heSigHeader = std::get_if<HeMuSigHeader>(&m_heSig);
        NS_ASSERT(heSigHeader);
        SetHeMuUserInfos(txVector, heSigHeader->m_ruAllocation, heSigHeader->m_contentChannels);
    }
}

std::pair<std::size_t, std::size_t>
EhtPpdu::GetNumRusPerEhtSigBContentChannel(uint16_t channelWidth,
                                           uint8_t ehtPpduType,
                                           const RuAllocation& ruAllocation)
{
    if (ehtPpduType == 1)
    {
        return {1, 0};
    }
    return HePpdu::GetNumRusPerHeSigBContentChannel(channelWidth, ruAllocation);
}

uint32_t
EhtPpdu::GetEhtSigFieldSize(uint16_t channelWidth,
                            const RuAllocation& ruAllocation,
                            uint8_t ehtPpduType)
{
    // FIXME: EHT-SIG is not implemented yet, hence this is a copy of HE-SIG-B
    auto commonFieldSize = 4 /* CRC */ + 6 /* tail */;
    if (channelWidth <= 40)
    {
        commonFieldSize += 8; // only one allocation subfield
    }
    else
    {
        commonFieldSize +=
            8 * (channelWidth / 40) /* one allocation field per 40 MHz */ + 1 /* center RU */;
    }

    auto numStaPerContentChannel =
        GetNumRusPerEhtSigBContentChannel(channelWidth, ehtPpduType, ruAllocation);
    auto maxNumStaPerContentChannel =
        std::max(numStaPerContentChannel.first, numStaPerContentChannel.second);
    auto maxNumUserBlockFields = maxNumStaPerContentChannel /
                                 2; // handle last user block with single user, if any, further down
    std::size_t userSpecificFieldSize =
        maxNumUserBlockFields * (2 * 21 /* user fields (2 users) */ + 4 /* tail */ + 6 /* CRC */);
    if (maxNumStaPerContentChannel % 2 != 0)
    {
        userSpecificFieldSize += 21 /* last user field */ + 4 /* CRC */ + 6 /* tail */;
    }

    return commonFieldSize + userSpecificFieldSize;
}

Ptr<WifiPpdu>
EhtPpdu::Copy() const
{
    return Ptr<WifiPpdu>(new EhtPpdu(*this), false);
}

} // namespace ns3
