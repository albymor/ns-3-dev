/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright 2007 University of Washington
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
 */
#include "ns3/log.h"
#include "ns3/ipv4-address.h"
#include "ns3/ipv6-address.h"
#include "ns3/nstime.h"
#include "ns3/inet-socket-address.h"
#include "ns3/inet6-socket-address.h"
#include "ns3/socket.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/packet.h"
#include "ns3/uinteger.h"
#include "ns3/trace-source-accessor.h"
#include "safety-udp-echo-client.h"
#include "failsafeTag.h"
#include "seqnumTag.h"
#include "ns3/core-module.h"
#include "ns3/timestamp-tag.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("SAFETYUdpEchoClientApplication");

NS_OBJECT_ENSURE_REGISTERED (SAFETYUdpEchoClient);

TypeId
SAFETYUdpEchoClient::GetTypeId (void)
{
  static TypeId tid =
      TypeId ("ns3::SAFETYUdpEchoClient")
          .SetParent<Application> ()
          .SetGroupName ("Applications")
          .AddConstructor<SAFETYUdpEchoClient> ()
          .AddAttribute ("MaxPackets", "The maximum number of packets the application will send",
                         UintegerValue (100), MakeUintegerAccessor (&SAFETYUdpEchoClient::m_count),
                         MakeUintegerChecker<uint32_t> ())
          .AddAttribute ("Interval", "The time to wait between packets", TimeValue (Seconds (1.0)),
                         MakeTimeAccessor (&SAFETYUdpEchoClient::m_interval), MakeTimeChecker ())
          .AddAttribute ("RemoteAddress", "The destination Address of the outbound packets",
                         AddressValue (), MakeAddressAccessor (&SAFETYUdpEchoClient::m_peerAddress),
                         MakeAddressChecker ())
          .AddAttribute ("RemotePort", "The destination port of the outbound packets",
                         UintegerValue (0), MakeUintegerAccessor (&SAFETYUdpEchoClient::m_peerPort),
                         MakeUintegerChecker<uint16_t> ())
          .AddAttribute ("PacketSize", "Size of echo data in outbound packets", UintegerValue (100),
                         MakeUintegerAccessor (&SAFETYUdpEchoClient::SetDataSize,
                                               &SAFETYUdpEchoClient::GetDataSize),
                         MakeUintegerChecker<uint32_t> ())
          .AddAttribute ("IsFailsafeSource", "Set the node as a failsafe source",
                          UintegerValue (0),
                          MakeUintegerAccessor (&SAFETYUdpEchoClient::m_isFailsafeSource),
                          MakeUintegerChecker<uint8_t> ())
          .AddAttribute ("FailSafeTriggerTime", "Time at which Fail safe is enabled", TimeValue (Seconds (3.0)),
                         MakeTimeAccessor (&SAFETYUdpEchoClient::m_FailSafeTriggerTime), MakeTimeChecker ())
          .AddAttribute ("WdtPeriod", "Period of the wdt", TimeValue (MilliSeconds (250.0)),
                         MakeTimeAccessor (&SAFETYUdpEchoClient::m_WdtPeriod), MakeTimeChecker ())
          .AddTraceSource ("Tx", "A new packet is created and is sent",
                           MakeTraceSourceAccessor (&SAFETYUdpEchoClient::m_txTrace),
                           "ns3::Packet::TracedCallback")
          .AddTraceSource ("Rx", "A packet has been received",
                           MakeTraceSourceAccessor (&SAFETYUdpEchoClient::m_rxTrace),
                           "ns3::Packet::TracedCallback")
          .AddTraceSource ("TxWithAddresses", "A new packet is created and is sent",
                           MakeTraceSourceAccessor (&SAFETYUdpEchoClient::m_txTraceWithAddresses),
                           "ns3::Packet::TwoAddressTracedCallback")
          .AddTraceSource ("RxWithAddresses", "A packet has been received",
                           MakeTraceSourceAccessor (&SAFETYUdpEchoClient::m_rxTraceWithAddresses),
                           "ns3::Packet::TwoAddressTracedCallback")
          .AddTraceSource ("Rtt", "Round-trip time",
                           MakeTraceSourceAccessor (&SAFETYUdpEchoClient::m_rttTrace),
                           "ns3::TracedValueCallback::Double");
  return tid;
}

