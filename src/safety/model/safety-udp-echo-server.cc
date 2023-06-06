/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright 2007 University of Washington
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
 */

#include "ns3/log.h"
#include "ns3/ipv4-address.h"
#include "ns3/ipv6-address.h"
#include "ns3/address-utils.h"
#include "ns3/nstime.h"
#include "ns3/inet-socket-address.h"
#include "ns3/inet6-socket-address.h"
#include "ns3/socket.h"
#include "ns3/udp-socket.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/packet.h"
#include "ns3/uinteger.h"

#include "safety-udp-echo-server.h"
#include "failsafeTag.h"
#include "seqnumTag.h"

#include "ns3/network-module.h"
#include "ns3/ipv4.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("SAFETYUdpEchoServerApplication");

NS_OBJECT_ENSURE_REGISTERED (SAFETYUdpEchoServer);

TypeId
SAFETYUdpEchoServer::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::SAFETYUdpEchoServer")
    .SetParent<Application> ()
    .SetGroupName("Applications")
    .AddConstructor<SAFETYUdpEchoServer> ()
    .AddAttribute ("Port", "Port on which we listen for incoming packets.",
                   UintegerValue (9),
                   MakeUintegerAccessor (&SAFETYUdpEchoServer::m_port),
                   MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("IsFailsafeSource", "Set the node as a failsafe source",
                   UintegerValue (0),
                   MakeUintegerAccessor (&SAFETYUdpEchoServer::m_isFailsafeSource),
                   MakeUintegerChecker<uint8_t> ())
    .AddAttribute ("FailSafeTriggerTime", "Time at which Fail safe is enabled", TimeValue (Seconds (3.0)),
                    MakeTimeAccessor (&SAFETYUdpEchoServer::m_FailSafeTriggerTime), MakeTimeChecker ())
    .AddAttribute ("WdtPeriod", "Period of the wdt", TimeValue (MilliSeconds (250.0)),
                    MakeTimeAccessor (&SAFETYUdpEchoServer::m_WdtPeriod), MakeTimeChecker ())
    .AddTraceSource ("Rx", "A packet has been received",
                     MakeTraceSourceAccessor (&SAFETYUdpEchoServer::m_rxTrace),
                     "ns3::Packet::TracedCallback")
    .AddTraceSource ("RxWithAddresses", "A packet has been received",
                     MakeTraceSourceAccessor (&SAFETYUdpEchoServer::m_rxTraceWithAddresses),
                     "ns3::Packet::TwoAddressTracedCallback")
    .AddTraceSource ("FailSafeSignal", "FailSafe Signal",
                      MakeTraceSourceAccessor (&SAFETYUdpEchoServer::m_failsafeTrace),
                      "ns3::TracedValueCallback::Uint8");
  ;
  return tid;
}

SAFETYUdpEchoServer::SAFETYUdpEchoServer ()
{
  NS_LOG_FUNCTION (this);
}

SAFETYUdpEchoServer::~SAFETYUdpEchoServer()
{
  NS_LOG_FUNCTION (this);
  m_socket = 0;
  m_socket6 = 0;
}

void
SAFETYUdpEchoServer::DoDispose (void)
{
  NS_LOG_FUNCTION (this);
  Application::DoDispose ();
}

void 
SAFETYUdpEchoServer::StartApplication (void)
{
  NS_LOG_FUNCTION (this);

  std::cout << "Server Watchdog " << m_WdtPeriod.GetMilliSeconds() << "ms" << std::endl;

  if (m_socket == 0)
    {
      TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
      m_socket = Socket::CreateSocket (GetNode (), tid);
      InetSocketAddress local = InetSocketAddress (Ipv4Address::GetAny (), m_port);
      if (m_socket->Bind (local) == -1)
        {
          NS_FATAL_ERROR ("Failed to bind socket");
        }
      if (addressUtils::IsMulticast (m_local))
        {
          Ptr<UdpSocket> udpSocket = DynamicCast<UdpSocket> (m_socket);
          if (udpSocket)
            {
              // equivalent to setsockopt (MCAST_JOIN_GROUP)
              udpSocket->MulticastJoinGroup (0, m_local);
            }
          else
            {
              NS_FATAL_ERROR ("Error: Failed to join multicast group");
            }
        }
    }

  if (m_socket6 == 0)
    {
      TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
      m_socket6 = Socket::CreateSocket (GetNode (), tid);
      Inet6SocketAddress local6 = Inet6SocketAddress (Ipv6Address::GetAny (), m_port);
      if (m_socket6->Bind (local6) == -1)
        {
          NS_FATAL_ERROR ("Failed to bind socket");
        }
      if (addressUtils::IsMulticast (local6))
        {
          Ptr<UdpSocket> udpSocket = DynamicCast<UdpSocket> (m_socket6);
          if (udpSocket)
            {
              // equivalent to setsockopt (MCAST_JOIN_GROUP)
              udpSocket->MulticastJoinGroup (0, local6);
            }
          else
            {
              NS_FATAL_ERROR ("Error: Failed to join multicast group");
            }
        }
    }

  m_socket->SetRecvCallback (MakeCallback (&SAFETYUdpEchoServer::HandleRead, this));
  m_socket6->SetRecvCallback (MakeCallback (&SAFETYUdpEchoServer::HandleRead, this));
  if(m_isFailsafeSource)
  {
    Simulator::Schedule (m_FailSafeTriggerTime, &SAFETYUdpEchoServer::EnableFailSafe, this, "Source");
  }
  //ScheduleWdtExpired();
}

