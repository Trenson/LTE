/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 *
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
 * Author: Billy Pinheiro <haquiticos@gmail.com>
 */

#include <fstream>
#include <string.h>

#include "ns3/csma-helper.h"
#include "ns3/evalvid-client-server-helper.h"

#include "ns3/lte-helper.h"
#include "ns3/epc-helper.h"
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/lte-module.h"
#include "ns3/applications-module.h"
#include "ns3/point-to-point-helper.h"
#include "ns3/config-store.h"
#include "ns3/netanim-module.h"
#include "ns3/lte-spectrum-phy.h"
#include "ns3/error-model.h"
#include <ns3/applications-module.h>
//#include "ns3/gtk-config-store.h"

using namespace ns3;

/**
 * Sample simulation script for LTE+EPC. It instantiates several eNodeB,
 * attaches one UE per eNodeB starts a flow for each UE to  and from a remote host.
 * It also  starts yet another flow between each UE pair.
 */
 
NS_LOG_COMPONENT_DEFINE ("EvalvidLTEExample");

int
main (int argc, char *argv[])
{
  LogComponentEnable ("EvalvidClient", LOG_LEVEL_ALL);
  LogComponentEnable ("EvalvidServer", LOG_LEVEL_ALL);
  LogComponentEnable ("LteRlcUm", LOG_LEVEL_ALL);
  LogComponentEnable ("LteRlcAm", LOG_LEVEL_ALL);

  uint16_t numberOfNodes = 1;
  // double simTime = 5.0;
  double distance = 60.0;
  // Inter packet interval in ms
  // double interPacketInterval = 1;
  // double interPacketInterval = 100;
  uint16_t port = 8000;

  Ptr<LteHelper> lteHelper = CreateObject<LteHelper> ();
  //Ptr<EpcHelper>  epcHelper = CreateObject<EpcHelper> ();
  Ptr<PointToPointEpcHelper>  epcHelper = CreateObject<PointToPointEpcHelper> ();
  lteHelper->SetEpcHelper (epcHelper);
  lteHelper->SetSchedulerType("ns3::RrFfMacScheduler");
  // RLC_SM_ALWAYS = 1, RLC_UM_ALWAYS = 2, RLC_AM_ALWAYS = 3,   PER_BASED = 4 }
  Config::SetDefault ("ns3::LteEnbRrc::EpsBearerToRlcMapping",EnumValue(LteHelper::RLC_UM_ALWAYS)); 

  Ptr<Node> pgw = epcHelper->GetPgwNode ();

  // Create a single RemoteHost
  NodeContainer remoteHostContainer;
  remoteHostContainer.Create (2);
  Ptr<Node> remoteHost = remoteHostContainer.Get (0);
  Ptr<Node> cbrHost = remoteHostContainer.Get (1);
  InternetStackHelper internet;
  internet.Install (remoteHostContainer);

  // Create the Internet
  PointToPointHelper p2ph;
  p2ph.SetDeviceAttribute ("DataRate", DataRateValue (DataRate ("100Gb/s")));
  p2ph.SetDeviceAttribute ("Mtu", UintegerValue (1500));
  p2ph.SetChannelAttribute ("Delay", TimeValue (Seconds (0.010)));
  NetDeviceContainer internetDevices = p2ph.Install (pgw, remoteHost);

  Ipv4AddressHelper ipv4h;
  ipv4h.SetBase ("1.0.0.0", "255.0.0.0");
  Ipv4InterfaceContainer internetIpIfaces = ipv4h.Assign (internetDevices);
  // interface 0 is localhost, 1 is the p2p device
  // Ipv4Address remoteHostAddr = internetIpIfaces.GetAddress (1);

  Ipv4StaticRoutingHelper ipv4RoutingHelper;
  Ptr<Ipv4StaticRouting> remoteHostStaticRouting = ipv4RoutingHelper.GetStaticRouting (remoteHost->GetObject<Ipv4> ());
  remoteHostStaticRouting->AddNetworkRouteTo (Ipv4Address ("7.0.0.0"), Ipv4Mask ("255.0.0.0"), 1);

  // Create the Internet of CBR
  PointToPointHelper p2phCbr;
  p2phCbr.SetDeviceAttribute ("DataRate", DataRateValue (DataRate ("100Gb/s")));
  p2phCbr.SetDeviceAttribute ("Mtu", UintegerValue (1500));
  p2phCbr.SetChannelAttribute ("Delay", TimeValue (Seconds (0.010)));   //0.010
  NetDeviceContainer internetCbrDevices = p2phCbr.Install (pgw, cbrHost);
  Ipv4AddressHelper ipv4hCbr;
  ipv4hCbr.SetBase ("2.0.0.0", "255.0.0.0");
  Ipv4InterfaceContainer internetCbrIpIfaces = ipv4hCbr.Assign (internetCbrDevices);
  Ipv4StaticRoutingHelper ipv4CbrRoutingHelper;
  Ptr<Ipv4StaticRouting> cbrHostStaticRouting = ipv4CbrRoutingHelper.GetStaticRouting (cbrHost->GetObject<Ipv4> ());
  cbrHostStaticRouting->AddNetworkRouteTo (Ipv4Address ("7.0.0.0"), Ipv4Mask ("255.0.0.0"), 1);

  NodeContainer ueNodes;
  NodeContainer enbNodes;
  enbNodes.Create(numberOfNodes);
  ueNodes.Create(numberOfNodes);

  // Install Mobility Model
  Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
  for (uint16_t i = 0; i < numberOfNodes; i++)
    {
      positionAlloc->Add (Vector(distance * i, 0, 0));
    }
  MobilityHelper mobility;
  mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
  mobility.SetPositionAllocator(positionAlloc);
  mobility.Install(enbNodes);
  mobility.Install(ueNodes);

  // Install LTE Devices to the nodes
  NetDeviceContainer enbLteDevs = lteHelper->InstallEnbDevice (enbNodes);
  NetDeviceContainer ueLteDevs = lteHelper->InstallUeDevice (ueNodes);
  
  Ptr<RateErrorModel> em = CreateObject<RateErrorModel> ();
  em->SetAttribute ("ErrorUnit", EnumValue (RateErrorModel::ERROR_UNIT_PACKET));
  em->SetAttribute ("ErrorRate", DoubleValue (0.05));
  em->SetAttribute ("RanVar", StringValue ("ns3::UniformRandomVariable[Min=0.0|Max=1.0]"));
  internetDevices.Get(0)->SetAttribute("ReceiveErrorModel", PointerValue (em));
 
  // Install the IP stack on the UEs
  internet.Install (ueNodes);
  Ipv4InterfaceContainer ueIpIface;
  ueIpIface = epcHelper->AssignUeIpv4Address (NetDeviceContainer (ueLteDevs));
  // Assign IP address to UEs, and install applications

  Ptr<Node> ueNode = ueNodes.Get (0);
  // Set the default gateway for the UE
  Ptr<Ipv4StaticRouting> ueStaticRouting = ipv4RoutingHelper.GetStaticRouting (ueNode->GetObject<Ipv4> ());
  ueStaticRouting->SetDefaultRoute (epcHelper->GetUeDefaultGatewayAddress (), 1);

  // Attach one UE per eNodeB
  lteHelper->Attach (ueLteDevs.Get(0), enbLteDevs.Get(0));
  // lteHelper->ActivateEpsBearer (ueLteDevs, EpsBearer (EpsBearer::NGBR_VIDEO_TCP_DEFAULT), EpcTft::Default ());
  
  NS_LOG_INFO ("Create Applications.");
  
  EvalvidServerHelper server(port);
  server.SetAttribute ("SenderTraceFilename", StringValue("st_foreman_cif_2M.st")); //foreman_qcif.st
  server.SetAttribute ("SenderDumpFilename", StringValue("sd"));
  ApplicationContainer apps = server.Install(remoteHostContainer.Get(0));
  apps.Start (Seconds (10.0));
  apps.Stop (Seconds (99.0));
  
  EvalvidClientHelper client (internetIpIfaces.GetAddress (1), port);
  client.SetAttribute ("ReceiverDumpFilename", StringValue("rd"));
  apps = client.Install (ueNodes.Get(0));
  apps.Start (Seconds (10.0));
  apps.Stop (Seconds (100.0));  

  // Set the CBR application on the cbrHost
  OnOffHelper onOff ("ns3::UdpSocketFactory",
                     Address (InetSocketAddress (ueIpIface.GetAddress (0), port)));
  onOff.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=10.0]"));
  onOff.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=10.0]"));
  onOff.SetAttribute ("DataRate", DataRateValue (DataRate ("8Mb/s")));

  apps = onOff.Install (cbrHost);
  // apps.Add (PacketSinkHelper.Install (ueNodes.Get(0)));
  apps.Start (Seconds (10));
  apps.Stop (Seconds (100));

  NS_LOG_INFO ("Run Simulation.");
  Simulator::Stop(Seconds(100));
  Simulator::Run ();         
  Simulator::Destroy ();
  
  NS_LOG_INFO ("Done.");
  return 0;
}