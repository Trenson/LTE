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
#include <math.h>


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
  m_maxPBuf = 250 * 1024; // 200
  m_pBuf = 0.0;
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
              m_pBuf += packet->GetSize();
              m_data += packet->GetSize();
              m_oneSdata += packet->GetSize();
              m_interrupData -= packet->GetSize();
              /* interruption finish */
              if (0 > m_interrupData && 1 == m_interruptFlag) {
                m_interrupDuration += (time - m_interrupTime);
                NS_LOG_DEBUG(">> Interruption finished: Current interruption time " << (time - m_interrupTime)
                             << "\tInterruption frequency: " << m_interruptCnt
                             << "\tsum of interruption time: " << m_interrupDuration << std::endl);
                m_interruptFlag = 0;
              }
              /* overflow finish */
              if (m_pBuf < m_maxPBuf && 1 == m_overflowFlag) {
                m_overflowDuration += (time - m_overflowTime);
                NS_LOG_DEBUG(">> Overflow finished: Current overflow time " << (time - m_overflowTime)
                             << "\toverflow frequency: " << m_overflowCnt
                             << "\tsum of overflow time: " << m_overflowDuration << std::endl);
                m_overflowFlag = 0;
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
                NS_LOG_DEBUG(">> Current frame No is " << m_frameNo
                             << "\tLast frame No is " << m_oldFrameNo << std::endl);
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
                        NS_LOG_DEBUG(">> Current time is " << time
                             << "\tLast time " << m_time << std::endl);
                        m_thoughout = 8*m_data/(1024*(time - m_time));
                        m_time = time;
                        NS_LOG_DEBUG(">> Thoughput in this tao is " << m_thoughout
                             << "\tData in this tao: " << m_data << std::endl);
                        m_data = 0;
                        m_sumThoughout += m_thoughout;
                        m_count++;
                        /* Decision algorithm of thoughput degradation */
                        if (m_thoughout < m_bitrate && m_flag == 0) {
                          NS_LOG_DEBUG(">> 1st feedback, frameNo: " << m_frameNo
                             << "\tframeId: " << m_frameId
                             << "\tbitrate: " << m_bitrate
                             << "\tthoughput: " << m_thoughout << std::endl);
                          m_flag = 1;
                        } else if (m_thoughout < m_bitrate && m_flag == 1){
                          NS_LOG_DEBUG(">> 2nd feedback, frameNo: " << m_frameNo
                             << "\tframeId: " << m_frameId
                             << "\tbitrate: " << m_bitrate
                             << "\tthoughput: " << m_thoughout << std::endl);
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
                          m_encoderSize -= packet->GetSize();
                          NS_LOG_DEBUG(">> CurrentFrame: " << currentFrame
                             << "\thalf of the head size of the chunk: " << X
                             << "\t2nd feedback is received at frame k: " << k
                             << "\tm_encoderSize: " << m_encoderSize << std::endl);
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
                } else if (m_flag == 3 && m_thoughout > m_bitrate && m_oldFrameNo != m_frameNo) {
                     m_videoRateFile  << m_thoughout * 1 << std::endl; 
                     m_flag = 4; // break for-loop
                }
              if (time - m_lastTime >= 1){
                m_thoughoutFile << std::fixed << std::setprecision(4) << time
                                << std::setfill(' ') << std::setw(16) << 8*m_oneSdata/(1024*(time - m_lastTime))
                                << std::setfill(' ') << std::setw(16) << m_bitrate
                                << std::endl; 
                /* m_oneSdata is used for calculate thoughput in 1s */
                NS_LOG_DEBUG(">> One second data: " << 8*m_oneSdata/1024
                             << "\ttime interval: " << (time - m_lastTime) << std::endl);
                
                /* playback interruption happens, calculate the playback interruption */
                if (m_bitrate * (time - m_lastTime) > 8*m_oneSdata/1024 && 0 == m_interruptFlag) {
                    m_interrupTime = time;
                    NS_LOG_DEBUG(">> Interruption happens, bitrate: " << m_bitrate
                             << "\ttime: " << m_interrupTime << std::endl);
                    m_interrupData = 1024 * m_bitrate * (time - m_lastTime)/8 - m_oneSdata;
                    m_interruptCnt ++;
                    m_interruptFlag = 1;
                }

                /* overflow happens, calculate the overflow */
                NS_LOG_DEBUG(">> Current buffer is: " << m_pBuf
                             << "\tMax buffer size: " << m_maxPBuf << std::endl);
                if (m_pBuf > m_maxPBuf && 0 == m_overflowFlag) {
                   m_overflowTime = time;
                   m_overflowCnt ++;
                   m_overflowFlag = 1;
                }
                m_pBuf -= m_bitrate * (time - m_lastTime) * 1024/8;
                m_lastTime = time;
                m_oneSdata = 0;
                // double mos = calO_41 (m_bitrate, m_interrupDuration/m_interruptCnt, m_interruptCnt);
                /*NS_LOG_DEBUG("MOS = " << calO_41 (m_bitrate, m_interrupDuration/m_interruptCnt, m_interruptCnt));
                NS_LOG_DEBUG("MOS1 = " << calO_41 (1397, 0, 0));
                NS_LOG_DEBUG("MOS2 = " << calO_41 (1079, 0.6, 3));
                NS_LOG_DEBUG("MOS3 = " << calO_41 (949, 0, 0));
                NS_LOG_DEBUG("MOS4 = " << calO_41 (1397, 0.1, 1));
                NS_LOG_DEBUG("MOS5 = " << calO_41 (600, 0.1, 1));
                NS_LOG_DEBUG("MOS6 = " << calO_41 (600, 0.2, 1));*/
                NS_LOG_DEBUG("MOS1 = " << calO_41 (1397, 0, 0, 0, 0));
                NS_LOG_DEBUG("MOS2 = " << calO_41 (1397, 0.1, 1, 0, 0));
                NS_LOG_DEBUG("MOS3 = " << calO_41 (1397, 0, 0, 0.1, 1));
                NS_LOG_DEBUG("MOS4 = " << calO_41 (1397, 0.1, 1, 0.1, 1));
              }
              m_oldFrameNo = m_frameNo;
              NS_LOG_DEBUG(">> Average thoughout = " << m_sumThoughout/m_count
                             << "\tSum thoughout: " << m_sumThoughout
                             << "\tCount: " << m_count << std::endl);
           }
        }
    }
}