void 
SAFETYUdpEchoServer::StopApplication ()
{
  NS_LOG_FUNCTION (this);

  if (m_socket != 0) 
    {
      m_socket->Close ();
      m_socket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
    }
  if (m_socket6 != 0) 
    {
      m_socket6->Close ();
      m_socket6->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
    }
}

void
SAFETYUdpEchoServer::ScheduleWdtExpired ()
{
  if (m_WdtPeriod > Time(0))
  {
    m_WdtExpireEvent = Simulator::Schedule(m_WdtPeriod, &SAFETYUdpEchoServer::EnableFailSafe, this, "WDT");
  }
}

void
SAFETYUdpEchoServer::DestroyWdtExpired ()
{
  Simulator::Cancel(m_WdtExpireEvent);
}

void
SAFETYUdpEchoServer::EnableFailSafe (std::string Reason)
{
  if(!m_failsafeTrace)
  {
    NS_LOG_INFO("FailSafe Enabled");
    Ptr <Node> PtrNode = this->GetNode();
    Ptr<Ipv4> ipv4 = PtrNode->GetObject<Ipv4> ();
    Ipv4InterfaceAddress iaddr = ipv4->GetAddress (1,0); 
    Ipv4Address ipAddr = iaddr.GetLocal ();
    std::cout << "FailSafe Enabled at " << Simulator::Now().GetMicroSeconds() << "us in slave " << ipAddr << " due to " << Reason << std::endl;
    m_failsafeTrace = true;
  }
}

void 
SAFETYUdpEchoServer::HandleRead (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);

  Ptr<Packet> packet;
  Address from;
  Address localAddress;
  while ((packet = socket->RecvFrom (from)))
    {
      socket->GetSockName (localAddress);
      m_rxTrace (packet);
      m_rxTraceWithAddresses (packet, from, localAddress);
      if (InetSocketAddress::IsMatchingType (from))
        {
          NS_LOG_INFO ("At time " << Simulator::Now ().As (Time::S) << " server received " << packet->GetSize () << " bytes from " <<
                       InetSocketAddress::ConvertFrom (from).GetIpv4 () << " port " <<
                       InetSocketAddress::ConvertFrom (from).GetPort ());
        }
      else if (Inet6SocketAddress::IsMatchingType (from))
        {
          NS_LOG_INFO ("At time " << Simulator::Now ().As (Time::S) << " server received " << packet->GetSize () << " bytes from " <<
                       Inet6SocketAddress::ConvertFrom (from).GetIpv6 () << " port " <<
                       Inet6SocketAddress::ConvertFrom (from).GetPort ());
        }

      DestroyWdtExpired();
      FailSafeTag tagRcv;
      packet->PeekPacketTag(tagRcv);  
      if(tagRcv.GetFailsafeValue())
      {
        EnableFailSafe("Trigger");
      }

      uint32_t seqnumber;
      SeqnumTag seqnumtagRcv;
      packet->PeekPacketTag(seqnumtagRcv);
      seqnumber = seqnumtagRcv.GetSeqNumValue();

      packet->RemoveAllPacketTags ();
      packet->RemoveAllByteTags ();

      struct DataPacket dataPacket;
      dataPacket.timeStamp = (uint32_t) Simulator::Now().GetMilliSeconds();
      dataPacket.seqnum = (uint16_t) seqnumber;
      dataPacket.safeTag = (uint8_t) m_failsafeTrace;
      
      uint8_t* m_data = new uint8_t[sizeof(dataPacket)];
      uint32_t m_dataSize = sizeof(dataPacket);
  
      memcpy (m_data, &dataPacket, sizeof(dataPacket));
      m_dataSize = sizeof(dataPacket);

      packet = Create<Packet> (m_data, m_dataSize);

      FailSafeTag tag;
      tag.SetFailsafeValue(m_failsafeTrace);
      packet->AddPacketTag(tag);

      SeqnumTag seqnumtag;
      seqnumtag.SetSeqNumValue(seqnumber);
      packet->AddPacketTag(seqnumtag);

      NS_LOG_LOGIC ("Echoing packet");
      socket->SendTo (packet, 0, from);
      if (Simulator::Now() > Seconds(2))
      {
        ScheduleWdtExpired();
      }

      if (InetSocketAddress::IsMatchingType (from))
        {
          NS_LOG_INFO ("At time " << Simulator::Now ().As (Time::S) << " server sent " << packet->GetSize () << " bytes to " <<
                       InetSocketAddress::ConvertFrom (from).GetIpv4 () << " port " <<
                       InetSocketAddress::ConvertFrom (from).GetPort ());
        }
      else if (Inet6SocketAddress::IsMatchingType (from))
        {
          NS_LOG_INFO ("At time " << Simulator::Now ().As (Time::S) << " server sent " << packet->GetSize () << " bytes to " <<
                       Inet6SocketAddress::ConvertFrom (from).GetIpv6 () << " port " <<
                       Inet6SocketAddress::ConvertFrom (from).GetPort ());
        }
    }
}

} // Namespace ns3
