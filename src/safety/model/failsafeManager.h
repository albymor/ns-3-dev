#ifndef FAILSAFE_MANAGER_H
#define FAILSAFE_MANAGER_H

#include "ns3/packet.h"
#include "ns3/simulator.h"
#include "ns3/uinteger.h"
#include "ns3/traced-value.h"

namespace ns3 {
class FailSafeManager: public Object
{
public:
  FailSafeManager();
  virtual ~FailSafeManager(); 

  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId (void) const;

  // these are our accessors to our tag structure
  void SetFailsafeValue (uint8_t value);
  uint8_t GetFailsafeValue () const;

private:
  uint8_t m_failSafeSignal;
  TracedValue<uint8_t> m_failsafeTrace;
};
} // namespace ns3

#endif /* FAILSAFE_TAG_H */