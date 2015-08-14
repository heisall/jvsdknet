/*****************************************************************************
written by
   Yunhong Gu, last updated 07/09/2009
*****************************************************************************/

#include <cmath>
#include "common.h"
#include "window.h"


CACKWindow::CACKWindow():
m_piACKSeqNo(NULL),
m_piACK(NULL),
m_pTimeStamp(NULL),
m_iSize(1024),
m_iHead(0),
m_iTail(0)
{
   m_piACKSeqNo = new int32_t[m_iSize];
   m_piACK = new int32_t[m_iSize];
   m_pTimeStamp = new uint64_t[m_iSize];

   m_piACKSeqNo[0] = -1;
}

CACKWindow::CACKWindow(const int& size):
m_piACKSeqNo(NULL),
m_piACK(NULL),
m_pTimeStamp(NULL),
m_iSize(size),
m_iHead(0),
m_iTail(0)
{
   m_piACKSeqNo = new int32_t[m_iSize];
   m_piACK = new int32_t[m_iSize];
   m_pTimeStamp = new uint64_t[m_iSize];

   m_piACKSeqNo[0] = -1;
}

CACKWindow::~CACKWindow()
{
	if(m_piACKSeqNo != NULL)
	{
		delete[] m_piACKSeqNo;
		m_piACKSeqNo = NULL;
	}
	if(m_piACK != NULL)
	{
		delete[] m_piACK;
		m_piACK = NULL;
	}
	if(m_pTimeStamp != NULL)
	{
		delete[] m_pTimeStamp;
		m_pTimeStamp = NULL;
	}
}

void CACKWindow::store(const int32_t& seq, const int32_t& ack)
{
   m_piACKSeqNo[m_iHead] = seq;
   m_piACK[m_iHead] = ack;
   m_pTimeStamp[m_iHead] = CTimer::getTime();

   m_iHead = (m_iHead + 1) % m_iSize;

   // overwrite the oldest ACK since it is not likely to be acknowledged
   if (m_iHead == m_iTail)
      m_iTail = (m_iTail + 1) % m_iSize;
}

int CACKWindow::acknowledge(const int32_t& seq, int32_t& ack)
{
   if (m_iHead >= m_iTail)
   {
      // Head has not exceeded the physical boundary of the window

      for (int i = m_iTail, n = m_iHead; i < n; ++ i)
         // looking for indentical ACK Seq. No.
         if (seq == m_piACKSeqNo[i])
         {
            // return the Data ACK it carried
            ack = m_piACK[i];

            // calculate RTT
            int rtt = int(CTimer::getTime() - m_pTimeStamp[i]);
            if (i + 1 == m_iHead)
            {
               m_iTail = m_iHead = 0;
               m_piACKSeqNo[0] = -1;
            }
            else
               m_iTail = (i + 1) % m_iSize;

            return rtt;
         }

      // Bad input, the ACK node has been overwritten
      return -1;
   }

   // Head has exceeded the physical window boundary, so it is behind tail
   for (int j = m_iTail, n = m_iHead + m_iSize; j < n; ++ j)
      // looking for indentical ACK seq. no.
      if (seq == m_piACKSeqNo[j % m_iSize])
      {
         // return Data ACK
         j %= m_iSize;
         ack = m_piACK[j];

         // calculate RTT
         int rtt = int(CTimer::getTime() - m_pTimeStamp[j]);
         if (j == m_iHead)
         {
            m_iTail = m_iHead = 0;
            m_piACKSeqNo[0] = -1;
         }
         else
            m_iTail = (j + 1) % m_iSize;

         return rtt;
      }

   // bad input, the ACK node has been overwritten
   return -1;
}

////////////////////////////////////////////////////////////////////////////////

CPktTimeWindow::CPktTimeWindow():
m_iAWSize(16),
m_piPktWindow(NULL),
m_iPktWindowPtr(0),
m_iPWSize(16),
m_piProbeWindow(NULL),
m_iProbeWindowPtr(0),
m_iLastSentTime(0),
m_iMinPktSndInt(1000000),
m_LastArrTime(),
m_CurrArrTime(),
m_ProbeTime()
{
   m_piPktWindow = new int[m_iAWSize];
   m_piProbeWindow = new int[m_iPWSize];

   m_LastArrTime = CTimer::getTime();

   for (int i = 0; i < m_iAWSize; ++ i)
      m_piPktWindow[i] = 1000000;

   for (int k = 0; k < m_iPWSize; ++ k)
      m_piProbeWindow[k] = 1000;
}

