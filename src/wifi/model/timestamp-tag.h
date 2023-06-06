#ifndef TIMESTAMP_TAG_H
#define TIMESTAMP_TAG_H

#include "ns3/packet.h"
#include "ns3/simulator.h"
#include "ns3/tag.h"
#include "ns3/uinteger.h"

namespace ns3 {
class TimestampTag : public Tag
{
public:
  static TypeId GetTypeId ();
  TypeId GetInstanceTypeId () const override;
  uint32_t GetSerializedSize () const override;
  void Serialize (TagBuffer i) const override;
  void Deserialize (TagBuffer i) override;
  void Print (std::ostream &os) const override;

  // these are our accessors to our tag structure
  void SetTimestamp (Time time);
  Time GetTimestamp (void) const;

private:
  Time m_timestamp;
};
} // namespace ns3

#endif /* SEQNUM_TAG_H */