/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
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
 * This simulation shows how the tas-queue-disc can be configured and used
 */
/*
 * 2020-08 (Aug 2020)
 * Author(s):
 *  - Implementation:
 *       Luca Wendling <lwendlin@rhrk.uni-kl.de>
 *  - Design & Implementation:
 *       Dennis Krummacker <dennis.krummacker@gmail.com>
 */

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/ipv4-global-routing-helper.h"

#include "ns3/tsn-module.h"
#include "ns3/traffic-control-module.h"
#include "ns3/config-store-module.h"

#include "ns3/yans-wifi-helper.h"
#include "ns3/ssid.h"
#include "ns3/mobility-helper.h"
#include "ns3/yans-wifi-channel.h"
#include "ns3/mobility-model.h"
#include "ns3/wifi-standards.h"
#include "ns3/wifi-net-device.h"

#include "stdio.h"
#include <array>
#include <string>
#include <chrono>

#define NUMBER_OF_NODES 4
#define DATA_PAYLOADE_SIZE 100
#define TRANSMISON_SPEED "2Mbps" // FIXME: If I use the actual wifi speed, the some packets are missing in the pcap

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("Tsn-exemple-wifi");

Time callbackfunc();

int32_t ipv4PacketFilter(Ptr<QueueDiscItem> item);
int32_t ipv4PacketFilterSer(Ptr<QueueDiscItem> item);