SAFETYUdpEchoClient::SAFETYUdpEchoClient ()
{
  NS_LOG_FUNCTION (this);
  m_sent = 0;
  m_socket = 0;
  m_sendEvent = EventId ();
  m_data = 0;
  m_dataSize = 0;
  m_failsafeManager = nullptr;
  Ptr<UniformRandomVariable> random = CreateObject<UniformRandomVariable> ();
  m_sequnceNumber = random->GetInteger (0, 65535);
  m_isPending = false;
  m_wdtenabled = false;
}

SAFETYUdpEchoClient::~SAFETYUdpEchoClient ()
{
  NS_LOG_FUNCTION (this);
  m_socket = 0;

  delete[] m_data;
  m_data = 0;
  m_dataSize = 0;
}

void
SAFETYUdpEchoClient::SetRemote (Address ip, uint16_t port)
{
  NS_LOG_FUNCTION (this << ip << port);
  m_peerAddress = ip;
  m_peerPort = port;
}

void
SAFETYUdpEchoClient::SetRemote (Address addr)
{
  NS_LOG_FUNCTION (this << addr);
  m_peerAddress = addr;
}

void
SAFETYUdpEchoClient::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  Application::DoDispose ();
}

void
SAFETYUdpEchoClient::StartApplication (void)
{
  NS_LOG_FUNCTION (this);

  std::cout << "Client Watchdog " << m_WdtPeriod.GetMilliSeconds() << "ms" << std::endl;

  if (m_socket == 0)
    {
      TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
      m_socket = Socket::CreateSocket (GetNode (), tid);
      if (Ipv4Address::IsMatchingType (m_peerAddress) == true)
        {
          if (m_socket->Bind () == -1)
            {
              NS_FATAL_ERROR ("Failed to bind socket");
            }
          m_socket->Connect (
              InetSocketAddress (Ipv4Address::ConvertFrom (m_peerAddress), m_peerPort));
        }
      else if (Ipv6Address::IsMatchingType (m_peerAddress) == true)
        {
          if (m_socket->Bind6 () == -1)
            {
              NS_FATAL_ERROR ("Failed to bind socket");
            }
          m_socket->Connect (
              Inet6SocketAddress (Ipv6Address::ConvertFrom (m_peerAddress), m_peerPort));
        }
      else if (InetSocketAddress::IsMatchingType (m_peerAddress) == true)
        {
          if (m_socket->Bind () == -1)
            {
              NS_FATAL_ERROR ("Failed to bind socket");
            }
          m_socket->Connect (m_peerAddress);
        }
      else if (Inet6SocketAddress::IsMatchingType (m_peerAddress) == true)
        {
          if (m_socket->Bind6 () == -1)
            {
              NS_FATAL_ERROR ("Failed to bind socket");
            }
          m_socket->Connect (m_peerAddress);
        }
      else
        {
          NS_ASSERT_MSG (false, "Incompatible address type: " << m_peerAddress);
        }
    }

  m_socket->SetRecvCallback (MakeCallback (&SAFETYUdpEchoClient::HandleRead, this));
  m_socket->SetAllowBroadcast (true);
  ScheduleTransmit (Seconds (0.));
  if(m_isFailsafeSource)
  {
    NS_LOG_INFO("FailSafe enabled");
    Simulator::Schedule (m_FailSafeTriggerTime, &SAFETYUdpEchoClient::EnableFailSafe, this, "Source");
  }
}

void
SAFETYUdpEchoClient::StopApplication ()
{
  NS_LOG_FUNCTION (this);

  if (m_socket != 0)
    {
      m_socket->Close ();
      m_socket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket>> ());
      m_socket = 0;
    }

  Simulator::Cancel (m_sendEvent);
}