CPktTimeWindow::CPktTimeWindow(const int& asize, const int& psize):
m_iAWSize(asize),
m_piPktWindow(NULL),
m_iPktWindowPtr(0),
m_iPWSize(psize),
m_piProbeWindow(NULL),
m_iProbeWindowPtr(0),
m_iLastSentTime(0),
m_iMinPktSndInt(1000000),
m_LastArrTime(),
m_CurrArrTime(),
m_ProbeTime()
{
   m_piPktWindow = new int[m_iAWSize];
   m_piProbeWindow = new int[m_iPWSize];

   m_LastArrTime = CTimer::getTime();

   for (int i = 0; i < m_iAWSize; ++ i)
      m_piPktWindow[i] = 1000000;

   for (int k = 0; k < m_iPWSize; ++ k)
      m_piProbeWindow[k] = 1000;
}

CPktTimeWindow::~CPktTimeWindow()
{
	if(m_piPktWindow != NULL)
	{
		delete[] m_piPktWindow;
		m_piPktWindow = NULL;
	}
	if(m_piProbeWindow != NULL)
	{ 
		delete[] m_piProbeWindow;
		m_piProbeWindow = NULL;
	}
}

int CPktTimeWindow::getMinPktSndInt() const
{
   return m_iMinPktSndInt;
}

int CPktTimeWindow::getPktRcvSpeed() const
{
   // sorting
   int* pi = m_piPktWindow;
   for (int i = 0, n = (m_iAWSize >> 1) + 1; i < n; ++ i)
   {
      int* pj = pi;
      for (int j = i, m = m_iAWSize; j < m; ++ j)
      {
         if (*pi > *pj)
         {
            int temp = *pi;
            *pi = *pj;
            *pj = temp;
         }
         ++ pj;
      }
      ++ pi;
   }

   // read the median value
   int median = (m_piPktWindow[(m_iAWSize >> 1) - 1] + m_piPktWindow[m_iAWSize >> 1]) >> 1;
   int count = 0;
   int sum = 0;
   int upper = median << 3;
   int lower = median >> 3;

   // median filtering
   int* pk = m_piPktWindow;
   for (int k = 0, l = m_iAWSize; k < l; ++ k)
   {
      if ((*pk < upper) && (*pk > lower))
      {
         ++ count;
         sum += *pk;
      }
      ++ pk;
   }

   // claculate speed, or return 0 if not enough valid value
   if (count > (m_iAWSize >> 1))
      return (int)ceil(1000000.0 / (sum / count));
   else
      return 0;
}

int CPktTimeWindow::getBandwidth() const
{
   // sorting
   int* pi = m_piProbeWindow;
   for (int i = 0, n = (m_iPWSize >> 1) + 1; i < n; ++ i)
   {
      int* pj = pi;
      for (int j = i, m = m_iPWSize; j < m; ++ j)
      {
         if (*pi > *pj)
         {
            int temp = *pi;
            *pi = *pj;
            *pj = temp;
         }
         ++ pj;
      }
      ++ pi;
   }

   // read the median value
   int median = (m_piProbeWindow[(m_iPWSize >> 1) - 1] + m_piProbeWindow[m_iPWSize >> 1]) >> 1;
   int count = 1;
   int sum = median;
   int upper = median << 3;
   int lower = median >> 3;

   // median filtering
   int* pk = m_piProbeWindow;
   for (int k = 0, l = m_iPWSize; k < l; ++ k)
   {
      if ((*pk < upper) && (*pk > lower))
      {
         ++ count;
         sum += *pk;
      }
      ++ pk;
   }

   return (int)ceil(1000000.0 / (double(sum) / double(count)));
}

void CPktTimeWindow::onPktSent(const int& currtime)
{
   int interval = currtime - m_iLastSentTime;

   if ((interval < m_iMinPktSndInt) && (interval > 0))
      m_iMinPktSndInt = interval;

   m_iLastSentTime = currtime;
}

void CPktTimeWindow::onPktArrival()
{
   m_CurrArrTime = CTimer::getTime();

   // record the packet interval between the current and the last one
   *(m_piPktWindow + m_iPktWindowPtr) = int(m_CurrArrTime - m_LastArrTime);

   // the window is logically circular
   ++ m_iPktWindowPtr;
   if (m_iPktWindowPtr == m_iAWSize)
      m_iPktWindowPtr = 0;

   // remember last packet arrival time
   m_LastArrTime = m_CurrArrTime;
}

void CPktTimeWindow::probe1Arrival()
{
   m_ProbeTime = CTimer::getTime();
}

void CPktTimeWindow::probe2Arrival()
{
   m_CurrArrTime = CTimer::getTime();

   // record the probing packets interval
   *(m_piProbeWindow + m_iProbeWindowPtr) = int(m_CurrArrTime - m_ProbeTime);
   // the window is logically circular
   ++ m_iProbeWindowPtr;
   if (m_iProbeWindowPtr == m_iPWSize)
      m_iProbeWindowPtr = 0;
}
