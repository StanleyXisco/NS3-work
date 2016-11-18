
/*
 * Implementation of a new network topology which works with a DMZ
 * The DMZ is more like an interconnection between two switch and two router
 * the major function of the DMZ is to replicate a network that works with a 
 * backup router which will set forwarding packet to the next router immediately
 * it is overloaded.
 * all codes are implemented in C++ using NS3 Modules
 */

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/csma-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include "ns3/bridge-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("MrAmadiPhdWork");

int 
main (int argc, char *argv[])
{

 uint32_t nWifi = 5;
 bool tracing = false;

CommandLine cmd;
bool verbose = true;
cmd.AddValue ("nWifi", "Number of wifi STA devices", nWifi);
cmd.AddValue ("verbose", "Tell echo applications to log if true", verbose);

cmd.Parse (argc,argv);

if (nWifi > 250)
    {
      std::cout << "Too many wifi nodes, no more than 250 each." << std::endl;
      return 1;
    }
 if (verbose)
    {
      LogComponentEnable ("UdpEchoClientApplication", LOG_LEVEL_INFO);
      LogComponentEnable ("UdpEchoServerApplication", LOG_LEVEL_INFO);
    }


  NodeContainer wifiApNode;
  wifiApNode.Create (1);

  NodeContainer FireSeverNode;
  FireSeverNodes.Create(2);


  NS_LOG_INFO ("INFO: Create nodes majors."); 
  Ptr<Node> tr  = CreateObject<Node> ();  //master  router
  Ptr<Node> br  = CreateObject<Node> ();  //slave router
  Ptr<Node> Fst  = CreateObject<Node> ();  // switch connection form the access point 
  Ptr<Node> Lst  = CreateObject<Node> ();  // switch connection from both router to the firwall

 // ----------------------------------------------------------------------
  // Give the nodes names
  // ----------------------------------------------------------------------

  Names::Add ("tr",  tr);
  Names::Add ("br",  br);
  Names::Add ("Fst", Fst);
  Names::Add ("Lst", Lst);

 // Builing link from the router to the 

  CsmaHelper csma;
  csma.SetChannelAttribute ("DataRate", StringValue ("100Mbps"));
  csma.SetChannelAttribute ("Delay", TimeValue (MilliSeconds (6560)));
  
  NS_LOG_INFO ("Creating Point-To-Point Link");

  PointToPointHelper pointToPoint;
  pointToPoint.SetDeviceAttribute  ("DataRate", StringValue ("5Mbps"));
  pointToPoint.SetChannelAttribute ("Delay",    StringValue ("2ms"));

 // -------------------------------------------------------------------------
// Now connecting the routers and the switches together with the access point
// --------------------------------------------------------------------------

// Point-Point connection switch connection from the AP
  

  NetDeviceContainer p2pDevices;
  p2pDevices = pointToPoint.Install (NodeContainer (wifiApNode.Get (0),  Fst));

// CSMA connection between first switch and the two routers
  NetDeviceContainer link_Fst_tr   = csma.Install (NodeContainer (Fst,  tr));
  NetDeviceContainer link_Fst_br   = csma.Install (NodeContainer (Fst,  br));

// CSMA connection between two routers and the last switch
  NetDeviceContainer link_tr_Lst   = csma.Install (NodeContainer (tr,  Lst));
  NetDeviceContainer link_br_Lst   = csma.Install (NodeContainer (br,  Lst));

NodeContainer fwNode = FireSeverNodes.Get(0);
// CSMA connection from the last switch to the firewall
  NetDeviceContainer link_Lst_Fw   = csma.Install (NodeContainer (Lst,  fwNode.Get(0)));

NodeContainer serverNode = FireSeverNodes.Get(1);
// CSMA connection from firewall to the server
  NetDeviceContainer link_Fw_Server   = csma.Install (NodeContainer (fwNode.Get(0),  serverNode.Get(0)));

// ======================================================================
// Manually create the list of NetDevices for each switch
// ----------------------------------------------------------------------

// First Switch NetDevice
  NetDeviceContainer Fstnd;
  Fstnd.Add (p2pDevices.Get (1));
  Fstnd.Add (link_Fst_tr.Get (0));
  Fstnd.Add (link_Fst_br.Get (0));

// Last Switch NetDevice
  NetDeviceContainer Lstnd;
  Lstnd.Add (link_tr_Lst.Get (1));
  Lstnd.Add (link_br_Lst.Get (1));
  Lstnd.Add (link_Fst_Fw.Get (0));


  NodeContainer wifiStaNodes;
  wifiStaNodes.Create (nWifi);
  
  
  YansWifiChannelHelper channel = YansWifiChannelHelper::Default ();
  YansWifiPhyHelper phy = YansWifiPhyHelper::Default ();
  phy.SetChannel (channel.Create ());

  WifiHelper wifi;
  wifi.SetRemoteStationManager ("ns3::AarfWifiManager");

  WifiMacHelper mac;
  Ssid ssid = Ssid ("ns-3-ssid");
  mac.SetType ("ns3::StaWifiMac",
               "Ssid", SsidValue (ssid),
               "ActiveProbing", BooleanValue (false));

  NetDeviceContainer staDevices;
  staDevices = wifi.Install (phy, mac, wifiStaNodes);

  mac.SetType ("ns3::ApWifiMac",
               "Ssid", SsidValue (ssid));

  NetDeviceContainer apDevices;
  apDevices = wifi.Install (phy, mac, wifiApNode);

  MobilityHelper mobility;

  mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
                                 "MinX", DoubleValue (0.0),
                                 "MinY", DoubleValue (0.0),
                                 "DeltaX", DoubleValue (5.0),
                                 "DeltaY", DoubleValue (10.0),
                                 "GridWidth", UintegerValue (3),
                                 "LayoutType", StringValue ("RowFirst"));

  mobility.SetMobilityModel ("ns3::RandomWalk2dMobilityModel",
                             "Bounds", RectangleValue (Rectangle (-50, 50, -50, 50)));
  mobility.Install (wifiStaNodes);

  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (wifiApNode);

  
  InternetStackHelper stack;
  stack.Install (wifiApNode);
  stack.Install (wifiStaNodes);

  NS_LOG_INFO ("L3: Install the ns3 IP stack on routers.");
  NodeContainer routerNodes (tr, br);
  stack.Install (routerNodes);

  NS_LOG_INFO ("Install the ns3 IP stack on the firewall and server");
  NodeContainer fsNodes (fwNode.Get(0),  serverNode.Get(0));
  stack.Install (fsNodes);

  BridgeHelper bridge;

  bridge.Install (Fst, Fstnd);
  bridge.Install (Lst, Lstnd);

//---------------------------------------------------------------------------
// Place the access point and the wifistation nodes in the same nodecontainer
//---------------------------------------------------------------------------

  NodeDeviceContainer 
  Ipv4AddressHelper address;

  address.SetBase ("10.1.1.0", "255.255.255.0");
  address.Assign (staDevices);
  address.Assign (apDevices);

// ----------------------------------------------------------------------
  // Assign IP address to the individual routers
  // ----------------------------------------------------------------------
  NS_LOG_INFO ("L3: Assign IP Addresses to the DMZ routers.");

  NetDeviceContainer routerIpDevices;        
  botLanIpDevices.Add (link_Fst_tr.Get (1));  
  botLanIpDevices.Add (link_Fst_br.Get (1));  
  
  address.SetBase ("192.168.2.0", "255.255.255.0");
  address.Assign  (routerIpDevices);

//-------------------------------------------------------------------------
// Assign IP address to the firewall node and the server node
//--------------------------------------------------------------------------
  address.SetBase ("10.1.2.0", "255.255.255.0");
  address.Assign (link_Fw_Server)


  // ======================================================================
  // Calculate and populate routing tables
  // ----------------------------------------------------------------------
  NS_LOG_INFO ("L3: Populate routing tables.");
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

 
