/*****************************************************************************
written by
   Yunhong Gu, last updated 07/09/2009
*****************************************************************************/


#include "core.h"
#include "ccc.h"
#include <cmath>
#include <cstring>


CCC::CCC():
m_iSYNInterval(CUDT::m_iSYNInterval),
m_dPktSndPeriod(1.0),
m_dCWndSize(16.0),
m_iBandwidth(),
m_dMaxCWndSize(),
m_iMSS(),
m_iSndCurrSeqNo(),
m_iRcvRate(),
m_iRTT(),
m_pcParam(NULL),
m_iPSize(0),
m_UDT(),
m_iACKPeriod(0),
m_iACKInterval(0),
m_bUserDefinedRTO(false),
m_iRTO(-1),
m_PerfInfo()
{
}

CCC::~CCC()
{
	if(m_pcParam != NULL)
	{
		delete[] m_pcParam;
		m_pcParam = NULL;
	}
}

void CCC::setACKTimer(const int& msINT)
{
   m_iACKPeriod = msINT;

   if (m_iACKPeriod > m_iSYNInterval)
      m_iACKPeriod = m_iSYNInterval;
}

void CCC::setACKInterval(const int& pktINT)
{
   m_iACKInterval = pktINT;
}

void CCC::setRTO(const int& usRTO)
{
   m_bUserDefinedRTO = true;
   m_iRTO = usRTO;
}

void CCC::sendCustomMsg(CPacket& pkt) const
{
   CUDT* u = CUDT::getUDTHandle(m_UDT);

   if (NULL != u)
   {
      pkt.m_iID = u->m_PeerID;
      u->m_pSndQueue->sendto(u->m_pPeerAddr, pkt, u->m_pRealPeerAddr, 0, "");
   }
}

const CPerfMon* CCC::getPerfInfo()
{
   CUDT* u = CUDT::getUDTHandle(m_UDT);
   if (NULL != u)
      u->sample(&m_PerfInfo, false);

   return &m_PerfInfo;
}

void CCC::setMSS(const int& mss)
{
   m_iMSS = mss;
}

void CCC::setBandwidth(const int& bw)
{
   m_iBandwidth = bw;
}

void CCC::setSndCurrSeqNo(const int32_t& seqno)
{
   m_iSndCurrSeqNo = seqno;
}

void CCC::setRcvRate(const int& rcvrate)
{
   m_iRcvRate = rcvrate;
}

void CCC::setMaxCWndSize(const int& cwnd)
{
   m_dMaxCWndSize = cwnd;
}

void CCC::setRTT(const int& rtt)
{
   m_iRTT = rtt;
}

void CCC::setUserParam(const char* param, const int& size)
{
   delete[] m_pcParam;
   m_pcParam = NULL;
   m_pcParam = new char[size];
   memcpy(m_pcParam, param, size);
   m_iPSize = size;
}

//
CUDTCC::CUDTCC():
m_iRCInterval(),
m_LastRCTime(),
m_bSlowStart(),
m_iLastAck(),
m_bLoss(),
m_iLastDecSeq(),
m_dLastDecPeriod(),
m_iNAKCount(),
m_iDecRandom(),
m_iAvgNAKNum(),
m_iDecCount()
{
	m_iRCInterval = m_iSYNInterval;//add by wm
}

void CUDTCC::init()
{
//   m_iRCInterval = m_iSYNInterval;
   m_LastRCTime = CTimer::getTime();
   setACKTimer(m_iRCInterval);

   setACKInterval(5);//(2);//wm

   m_bSlowStart = true;//false;//true;
   m_iLastAck = m_iSndCurrSeqNo;
   m_bLoss = false;
   m_iLastDecSeq = CSeqNo::decseq(m_iLastAck);
   m_dLastDecPeriod = 1;
   m_iAvgNAKNum = 0;
   m_iNAKCount = 0;
   m_iDecRandom = 1;

   m_dCWndSize = 16;
   m_dPktSndPeriod = 1;
}