void
SAFETYUdpEchoClient::SetDataSize (uint32_t dataSize)
{
  NS_LOG_FUNCTION (this << dataSize);

  //
  // If the client is setting the echo packet data size this way, we infer
  // that she doesn't care about the contents of the packet at all, so
  // neither will we.
  //
  delete[] m_data;
  m_data = 0;
  m_dataSize = 0;
  m_size = dataSize;
}

uint32_t
SAFETYUdpEchoClient::GetDataSize (void) const
{
  NS_LOG_FUNCTION (this);
  return m_size;
}

void
SAFETYUdpEchoClient::SetFill (std::string fill)
{
  NS_LOG_FUNCTION (this << fill);

  uint32_t dataSize = fill.size () + 1;

  if (dataSize != m_dataSize)
    {
      delete[] m_data;
      m_data = new uint8_t[dataSize];
      m_dataSize = dataSize;
    }

  memcpy (m_data, fill.c_str (), dataSize);

  //
  // Overwrite packet size attribute.
  //
  m_size = dataSize;
}

void
SAFETYUdpEchoClient::SetFill (uint8_t fill, uint32_t dataSize)
{
  NS_LOG_FUNCTION (this << fill << dataSize);
  if (dataSize != m_dataSize)
    {
      delete[] m_data;
      m_data = new uint8_t[dataSize];
      m_dataSize = dataSize;
    }

  memset (m_data, fill, dataSize);

  //
  // Overwrite packet size attribute.
  //
  m_size = dataSize;
}

void
SAFETYUdpEchoClient::SetFill (uint8_t *fill, uint32_t fillSize, uint32_t dataSize)
{
  NS_LOG_FUNCTION (this << fill << fillSize << dataSize);
  if (dataSize != m_dataSize)
    {
      delete[] m_data;
      m_data = new uint8_t[dataSize];
      m_dataSize = dataSize;
    }

  if (fillSize >= dataSize)
    {
      memcpy (m_data, fill, dataSize);
      m_size = dataSize;
      return;
    }

  //
  // Do all but the final fill.
  //
  uint32_t filled = 0;
  while (filled + fillSize < dataSize)
    {
      memcpy (&m_data[filled], fill, fillSize);
      filled += fillSize;
    }

  //
  // Last fill may be partial
  //
  memcpy (&m_data[filled], fill, dataSize - filled);

  //
  // Overwrite packet size attribute.
  //
  m_size = dataSize;
}

void
SAFETYUdpEchoClient::SetFailSafeManager (Ptr<FailSafeManager> fsm)
{
  m_failsafeManager = fsm;
}

void
SAFETYUdpEchoClient::ScheduleTransmit (Time dt)
{
  NS_LOG_FUNCTION (this << dt);
  m_sendEvent = Simulator::Schedule (dt, &SAFETYUdpEchoClient::Send, this);
}

void
SAFETYUdpEchoClient::ScheduleTransmitNow ()
{
  m_sendEvent = Simulator::ScheduleNow(&SAFETYUdpEchoClient::Send, this);
}

void
SAFETYUdpEchoClient::ScheduleWdtExpired ()
{
  if (m_WdtPeriod > Time(0))
  {
    m_WdtExpireEvent = Simulator::Schedule(m_WdtPeriod, &SAFETYUdpEchoClient::EnableFailSafe, this, "WDT");
  }
}

void
SAFETYUdpEchoClient::DestroyWdtExpired ()
{
  Simulator::Cancel(m_WdtExpireEvent);
}

void
SAFETYUdpEchoClient::EnableFailSafe (std::string Reason)
{
  if(!m_failsafeManager->GetFailsafeValue())
  {
    NS_LOG_WARN("FailSafe Enabled");
    std::cout << "FailSafe Enabled at " << Simulator::Now().GetMicroSeconds() << "us in master " << Ipv4Address::ConvertFrom (m_peerAddress) << " due to " << Reason << std::endl;
    m_failsafeManager->SetFailsafeValue(true);
  }
}

