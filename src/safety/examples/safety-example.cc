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
 */

// Network topology
//
//       n0    n1   n2   n3
//       |     |    |    |
//       =================
//              LAN
//
// - UDP flows from n0 to n1 and back
// - DropTail queues
// - Tracing of queues and packet receptions to file "udp-echo.tr"

#include <fstream>
#include "ns3/core-module.h"
#include "ns3/csma-module.h"
#include "ns3/applications-module.h"
#include "ns3/internet-module.h"

#include "ns3/command-line.h"
#include "ns3/config.h"
#include "ns3/string.h"
#include "ns3/log.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/ssid.h"
#include "ns3/mobility-helper.h"
#include "ns3/on-off-helper.h"
#include "ns3/yans-wifi-channel.h"
#include "ns3/mobility-model.h"
#include "ns3/packet-sink.h"
#include "ns3/packet-sink-helper.h"
#include "ns3/tcp-westwood.h"
#include "ns3/internet-stack-helper.h"
#include "ns3/ipv4-address-helper.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/file-helper.h"
#include "ns3/safety-udp-echo-helper.h"
#include "ns3/names.h"
#include "ns3/stats-module.h"
#include "ns3/failsafeManager.h"
#include "ns3/random-variable-stream.h"
#include "ns3/wifi-standards.h"
#include "ns3/wifi-net-device.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("SafetyUdpEchoExample");

