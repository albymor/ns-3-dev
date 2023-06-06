#include "failsafeManager.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("FailSafeManager");

NS_OBJECT_ENSURE_REGISTERED (FailSafeManager);



TypeId 
FailSafeManager::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::FailSafeManager")
    .SetParent<Object> ()
    .SetGroupName ("FailSafeManager")
    .AddTraceSource ("FailSafeSignal", "FailSafe Signal",
                           MakeTraceSourceAccessor (&FailSafeManager::m_failsafeTrace),
                           "ns3::TracedValueCallback::Uint8");
  ;
  return tid;
}
TypeId 
FailSafeManager::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

FailSafeManager::FailSafeManager()
{
  m_failSafeSignal = 0;
}

FailSafeManager::~FailSafeManager()
{
  m_failSafeSignal = 0;
}

void
FailSafeManager::SetFailsafeValue (uint8_t value)
{
  m_failSafeSignal = value;
  m_failsafeTrace = value;
}

uint8_t
FailSafeManager::GetFailsafeValue () const
{
  return m_failSafeSignal;
}
} // namespace ns3