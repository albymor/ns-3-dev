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

#include "ns3/safety-module.h"

#include "ns3/file-helper.h"

#include "stdio.h"
#include <array>
#include <string>
#include <chrono>
#include <map>

#include <iostream>
#include <fstream>

#define DATA_PAYLOADE_SIZE 8
#define TRANSMISON_SPEED "52Mbps" // FIXME: If I use the actual wifi speed, the some packets are missing in the pcap

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("Tsn-exemple-wifi");

Time callbackfunc ();

int32_t ipv4PacketFilter0  (Ptr<QueueDiscItem> item);
int32_t ipv4PacketFilter1 (Ptr<QueueDiscItem> item);
int32_t ipv4PacketFilter2 (Ptr<QueueDiscItem> item);
int32_t ipv4PacketFilter3 (Ptr<QueueDiscItem> item);
int32_t ipv4PacketFilter4 (Ptr<QueueDiscItem> item);
int32_t ipv4PacketFilter5 (Ptr<QueueDiscItem> item);

std::map<uint32_t, int64_t> gig;

std::ofstream CycleTimeFile;
std::ofstream RttTimeFile;


void TxTrace (std::string context, Ptr <const Packet> packet , const Address & from, const Address & to)
{
  //std::cout << "Tx," << Now().GetMicroSeconds() << " "<<  InetSocketAddress::ConvertFrom (from).GetIpv4().Get() << " "<< InetSocketAddress::ConvertFrom (to).GetIpv4().Get() << std::endl;

  uint32_t dev = InetSocketAddress::ConvertFrom (to).GetIpv4().Get();

  int64_t time = Now().GetMicroSeconds();

  CycleTimeFile << "cycle " << Now().GetSeconds() << " " << dev <<" " <<  time - gig[dev] << std::endl;
  gig[dev] = time;
}
void RxTrace (std::string context, Ptr <const Packet> packet , const Address & from, const Address & to)
{
  // your code to run when UdpEchoClient receives a packet
  //std::cout << "Rx," << Now().GetNanoSeconds() << " "<<  InetSocketAddress::ConvertFrom (from).GetIpv4() << " "<< InetSocketAddress::ConvertFrom (to).GetIpv4() << std::endl;

  int64_t time = Now().GetMicroSeconds();
  uint32_t dev = InetSocketAddress::ConvertFrom (from).GetIpv4().Get();

  RttTimeFile << "rtt " << Now().GetSeconds() << " " << dev <<" " <<  time - gig[dev] << std::endl;
}

void
PhyTxDrop(Ptr<const Packet> p)
{
  std::cout << " PhyTxDrop "<< std::endl;
}

void
PhyRxDrop(Ptr<const Packet> p, WifiPhyRxfailureReason reason)
{
  int64_t time = Now().GetMicroSeconds();
  std::cout << time << " PhyRxDrop " << reason << std::endl;

}

void
MacTxDrop(Ptr<const Packet> p)
{
  std::cout << " MacTXDrop "<< std::endl;
}

void
MacRxDrop(Ptr<const Packet> p)
{

  std::cout << " MacRXDrop "<< std::endl;

}

Ptr<WifiPhyStateHelper>
GetWifiPhyStateHelper(Ptr<NetDevice> device)
{
    Ptr<WifiNetDevice> wnd = device->GetObject<WifiNetDevice> ();
    Ptr<WifiPhy> wifiPhyPtr = wnd->GetPhy ();
    Ptr<WifiPhyStateHelper> m_state = wifiPhyPtr->GetState();
    return m_state;
}