void
SAFETYUdpEchoClient::Send (void)
{
  NS_LOG_FUNCTION (this);

  NS_ASSERT (m_sendEvent.IsExpired ());

  if(m_interval > Time(0))
  {
    ScheduleTransmit (m_interval);
  }

  if(m_isPending)
  {
    return;
  }

  m_isPending = true;

  Time deadline;
  if(m_interval > Time(0))
  {
    int64_t femtoInterval = m_interval.GetMicroSeconds();
    int64_t femtoNow = Simulator::Now ().GetMicroSeconds();
    deadline = MicroSeconds(((femtoNow / femtoInterval)*femtoInterval) + femtoInterval);
    //std::cout << femtoNow << " " << femtoInterval << " " << deadline << std::endl;
  } 

  struct DataPacket dataPacket;
  dataPacket.timeStamp = (uint32_t) Simulator::Now().GetMilliSeconds();
  dataPacket.seqnum = (uint16_t) m_sequnceNumber;
  dataPacket.safeTag = (uint8_t) m_failsafeManager->GetFailsafeValue();

  //std::cout << dataPacket.timeStamp  << std::endl;
  NS_ASSERT_MSG (sizeof(dataPacket) == m_size,
                     "SAFETYUdpEchoClient::Send(): m_size and dataPacket inconsistent");
  if (sizeof(dataPacket) != m_dataSize)
  {
    delete[] m_data;
    m_data = new uint8_t[sizeof(dataPacket)];
    m_dataSize = sizeof(dataPacket);
  }
  memcpy (m_data, &dataPacket, sizeof(dataPacket));
  m_dataSize = sizeof(dataPacket);


  Ptr<Packet> p;
  if (m_dataSize)
    {
      //
      // If m_dataSize is non-zero, we have a data buffer of the same size that we
      // are expected to copy and send.  This state of affairs is created if one of
      // the Fill functions is called.  In this case, m_size must have been set
      // to agree with m_dataSize
      //
      NS_ASSERT_MSG (m_dataSize == m_size,
                     "SAFETYUdpEchoClient::Send(): m_size and m_dataSize inconsistent");
      NS_ASSERT_MSG (m_data, "SAFETYUdpEchoClient::Send(): m_dataSize but no m_data");
      p = Create<Packet> (m_data, m_dataSize);
    }
  else
    {
      //
      // If m_dataSize is zero, the client has indicated that it doesn't care
      // about the data itself either by specifying the data size by setting
      // the corresponding attribute or by not calling a SetFill function.  In
      // this case, we don't worry about it either.  But we do allow m_size
      // to have a value different from the (zero) m_dataSize.
      //
      p = Create<Packet> (m_size);
    }
  if(m_interval > Time(0))
  {
    TimestampTag timestamp;
    timestamp.SetTimestamp (deadline);
    p->AddByteTag (timestamp);
  }

  FailSafeTag tag;
  tag.SetFailsafeValue(m_failsafeManager->GetFailsafeValue());
  SeqnumTag seqnumTag;
  p->AddPacketTag(tag);
  seqnumTag.SetSeqNumValue((uint32_t)m_sequnceNumber);
  p->AddPacketTag(seqnumTag);
  aCopy = p->Copy();
  Address localAddress;
  m_socket->GetSockName (localAddress);
  // call to the trace sinks before the packet is actually sent,
  // so that tags added to the packet can be sent as well
  m_txTrace (p);
  if (Ipv4Address::IsMatchingType (m_peerAddress))
    {
      m_txTraceWithAddresses (
          p, localAddress,
          InetSocketAddress (Ipv4Address::ConvertFrom (m_peerAddress), m_peerPort));
    }
  else if (Ipv6Address::IsMatchingType (m_peerAddress))
    {
      m_txTraceWithAddresses (
          p, localAddress,
          Inet6SocketAddress (Ipv6Address::ConvertFrom (m_peerAddress), m_peerPort));
    }
  m_socket->Send (p);
  m_rtt = Simulator::Now () - m_sendTime;
  m_rttTrace = m_rtt.GetMicroSeconds ();
  m_sendTime = Simulator::Now ();

  //std::cout << "Sent at " << m_sendTime.GetMilliSeconds() << std::endl;
  if (m_wdtenabled && (Simulator::Now() > Seconds(2)))
  {
    ScheduleWdtExpired();
  }
  ++m_sent;

  if (Ipv4Address::IsMatchingType (m_peerAddress))
    {
      NS_LOG_INFO ("At time " << Simulator::Now ().As (Time::S) << " client sent " << m_size
                              << " bytes to " << Ipv4Address::ConvertFrom (m_peerAddress)
                              << " port " << m_peerPort);
    }
  else if (Ipv6Address::IsMatchingType (m_peerAddress))
    {
      NS_LOG_INFO ("At time " << Simulator::Now ().As (Time::S) << " client sent " << m_size
                              << " bytes to " << Ipv6Address::ConvertFrom (m_peerAddress)
                              << " port " << m_peerPort);
    }
  else if (InetSocketAddress::IsMatchingType (m_peerAddress))
    {
      NS_LOG_INFO (
          "At time " << Simulator::Now ().As (Time::S) << " client sent " << m_size << " bytes to "
                     << InetSocketAddress::ConvertFrom (m_peerAddress).GetIpv4 () << " port "
                     << InetSocketAddress::ConvertFrom (m_peerAddress).GetPort ());
    }
  else if (Inet6SocketAddress::IsMatchingType (m_peerAddress))
    {
      NS_LOG_INFO (
          "At time " << Simulator::Now ().As (Time::S) << " client sent " << m_size << " bytes to "
                     << Inet6SocketAddress::ConvertFrom (m_peerAddress).GetIpv6 () << " port "
                     << Inet6SocketAddress::ConvertFrom (m_peerAddress).GetPort ());
    }

  

  


}

