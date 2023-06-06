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
 * Contributions: Gary Pei <guangyu.pei@boeing.com> for fixed RSS
 * Contributions: Tom Hewer <tomhewer@mac.com> for two ray ground model
 *                Pavel Boyko <boyko@iitp.ru> for matrix
 */

#ifndef IEEE_PROPAGATION_LOSS_MODEL_H
#define IEEE_PROPAGATION_LOSS_MODEL_H

#include "ns3/object.h"
#include "ns3/random-variable-stream.h"
#include "ns3/propagation-loss-model.h"
#include <map>

namespace ns3 {

class IEEEPropagationLossModel : public PropagationLossModel
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);
  IEEEPropagationLossModel ();

  // Delete copy constructor and assignment operator to avoid misuse
  IEEEPropagationLossModel (const IEEEPropagationLossModel &) = delete;
  IEEEPropagationLossModel & operator = (const IEEEPropagationLossModel &) = delete;

  /**
   * \param frequency (Hz)
   *
   * Set the carrier frequency used in the Friis model 
   * calculation.
   */
  void SetFrequency (double frequency);
  /**
   * \param systemLoss (dimension-less)
   *
   * Set the system loss used by the Friis propagation model.
   */
  void SetSystemLoss (double systemLoss);

  /**
   * \param minLoss the minimum loss (dB)
   *
   * no matter how short the distance, the total propagation loss (in
   * dB) will always be greater or equal than this value 
   */
  void SetMinLoss (double minLoss);

  /**
   * \return the minimum loss.
   */
  double GetMinLoss (void) const;

  /**
   * \returns the current frequency (Hz)
   */
  double GetFrequency (void) const;
  /**
   * \returns the current system loss (dimension-less)
   */
  double GetSystemLoss (void) const;


  void SetBreakpointDistance (double breakpointDistance);
  double GetBreakpointDistance (void) const;
  void SetAlpha2 (double alpha2);
  double GetAlpha2 (void) const;

private:
  double DoCalcRxPower (double txPowerDbm,
                        Ptr<MobilityModel> a,
                        Ptr<MobilityModel> b) const override;

  double CalcIEEEPathLoss (double distance) const;
  int64_t DoAssignStreams (int64_t stream) override;

  /**
   * Transforms a Dbm value to Watt
   * \param dbm the Dbm value
   * \return the Watts
   */
  double DbmToW (double dbm) const;

  /**
   * Transforms a Watt value to Dbm
   * \param w the Watt value
   * \return the Dbm
   */
  double DbmFromW (double w) const;

  double m_lambda;        //!< the carrier wavelength
  double m_frequency;     //!< the carrier frequency
  double m_systemLoss;    //!< the system loss
  double m_minLoss;       //!< the minimum loss
  double m_breakpointDistance; //!< the distance at which the loss model changes
  double m_alpha2;        //!< the path loss exponent after the breakpoint distance
};

} // namespace ns3

#endif /* PROPAGATION_LOSS_MODEL_H */
