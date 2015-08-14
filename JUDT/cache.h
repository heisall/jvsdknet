/*****************************************************************************
written by
   Yunhong Gu, last updated 05/06/2009
*****************************************************************************/

#ifndef __UDT_CACHE_H__
#define __UDT_CACHE_H__

#include "udt.h"
#include "common.h"
#include <set>
#include <map>


class CUDT;

struct CInfoBlock
{
   uint32_t m_piIP[4];
   int m_iIPversion;
   uint64_t m_ullTimeStamp;
   int m_iRTT;
   int m_iBandwidth;
   int m_iLossRate;
   int m_iReorderDistance;
   double m_dInterval;
   double m_dCWnd;
};

struct CIPComp
{
   bool operator()(const CInfoBlock* ib1, const CInfoBlock* ib2) const;
};

struct CTSComp
{
   bool operator()(const CInfoBlock* ib1, const CInfoBlock* ib2) const;
};

class CCache
{
public:
   CCache();
   CCache(const unsigned int& size);
   ~CCache();

public:
   int lookup(const sockaddr* addr, const int& ver, CInfoBlock* hb);
   void update(const sockaddr* addr, const int& ver, CInfoBlock* hb);

private:
   void convert(const sockaddr* addr, const int& ver, uint32_t* ip);

private:
   unsigned int m_uiSize;
   std::set<CInfoBlock*, CIPComp> m_sIPIndex;
   std::set<CInfoBlock*, CTSComp> m_sTSIndex;

   pthread_mutex_t m_Lock;

private:
   CCache(const CCache&);
   CCache& operator=(const CCache&);
};

#endif
