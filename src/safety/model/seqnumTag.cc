#include "seqnumTag.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("SeqnumTag");

NS_OBJECT_ENSURE_REGISTERED (SeqnumTag);

TypeId
SeqnumTag::GetTypeId ()
{
  static TypeId tid = TypeId ("ns3::SeqnumTag")
                          .SetParent<Tag> ()
                          .AddConstructor<SeqnumTag> ()
                          .AddAttribute ("SimpleValue", "A simple value", EmptyAttributeValue (),
                                         MakeUintegerAccessor (&SeqnumTag::GetSeqNumValue),
                                         MakeUintegerChecker<uint32_t> ());
  return tid;
}

TypeId
SeqnumTag::GetInstanceTypeId () const
{
  return GetTypeId ();
}

uint32_t
SeqnumTag::GetSerializedSize () const
{
  return 4;
}

void
SeqnumTag::Serialize (TagBuffer i) const
{
  i.WriteU32 (m_simpleValue);
}

void
SeqnumTag::Deserialize (TagBuffer i)
{
  m_simpleValue = i.ReadU32 ();
}

void
SeqnumTag::Print (std::ostream &os) const
{
  os << "v=" << (uint32_t) m_simpleValue;
}

void
SeqnumTag::SetSeqNumValue (uint32_t value)
{
  m_simpleValue = value;
}

uint32_t
SeqnumTag::GetSeqNumValue () const
{
  return m_simpleValue;
}
} // namespace ns3