/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2011 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
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
 * Author: Manuel Requena <manuel.requena@cttc.es>
 */

#ifndef LTE_RLC_UM_H
#define LTE_RLC_UM_H

#include "ns3/lte-rlc-sequence-number.h"
#include "ns3/lte-rlc.h"

#include <ns3/event-id.h>
#include <map>
using namespace std;
namespace ns3 {

/**
 * LTE RLC Unacknowledged Mode (UM), see 3GPP TS 36.322
 */
class LteRlcUm : public LteRlc
{
public:
  LteRlcUm ();
  virtual ~LteRlcUm ();
  static TypeId GetTypeId (void);
  virtual void DoDispose ();

  /**
   * RLC SAP
   */
  virtual void DoTransmitPdcpPdu (Ptr<Packet> p);

  /**
   * MAC SAP
   */
  virtual void DoNotifyTxOpportunity (uint32_t bytes, uint8_t layer, uint8_t harqId);
  virtual void DoNotifyHarqDeliveryFailure ();
  virtual void DoReceivePdu (Ptr<Packet> p);

private:
  void ExpireReorderingTimer (void);
  void ExpireRbsTimer (void);

  bool IsInsideReorderingWindow (SequenceNumber10 seqNumber);

  void ReassembleOutsideWindow (void);
  void ReassembleSnInterval (SequenceNumber10 lowSeqNumber, SequenceNumber10 highSeqNumber);

  void ReassembleAndDeliver (Ptr<Packet> packet);

  void DoReportBufferStatus ();

private:
  uint32_t m_maxTxBufferSize;
  uint32_t m_txBufferSize;
  std::vector < Ptr<Packet> > m_txBuffer;       // Transmission buffer
  std::map <uint16_t, Ptr<Packet> > m_rxBuffer; // Reception buffer
  std::vector < Ptr<Packet> > m_reasBuffer;     // Reassembling buffer
  std::vector < Ptr<Packet> > m_hBuffer;        // H-frame backup
  std::vector < Ptr<Packet> > m_pBuffer;        // P-frame backup

  std::list < Ptr<Packet> > m_sdusBuffer;       // List of SDUs in a packet

  /**
   * State variables. See section 7.1 in TS 36.322
   */
  SequenceNumber10 m_sequenceNumber; // VT(US)

  SequenceNumber10 m_vrUr;           // VR(UR)
  SequenceNumber10 m_vrUx;           // VR(UX)
  SequenceNumber10 m_vrUh;           // VR(UH)
  uint32_t UMErrorModel();
  double pG;
  double pB;
  double pGB;
  double pBG;

  double piG;
  double piB;
  double pGG;
  double pBB;
  uint32_t current_state;
  uint32_t flag;
  double current_p;
  uint32_t m_nackNum;
  uint32_t m_ackNum;
  uint32_t MINPDUs;
  double m_ratio;
  double calNackRatio();
  string m_revVideoTypeFileName;
  string m_frameType;
  uint32_t m_frameSize;
  uint32_t m_frameId;
  uint32_t m_prevFrame;
  uint32_t m_pPrevFrame;
  uint32_t m_pPPrevFrame;
  uint32_t m_nackCount;
  /**
   * Constants. See section 7.2 in TS 36.322
   */
  uint16_t m_windowSize;

  /**
   * Timers. See section 7.3 in TS 36.322
   */
  EventId m_reorderingTimer;
  EventId m_rbsTimer;

  /**
   * Reassembling state
   */
  typedef enum { NONE            = 0,
                 WAITING_S0_FULL = 1,
                 WAITING_SI_SF   = 2 } ReassemblingState_t;
  ReassemblingState_t m_reassemblingState;
  Ptr<Packet> m_keepS0;

  /**
   * Expected Sequence Number
   */
  SequenceNumber10 m_expectedSeqNumber;

};


} // namespace ns3

#endif // LTE_RLC_UM_H
