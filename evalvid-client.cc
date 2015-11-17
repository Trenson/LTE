/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
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
 *         Saulo da Mata <damata.saulo@gmail.com>
 */
#include "ns3/log.h"
#include "ns3/ipv4-address.h"
#include "ns3/nstime.h"
#include "ns3/inet-socket-address.h"
#include "ns3/socket.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/packet.h"
#include "ns3/uinteger.h"
#include "evalvid-client.h"

#include <stdlib.h>
#include <stdio.h>
#include "ns3/string.h"
#include "ns3/qos-tag.h"


namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("EvalvidClient");
NS_OBJECT_ENSURE_REGISTERED (EvalvidClient);

TypeId
EvalvidClient::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::EvalvidClient")
    .SetParent<Application> ()
    .AddConstructor<EvalvidClient> ()
    .AddAttribute ("RemoteAddress",
                   "The destination Ipv4Address of the outbound packets",
                   Ipv4AddressValue (),
                   MakeIpv4AddressAccessor (&EvalvidClient::m_peerAddress),
                   MakeIpv4AddressChecker ())
    .AddAttribute ("RemotePort", "The destination port of the outbound packets",
                   UintegerValue (100),
                   MakeUintegerAccessor (&EvalvidClient::m_peerPort),
                   MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("ReceiverDumpFilename",
                   "Receiver Dump Filename",
                   StringValue(""),
                   MakeStringAccessor(&EvalvidClient::receiverDumpFileName),
                   MakeStringChecker())
    ;
  return tid;
}

EvalvidClient::EvalvidClient ()
{
  NS_LOG_FUNCTION_NOARGS ();
  m_sendEvent = EventId ();
  m_time = -1;
  m_sumThoughout = 0;
  m_count = 0;
  m_flag = 0;
  m_lastFrame = 0;
  m_encoderSize = 0.0;
  m_oneSdata = 0.0;
  X = 0.0;
  m_interrupDuration = 0.0;
  m_videoRateFileName = "videoRate";
  m_videoRateFile.open(m_videoRateFileName.c_str(), ios::out);
  if (m_videoRateFile.fail())
   {
     NS_FATAL_ERROR(">> EvalvidServer: Error while opening video rate file: " << m_videoRateFileName.c_str());
     return;
   }
  m_thoughoutFileName = "thoughoutFile";
  m_thoughoutFile.open(m_thoughoutFileName.c_str(), ios::out);
  if (m_thoughoutFile.fail())
   {
     NS_FATAL_ERROR(">> EvalvidServer: Error while opening video rate file: " << m_thoughoutFileName.c_str());
     return;
   }
}

EvalvidClient::~EvalvidClient ()
{
  NS_LOG_FUNCTION_NOARGS ();
}

void
EvalvidClient::SetRemote (Ipv4Address ip, uint16_t port)
{
  m_peerAddress = ip;
  m_peerPort = port;
}

void
EvalvidClient::DoDispose (void)
{
  NS_LOG_FUNCTION_NOARGS ();
  Application::DoDispose ();
}

void
EvalvidClient::StartApplication (void)
{
  NS_LOG_FUNCTION_NOARGS();

  if (m_socket == 0)
    {
      TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
      m_socket = Socket::CreateSocket (GetNode (), tid);
      m_socket->Bind ();
      m_socket->Connect (InetSocketAddress (m_peerAddress, m_peerPort));
    }


  receiverDumpFile.open(receiverDumpFileName.c_str(), ios::out);
  if (receiverDumpFile.fail())
    {
      NS_FATAL_ERROR(">> EvalvidClient: Error while opening output file: " << receiverDumpFileName.c_str());
      return;
    }

  m_socket->SetRecvCallback (MakeCallback (&EvalvidClient::HandleRead, this));

  //Delay requesting to get server on line.
  m_sendEvent = Simulator::Schedule ( Seconds(0.1) , &EvalvidClient::Send, this);

}

