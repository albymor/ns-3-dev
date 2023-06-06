/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2008 INRIA
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
 */
#include "safety-udp-echo-helper.h"
#include "safety-udp-echo-client.h"
#include "safety-udp-echo-server.h"
#include "ns3/uinteger.h"
#include "ns3/names.h"

namespace ns3 {

SAFETYUdpEchoServerHelper::SAFETYUdpEchoServerHelper (uint16_t port)
{
  m_factory.SetTypeId (SAFETYUdpEchoServer::GetTypeId ());
  SetAttribute ("Port", UintegerValue (port));
}

SAFETYUdpEchoServerHelper::SAFETYUdpEchoServerHelper (uint16_t port, float wdt_period)
{
  m_factory.SetTypeId (SAFETYUdpEchoServer::GetTypeId ());
  SetAttribute ("Port", UintegerValue (port));
  SetAttribute ("WdtPeriod", TimeValue (MilliSeconds(wdt_period)));
}

void 
SAFETYUdpEchoServerHelper::SetAttribute (
  std::string name, 
  const AttributeValue &value)
{
  m_factory.Set (name, value);
}

ApplicationContainer
SAFETYUdpEchoServerHelper::Install (Ptr<Node> node) const
{
  return ApplicationContainer (InstallPriv (node));
}

ApplicationContainer
SAFETYUdpEchoServerHelper::Install (std::string nodeName) const
{
  Ptr<Node> node = Names::Find<Node> (nodeName);
  return ApplicationContainer (InstallPriv (node));
}

ApplicationContainer
SAFETYUdpEchoServerHelper::Install (NodeContainer c) const
{
  ApplicationContainer apps;
  for (NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i)
    {
      apps.Add (InstallPriv (*i));
    }

  return apps;
}

Ptr<Application>
SAFETYUdpEchoServerHelper::InstallPriv (Ptr<Node> node) const
{
  Ptr<Application> app = m_factory.Create<SAFETYUdpEchoServer> ();
  node->AddApplication (app);

  return app;
}


SAFETYUdpEchoClientHelper::SAFETYUdpEchoClientHelper (Address address, uint16_t port)
{
  m_factory.SetTypeId (SAFETYUdpEchoClient::GetTypeId ());
  SetAttribute ("RemoteAddress", AddressValue (address));
  SetAttribute ("RemotePort", UintegerValue (port));
}

SAFETYUdpEchoClientHelper::SAFETYUdpEchoClientHelper (Address address)
{
  m_factory.SetTypeId (SAFETYUdpEchoClient::GetTypeId ());
  SetAttribute ("RemoteAddress", AddressValue (address));
}

void 
SAFETYUdpEchoClientHelper::SetAttribute (
  std::string name, 
  const AttributeValue &value)
{
  m_factory.Set (name, value);
}

void
SAFETYUdpEchoClientHelper::SetFill (Ptr<Application> app, std::string fill)
{
  app->GetObject<SAFETYUdpEchoClient>()->SetFill (fill);
}

void
SAFETYUdpEchoClientHelper::SetFill (Ptr<Application> app, uint8_t fill, uint32_t dataLength)
{
  app->GetObject<SAFETYUdpEchoClient>()->SetFill (fill, dataLength);
}

void
SAFETYUdpEchoClientHelper::SetFill (Ptr<Application> app, uint8_t *fill, uint32_t fillLength, uint32_t dataLength)
{
  app->GetObject<SAFETYUdpEchoClient>()->SetFill (fill, fillLength, dataLength);
}

void
SAFETYUdpEchoClientHelper::SetFailSafeManager (Ptr<Application> app, Ptr<FailSafeManager> fsm)
{
  app->GetObject<SAFETYUdpEchoClient>()->SetFailSafeManager (fsm);
}

ApplicationContainer
SAFETYUdpEchoClientHelper::Install (Ptr<Node> node) const
{
  return ApplicationContainer (InstallPriv (node));
}

ApplicationContainer
SAFETYUdpEchoClientHelper::Install (std::string nodeName) const
{
  Ptr<Node> node = Names::Find<Node> (nodeName);
  return ApplicationContainer (InstallPriv (node));
}

ApplicationContainer
SAFETYUdpEchoClientHelper::Install (NodeContainer c) const
{
  ApplicationContainer apps;
  for (NodeContainer::Iterator i = c.Begin (); i != c.End (); ++i)
    {
      apps.Add (InstallPriv (*i));
    }

  return apps;
}

Ptr<Application>
SAFETYUdpEchoClientHelper::InstallPriv (Ptr<Node> node) const
{
  Ptr<Application> app = m_factory.Create<SAFETYUdpEchoClient> ();
  node->AddApplication (app);

  return app;
}

} // namespace ns3