/* Model output O.23 */
double
EvalvidClient::calO_23 (double v_br)
{
  double v_mosc;
  double v_dc;
  double v_abif = 31000; // average number of bytes per I-frame
  double v_nbr = (v_br * 8 * 30)/(1000 * 30); // Eq. 6-31
  double v_ccf = sqrt(v_br/(v_abif * 15.0)) > 1.10 ? 1.10 : sqrt(v_br/(v_abif * 15.0)); // Eq. 6-32
  v_dc = 4/(1 + pow(v_nbr/(104.0*v_ccf + 1.0),(0.01*v_ccf + 1.1))); // Eq. 6-40
  v_mosc = (5 - v_dc)*(1 + 3.4*v_ccf - 0.969*v_ccf*log(1000/30)); // Eq.6-38
  return calO_32 (v_mosc);
}

/* Model output O.32 */
double
EvalvidClient::calO_32 (double o23)
{
  double av_mosc;
  av_mosc = o23; //0.7977*o23 + 0.02472*o23; because no audio
  return av_mosc;
}

/* Model output O.24 */
double
EvalvidClient::calO_24 (double L, uint32_t N)
{
  double pBufInd;
  double tmpStall1 = 1.66 - 1.72*(pow(2.718, (-0.04*L - 0.36)*N)); // III-1
  double tmpStall2 = tmpStall1 > 4 ? 4 : tmpStall1; // min
  double degStall = tmpStall2 > 0 ? tmpStall2 : 0; // max
  double degT0 = 0; // III-3
  double tmpPb = (degStall + degT0) < 4 ? (degStall + degT0) : 4; // III-4
  pBufInd = 5 - (tmpPb > 0 ? tmpPb : 0);
  return pBufInd;
}

/* Model output O.25 */
double
EvalvidClient::calO_25 (double T, uint32_t M)
{
  double pBufInd;
  double tmp1 = 1.66 - 1.72*(pow(2.718, (-0.04*T - 0.36)*M)); // III-1
  double tmp2 = tmp1 > 4 ? 4 : tmp1; // min
  double degOverflow = tmp2 > 0 ? tmp2 : 0; // max
  double tmpPb = degOverflow < 4 ? degOverflow : 4; // III-4
  pBufInd = 5 - (tmpPb > 0 ? tmpPb : 0);
  return pBufInd;
}

/* Model output O.41 */
double
EvalvidClient::calO_41 (double v_br, double L, uint32_t N, double T, uint32_t M)
{
  double a1 = 0.4175;
  double a2 = 0.4175;
  double a3 = 0.0247;
  double a4 = 0.1403;
  double o32 = calO_23 (v_br);
  double o24 = a1*calO_24 (L, N) + a2*calO_25 (T, M) + a3*calO_24 (L, N)*calO_25 (T, M) + a4;
  double tmp = (o32 - 5 + o24) < 5 ? (o32 - 5 + o24) : 5;
  double Qms = tmp > 1 ? tmp : 1;
  return Qms;
}

} // Namespace ns3