void
EvalvidClient::Send (void)
{
  NS_LOG_FUNCTION_NOARGS ();

  Ptr<Packet> p = Create<Packet> ();

  SeqTsHeader seqTs;
  seqTs.SetSeq (0);
  p->AddHeader (seqTs);

  m_socket->Send (p);

  NS_LOG_INFO (">> EvalvidClient: Sending request for video streaming to EvalvidServer at "
                << m_peerAddress << ":" << m_peerPort);
}


void
EvalvidClient::StopApplication ()
{
  NS_LOG_FUNCTION_NOARGS ();
  receiverDumpFile.close();
  Simulator::Cancel (m_sendEvent);
}

void
EvalvidClient::HandleRead (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
  Ptr<Packet> packet;
  Address from;
  while ((packet = socket->RecvFrom (from)))
    {
      if (InetSocketAddress::IsMatchingType (from))
        {
          if (packet->GetSize () > 0)
            {
              SeqTsHeader seqTs;
              packet->RemoveHeader (seqTs);
              uint32_t packetId = seqTs.GetSeq ();

              NS_LOG_DEBUG(">> EvalvidClient: Received packet at " << Simulator::Now().GetSeconds()
                           << "s\tid: " << packetId
                           << "\tudp\t" << packet->GetSize() << std::endl);

              receiverDumpFile << std::fixed << std::setprecision(4) << Simulator::Now().ToDouble(ns3::Time::S)
                               << std::setfill(' ') << std::setw(16) <<  "id " << packetId
                               << std::setfill(' ') <<  std::setw(16) <<  "udp " << packet->GetSize()
                               << std::endl;
              double time = Simulator::Now().ToDouble(ns3::Time::S);
              m_data += packet->GetSize();
              m_oneSdata += packet->GetSize();
              m_interrupData -= packet->GetSize();
              if (0 > m_interrupData && 1 == m_interruptFlag) {
                m_interrupDuration += (time - m_interrupTime);
                NS_LOG_DEBUG("interruption time: " << time - m_interrupTime); 
                NS_LOG_DEBUG("interruption count: " << m_interruptCnt);
                NS_LOG_DEBUG("interruption duration: " << m_interrupDuration); 
                m_interruptFlag = 0;
              }
              if(m_time < 0) {
                  m_time = time;
                  m_lastTime = time;
              }
                  uint32_t frameId;
                  uint32_t Uid;
                  uint32_t frameNo;
                  string frameType;
                  uint32_t frameSize;
                  m_revVideoTypeFileName = "videoType1";
                  ifstream revVideoTypeFile(m_revVideoTypeFileName.c_str(), ios::in);
                  if (revVideoTypeFile.fail())
                    {
                      NS_FATAL_ERROR(">> EvalvidServer: Error while opening receive video trace file: " << m_revVideoTypeFileName.c_str());
                      return;
                  }
                  while (revVideoTypeFile >> frameId >> Uid >> frameNo >> frameType >> frameSize)
                    {
                      if(packetId == frameId) {
                          m_frameType = frameType;
                          m_frameSize = frameSize;
                          m_frameId = frameId;
                          m_frameNo = frameNo;
                      }
                    }
                NS_LOG_DEBUG("m_oldFrameNo: " << m_oldFrameNo << ", m_frameNo: " << m_frameNo);
                if(m_oldFrameNo != m_frameNo) {
                  m_encoderSize = 0;
                }
                m_encoderSize += packet->GetSize();
                double bitrate = 0.0;
                uint32_t currentFrame;
                string fileName;
                m_bitRateFileName = "bitRate";
                ifstream bitRateFile(m_bitRateFileName.c_str(), ios::in);
                if (bitRateFile.fail())
                {
                        NS_FATAL_ERROR(">> EvalvidServer: Error while opening receive bit rate file: " << m_bitRateFileName.c_str());
                        return;
                }
                while (bitRateFile >> bitrate >> currentFrame >> fileName)
                {
                }
                if(m_bitrate != bitrate) {
                                   
                }
                if (currentFrame != m_lastFrame) {
                        N = currentFrame - m_lastFrame; // N frame in a chunk
                        m_lastFrame = currentFrame;
                        m_flag = 0; // mean a new chunk
                }
                f = m_frameNo - currentFrame;
                m_bitrate = bitrate; // current chunk bitrate
              if (time - m_time >= 0.2){
                        NS_LOG_DEBUG(">> time = " << time << ",>> m_time = " << m_time);
                        m_thoughout = 8*m_data/(1024*(time - m_time));
                        m_time = time;
                        NS_LOG_DEBUG(">> thoughout = " << m_thoughout << ",>> m_data = " << m_data);
                        m_data = 0;
                        m_sumThoughout += m_thoughout;
                        m_count++;
                        /* Decision algorithm of thoughput degradation */
                        if (m_thoughout < m_bitrate && m_flag == 0) {
                          NS_LOG_DEBUG("1st feedback, " << "frameNo: " << m_frameNo << ", frameId: " << m_frameId);
                          NS_LOG_DEBUG(">> 1st bitrate = " << m_bitrate << ", thoughput = " << m_thoughout);
                          m_flag = 1;
                        } else if (m_thoughout < m_bitrate && m_flag == 1){
                          NS_LOG_DEBUG("2nd feedback, " << "frameNo: " << m_frameNo << ", frameId: " << m_frameId);
                          NS_LOG_DEBUG(">> 2nd bitrate = " << m_bitrate << ", thoughput = " << m_thoughout);
                          m_flag = 2;
                          k = m_frameNo; // 2nd feedback is received at frame k
                          ifstream revVideoTypeFile(m_revVideoTypeFileName.c_str(), ios::in);
                          while (revVideoTypeFile >> frameId >> Uid >> frameNo >> frameType >> frameSize)
                          {
                              if(frameNo == (currentFrame + 1)) {
                                  X = frameSize*1.0/2;
                                  break;
                              }
                          }
                          NS_LOG_DEBUG("XXX: " << X << ", currentFrame: " << currentFrame);
                          m_encoderSize -= packet->GetSize();
                          NS_LOG_DEBUG("k: " << k << ", m_encoderSize: " << m_encoderSize);
                        }
              }
                /* Decision of bitrate shift */
                if (m_flag == 2 && m_oldFrameNo != m_frameNo) {
                  if(m_encoderSize > X){
                     m_flag = 3; // break for-loop
                     m_videoRateFile  << m_thoughout << std::endl; 
                     NS_LOG_DEBUG("conduct bitrate shift 1: f = " << f);
                  }
                  if (f - k > 5 || f == N) {
                     m_flag = 3; // break for-loop
                     m_videoRateFile  << m_thoughout << std::endl; 
                     NS_LOG_DEBUG("conduct bitrate shift 2: f = " << f);
                  }
                  NS_LOG_DEBUG("fff: " << f);
                } else if (m_flag == 3 && m_thoughout > m_bitrate && m_oldFrameNo != m_frameNo) {
                     m_videoRateFile  << m_thoughout*1.3 << std::endl; 
                     m_flag = 4; // break for-loop
                }
              if (time - m_lastTime >= 1){
                m_thoughoutFile << std::fixed << std::setprecision(4) << time
                                << std::setfill(' ') << std::setw(16) << 8*m_oneSdata/(1024*(time - m_lastTime))
                                << std::setfill(' ') << std::setw(16) << m_bitrate
                                << std::endl; 
                NS_LOG_DEBUG("one second data: " << 8*m_oneSdata/1024 << ",time interval: " << (time - m_lastTime));
                
                if (m_bitrate * (time - m_lastTime) > 8*m_oneSdata/1024) {
                    m_interrupTime = time;
                    NS_LOG_DEBUG("interruption, bitrate: " << m_bitrate << ", interrupTime: " << m_interrupTime);
                    m_interrupData = 1024 * m_bitrate * (time - m_lastTime)/8 - m_oneSdata;
                    m_interruptCnt ++;
                    m_interruptFlag = 1;
                }
                m_lastTime = time;
                m_oneSdata = 0;
              }
              m_oldFrameNo = m_frameNo;
              NS_LOG_DEBUG(">> average thoughout = " << m_sumThoughout/m_count << ">> m_sumThoughout = " << m_sumThoughout << "m_count = " << m_count);
           }
        }
    }
}

} // Namespace ns3
