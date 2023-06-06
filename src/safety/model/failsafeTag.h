#ifndef FAILSAFE_TAG_H
#define FAILSAFE_TAG_H

#include "ns3/packet.h"
#include "ns3/simulator.h"
#include "ns3/tag.h"
#include "ns3/uinteger.h"

namespace ns3 {
class FailSafeTag : public Tag
{
public:
  static TypeId GetTypeId ();
  TypeId GetInstanceTypeId () const override;
  uint32_t GetSerializedSize () const override;
  void Serialize (TagBuffer i) const override;
  void Deserialize (TagBuffer i) override;
  void Print (std::ostream &os) const override;

  // these are our accessors to our tag structure
  void SetFailsafeValue (uint8_t value);
  uint8_t GetFailsafeValue () const;

private:
  uint8_t m_simpleValue;
};
} // namespace ns3

#endif /* FAILSAFE_TAG_H */