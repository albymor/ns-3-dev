#ifndef SEQNUM_TAG_H
#define SEQNUM_TAG_H

#include "ns3/packet.h"
#include "ns3/simulator.h"
#include "ns3/tag.h"
#include "ns3/uinteger.h"

namespace ns3 {
class SeqnumTag : public Tag
{
public:
  static TypeId GetTypeId ();
  TypeId GetInstanceTypeId () const override;
  uint32_t GetSerializedSize () const override;
  void Serialize (TagBuffer i) const override;
  void Deserialize (TagBuffer i) override;
  void Print (std::ostream &os) const override;

  // these are our accessors to our tag structure
  void SetSeqNumValue (uint32_t value);
  uint32_t GetSeqNumValue () const;

private:
  uint32_t m_simpleValue;
};
} // namespace ns3

#endif /* SEQNUM_TAG_H */