int
main (int argc, char *argv[])
{
  CommandLine cmd;
  cmd.Parse (argc, argv);

  Time::SetResolution (Time::NS);
  Time sendPeriod,scheduleDuration,simulationDuration;
  scheduleDuration = Seconds(1);
  simulationDuration = 10*scheduleDuration;
  sendPeriod = scheduleDuration/4;
  sendPeriod -= MicroSeconds(1);
  
  // LogComponentEnable("TasQueueDisc",LOG_LEVEL_LOGIC);
  // LogComponentEnable("TransmissonGateQdisc",LOG_LEVEL_LOGIC);
  // LogComponentEnable("QueueDisc",LOG_LEVEL_LOGIC);
  LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
  LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO);

  NS_LOG_INFO ("Create nodes.");
  NodeContainer ap;
  ap.Create (1);

  NodeContainer stas;
  stas.Create (1);

  NS_LOG_INFO ("Create channels.");

  WifiMacHelper wifiMac;
  WifiHelper wifiHelper;
  wifiHelper.SetStandard (WIFI_STANDARD_80211n);

  /* Set up Legacy Channel */
  YansWifiChannelHelper wifiChannel;
  wifiChannel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
  wifiChannel.AddPropagationLoss ("ns3::FriisPropagationLossModel", "Frequency",
                                  DoubleValue (2.4*1e9));

  /* Setup Physical Layer */
  YansWifiPhyHelper wifiPhy;
  wifiPhy.SetChannel (wifiChannel.Create ());
  wifiPhy.SetErrorRateModel ("ns3::YansErrorRateModel");
  wifiHelper.SetRemoteStationManager ("ns3::ConstantRateWifiManager", "DataMode",
                                      StringValue ("HtMcs7"), "ControlMode", StringValue ("HtMcs0"));

  // Set MIMO capabilities
  wifiPhy.Set ("Antennas", UintegerValue (1));
  wifiPhy.Set ("MaxSupportedTxSpatialStreams", UintegerValue (1));
  wifiPhy.Set ("MaxSupportedRxSpatialStreams", UintegerValue (1));

  Ptr<Node> apWifiNode = ap.Get (0);

  /* Configure AP */
  Ssid ssid = Ssid ("network");
  wifiMac.SetType ("ns3::ApWifiMac", "Ssid", SsidValue (ssid));

  NetDeviceContainer apDevice;
  apDevice = wifiHelper.Install (wifiPhy, wifiMac, apWifiNode);

  /* Configure STA */
  wifiMac.SetType ("ns3::StaWifiMac", "Ssid", SsidValue (ssid));

  NetDeviceContainer staDevices;
  staDevices = wifiHelper.Install (wifiPhy, wifiMac, stas);

  /* Mobility model */
  MobilityHelper mobility;

  mobility.SetPositionAllocator ("ns3::GridPositionAllocator", "MinX", DoubleValue (10.0), "MinY",
                                 DoubleValue (0.0), "DeltaX", DoubleValue (15.0), "DeltaY",
                                 DoubleValue (20.0), "GridWidth", UintegerValue (20), "LayoutType",
                                 StringValue ("RowFirst"));

  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (stas);

  // iterate our nodes and print their position.
  for (NodeContainer::Iterator j = stas.Begin (); j != stas.End (); ++j)
    {
      Ptr<Node> object = *j;
      Ptr<MobilityModel> position = object->GetObject<MobilityModel> ();
      NS_ASSERT (position != 0);
      Vector pos = position->GetPosition ();
      std::cout << "x=" << pos.x << ", y=" << pos.y << ", z=" << pos.z << std::endl;
    }

  Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
  positionAlloc->Add (Vector (0.0, 0.0, 0.0));
  positionAlloc->Add (Vector (0.0, 0.0, 0.0));

  mobility.SetPositionAllocator (positionAlloc);
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (apWifiNode);

  Ptr<MobilityModel> position = apWifiNode->GetObject<MobilityModel> ();
  NS_ASSERT (position != 0);
  Vector pos = position->GetPosition ();
  std::cout << "Master x=" << pos.x << ", y=" << pos.y << ", z=" << pos.z << std::endl;

  InternetStackHelper stack;
  stack.Install (ap);
  stack.Install (stas);

  CallbackValue timeSource = MakeCallback(&callbackfunc);

  TsnHelper tsnHelperServer,tsnHelperClient;
  NetDeviceListConfig schedulePlanServer,schedulePlanClient;

  schedulePlanClient.Add(scheduleDuration,{1,1,1,1,1,1,1,1});  // FIXME: We need to keep queue 1 always open otherwise ARP does not work
  schedulePlanClient.Add(scheduleDuration,{0,1,0,0,0,0,0,0});

  schedulePlanServer.Add(scheduleDuration,{0,1,0,0,0,0,0,0});
  schedulePlanServer.Add(scheduleDuration,{1,1,1,1,1,1,1,1});


  tsnHelperClient.SetRootQueueDisc("ns3::TasQueueDisc", "NetDeviceListConfig", NetDeviceListConfigValue(schedulePlanClient), "TimeSource", timeSource,"DataRate", StringValue (TRANSMISON_SPEED));
  tsnHelperClient.AddPacketFilter(0,"ns3::TsnIpv4PacketFilter","Classify",CallbackValue(MakeCallback(&ipv4PacketFilter)));

  tsnHelperServer.SetRootQueueDisc("ns3::TasQueueDisc", "NetDeviceListConfig", NetDeviceListConfigValue(schedulePlanServer), "TimeSource", timeSource,"DataRate", StringValue (TRANSMISON_SPEED));
  tsnHelperServer.AddPacketFilter(0,"ns3::TsnIpv4PacketFilter","Classify",CallbackValue(MakeCallback(&ipv4PacketFilterSer)));



  Ipv4AddressHelper address;
  address.SetBase ("0.0.0.0", "255.255.255.0");
  Ipv4InterfaceContainer apInterface;
  apInterface = address.Assign (apDevice);
  Ipv4InterfaceContainer staInterface;
  staInterface = address.Assign (staDevices);

  Ipv4Address multicastSource ("10.1.1.1");
  Ipv4Address multicastGroup ("225.1.2.4");

  /* Populate routing table */
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();


  uint16_t port = 9; // well-known echo port number
  UdpEchoServerHelper server (port);
  ApplicationContainer apps = server.Install (stas);
  apps.Start (Seconds (0));
  apps.Stop (simulationDuration);


  uint32_t nNodes = staInterface.GetN ();
  for (uint32_t i = 0; i < nNodes; ++i)
  {
    UdpEchoClientHelper client (Address (staInterface.GetAddress (i)), port);
    client.SetAttribute ("MaxPackets", UintegerValue (simulationDuration.GetInteger()/sendPeriod.GetInteger()));
    client.SetAttribute ("Interval", TimeValue (sendPeriod));
    client.SetAttribute ("PacketSize", UintegerValue (DATA_PAYLOADE_SIZE));
    apps = client.Install (ap);
    apps.Start (Seconds (0));
    apps.Stop (simulationDuration);
  }

  tsnHelperServer.Uninstall (staDevices.Get(0));
  tsnHelperClient.Uninstall (apDevice.Get(0));
  QueueDiscContainer qdiscsServer = tsnHelperServer.Install (staDevices.Get(0));
  QueueDiscContainer qdiscsClient1 = tsnHelperClient.Install (apDevice.Get(0));


  wifiPhy.EnablePcap ("tas-exemple-wifi-ap", ap, true);
  wifiPhy.EnablePcap ("tas-exemple-wifi-stas", stas, true);
  //wifiPhy.EnablePcapAll("tas-test-wifi");

  Simulator::Stop (simulationDuration + Seconds (1));
  std::chrono::time_point<std::chrono::high_resolution_clock> start = std::chrono::high_resolution_clock::now();
  Simulator::Run ();
  std::chrono::time_point<std::chrono::high_resolution_clock> stop = std::chrono::high_resolution_clock::now();

  Simulator::Destroy ();
  std::cout << "Tsn-exemple-wifi " << std::endl;
  std::cout << NUMBER_OF_NODES << " Nodes " << std::endl;
  std::cout << "Total simulated Time: "<< simulationDuration.GetSeconds() << "s" << std::endl;
  std::cout << "Expectated number of packages in pcap: " << 2*simulationDuration.GetInteger()/sendPeriod.GetInteger() << std::endl;
  std::cout << "Schedule duration: "<< scheduleDuration.GetSeconds() <<" Sec Execution Time " << std::chrono::duration_cast<std::chrono::milliseconds>(stop-start).count() << " ms" << std::endl;
  std::cout << "Simulation Rate: "<< simulationDuration.GetMilliSeconds() / std::chrono::duration_cast<std::chrono::milliseconds>(stop-start).count()<< std::endl;
  return 0;
}

int32_t ipv4PacketFilter(Ptr<QueueDiscItem> item){
  return 4;
}

int32_t ipv4PacketFilterSer(Ptr<QueueDiscItem> item){
  return 5;
}
Time callbackfunc(){
  return Simulator::Now();
}