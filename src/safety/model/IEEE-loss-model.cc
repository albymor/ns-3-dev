/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2005,2006,2007 INRIA
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
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 * Contributions: Timo Bingmann <timo.bingmann@student.kit.edu>
 * Contributions: Tom Hewer <tomhewer@mac.com> for Two Ray Ground Model
 *                Pavel Boyko <boyko@iitp.ru> for matrix
 */

#include "IEEE-loss-model.h"
#include "ns3/log.h"
#include "ns3/mobility-model.h"
#include "ns3/boolean.h"
#include "ns3/double.h"
#include "ns3/string.h"
#include "ns3/pointer.h"
#include <cmath>

namespace ns3 {
NS_LOG_COMPONENT_DEFINE ("IEEEPropagationLossModel");
NS_OBJECT_ENSURE_REGISTERED (IEEEPropagationLossModel);

TypeId 
IEEEPropagationLossModel::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::IEEEPropagationLossModel")
    .SetParent<PropagationLossModel> ()
    .SetGroupName ("Propagation")
    .AddConstructor<IEEEPropagationLossModel> ()
    .AddAttribute ("Frequency", 
                   "The carrier frequency (in Hz) at which propagation occurs  (default is 2.4 GHz).",
                   DoubleValue (2.4e9),
                   MakeDoubleAccessor (&IEEEPropagationLossModel::SetFrequency,
                                       &IEEEPropagationLossModel::GetFrequency),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("SystemLoss", "The system loss",
                   DoubleValue (1.0),
                   MakeDoubleAccessor (&IEEEPropagationLossModel::m_systemLoss),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("MinLoss", 
                   "The minimum value (dB) of the total loss, used at short ranges. Note: ",
                   DoubleValue (0.0),
                   MakeDoubleAccessor (&IEEEPropagationLossModel::SetMinLoss,
                                       &IEEEPropagationLossModel::GetMinLoss),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("BreakpointDistance", 
                   "Distance (m) at which the alpha value is changed.",
                   DoubleValue (10.0),
                   MakeDoubleAccessor (&IEEEPropagationLossModel::SetBreakpointDistance,
                                       &IEEEPropagationLossModel::GetBreakpointDistance),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("Alpha2", 
                   "The exponent of the path loss in the second region.",
                   DoubleValue (3.5),
                   MakeDoubleAccessor (&IEEEPropagationLossModel::SetAlpha2,
                                       &IEEEPropagationLossModel::GetAlpha2),
                   MakeDoubleChecker<double> ())
  ;
  return tid;
}

IEEEPropagationLossModel::IEEEPropagationLossModel ()
{
}
void
IEEEPropagationLossModel::SetSystemLoss (double systemLoss)
{
  m_systemLoss = systemLoss;
}
double
IEEEPropagationLossModel::GetSystemLoss (void) const
{
  return m_systemLoss;
}
void
IEEEPropagationLossModel::SetMinLoss (double minLoss)
{
  m_minLoss = minLoss;
}
double
IEEEPropagationLossModel::GetMinLoss (void) const
{
  return m_minLoss;
}

void
IEEEPropagationLossModel::SetFrequency (double frequency)
{
  m_frequency = frequency;
  static const double C = 299792458.0; // speed of light in vacuum
  m_lambda = C / frequency;
}

double
IEEEPropagationLossModel::GetFrequency (void) const
{
  return m_frequency;
}

void
IEEEPropagationLossModel::SetBreakpointDistance (double breakpointDistance)
{
  m_breakpointDistance = breakpointDistance;
}


double
IEEEPropagationLossModel::GetBreakpointDistance (void) const
{
  return m_breakpointDistance;
}

void
IEEEPropagationLossModel::SetAlpha2 (double alpha2)
{
  m_alpha2 = alpha2;
}


double
IEEEPropagationLossModel::GetAlpha2 (void) const
{
  return m_alpha2;
}

double
IEEEPropagationLossModel::DbmToW (double dbm) const
{
  double mw = std::pow (10.0,dbm/10.0);
  return mw / 1000.0;
}

double
IEEEPropagationLossModel::DbmFromW (double w) const
{
  double dbm = std::log10 (w * 1000.0) * 10.0;
  return dbm;
}

double IEEEPropagationLossModel::CalcIEEEPathLoss (double distance) const
{
  double numerator = m_lambda * m_lambda;
  double denominator = 16 * M_PI * M_PI * distance * distance * m_systemLoss;
  double lossDb = -10 * log10 (numerator / denominator);

  return lossDb;

}


double 
IEEEPropagationLossModel::DoCalcRxPower (double txPowerDbm,
                                          Ptr<MobilityModel> a,
                                          Ptr<MobilityModel> b) const
{
  /*
   * Friis free space equation:
   * where Pt, Gr, Gr and P are in Watt units
   * L is in meter units.
   *
   *    P     Gt * Gr * (lambda^2)
   *   --- = ---------------------
   *    Pt     (4 * pi * d)^2 * L
   *
   * Gt: tx gain (unit-less)
   * Gr: rx gain (unit-less)
   * Pt: tx power (W)
   * d: distance (m)
   * L: system loss
   * lambda: wavelength (m)
   *
   * Here, we ignore tx and rx gain and the input and output values 
   * are in dB or dBm:
   *
   *                           lambda^2
   * rx = tx +  10 log10 (-------------------)
   *                       (4 * pi * d)^2 * L
   *
   * rx: rx power (dB)
   * tx: tx power (dB)
   * d: distance (m)
   * L: system loss (unit-less)
   * lambda: wavelength (m)
   */
  double distance = a->GetDistanceFrom (b);
  if (distance < 3*m_lambda)
    {
  NS_LOG_WARN ("distance not within the far field region => inaccurate propagation loss value");
    }
  if (distance <= 0)
    {
      return txPowerDbm - m_minLoss;
    }
  double lossDb;
  if (distance <= m_breakpointDistance)
    {
      lossDb = CalcIEEEPathLoss (distance);
      NS_LOG_DEBUG ("distance=" << distance<< "m, loss=" << lossDb <<"dB");
    }
  else
    {
      lossDb = CalcIEEEPathLoss (m_breakpointDistance) - (10 * m_alpha2 * log10 (m_breakpointDistance / distance));
      NS_LOG_DEBUG ("distance=" << distance<< "m, loss=" << lossDb <<"dB"<<", alpha2="<<m_alpha2);
    }
  
  return txPowerDbm - std::max (lossDb, m_minLoss);

  return 0;
}

int64_t
IEEEPropagationLossModel::DoAssignStreams (int64_t stream)
{
  return 0;
}

} // namespace ns3