int
main (int argc, char *argv[])
{
  double simulationTime = 10; /* Simulation time in seconds. */
  uint8_t mcs = 0; /* Modulation and Coding Scheme. */
  std::string wifiStandard = "n";
  double frequency = 2.4; /* Frequency in GHz. */
  double BreakpointDistance = 10; // Default for TGn model D

  // SeedManager::SetSeed (3);  // Changes seed from default of 1 to 3
  // SeedManager::SetRun (7);  // Changes run number from default of 1 to 7

  //
  // Users may find it convenient to turn on explicit debugging
  // for selected modules; the below lines suggest how to do this
  //
  LogComponentEnable ("SafetyUdpEchoExample", LOG_LEVEL_INFO);
  LogComponentEnable ("SAFETYUdpEchoClientApplication", LOG_LEVEL_INFO);
  //LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO);
  //LogComponentEnable ("IEEEPropagationLossModel", LOG_LEVEL_DEBUG);
#if 0
#endif
  //
  // Allow the user to override any of the defaults and the above Bind() at
  // run-time, via command-line arguments
  //
  bool useV6 = false;
  uint8_t numberOfSalves = 5;
  Address serverAddress;

  CommandLine cmd (__FILE__);
  cmd.AddValue ("useIpv6", "Use Ipv6", useV6);
  cmd.AddValue ("mcs", "MCS index", mcs);
  cmd.AddValue ("wifiStandard", "Wifi Standard (n,ac,ax)", wifiStandard);
  cmd.AddValue ("frequency", "Frequency in GHz", frequency);
  cmd.AddValue ("BreakpointDistance", "Breakpoint distance for TGn models (Default: TGn model D 10m)", BreakpointDistance);
  cmd.Parse (argc, argv);

  uint8_t nStreams = 1 + (mcs / 8); //number of MIMO streams

  std::string mcs_pre;
  WifiStandard standard; 
  if (wifiStandard == "n")
  {
    mcs_pre = "HtMcs";
    standard = WIFI_STANDARD_80211n;
  }
  else if (wifiStandard == "ac")
  {
    mcs_pre = "VhtMcs";
    standard = WIFI_STANDARD_80211ac;
    mcs = mcs%8;
  }
  else if (wifiStandard == "ax")
  {
    mcs_pre = "HeMcs";
    standard = WIFI_STANDARD_80211ax;
    mcs = mcs%8;
  }
  else
  {
    NS_FATAL_ERROR ("Wifi type not supported");
  }

  std::ostringstream oss;
  oss << mcs_pre << (uint16_t) mcs;
  std::string phyRateData = oss.str ();

  std::ostringstream oss1;
  oss1 << mcs_pre << (uint16_t) 0;
  std::string phyRateControl = oss1.str ();  

  std::cout << "Standard: " << wifiStandard << ", MCS index: " << phyRateData << ", MIMO streams: " << (uint16_t) nStreams << ", Frequeency: " << frequency << "Ghz, Breakpoint distance: " << BreakpointDistance << "m" << std::endl;
  //
  // Explicitly create the nodes required by the topology (shown above).
  //
  NS_LOG_INFO ("Create nodes.");
  NodeContainer ap;
  ap.Create (1);

  NodeContainer stas;
  stas.Create (numberOfSalves);

  NS_LOG_INFO ("Create channels.");

  WifiMacHelper wifiMac;
  WifiHelper wifiHelper;
  wifiHelper.SetStandard (standard);

  /* Set up Legacy Channel */
  YansWifiChannelHelper wifiChannel;
  wifiChannel.SetPropagationDelay ("ns3::ConstantSpeedPropagationDelayModel");
  wifiChannel.AddPropagationLoss ("ns3::IEEEPropagationLossModel", "Frequency",
                                  DoubleValue (frequency*1e9), "BreakpointDistance", DoubleValue (BreakpointDistance));

  /* Setup Physical Layer */
  YansWifiPhyHelper wifiPhy;
  wifiPhy.SetChannel (wifiChannel.Create ());
  wifiPhy.SetErrorRateModel ("ns3::YansErrorRateModel");
  wifiHelper.SetRemoteStationManager ("ns3::ConstantRateWifiManager", "DataMode",
                                      StringValue (phyRateData), "ControlMode", StringValue (phyRateControl));

  // Set MIMO capabilities
  wifiPhy.Set ("Antennas", UintegerValue (nStreams));
  wifiPhy.Set ("MaxSupportedTxSpatialStreams", UintegerValue (nStreams));
  wifiPhy.Set ("MaxSupportedRxSpatialStreams", UintegerValue (nStreams));

  Ptr<Node> apWifiNode = ap.Get (0);
  //Ptr<Node> staWifiNode = n.Ge;

  /* Configure AP */
  Ssid ssid = Ssid ("network");
  wifiMac.SetType ("ns3::ApWifiMac", "Ssid", SsidValue (ssid));

  NetDeviceContainer apDevice;
  apDevice = wifiHelper.Install (wifiPhy, wifiMac, apWifiNode);

  /* Configure STA */
  wifiMac.SetType ("ns3::StaWifiMac", "Ssid", SsidValue (ssid));

  NetDeviceContainer staDevices;
  staDevices = wifiHelper.Install (wifiPhy, wifiMac, stas);

  Ptr<NetDevice> ndSta = staDevices.Get (0);
  Ptr<WifiNetDevice> wndSta = ndSta->GetObject<WifiNetDevice> ();
  Ptr<WifiPhy> wifiPhyPtrSta = wndSta->GetPhy ();
  uint8_t channelWidth = wifiPhyPtrSta->GetChannelWidth ();

  if (wifiStandard == "ac" && mcs == 6 && channelWidth == 80 && nStreams == 3)
  {
    std::cout << "VHT MCS " << +mcs << " forbidden at " << +channelWidth << " MHz when NSS is " << +nStreams << std::endl;
    std::cout << "FailSafe Enabled at 0us in Fake 0.0.0.0 due to Fake" << std::endl;
    std::cout << "FailSafe Enabled at 0us in Fake 0.0.0.0 due to Fake" << std::endl;
    return 0;
  }

  /* Mobility model */
  MobilityHelper mobility;

  // setup the grid itself: objects are layed out
  // started from (-100,-100) with 20 objects per row,
  // the x interval between each object is 5 meters
  // and the y interval between each object is 20 meters
  mobility.SetPositionAllocator ("ns3::GridPositionAllocator", "MinX", DoubleValue (10.0), "MinY",
                                 DoubleValue (0.0), "DeltaX", DoubleValue (15.0), "DeltaY",
                                 DoubleValue (20.0), "GridWidth", UintegerValue (20), "LayoutType",
                                 StringValue ("RowFirst"));
  // each object will be attached a static position.
  // i.e., once set by the "position allocator", the
  // position will never change.
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  // finalize the setup by attaching to each object
  // in the input array a position and initializing
  // this position with the calculated coordinates.
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

  //mobility.Install (stas);

  /* Internet stack */
  InternetStackHelper stack;
  stack.Install (ap);
  stack.Install (stas);

  Ipv4AddressHelper address;
  address.SetBase ("10.0.0.0", "255.255.255.0");
  Ipv4InterfaceContainer apInterface;
  apInterface = address.Assign (apDevice);
  Ipv4InterfaceContainer staInterface;
  staInterface = address.Assign (staDevices);

  serverAddress = Address (staInterface.GetAddress (0));

  /* Populate routing table */
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  NS_LOG_INFO ("Create Applications.");

  Ptr<UniformRandomVariable> random = CreateObject<UniformRandomVariable> ();
  uint32_t triggerNode = random->GetInteger (0, numberOfSalves);
  double triggerTime = random->GetValue (1.0, simulationTime);
  std::cout << triggerNode << " " << triggerTime << std::endl;
  //
  // Create a UdpEchoServer application on node one.
  //
  uint16_t port = 9; // well-known echo port number
  SAFETYUdpEchoServerHelper server (port);
  ApplicationContainer apps = server.Install (stas);
  apps.Start (Seconds (1.0));
  apps.Stop (Seconds (simulationTime));
  if (triggerNode >= 1)
    {
      apps.Get (triggerNode - 1)->SetAttribute ("IsFailsafeSource", UintegerValue (true));
      apps.Get (triggerNode - 1)
          ->SetAttribute ("FailSafeTriggerTime", TimeValue (Seconds (triggerTime)));
    }

  //
  // Create a UdpEchoClient application to send UDP datagrams from node zero to
  // node one.
  //
  uint32_t packetSize = 8;
  uint32_t maxPacketCount = 100000000;
  Time interPacketInterval = Seconds (1.);
  uint32_t nNodes = staInterface.GetN ();
  //FailSafeManager failSafeManager;
  Ptr<FailSafeManager> failSafeManager = CreateObject<FailSafeManager> ();
  Names::Add ("/Names/FailSafe", failSafeManager);
  for (uint32_t i = 0; i < nNodes; ++i)
    {
      SAFETYUdpEchoClientHelper client (Address (staInterface.GetAddress (i)), port);
      client.SetAttribute ("MaxPackets", UintegerValue (maxPacketCount));
      client.SetAttribute ("Interval", TimeValue (interPacketInterval));
      client.SetAttribute ("PacketSize", UintegerValue (packetSize));
      if (triggerNode == 0)
        {
          client.SetAttribute ("IsFailsafeSource", UintegerValue (true));
          client.SetAttribute ("FailSafeTriggerTime", TimeValue (Seconds (triggerTime)));
        }
      apps = client.Install (ap);
      client.SetFailSafeManager (apps.Get (0), failSafeManager);
      apps.Start (Seconds (1.0));
      apps.Stop (Seconds (simulationTime));
    }

#if 0
//
// Users may find it convenient to initialize echo packets with actual data;
// the below lines suggest how to do this
//
  client.SetFill (apps.Get (0), "Hello World");

  client.SetFill (apps.Get (0), 0xa5, 1024);

  uint8_t fill[] = { 0, 1, 2, 3, 4, 5, 6};
  client.SetFill (apps.Get (0), fill, sizeof(fill), 1024);
#endif

  //AsciiTraceHelper ascii;
  //wifiPhy.EnableAsciiAll (ascii.CreateFileStream ("udp-echo.tr"));
  wifiPhy.EnablePcap ("udp-echo", ap, false);

  FileHelper fileHelper;
  fileHelper.ConfigureFile ("Rtt", FileAggregator::TAB_SEPARATED);
  //fileHelper.WriteProbe ("ns3::PacketProbe", "/NodeList/*/$ns3::UdpEchoClient/Tx", "Output");
  fileHelper.WriteProbe ("ns3::DoubleProbe",
                         "/NodeList/*/ApplicationList/*/$ns3::SAFETYUdpEchoClient/Rtt", "Output");

  FileHelper fileHelper_FS_Master;
  fileHelper_FS_Master.ConfigureFile ("MasterFailsafe", FileAggregator::TAB_SEPARATED);
  //fileHelper_FS_Master.WriteProbe ("ns3::PacketProbe", "/NodeList/*/$ns3::UdpEchoClient/Tx", "Output");
  fileHelper_FS_Master.WriteProbe ("ns3::Uinteger8Probe", "/Names/FailSafe/FailSafeSignal",
                                   "Output");

  FileHelper fileHelper_FS_Slave;
  fileHelper_FS_Slave.ConfigureFile ("SlaveFailsafe", FileAggregator::TAB_SEPARATED);
  //fileHelper_FS_Slave.WriteProbe ("ns3::PacketProbe", "/NodeList/*/$ns3::UdpEchoClient/Tx", "Output");
  fileHelper_FS_Slave.WriteProbe (
      "ns3::Uinteger8Probe", "/NodeList/*/ApplicationList/*/$ns3::SAFETYUdpEchoServer/FailSafeSignal",
      "Output");

  // Create the gnuplot helper.
  GnuplotHelper plotHelper;
  // Configure the plot.
  plotHelper.ConfigurePlot ("gnuplot-rtt", "Polling time vs. Time", "Time (Seconds)",
                            "Polling time (us)", "pdf");
  // Plot the values generated by the probe.  The path that we provide
  // helps to disambiguate the source of the trace.
  plotHelper.PlotProbe ("ns3::DoubleProbe",
                        "/NodeList/*/ApplicationList/*/$ns3::SAFETYUdpEchoClient/Rtt", "Output",
                        "Polling time", GnuplotAggregator::KEY_INSIDE);
  //fileHelper.WriteProbe ("ns3::ApplicationPacketProbe", "/NodeList/*/ApplicationList/*/$ns3::SAFETYUdpEchoClient/Tx", "OutputBytes");
  //
  // Now, do the actual simulation.
  //
  NS_LOG_INFO ("Run Simulation.");
  Simulator::Stop (Seconds (simulationTime + 1));
  Simulator::Run ();
  Simulator::Destroy ();
  NS_LOG_INFO ("Done.");
}