void CUDTCC::onACK(const int32_t& ack)
{
   uint64_t currtime = CTimer::getTime();
   if (currtime - m_LastRCTime < (uint64_t)m_iRCInterval)
      return;

   m_LastRCTime = currtime;

   if (m_bSlowStart)
   {
      m_dCWndSize += CSeqNo::seqlen(m_iLastAck, ack);
      m_iLastAck = ack;

      if (m_dCWndSize > m_dMaxCWndSize)
      {
         m_bSlowStart = false;
         if (m_iRcvRate > 0)
            m_dPktSndPeriod = 1000000.0 / m_iRcvRate;
         else
            m_dPktSndPeriod = m_dCWndSize / (m_iRTT + m_iRCInterval);
      }
   }
   else
      m_dCWndSize = m_iRcvRate / 1000000.0 * (m_iRTT + m_iRCInterval) + 16;

   // During Slow Start, no rate increase
   if (m_bSlowStart)
      return;
//wm
   if (m_bLoss)
   {
      m_bLoss = false;
      return;
   }

   int64_t B = (int64_t)(m_iBandwidth - 1000000.0 / m_dPktSndPeriod);
   if ((m_dPktSndPeriod > m_dLastDecPeriod) && ((m_iBandwidth / 9) < B))
      B = m_iBandwidth / 9;

   double inc;

   if (B <= 0)
      inc = 1.0 / m_iMSS;
   else
   {
      // inc = max(10 ^ ceil(log10( B * MSS * 8 ) * Beta / MSS, 1/MSS)
      // Beta = 1.5 * 10^(-6)

      inc = pow(10.0, ceil(log10(B * m_iMSS * 8.0))) * 0.0000015 / m_iMSS;

      if (inc < 1.0/m_iMSS)
         inc = 1.0/m_iMSS;
   }

   m_dPktSndPeriod = (m_dPktSndPeriod * m_iRCInterval) / (m_dPktSndPeriod * inc + m_iRCInterval);

   //set maximum transfer rate
   if ((NULL != m_pcParam) && (m_iPSize == 8))
   {
      int64_t maxSR = *(int64_t*)m_pcParam;
      if (maxSR <= 0)
         return;

      double minSP = 1000000.0 / (double(maxSR) / m_iMSS);
      if (m_dPktSndPeriod < minSP)
         m_dPktSndPeriod = minSP;
   }
}

void CUDTCC::onLoss(const int32_t* losslist, const int&)
{
   //Slow Start stopped, if it hasn't yet
   if (m_bSlowStart)
   {
      m_bSlowStart = false;
      if (m_iRcvRate > 0)
         m_dPktSndPeriod = 1000000.0 / m_iRcvRate;
      else
         m_dPktSndPeriod = m_dCWndSize / (m_iRTT + m_iRCInterval);
   }

   m_bLoss = true;

   if (CSeqNo::seqcmp(losslist[0] & 0x7FFFFFFF, m_iLastDecSeq) > 0)
   {
      m_dLastDecPeriod = m_dPktSndPeriod;
      m_dPktSndPeriod = ceil(m_dPktSndPeriod * 1.125);

      m_iAvgNAKNum = (int)ceil(m_iAvgNAKNum * 0.875 + m_iNAKCount * 0.125);
      m_iNAKCount = 1;
      m_iDecCount = 1;

      m_iLastDecSeq = m_iSndCurrSeqNo;

      // remove global synchronization using randomization
      srand(m_iLastDecSeq);
      m_iDecRandom = (int)ceil(m_iAvgNAKNum * (double(rand()) / RAND_MAX));
      if (m_iDecRandom < 1)
         m_iDecRandom = 1;
   }
   else if ((m_iDecCount ++ < 5) && (0 == (++ m_iNAKCount % m_iDecRandom)))
   {
      // 0.875^5 = 0.51, rate should not be decreased by more than half within a congestion period
      m_dPktSndPeriod = ceil(m_dPktSndPeriod * 1.125);
      m_iLastDecSeq = m_iSndCurrSeqNo;
   }
}

void CUDTCC::onTimeout()
{
   if (m_bSlowStart)
   {
      m_bSlowStart = false;
      if (m_iRcvRate > 0)
         m_dPktSndPeriod = 1000000.0 / m_iRcvRate;
      else
         m_dPktSndPeriod = m_dCWndSize / (m_iRTT + m_iRCInterval);
   }
   else
   {
      /*
      m_dLastDecPeriod = m_dPktSndPeriod;
      m_dPktSndPeriod = ceil(m_dPktSndPeriod * 2);
      m_iLastDecSeq = m_iLastAck;
      */
   }
}





