void
SAFETYUdpEchoClient::HandleRead (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
  Ptr<Packet> packet;
  Address from;
  Address localAddress;
  while ((packet = socket->RecvFrom (from)))
    {
      if (InetSocketAddress::IsMatchingType (from))
        {
          NS_LOG_INFO ("At time " << Simulator::Now ().As (Time::S) << " client received "
                                  << packet->GetSize () << " bytes from "
                                  << InetSocketAddress::ConvertFrom (from).GetIpv4 () << " port "
                                  << InetSocketAddress::ConvertFrom (from).GetPort ());
        }
      else if (Inet6SocketAddress::IsMatchingType (from))
        {
          NS_LOG_INFO ("At time " << Simulator::Now ().As (Time::S) << " client received "
                                  << packet->GetSize () << " bytes from "
                                  << Inet6SocketAddress::ConvertFrom (from).GetIpv6 () << " port "
                                  << Inet6SocketAddress::ConvertFrom (from).GetPort ());
        }

      DestroyWdtExpired();

      m_isPending = false;
      m_wdtenabled = true;

      SeqnumTag tagSeqnum;
      packet->PeekPacketTag(tagSeqnum);
      if(tagSeqnum.GetSeqNumValue() != m_sequnceNumber)
      {
        std::cout << "Seqnum not equal. Got " << tagSeqnum.GetSeqNumValue() << " expected " << m_sequnceNumber  << std::endl;
      }
      else
      {
        m_sequnceNumber++;
      }


      socket->GetSockName (localAddress);
      m_rxTrace (packet);
      m_rxTraceWithAddresses (packet, from, localAddress);

      FailSafeTag tagRcv;
      packet->PeekPacketTag(tagRcv);
      
      if(tagRcv.GetFailsafeValue())
      {
        EnableFailSafe("Trigger");
      }

      if ((m_sent < m_count) && m_interval == Time(0))
      {
        ScheduleTransmitNow ();
      }

    }
}

} // Namespace ns3
