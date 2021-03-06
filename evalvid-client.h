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

#ifndef __EVALVID_CLIENT_H__
#define __EVALVID_CLIENT_H__

#include "ns3/application.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "ns3/ipv4-address.h"
#include "ns3/seq-ts-header.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>

#include <fstream>
#include <iostream>
#include <fstream>
#include <iomanip>
#include "ns3/qos-tag.h"

using std::ifstream;
using std::ofstream;
using std::ostream;

using std::ios;
using std::endl;

using namespace std;

namespace ns3 {

class Socket;
class Packet;

/**
 * \ingroup evalvid
 * \class EvalvidClient
 * \brief A Udp client. Sends UDP packet carrying sequence number and time stamp
 *  in their payloads
 *
 */
class EvalvidClient : public Application
{
public:


  static TypeId
  GetTypeId (void);

  EvalvidClient ();

  virtual ~EvalvidClient ();

  /**
   * \brief set the remote address and port
   * \param ip remote IP address
   * \param port remote port
   */
  void SetRemote (Ipv4Address ip, uint16_t port);

protected:
  virtual void DoDispose (void);

private:

  virtual void StartApplication (void);
  virtual void StopApplication (void);

  void Send (void);
  void HandleRead (Ptr<Socket> socket);
  /* ITU-T P.1201 */
  double calO_23 (double v_br);
  double calO_32 (double o23);
  double calO_24 (double L, uint32_t N);
  double calO_25 (double T, uint32_t M);
  double calO_41 (double v_br, double L, uint32_t N, double T, uint32_t M);
  /* buffer management */
  double constraint (double b, double lamda, double va);
  double expf (double x);
  double phi (double a);
  double objfunc (double b, double lamda, double va);

  ofstream    receiverDumpFile;
  string      receiverDumpFileName;
  Ptr<Socket> m_socket;
  Ipv4Address m_peerAddress;
  uint16_t    m_peerPort;
  EventId     m_sendEvent;
  string      m_bitRateFileName;
  ofstream    m_bitRateFile;
  double      m_time;
  double      m_lastTime;
  double      m_interrupTime;
  double      m_interrupData;
  uint16_t    m_interruptCnt;
  uint16_t    m_interruptFlag;
  double      m_interrupDuration;
  double      m_data;
  double      m_oneSdata;
  double      m_sumThoughout;
  uint16_t    m_count;
  uint16_t    m_flag;
  double      m_bitrate;
  string      m_revVideoTypeFileName;
  string      m_frameType;
  uint32_t    m_frameSize;
  uint32_t    m_frameId;
  uint32_t    m_frameNo;
  uint32_t    k;
  uint32_t    N;
  uint32_t    f;
  uint32_t    m_lastFrame;
  double      X;
  double      m_encoderSize;
  uint32_t    m_oldFrameNo;
  string      m_videoRateFileName;
  ofstream    m_videoRateFile;
  string      m_thoughoutFileName;
  ofstream    m_thoughoutFile;
  double      m_thoughout;
  double      m_pBuf; // playback buffer
  double      m_maxPBuf;
  double      m_overflowTime;
  uint16_t    m_overflowCnt;
  uint16_t    m_overflowFlag;
  double      m_overflowDuration;
  double      m_b; // threshold
  double      m_iter;
  double      m_objf;
  double      m_c;
  double      m_staTime;
  uint32_t    m_staPkt;
  uint32_t    m_staFlag;
  std::vector < uint32_t > m_pktNum; 
  uint32_t    m_intervalNum;
  double      m_lamda;
  double      m_va;
  uint32_t    m_avgPktSize;
  uint32_t    m_detechCnt;
  double      m_detechTime;
};

} // namespace ns3

#endif // __EVALVID_CLIENT_H__
