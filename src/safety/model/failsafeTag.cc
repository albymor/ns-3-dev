#include "failsafeTag.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("FailSafeTag");

NS_OBJECT_ENSURE_REGISTERED (FailSafeTag);

TypeId
FailSafeTag::GetTypeId ()
{
  static TypeId tid = TypeId ("ns3::FailSafeTag")
                          .SetParent<Tag> ()
                          .AddConstructor<FailSafeTag> ()
                          .AddAttribute ("SimpleValue", "A simple value", EmptyAttributeValue (),
                                         MakeUintegerAccessor (&FailSafeTag::GetFailsafeValue),
                                         MakeUintegerChecker<uint8_t> ());
  return tid;
}

TypeId
FailSafeTag::GetInstanceTypeId () const
{
  return GetTypeId ();
}

uint32_t
FailSafeTag::GetSerializedSize () const
{
  return 1;
}

void
FailSafeTag::Serialize (TagBuffer i) const
{
  i.WriteU8 (m_simpleValue);
}

void
FailSafeTag::Deserialize (TagBuffer i)
{
  m_simpleValue = i.ReadU8 ();
}

void
FailSafeTag::Print (std::ostream &os) const
{
  os << "v=" << (uint32_t) m_simpleValue;
}

void
FailSafeTag::SetFailsafeValue (uint8_t value)
{
  m_simpleValue = value;
}

uint8_t
FailSafeTag::GetFailsafeValue () const
{
  return m_simpleValue;
}
} // namespace ns3