int
main (int argc, char *argv[])
{
  CycleTimeFile.open ("CycleTime.txt");
  RttTimeFile.open ("RttTime.txt");
  double simulationTime = 10; /* Simulation time in seconds. */
  uint8_t mcs = 7; /* Modulation and Coding Scheme. */
  std::string wifiStandard = "ac";
  double frequency = 5; /* Frequency in GHz. */
  bool tsn_enabled = 1;
  double BreakpointDistance = 10; // Default for TGn model D
  bool RandomPositions = true;
  uint32_t TsnCycle = 3000; //us
  uint32_t app_start_time = 1000000; //us
  float safety_wdt = 250; //ms



  bool useV6 = false;
  uint8_t numberOfSlaves = 5;
  Address serverAddress;

  CommandLine cmd (__FILE__);
  cmd.AddValue ("useIpv6", "Use Ipv6", useV6);
  cmd.AddValue ("mcs", "MCS index", mcs);
  cmd.AddValue ("wifiStandard", "Wifi Standard (n,ac,ax)", wifiStandard);
  cmd.AddValue ("frequency", "Frequency in GHz", frequency);
  cmd.AddValue ("tsn_enabled", "enable TSN", tsn_enabled);
  cmd.AddValue ("BreakpointDistance", "Breakpoint distance for TGn models (Default: TGn model D 10m)", BreakpointDistance);
  cmd.AddValue ("RandomPositions", "Randomize positions of nodes", RandomPositions);
  cmd.AddValue ("NumberOfSlaves", "Number of slaves", numberOfSlaves);
  cmd.AddValue ("TsnCycle", "Tsn Cycle in us", TsnCycle);
  cmd.AddValue ("Wdt", "Safety watchdog in ms", safety_wdt);
  cmd.Parse (argc, argv);


  uint32_t relative_start_time = app_start_time % TsnCycle;
  app_start_time = app_start_time + (TsnCycle - relative_start_time);

  std::cout << "App start time: " << app_start_time << " Relative " << relative_start_time << " cicle " << TsnCycle <<  std::endl;

  uint8_t nStreams = 1 + (mcs / 8); //number of MIMO streams
  std::map<uint8_t, std::string> tx_speed{{0,"29.3Mbps"},
                                          {1,"58.5Mbps"},
                                          {2,"87.8Mbps"},
                                          {3,"117Mbps"},
                                          {4,"175.5Mbps"},
                                          {5,"234Mbps"},
                                          {6,"263.3Mbps"},
                                          {7,"292.5Mbps"}};

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

  std::cout << "Standard: " << wifiStandard << ", MCS index: " << phyRateData << ", Speed: " << tx_speed[mcs] << ", MIMO streams: " << (uint16_t) nStreams << ", Frequeency: " << frequency << "Ghz, TSN enabled: " << +tsn_enabled  <<", Breakpoint distance: " << BreakpointDistance << "m, Num slaves; " << +numberOfSlaves << ", Tsn Cycle: " << TsnCycle << ", Wdt: " << safety_wdt << "ms" << std::endl;

  Time::SetResolution (Time::NS);
  Time sendPeriod, scheduleDuration, simulationDuration;
  scheduleDuration = MicroSeconds (TsnCycle/2);
  simulationDuration = Seconds (simulationTime);
  sendPeriod = scheduleDuration * 2;
  //sendPeriod -= MicroSeconds(1);

  // LogComponentEnable("TasQueueDisc",LOG_LEVEL_LOGIC);
  // LogComponentEnable("TransmissonGateQdisc",LOG_LEVEL_LOGIC);
  // LogComponentEnable("QueueDisc",LOG_LEVEL_LOGIC);
  // LogComponentEnable ("SAFETYUdpEchoServerApplication", LOG_LEVEL_INFO);
  // LogComponentEnable ("SAFETYUdpEchoClientApplication", LOG_LEVEL_INFO);

  NS_LOG_INFO ("Create nodes.");
  NodeContainer ap;
  ap.Create (1);

  NodeContainer stas;
  stas.Create (numberOfSlaves);

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
  //wifiPhy.SetErrorRateModel ("ns3::YansErrorRateModel");
  wifiHelper.SetRemoteStationManager ("ns3::ConstantRateWifiManager", "DataMode",
                                      StringValue (phyRateData), "ControlMode",
                                      StringValue (phyRateControl));

  // Set MIMO capabilities
  wifiPhy.Set ("Antennas", UintegerValue (nStreams));
  wifiPhy.Set ("MaxSupportedTxSpatialStreams", UintegerValue (nStreams));
  wifiPhy.Set ("MaxSupportedRxSpatialStreams", UintegerValue (nStreams));
  //wifiPhy.Set("CcaEdThreshold", DoubleValue(-0.0));
  wifiPhy.SetPcapDataLinkType (YansWifiPhyHelper::DLT_IEEE802_11_RADIO);

  Ptr<Node> apWifiNode = ap.Get (0);

  /* Configure AP */
  Ssid ssid = Ssid ("network");
  wifiMac.SetType ("ns3::ApWifiMac", "Ssid", SsidValue (ssid), "BeaconGeneration", BooleanValue (true));

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


  if (RandomPositions)
  {
    ObjectFactory pos;
    pos.SetTypeId ("ns3::RandomRectanglePositionAllocator");
    pos.Set ("X", StringValue ("ns3::UniformRandomVariable[Min=0.0|Max=43.0]"));
    pos.Set ("Y", StringValue ("ns3::UniformRandomVariable[Min=0.0|Max=50.0]"));

    Ptr<PositionAllocator> taPositionAlloc = pos.Create ()->GetObject<PositionAllocator> ();
    mobility.SetPositionAllocator (taPositionAlloc);
  }
  else
  {
    mobility.SetPositionAllocator ("ns3::GridPositionAllocator", "MinX", DoubleValue (10.0), "MinY",
                                  DoubleValue (0.0), "DeltaX", DoubleValue (1.0), "DeltaY",
                                  DoubleValue (1.0), "GridWidth", UintegerValue (20), "LayoutType",
                                  StringValue ("RowFirst"));
  }

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
  positionAlloc->Add (Vector (43/2, 50/2, 0.0));

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

  std::vector<NetDeviceListConfig> schedulePlanServerArray;
  std::vector<TsnHelper> tsnHelperServerArray(numberOfSlaves);

  TsnHelper tsnHelperClient;
  if (tsn_enabled)
  {

    //Ptr<WifiPhyStateHelper> m_state = wifiPhyPtrSta->GetState();
    Ptr<WifiPhyStateHelper> m_state = GetWifiPhyStateHelper(apDevice.Get(0));

    std::cout << "PHY state " << m_state << std::endl;

    CallbackValue timeSource = MakeCallback (&callbackfunc);

    NetDeviceListConfig schedulePlanClient;

    schedulePlanClient.Add (scheduleDuration, {0, 1, 0, 0, 1, 0, 0, 0}); // FIXME: We need to keep queue 1 always open otherwise ARP does not work
    schedulePlanClient.Add (scheduleDuration, {0,1,0,0,0,0,0,0});

    /* for tests with interfering traffic*/
    // schedulePlanClient.Add (MilliSeconds(5), {0, 1, 0, 0, 1, 0, 0, 0}); // FIXME: We need to keep queue 1 always open otherwise ARP does not work
    // schedulePlanClient.Add (MilliSeconds(3), {0,1,1,0,0,0,0,0});


    std::cout << "> Master" << schedulePlanClient << std::endl;


    for (size_t i = 0; i < numberOfSlaves; i++)
      {
        NetDeviceListConfig schedulePlanServer;
        schedulePlanServer.Add (scheduleDuration + (scheduleDuration / numberOfSlaves) * (i), {0,1,0,0,0,0,0,0});
        schedulePlanServer.Add ((scheduleDuration / numberOfSlaves), {0,1,0,0,1,0,0,0});
        schedulePlanServer.Add ((scheduleDuration / numberOfSlaves) * (numberOfSlaves - (1+ (1 * i))),
                                {0,1,0,0,0,0,0,0});

        /* for tests with interfering traffic*/
        // schedulePlanServer.Add (MilliSeconds(5), {0, 1, 0, 0, 1, 0, 0, 0}); // FIXME: We need to keep queue 1 always open otherwise ARP does not work
        // schedulePlanServer.Add (MilliSeconds(3), {0,1,1,0,0,0,0,0});
        schedulePlanServerArray.push_back(schedulePlanServer);

        std::cout << "> Slave " << +i << schedulePlanServer << std::endl;
      }


    tsnHelperClient.SetRootQueueDisc ("ns3::TasQueueDisc", "NetDeviceListConfig",
                                      NetDeviceListConfigValue (schedulePlanClient), "TimeSource",
                                      timeSource, "DataRate", StringValue (tx_speed[mcs]), "CsmaChannel", PointerValue (m_state));
    tsnHelperClient.AddPacketFilter (0, "ns3::TsnIpv4PacketFilter", "Classify",
                                    CallbackValue (MakeCallback (&ipv4PacketFilter0)));


    for (size_t i = 0; i < numberOfSlaves; i++)
      {
        tsnHelperServerArray[i].SetRootQueueDisc (
            "ns3::TasQueueDisc", "NetDeviceListConfig",
            NetDeviceListConfigValue (schedulePlanServerArray[i]), "TimeSource", timeSource,
            "DataRate", StringValue (tx_speed[mcs]));
        tsnHelperServerArray[i].AddPacketFilter (0, "ns3::TsnIpv4PacketFilter", "Classify",
                                                CallbackValue (MakeCallback (&ipv4PacketFilter1)));
      }
  }



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

  UdpEchoServerHelper dummy (0);

  Ptr<UniformRandomVariable> random = CreateObject<UniformRandomVariable> ();
  uint32_t triggerNode = random->GetInteger (0, numberOfSlaves);
  double triggerTime = random->GetValue (1.0, simulationTime-1.5);
  std::cout << triggerNode << " " << triggerTime << std::endl;

  uint16_t port = 9; // well-known echo port number
  ApplicationContainer StasApps;
  SAFETYUdpEchoServerHelper server (port, safety_wdt);
  StasApps = server.Install (stas);
  StasApps.Start (MicroSeconds (app_start_time));
  StasApps.Stop (simulationDuration);

  if (triggerNode >= 1)
  {
    StasApps.Get (triggerNode - 1)->SetAttribute ("IsFailsafeSource", UintegerValue (true));
    StasApps.Get (triggerNode - 1)
        ->SetAttribute ("FailSafeTriggerTime", TimeValue (Seconds (triggerTime)));
  }

  // uint32_t nNodes = staInterface.GetN ();
  // for (uint32_t i = 0; i < nNodes; ++i)
  // {
  //   SAFETYUdpEchoClientHelper client (Address (staInterface.GetAddress (i)), port);
  //   client.SetAttribute ("MaxPackets", UintegerValue (4294967295u));
  //   client.SetAttribute ("Interval", TimeValue (Seconds(999999999)));
  //   client.SetAttribute ("PacketSize", UintegerValue (DATA_PAYLOADE_SIZE));
  //   apps = client.Install (ap);
  //   apps.Start (Seconds(1));
  //   apps.Stop (simulationDuration);
  // }

  uint32_t packetSize = 8;
  uint32_t maxPacketCount = 100000000;
  Time interPacketInterval;
  if (tsn_enabled)
    {
      interPacketInterval = sendPeriod;
    }
  else
    {
      interPacketInterval = Seconds (0);
    }
  
  uint32_t nNodes = staInterface.GetN ();
  //FailSafeManager failSafeManager;
  ApplicationContainer apps;
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
    client.SetAttribute ("WdtPeriod", TimeValue (MilliSeconds (safety_wdt)));
    apps = client.Install (ap);
    client.SetFailSafeManager (apps.Get (0), failSafeManager);
    if(tsn_enabled)
    {
        apps.Start (MicroSeconds (app_start_time) + ((scheduleDuration / (numberOfSlaves)) * (i)));
    }
    else
    {
      apps.Start (MicroSeconds (app_start_time)); // + ((scheduleDuration / (numberOfSlaves)) * (i)));
    }
    apps.Stop (simulationDuration);
  }

    // uint16_t videoport = 50000;
    // Address hubLocalAddress (InetSocketAddress (Ipv4Address::GetAny (), videoport));
    // PacketSinkHelper packetSinkHelper ("ns3::TcpSocketFactory", hubLocalAddress);
    // ApplicationContainer hubApp = packetSinkHelper.Install (ap);
    // hubApp.Start (Seconds (1.0));
    // hubApp.Stop (Seconds (10.0));

    // std::string flowsDatarate = "10Mbps";
    // uint32_t flowsPacketsSize = 1000;

    // InetSocketAddress socketAddressUp = InetSocketAddress (apInterface.GetAddress (0), videoport);
    // OnOffHelper onOffHelperUp ("ns3::TcpSocketFactory", Address ());
    // onOffHelperUp.SetAttribute ("Remote", AddressValue (socketAddressUp));
    // onOffHelperUp.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
    // onOffHelperUp.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
    // onOffHelperUp.SetAttribute ("PacketSize", UintegerValue (flowsPacketsSize));
    // onOffHelperUp.SetAttribute ("DataRate", StringValue (flowsDatarate));
    // //onOffHelperUp.Install (stas);
    // ApplicationContainer VideoSink = onOffHelperUp.Install (stas);
    // VideoSink.Start (Seconds (1.0));
    // VideoSink.Stop (Seconds (10.0));

  if (tsn_enabled)
  {
    for (size_t i = 0; i < numberOfSlaves; i++)
      {
        tsnHelperServerArray[i].Uninstall (staDevices.Get (i));
        tsnHelperServerArray[i].Install (staDevices.Get (i));
      }

    tsnHelperClient.Uninstall (apDevice.Get (0));
    QueueDiscContainer qdiscsClient1 = tsnHelperClient.Install (apDevice.Get (0));

  }

  wifiPhy.EnablePcap ("tas-exemple-wifi-ap", ap, true);
  //wifiPhy.EnablePcap ("tas-exemple-wifi-stas", stas, true);
  //wifiPhy.EnablePcapAll("tas-test-wifi");

  // FileHelper fileHelper;
  // fileHelper.ConfigureFile ("Rtt", FileAggregator::TAB_SEPARATED);
  // //fileHelper.WriteProbe ("ns3::PacketProbe", "/NodeList/*/$ns3::UdpEchoClient/Tx", "Output");
  // fileHelper.WriteProbe ("ns3::DoubleProbe",
  //                        "/NodeList/*/ApplicationList/*/$ns3::SAFETYUdpEchoClient/Rtt", "Output");



  Config::Connect ("/NodeList/0/ApplicationList/*/$ns3::SAFETYUdpEchoClient/TxWithAddresses", MakeCallback (&TxTrace));
  Config::Connect ("/NodeList/0/ApplicationList/*/$ns3::SAFETYUdpEchoClient/RxWithAddresses", MakeCallback (&RxTrace));
  // Config::ConnectWithoutContext("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Mac/MacTxDrop", MakeCallback(&MacTxDrop));
  // Config::ConnectWithoutContext("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Mac/MacRxDrop", MakeCallback(&MacRxDrop));
  // Config::ConnectWithoutContext("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Phy/PhyRxDrop", MakeCallback(&PhyRxDrop));
  // Config::ConnectWithoutContext("/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Phy/PhyTxDrop", MakeCallback(&PhyTxDrop));


  // FileHelper fileHelper_FS_Master;
  // fileHelper_FS_Master.ConfigureFile ("MasterFailsafe", FileAggregator::TAB_SEPARATED);
  // //fileHelper_FS_Master.WriteProbe ("ns3::PacketProbe", "/NodeList/*/$ns3::UdpEchoClient/Tx", "Output");
  // fileHelper_FS_Master.WriteProbe ("ns3::Uinteger8Probe", "/Names/FailSafe/FailSafeSignal",
  //                                  "Output");

  // FileHelper fileHelper_FS_Slave;
  // fileHelper_FS_Slave.ConfigureFile ("SlaveFailsafe", FileAggregator::TAB_SEPARATED);
  // //fileHelper_FS_Slave.WriteProbe ("ns3::PacketProbe", "/NodeList/*/$ns3::UdpEchoClient/Tx", "Output");
  // fileHelper_FS_Slave.WriteProbe (
  //     "ns3::Uinteger8Probe",
  //     "/NodeList/*/ApplicationList/*/$ns3::SAFETYUdpEchoServer/FailSafeSignal", "Output");

  Simulator::Schedule (Seconds (0), &Ipv4GlobalRoutingHelper::PopulateRoutingTables);
  Simulator::Stop (simulationDuration + Seconds (1));
  std::chrono::time_point<std::chrono::high_resolution_clock> start =
      std::chrono::high_resolution_clock::now ();
  Simulator::Run ();
  std::chrono::time_point<std::chrono::high_resolution_clock> stop =
      std::chrono::high_resolution_clock::now ();

  Simulator::Destroy ();
  CycleTimeFile.close();
  RttTimeFile.close();
  return 0;
}

int32_t
ipv4PacketFilter0 (Ptr<QueueDiscItem> item)
{
  FailSafeTag tagRcv;
  if( item->GetPacket ()->PeekPacketTag (tagRcv))
  {
    //std::cout << "FailSafeTag: " << std::endl;
    return 4;
  }
  else
  {
    //std::cout << "No FailSafeTag: " << std::endl;
    return 2;
  }
  //return 4;
}

int32_t
ipv4PacketFilter1 (Ptr<QueueDiscItem> item)
{
  FailSafeTag tagRcv;
  if( item->GetPacket ()->PeekPacketTag (tagRcv))
  {
    //std::cout << "FailSafeTag: " << std::endl;
    return 4;
  }
  else
  {
    //std::cout << "No FailSafeTag: " << std::endl;
    return 2;
  }
}


Time
callbackfunc ()
{
  return Simulator::Now ();
}


