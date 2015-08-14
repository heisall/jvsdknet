/*****************************************************************************
written by
   Yunhong Gu, last updated 07/10/2009
*****************************************************************************/

#ifndef WIN32
   #include <unistd.h>
   #include <netdb.h>
   #include <arpa/inet.h>
   #include <cerrno>
   #include <cstring>
   #include <cstdlib>
#else
   #include <winsock2.h>
   #include <ws2tcpip.h>
   #ifdef LEGACY_WIN32
      #include <wspiapi.h>
   #endif
#endif
#include <cmath>
#include "queue.h"
#include "core.h"

using namespace std;


CUDTUnited CUDT::s_UDTUnited;

const UDTSOCKET CUDT::INVALID_SOCK = -1;
const int CUDT::ERROR = -1;

const UDTSOCKET UDT::INVALID_SOCK = CUDT::INVALID_SOCK;
const int UDT::ERROR = CUDT::ERROR;

const int32_t CSeqNo::m_iSeqNoTH = 0x3FFFFFFF;
const int32_t CSeqNo::m_iMaxSeqNo = 0x7FFFFFFF;
const int32_t CAckNo::m_iMaxAckSeqNo = 0x7FFFFFFF;
const int32_t CMsgNo::m_iMsgNoTH = 0xFFFFFFF;
const int32_t CMsgNo::m_iMaxMsgNo = 0x1FFFFFFF;

const int CUDT::m_iVersion = 4;
const int CUDT::m_iSYNInterval = 10000;
const int CUDT::m_iSelfClockInterval = 64;


CUDT::CUDT()
{
	m_pSndBuffer = NULL;
	m_pRcvBuffer = NULL;
	m_pSndLossList = NULL;
	m_pRcvLossList = NULL;
	m_pACKWindow = NULL;
	m_pSndTimeWindow = NULL;
	m_pRcvTimeWindow = NULL;
	
	m_pSndQueue = NULL;
	m_pRcvQueue = NULL;
	m_pPeerAddr = NULL;
	m_pRealPeerAddr = NULL;//
	m_pRealSelfAddr = NULL;//
	m_pSNode = NULL;
	m_pRNode = NULL;
	
	m_iRealPeerPort = 0;//
	m_iRealSelfPort = 0;//

	m_nPTLinkID = 0;//
	m_nPTYSTNO = 0;//
	m_nPTYSTADDR = 0;//
	m_nYSTLV = 0;//
	m_nYSTFV = 0;//
	m_nYSTLV_2 = 0;//
	m_nYSTFV_2 = 0;//
	memset(m_chCheckGroup, 0, 4);//
	m_nCheckYSTNO = 0;//
	m_uchVirtual = 0;
	m_nVChannelID = 0;
	m_bLTCP = false;
	m_bFTCP = false;
	// Initilize mutex and condition variables
	initSynch();
	
	// Default UDT configurations
	m_iMSS = 1340;//1500;
	m_bSynSending = false;//true;
	m_bSynRecving = false;//true;
	m_iFlightFlagSize = 25600;
	m_iSndBufSize = 8192;
	m_iRcvBufSize = 8192; //Rcv buffer MUST NOT be bigger than Flight Flag size
	m_Linger.l_onoff = 1;
	m_Linger.l_linger = 180;
	m_iUDPSndBufSize = 65536;
	m_iUDPRcvBufSize = m_iRcvBufSize * m_iMSS;
	m_iIPversion = AF_INET;
	m_bRendezvous = false;
	m_iSndTimeOut = -1;
	m_iRcvTimeOut = -1;
	m_bReuseAddr = true;
	m_llMaxBW = -1;
	m_iTRcvBufSize = 32;//256;
	m_bIFJVP2P = true;
	m_bIFWAIT = true;
	m_bIFConnLimit = true;

	
	m_pCCFactory = new CCCFactory<CUDTCC>;
	m_pCC = NULL;
	m_pCache = NULL;
	
	// Initial status
	m_bOpened = false;
	m_bListening = false;
	m_bConnected = false;
	m_bClosing = false;
	m_bShutdown = false;
	m_bBroken = false;
}

CUDT::CUDT(const CUDT& ancestor)
{
	m_pSndBuffer = NULL;
	m_pRcvBuffer = NULL;
	m_pSndLossList = NULL;
	m_pRcvLossList = NULL;
	m_pACKWindow = NULL;
	m_pSndTimeWindow = NULL;
	m_pRcvTimeWindow = NULL;
	
	m_pSndQueue = NULL;
	m_pRcvQueue = NULL;
	m_pPeerAddr = NULL;
	m_pRealPeerAddr = NULL;//
	m_pRealSelfAddr = NULL;//
	m_iRealPeerPort = 0;//
	m_iRealSelfPort = 0;//
	m_pSNode = NULL;
	m_pRNode = NULL;
	
	m_nPTLinkID = 0;//
	m_nPTYSTNO = 0;//
	m_nPTYSTADDR = 0;//
	m_nYSTLV = 0;//
	m_nYSTFV = 0;//
	m_nYSTLV_2 = 0;//
	m_nYSTFV_2 = 0;//
	memset(m_chCheckGroup, 0, 4);//
	m_nCheckYSTNO = 0;//
	m_uchVirtual = 0;
	m_nVChannelID = 0;
	m_bLTCP = false;
	m_bFTCP = false;
	// Initilize mutex and condition variables
	initSynch();
	
	// Default UDT configurations
	m_iMSS = ancestor.m_iMSS;
	m_bSynSending = ancestor.m_bSynSending;
	m_bSynRecving = ancestor.m_bSynRecving;
	m_iFlightFlagSize = ancestor.m_iFlightFlagSize;
	m_iSndBufSize = ancestor.m_iSndBufSize;
	m_iRcvBufSize = ancestor.m_iRcvBufSize;
	m_Linger = ancestor.m_Linger;
	m_iUDPSndBufSize = ancestor.m_iUDPSndBufSize;
	m_iUDPRcvBufSize = ancestor.m_iUDPRcvBufSize;
	m_iSockType = ancestor.m_iSockType;
	m_iIPversion = ancestor.m_iIPversion;
	m_bRendezvous = ancestor.m_bRendezvous;
	m_iSndTimeOut = ancestor.m_iSndTimeOut;
	m_iRcvTimeOut = ancestor.m_iRcvTimeOut;
	m_bReuseAddr = true;	// this must be true, because all accepted sockets shared the same port with the listener
	m_llMaxBW = ancestor.m_llMaxBW;
	m_iTRcvBufSize = ancestor.m_iTRcvBufSize;
	
	m_bIFJVP2P = ancestor.m_bIFJVP2P;//
	m_bIFWAIT = ancestor.m_bIFWAIT;//
	m_bIFConnLimit = ancestor.m_bIFConnLimit;//
	m_bLTCP = ancestor.m_bLTCP;//
	m_bFTCP = ancestor.m_bFTCP;
	m_nYSTLV = ancestor.m_nYSTLV;//
	m_nYSTLV_2 = ancestor.m_nYSTLV_2;//
	memcpy(m_chCheckGroup, ancestor.m_chCheckGroup, 4);//
	m_nCheckYSTNO = ancestor.m_nCheckYSTNO;//
	
	m_pCCFactory = ancestor.m_pCCFactory->clone();
	m_pCC = NULL;
	m_pCache = ancestor.m_pCache;
	
	// Initial status
	m_bOpened = false;
	m_bListening = false;
	m_bConnected = false;
	m_bClosing = false;
	m_bShutdown = false;
	m_bBroken = false;
}

CUDT::~CUDT()
{
	// release mutex/condtion variables
	destroySynch();
	
	// destroy the data structures
	if(m_pSndBuffer != NULL) 
	{
		delete m_pSndBuffer;
		m_pSndBuffer = NULL;
	}
	if(m_pRcvBuffer != NULL)
	{
		delete m_pRcvBuffer;
		m_pRcvBuffer = NULL;
	}
	if(m_pSndLossList != NULL)
	{
		delete m_pSndLossList;
		m_pSndLossList = NULL;
	}
	if(m_pRcvLossList != NULL)
	{
		delete m_pRcvLossList;
		m_pRcvLossList = NULL;
	}
	if(m_pACKWindow != NULL) 
	{
		delete m_pACKWindow;
		m_pACKWindow = NULL;
	}
	if(m_pSndTimeWindow != NULL)
	{
		delete m_pSndTimeWindow;
		m_pSndTimeWindow = NULL;
	}
	if(m_pRcvTimeWindow != NULL) 
	{
		delete m_pRcvTimeWindow;
		m_pRcvTimeWindow = NULL;
	}
	if(m_pCCFactory != NULL) 
	{
		delete m_pCCFactory;
		m_pCCFactory = NULL;
	}
	if(m_pCC != NULL) 
	{
		delete m_pCC;
		m_pCC = NULL;
	}
	if(m_pPeerAddr != NULL) 
	{
		delete m_pPeerAddr;
		m_pPeerAddr = NULL;
	}
	if(m_pRealPeerAddr != NULL) 
	{
		delete m_pRealPeerAddr;
		m_pRealPeerAddr = NULL;
	}
	if(m_pRealSelfAddr != NULL) 
	{
		delete m_pRealSelfAddr;
		m_pRealSelfAddr = NULL;
	}
	if(m_pSNode != NULL)
	{
		delete m_pSNode;
		m_pSNode = NULL;
	}
	if(m_pRNode != NULL) 
	{
		delete m_pRNode;
		m_pRNode = NULL;
	}
}

void CUDT::setOpt(UDTOpt optName, const void* optval, const int&)
{
	CGuard cg(m_ConnectionLock);
	CGuard sendguard(m_SendLock);
	CGuard recvguard(m_RecvLock);
	
	switch (optName)
	{
	case UDT_MSS:
		if (m_bOpened)
			throw CUDTException(5, 1, 0);
		
		if (*(int*)optval < int(28 + sizeof(CHandShake)))
			throw CUDTException(5, 3, 0);
		
		m_iMSS = *(int*)optval;
		
		// Packet size cannot be greater than UDP buffer size
		if (m_iMSS > m_iUDPSndBufSize)
			m_iMSS = m_iUDPSndBufSize;
		if (m_iMSS > m_iUDPRcvBufSize)
			m_iMSS = m_iUDPRcvBufSize;
		
		break;
		
	case UDT_SNDSYN:
		m_bSynSending = *(bool *)optval;
		break;
		
	case UDT_RCVSYN:
		m_bSynRecving = *(bool *)optval;
		break;
		
	case UDT_CC:
		if (m_bConnected)
			throw CUDTException(5, 1, 0);
		if (NULL != m_pCCFactory)
		{
			delete m_pCCFactory;
			m_pCCFactory = NULL;
		}
		m_pCCFactory = ((CCCVirtualFactory *)optval)->clone();
		
		break;
		
	case UDT_FC:
		if (m_bConnected)
			throw CUDTException(5, 2, 0);
		
		if (*(int*)optval < 1)
			throw CUDTException(5, 3);
		
		// Mimimum recv flight flag size is 32 packets
		if (*(int*)optval > 32)
			m_iFlightFlagSize = *(int*)optval;
		else
			m_iFlightFlagSize = 32;
		
		break;
		
	case UDT_SNDBUF:
		if (m_bOpened)
			throw CUDTException(5, 1, 0);
		
		if (*(int*)optval <= 0)
			throw CUDTException(5, 3, 0);
		
		m_iSndBufSize = *(int*)optval / (m_iMSS - 28);
		
		break;
		
	case UDT_RCVBUF:
		if (m_bOpened)
			throw CUDTException(5, 1, 0);
		
		if (*(int*)optval <= 0)
			throw CUDTException(5, 3, 0);
		
		// Mimimum recv buffer size is 32 packets
		if (*(int*)optval > (m_iMSS - 28) * 32)
			m_iRcvBufSize = *(int*)optval / (m_iMSS - 28);
		else
			m_iRcvBufSize = 32;
		
		// recv buffer MUST not be greater than FC size
		if (m_iRcvBufSize > m_iFlightFlagSize)
			m_iRcvBufSize = m_iFlightFlagSize;
		
		break;

	case UDT_TRCVBUF:
		if (m_bOpened)
			throw CUDTException(5, 1, 0);
		
		if (*(int*)optval <= 0)
			throw CUDTException(5, 3, 0);
		
		// Mimimum recv buffer size is 32 packets
		if (*(int*)optval > (m_iMSS - 28) * 32)//256
			m_iTRcvBufSize = *(int*)optval / (m_iMSS - 28);
		else
			m_iTRcvBufSize = 32;//256;
		
		break;

	case UDT_IFJVP2P:
//		if (m_bOpened)
//			throw CUDTException(5, 1, 0);
		
		if (*(int*)optval < 0)
			throw CUDTException(5, 3, 0);
		
		if (*(int*)optval > 0)
			m_bIFJVP2P = true;
		else
			m_bIFJVP2P = false;
		
		break;
	case UDT_IFWAIT:
		if (*(int*)optval < 0)
			throw CUDTException(5, 3, 0);
		
		if (*(int*)optval > 0)
			m_bIFWAIT = true;
		else
			m_bIFWAIT = false;
		
		break;
	case UDT_CONNLIMIT:
		if (*(int*)optval < 0)
			throw CUDTException(5, 3, 0);
		
		if (*(int*)optval > 0)
			m_bIFConnLimit = true;
		else
			m_bIFConnLimit = false;
		
		break;
	case UDT_LTCP:
		if (*(int*)optval < 0)
			throw CUDTException(5, 3, 0);
		
		if (*(int*)optval > 0)
			m_bLTCP = true;
		else
			m_bLTCP = false;
		
		break;
	
	case UDT_YSTVER:
		m_nYSTLV = *(int*)optval;
		m_nYSTLV_2 = *(int*)optval;
		break;
	case UDT_CHECKGROUP:
		memcpy(m_chCheckGroup, (char*)optval, 4);
		break;
	case UDT_CHECKYSTNO:
		m_nCheckYSTNO = *(int*)optval;
		break;
		
	case UDT_LINGER:
		m_Linger = *(linger*)optval;
		break;
		
	case UDP_SNDBUF:
		if (m_bOpened)
			throw CUDTException(5, 1, 0);
		
		m_iUDPSndBufSize = *(int*)optval;
		
		if (m_iUDPSndBufSize < m_iMSS)
			m_iUDPSndBufSize = m_iMSS;
		
		break;
		
	case UDP_RCVBUF:
		if (m_bOpened)
			throw CUDTException(5, 1, 0);
		
		m_iUDPRcvBufSize = *(int*)optval;
		
		if (m_iUDPRcvBufSize < m_iMSS)
			m_iUDPRcvBufSize = m_iMSS;
		
		break;
		
	case UDT_RENDEZVOUS:
		if (m_bConnected)
			throw CUDTException(5, 1, 0);
		m_bRendezvous = *(bool *)optval;
		break;
		
	case UDT_SNDTIMEO: 
		m_iSndTimeOut = *(int*)optval; 
		break; 
		
	case UDT_RCVTIMEO: 
		m_iRcvTimeOut = *(int*)optval; 
		break; 
		
	case UDT_REUSEADDR:
		if (m_bOpened)
			throw CUDTException(5, 1, 0);
		m_bReuseAddr = *(bool*)optval;
		break;
		
	case UDT_MAXBW:
		if (m_bConnected)
			throw CUDTException(5, 1, 0);
		m_llMaxBW = *(int64_t*)optval;
		break;
		
	default:
		throw CUDTException(5, 0, 0);
   }
}

void CUDT::getOpt(UDTOpt optName, void* optval, int& optlen)
{
	CGuard cg(m_ConnectionLock);
	
	switch (optName)
	{
	case UDT_MSS:
		*(int*)optval = m_iMSS;
		optlen = sizeof(int);
		break;
		
	case UDT_SNDSYN:
		*(bool*)optval = m_bSynSending;
		optlen = sizeof(bool);
		break;
		
	case UDT_RCVSYN:
		*(bool*)optval = m_bSynRecving;
		optlen = sizeof(bool);
		break;
		
	case UDT_CC:
		if (!m_bOpened)
			throw CUDTException(5, 5, 0);
		*(CCC**)optval = m_pCC;
		optlen = sizeof(CCC*);
		
		break;
		
	case UDT_FC:
		*(int*)optval = m_iFlightFlagSize;
		optlen = sizeof(int);
		break;
		
	case UDT_SNDBUF:
		*(int*)optval = m_iSndBufSize * (m_iMSS - 28);
		optlen = sizeof(int);
		break;
		
	case UDT_RCVBUF:
		*(int*)optval = m_iRcvBufSize * (m_iMSS - 28);
		optlen = sizeof(int);
		break;
		
	case UDT_TRCVBUF:
		*(int*)optval = m_iTRcvBufSize * (m_iMSS - 28);
		optlen = sizeof(int);
		break;
	case UDT_IFJVP2P:
		*(int*)optval = (m_bIFJVP2P?1:0);
		optlen = sizeof(int);
		break;
	case UDT_IFWAIT:
		*(int*)optval = (m_bIFWAIT?1:0);
		optlen = sizeof(int);
		break;
	case UDT_CONNLIMIT:
		*(int*)optval = (m_bIFConnLimit?1:0);
		optlen = sizeof(int);
		break;
	case UDT_LTCP:
		*(int*)optval = (m_bLTCP?1:0);
		optlen = sizeof(int);
		break;

	case UDT_YSTVER:
		//*(int*)optval = m_nYSTLV;
		*(int*)optval = m_nYSTLV_2;
		optlen = sizeof(int);
		break;
	case UDT_CHECKGROUP:
		memcpy((char*)optval, m_chCheckGroup, 4);
		optlen = 4;
		break;
	case UDT_CHECKYSTNO:
		*(int*)optval = m_nCheckYSTNO;
		optlen = sizeof(int);
		break;

	case UDT_LINGER:
		if (optlen < (int)(sizeof(linger)))
			throw CUDTException(5, 3, 0);
		
		*(linger*)optval = m_Linger;
		optlen = sizeof(linger);
		break;
		
	case UDP_SNDBUF:
		*(int*)optval = m_iUDPSndBufSize;
		optlen = sizeof(int);
		break;
		
	case UDP_RCVBUF:
		*(int*)optval = m_iUDPRcvBufSize;
		optlen = sizeof(int);
		break;
		
	case UDT_RENDEZVOUS:
		*(bool *)optval = m_bRendezvous;
		optlen = sizeof(bool);
		break;
		
	case UDT_SNDTIMEO: 
		*(int*)optval = m_iSndTimeOut; 
		optlen = sizeof(int); 
		break; 
		
	case UDT_RCVTIMEO: 
		*(int*)optval = m_iRcvTimeOut; 
		optlen = sizeof(int); 
		break; 
		
	case UDT_REUSEADDR:
		*(bool *)optval = m_bReuseAddr;
		optlen = sizeof(bool);
		break;
		
	case UDT_MAXBW:
		*(int64_t*)optval = m_llMaxBW;
		break;
		
	default:
		throw CUDTException(5, 0, 0);
	}
}

void CUDT::open()
{
	CGuard cg(m_ConnectionLock);
	
	// Initial sequence number, loss, acknowledgement, etc.
	m_iPktSize = m_iMSS - 28;
	m_iPayloadSize = m_iPktSize - CPacket::m_iPktHdrSize;
	
	m_iEXPCount = 1;
	m_iBandwidth = 1;
	m_iDeliveryRate = 16;
   m_iAckSeqNo = 0;
   m_ullLastAckTime = 0;

   // trace information
   m_StartTime = CTimer::getTime();
   m_llSentTotal = m_llRecvTotal = m_iSndLossTotal = m_iRcvLossTotal = m_iRetransTotal = m_iSentACKTotal = m_iRecvACKTotal = m_iSentNAKTotal = m_iRecvNAKTotal = 0;
   m_LastSampleTime = CTimer::getTime();
   m_llTraceSent = m_llTraceRecv = m_iTraceSndLoss = m_iTraceRcvLoss = m_iTraceRetrans = m_iSentACK = m_iRecvACK = m_iSentNAK = m_iRecvNAK = 0;
   m_llSndDuration = m_llSndDurationTotal = 0;

   // structures for queue
   if (NULL == m_pSNode)
      m_pSNode = new CSNode;
   m_pSNode->m_pUDT = this;
   m_pSNode->m_llTimeStamp = 1;
   m_pSNode->m_iHeapLoc = -1;

   if (NULL == m_pRNode)
   {
	   m_pRNode = new CRNode;
   }
   m_pRNode->m_pUDT = this;
   m_pRNode->m_llTimeStamp = 1;
   m_pRNode->m_pPrev = m_pRNode->m_pNext = NULL;
   m_pRNode->m_bOnList = false;

   m_iRTT = 10 * m_iSYNInterval;
   m_iRTTVar = m_iRTT >> 1;
   m_ullCPUFrequency = CTimer::getCPUFrequency();

   // set up the timers
   m_ullSYNInt = m_iSYNInterval * m_ullCPUFrequency;
   
   m_ullACKInt = m_ullSYNInt;//m_ullSYNInt/10;//m_ullSYNInt;
   m_ullNAKInt = (m_iRTT + 4 * m_iRTTVar) * m_ullCPUFrequency;
   m_ullMinEXPInt = 100000 * m_ullCPUFrequency;
   m_llLastRspTime = CTimer::getTime();

   CTimer::rdtsc(m_ullNextACKTime);
   m_ullNextACKTime += m_ullSYNInt;
   CTimer::rdtsc(m_ullNextNAKTime);
   m_ullNextNAKTime += m_ullNAKInt;
   CTimer::rdtsc(m_ullNextEXPTime);
   m_ullNextEXPTime += m_ullMinEXPInt;

   m_iPktCount = 0;
   m_iLightACKCount = 1;

   m_ullTargetTime = 0;
   m_ullTimeDiff = 0;

   // Now UDT is opened.
   m_bOpened = true;
}

void CUDT::listen()
{
   CGuard cg(m_ConnectionLock);

   if (!m_bOpened)
      throw CUDTException(5, 0, 0);

   if (m_bConnected)
      throw CUDTException(5, 2, 0);

   // listen can be called more than once
   if (m_bListening)
      return;

   // if there is already another socket listening on the same port
   if (m_pRcvQueue->setListener(this) < 0)
      throw CUDTException(5, 11, 0);

//   m_bListening = true;
}

void CUDT::connect(const sockaddr* serv_addr, 
				   int nChannelID, 
				   char chGroup[4], int nYSTNO, 
				   int nPTLinkID, int nPTYSTNO, int nPTYSTADDR, 
				   char chCheckGroup[4],
				   int nCheckYSTNO,
				   int nYSTLV_new,
				   int nYSTLV_old,
				   int nMinTime,
				   unsigned char uchVirtual,
				   int nVChannelID,
                   unsigned char uchLLTCP,
                   BOOL *pbQuickExit)
{
   CGuard cg(m_ConnectionLock);

   if (!m_bOpened)
      throw CUDTException(5, 0, 0);

   if (m_bListening)
      throw CUDTException(5, 2, 0);

   if (m_bConnected)
      throw CUDTException(5, 2, 0);

   // register this socket in the rendezvous queue
   m_pRcvQueue->m_pRendezvousQueue->insert(m_SocketID, m_iIPversion, serv_addr);

   CPacket request;
   char* reqdata = new char [m_iPayloadSize];
   memset(reqdata,0,m_iPayloadSize);
   CHandShake* req = (CHandShake *)reqdata;

   CPacket response;
   char* resdata = new char [m_iPayloadSize];
   memset(resdata,0,m_iPayloadSize);
   CHandShake* res = (CHandShake *)resdata;

   // This is my current configurations.
   req->m_iVersion = m_iVersion;
   req->m_iType = m_iSockType;
   req->m_iMSS = m_iMSS;
   req->m_iFlightFlagSize = (m_iRcvBufSize < m_iFlightFlagSize)? m_iRcvBufSize : m_iFlightFlagSize;
   req->m_iReqType = (!m_bRendezvous) ? 1 : 0;
   req->m_iID = m_SocketID;
   CIPAddress::ntop(serv_addr, req->m_piPeerIP, m_iIPversion);

   req->m_piRealPeerIP[0] = 0;//
   req->m_piRealPeerIP[1] = 0;//
   req->m_piRealPeerIP[2] = 0;//
   req->m_piRealPeerIP[3] = 0;//
   req->m_piRealSelfIP[0] = 0;//
   req->m_piRealSelfIP[1] = 0;//
   req->m_piRealSelfIP[2] = 0;//
   req->m_piRealSelfIP[3] = 0;//
   req->m_iRealPeerPort = 0;//
   req->m_iRealSelfPort = 0;//

   req->m_nChannelID = nChannelID;
   req->m_nPTLinkID = nPTLinkID;
   req->m_nPTYSTNO = nPTYSTNO;
   req->m_nPTYSTADDR = nPTYSTADDR;

   memcpy(req->m_chCheckGroup, chCheckGroup,4);
   req->m_nCheckYSTNO = nCheckYSTNO;
   req->m_nYSTLV = nYSTLV_old;
   req->m_nYSTFV = 0;
   req->m_nYSTLV_2 = nYSTLV_new;
   req->m_nYSTFV_2 = 0;

   req->m_uchVirtual = uchVirtual;
   req->m_nVChannelID = nVChannelID;

   req->m_uchLTCP = uchLLTCP;
   req->m_uchFTCP = 0;
   // Random Initial Sequence Number
   srand((unsigned int)CTimer::getTime());
   m_iISN = req->m_iISN = (int32_t)(CSeqNo::m_iMaxSeqNo * (double(rand()) / RAND_MAX));

   m_iLastDecSeq = req->m_iISN - 1;
   m_iSndLastAck = req->m_iISN;
   m_iSndLastDataAck = req->m_iISN;
   m_iSndCurrSeqNo = req->m_iISN - 1;
   m_iSndLastAck2 = req->m_iISN;
   m_ullSndLastAck2Time = CTimer::getTime();

   // Inform the server my configurations.
   request.pack(0, NULL, reqdata, sizeof(CHandShake));
   // ID = 0, connection request
   request.m_iID = 0;

   // Wait for the negotiated configurations from the peer side.
   response.pack(0, NULL, resdata, sizeof(CHandShake));

//   uint64_t timeo = 3000000;//1000000;
//   if(nYSTNO > 0)
//   {//转发链接时适当延长连接时间
//	   timeo = 3000000;
//   }
   DWORD dwbegin = CTimer::GetTime();
   if(nMinTime <= 0 || nMinTime > 30000)
   {
	   nMinTime = 3000;
   }
   int ntimeout = nMinTime;//3000;//连接超时时间3s，当明确收到握手包时，连接超时时间变为6s，尽可能建立连接

//   if (m_bRendezvous)
//   {
//	   timeo *= 10;
//   }
//   uint64_t entertime = CTimer::getTime();
   CUDTException e(0, 0);

   char* tmp = NULL;

   //while (!m_bClosing)
    while ((pbQuickExit == NULL && !m_bClosing) || ((pbQuickExit && *pbQuickExit ==  false) && !m_bClosing))
   {
	  m_pSndQueue->sendto(serv_addr, request, NULL, nYSTNO, chGroup);

      response.setLength(m_iPayloadSize);
      if (m_pRcvQueue->recvfrom(m_SocketID, response) > 0)
      {
         if (m_bRendezvous && ((0 == response.getFlag()) || (1 == response.getType())) && (NULL != tmp))
         {
            // a data packet or a keep-alive packet comes, which means the peer side is already connected
            // in this situation, a previously recorded response (tmp) will be used
            memcpy(resdata, tmp, sizeof(CHandShake));
            memcpy(m_piSelfIP, res->m_piPeerIP, 16);

			memcpy(m_piRealSelfIP, res->m_piRealPeerIP, 16);//
			memcpy(m_piRealPeerIP, res->m_piRealSelfIP, 16);//
			m_iRealPeerPort = res->m_iRealSelfPort;
			m_iRealSelfPort = res->m_iRealPeerPort;
            break;
         }

         if ((1 != response.getFlag()) || (0 != response.getType()))
            response.setLength(-1);

         if (m_bRendezvous)
         {
            // regular connect should NOT communicate with rendezvous connect
            // rendezvous connect require 3-way handshake
            if (1 == res->m_iReqType)
               response.setLength(-1);
            else if ((0 == res->m_iReqType) || (0 == req->m_iReqType))
            {
				if(tmp == NULL)
				{
					tmp = new char [m_iPayloadSize];
				}
               
               memcpy(tmp, resdata, sizeof(CHandShake));

               req->m_iReqType = -1;
               request.m_iID = res->m_iID;
               response.setLength(-1);
            }
         }
         else
         {
			//timeo = 6000000;//收到握手包 为提高连通率 加长握手时间(初始时间较短能避免完全不通时浪费时间)
			 ntimeout = 6000;
            // set cookie
            if (1 == res->m_iReqType)
            {
               req->m_iReqType = -1;
               req->m_iCookie = res->m_iCookie;
               response.setLength(-1);
            }
         }
      }

      if (response.getLength() > 0)
      {
         memcpy(m_piSelfIP, res->m_piPeerIP, 16);
		 memcpy(m_piRealSelfIP, res->m_piRealPeerIP, 16);//
		 memcpy(m_piRealPeerIP, res->m_piRealSelfIP, 16);//
		 m_iRealPeerPort = res->m_iRealSelfPort;
		 m_iRealSelfPort = res->m_iRealPeerPort;
         break;
      }

      //if (CTimer::getTime() > entertime + timeo)
	  if(CTimer::GetTime() > dwbegin + ntimeout)
      {
         // timeout
         e = CUDTException(1, 1, 0);
         break;
      }
   }

    if (pbQuickExit && *pbQuickExit==true) {
        printf("quick exit \n");
    }
   delete[] tmp;
   tmp = NULL;
   delete[] reqdata;
   reqdata = NULL;

   if (e.getErrorCode() == 0)
   {
      if (m_bClosing)						// if the socket is closed before connection...
         e = CUDTException(1);
      else if (1002 == res->m_iReqType)				// connection request rejected
         e = CUDTException(1, 2, 0);
      else if ((!m_bRendezvous) && (m_iISN != res->m_iISN))	// secuity check
         e = CUDTException(1, 4, 0);
   }

   if (e.getErrorCode() != 0)
   {
      // connection failure, clean up and throw exception
      delete[] resdata;
	  resdata = NULL;

//      if (m_bRendezvous)
         m_pRcvQueue->m_pRendezvousQueue->remove(m_SocketID);

      throw e;
   }

   // Got it. Re-configure according to the negotiated values.
   m_iMSS = res->m_iMSS;
   m_iFlowWindowSize = res->m_iFlightFlagSize;
   m_iPktSize = m_iMSS - 28;
   m_iPayloadSize = m_iPktSize - CPacket::m_iPktHdrSize;
   m_iPeerISN = res->m_iISN;
   m_iRcvLastAck = res->m_iISN;
   m_iRcvLastAckAck = res->m_iISN;
   m_iRcvCurrSeqNo = res->m_iISN - 1;
   m_PeerID = res->m_iID;

   m_nYSTFV = res->m_nYSTFV;
   m_nYSTLV = nYSTLV_old;
   m_nYSTFV_2 = res->m_nYSTFV_2;
   m_nYSTLV_2 = nYSTLV_new;

   m_bFTCP = (res->m_uchFTCP==1?true:false);
   m_bLTCP = (uchLLTCP==1?true:false);

   delete[] resdata;
   resdata = NULL;

   // Prepare all data structures
   try
   {
      m_pSndBuffer = new CSndBuffer(32, m_iPayloadSize);//32//256
      m_pRcvBuffer = new CRcvBuffer(m_iRcvBufSize, &(m_pRcvQueue->m_UnitQueue));
      // after introducing lite ACK, the sndlosslist may not be cleared in time, so it requires twice space.
      m_pSndLossList = new CSndLossList(m_iFlowWindowSize * 2);
      m_pRcvLossList = new CRcvLossList(m_iFlightFlagSize);
      m_pACKWindow = new CACKWindow(4096);
      m_pRcvTimeWindow = new CPktTimeWindow(16, 64);
      m_pSndTimeWindow = new CPktTimeWindow();
   }
   catch (...)
   {
	   if(m_pSndBuffer != NULL)
	   {
		   delete m_pSndBuffer;
		   m_pSndBuffer = NULL;
	   }
	   if(m_pRcvBuffer != NULL)
	   {
		   delete m_pRcvBuffer;
		   m_pRcvBuffer = NULL;
	   }
	   if(m_pSndLossList != NULL)
	   {
		   delete m_pSndLossList;
		   m_pSndLossList = NULL;
	   }
	   if(m_pRcvLossList != NULL)
	   {
		   delete m_pRcvLossList;
		   m_pRcvLossList = NULL;
	   }
	   if(m_pACKWindow != NULL)
	   {
		   delete m_pACKWindow;
		   m_pACKWindow = NULL;
	   }
	   if(m_pRcvTimeWindow != NULL)
	   {
		   delete m_pRcvTimeWindow;
		   m_pRcvTimeWindow = NULL;
	   }
	   if(m_pSndTimeWindow != NULL)
	   {
		   delete m_pSndTimeWindow;
		   m_pSndTimeWindow = NULL;
	   }
      throw CUDTException(3, 2, 0);
   }

   m_pCC = m_pCCFactory->create();
   m_pCC->m_UDT = m_SocketID;
   m_ullInterval = (uint64_t)(m_pCC->m_dPktSndPeriod * m_ullCPUFrequency);
   m_dCongestionWindow = m_pCC->m_dCWndSize;

   CInfoBlock ib;
   if (m_pCache->lookup(serv_addr, m_iIPversion, &ib) >= 0)
   {
      m_iRTT = ib.m_iRTT;
      m_iBandwidth = ib.m_iBandwidth;
   }

   m_pCC->setMSS(m_iMSS);
   m_pCC->setMaxCWndSize((int&)m_iFlowWindowSize);
   m_pCC->setSndCurrSeqNo((int32_t&)m_iSndCurrSeqNo);
   m_pCC->setRcvRate(m_iDeliveryRate);
   m_pCC->setRTT(m_iRTT);
   m_pCC->setBandwidth(m_iBandwidth);
   if (m_llMaxBW > 0) m_pCC->setUserParam((char*)&(m_llMaxBW), 8);
   m_pCC->init();

   m_pPeerAddr = (AF_INET == m_iIPversion) ? (sockaddr*)new sockaddr_in : (sockaddr*)new sockaddr_in6;
   memcpy(m_pPeerAddr, serv_addr, (AF_INET == m_iIPversion) ? sizeof(sockaddr_in) : sizeof(sockaddr_in6));

   if(m_piRealPeerIP[0] != 0 || m_piRealPeerIP[1] != 0 || m_piRealPeerIP[2] != 0 || m_piRealPeerIP[3] != 0 && m_iRealPeerPort > 0)
   {
	   m_pRealPeerAddr = (AF_INET == m_iIPversion) ? (sockaddr*)new sockaddr_in : (sockaddr*)new sockaddr_in6;
	   m_pRealSelfAddr = (AF_INET == m_iIPversion) ? (sockaddr*)new sockaddr_in : (sockaddr*)new sockaddr_in6;
	   CIPAddress::pton(m_pRealPeerAddr, m_piRealPeerIP, m_iIPversion);//..
	   CIPAddress::pton(m_pRealSelfAddr, m_piRealSelfIP, m_iIPversion);//..
	   ((SOCKADDR_IN*)m_pRealPeerAddr)->sin_port = htons(m_iRealPeerPort);
	   ((SOCKADDR_IN*)m_pRealSelfAddr)->sin_port = htons(m_iRealSelfPort);
   }
   
   // And, I am connected too.
   m_bConnected = true;

   // register this socket for receiving data packets
   m_pRNode->m_bOnList = true;
   m_pRcvQueue->setNewEntry(this);

   // remove from rendezvous queue
   m_pRcvQueue->m_pRendezvousQueue->remove(m_SocketID);
}

void CUDT::connect(const sockaddr* peer, CHandShake* hs)
{
   // Type 0 (handshake) control packet
   CPacket initpkt;
   CHandShake ci;
   memcpy(&ci, hs, sizeof(CHandShake));
   initpkt.pack(0, NULL, &ci, sizeof(CHandShake));

   // Uses the smaller MSS between the peers        
   if (ci.m_iMSS > m_iMSS)
      ci.m_iMSS = m_iMSS;
   else
      m_iMSS = ci.m_iMSS;

   // exchange info for maximum flow window size
   m_iFlowWindowSize = ci.m_iFlightFlagSize;
   ci.m_iFlightFlagSize = (m_iRcvBufSize < m_iFlightFlagSize)? m_iRcvBufSize : m_iFlightFlagSize;

   m_iPeerISN = ci.m_iISN;

   m_iRcvLastAck = ci.m_iISN;
   m_iRcvLastAckAck = ci.m_iISN;
   m_iRcvCurrSeqNo = ci.m_iISN - 1;

   m_PeerID = ci.m_iID;
   ci.m_iID = m_SocketID;

   // use peer's ISN and send it back for security check
   m_iISN = ci.m_iISN;

   m_iLastDecSeq = m_iISN - 1;
   m_iSndLastAck = m_iISN;
   m_iSndLastDataAck = m_iISN;
   m_iSndCurrSeqNo = m_iISN - 1;
   m_iSndLastAck2 = m_iISN;
   m_ullSndLastAck2Time = CTimer::getTime();

   // this is a reponse handshake
   ci.m_iReqType = -1;

   // get local IP address and send the peer its IP address (because UDP cannot get local IP address)
   memcpy(m_piSelfIP, ci.m_piPeerIP, 16);
   CIPAddress::ntop(peer, ci.m_piPeerIP, m_iIPversion);

   // Save the negotiated configurations.
   memcpy(hs, &ci, sizeof(CHandShake));
  
   m_iPktSize = m_iMSS - 28;
   m_iPayloadSize = m_iPktSize - CPacket::m_iPktHdrSize;

   // Prepare all structures
   try
   {
      m_pSndBuffer = new CSndBuffer(32, m_iPayloadSize);//32//256
      m_pRcvBuffer = new CRcvBuffer(m_iRcvBufSize, &(m_pRcvQueue->m_UnitQueue));
      m_pSndLossList = new CSndLossList(m_iFlowWindowSize * 2);
      m_pRcvLossList = new CRcvLossList(m_iFlightFlagSize);
      m_pACKWindow = new CACKWindow(4096);
      m_pRcvTimeWindow = new CPktTimeWindow(16, 64);
      m_pSndTimeWindow = new CPktTimeWindow();
   }
   catch (...)
   {
	   if(m_pSndBuffer != NULL)
	   {
		   delete m_pSndBuffer;
		   m_pSndBuffer = NULL;
	   }
	   if(m_pRcvBuffer != NULL)
	   {
		   delete m_pRcvBuffer;
		   m_pRcvBuffer = NULL;
	   }
	   if(m_pSndLossList != NULL)
	   {
		   delete m_pSndLossList;
		   m_pSndLossList = NULL;
	   }
	   if(m_pRcvLossList != NULL)
	   {
		   delete m_pRcvLossList;
		   m_pRcvLossList = NULL;
	   }
	   if(m_pACKWindow != NULL)
	   {
		   delete m_pACKWindow;
		   m_pACKWindow = NULL;
	   }
	   if(m_pRcvTimeWindow != NULL)
	   {
		   delete m_pRcvTimeWindow;
		   m_pRcvTimeWindow = NULL;
	   }
	   if(m_pSndTimeWindow != NULL)
	   {
		   delete m_pSndTimeWindow;
		   m_pSndTimeWindow = NULL;
	   }
      throw CUDTException(3, 2, 0);
   }

   m_pCC = m_pCCFactory->create();
   m_pCC->m_UDT = m_SocketID;
   m_ullInterval = (uint64_t)(m_pCC->m_dPktSndPeriod * m_ullCPUFrequency);
   m_dCongestionWindow = m_pCC->m_dCWndSize;

   CInfoBlock ib;
   if (m_pCache->lookup(peer, m_iIPversion, &ib) >= 0)
   {
      m_iRTT = ib.m_iRTT;
      m_iBandwidth = ib.m_iBandwidth;
   }

   m_pCC->setMSS(m_iMSS);
   m_pCC->setMaxCWndSize((int&)m_iFlowWindowSize);
   m_pCC->setSndCurrSeqNo((int32_t&)m_iSndCurrSeqNo);
   m_pCC->setRcvRate(m_iDeliveryRate);
   m_pCC->setRTT(m_iRTT);
   m_pCC->setBandwidth(m_iBandwidth);
   if (m_llMaxBW > 0) m_pCC->setUserParam((char*)&(m_llMaxBW), 8);
   m_pCC->init();

   m_pPeerAddr = (AF_INET == m_iIPversion) ? (sockaddr*)new sockaddr_in : (sockaddr*)new sockaddr_in6;
   memcpy(m_pPeerAddr, peer, (AF_INET == m_iIPversion) ? sizeof(sockaddr_in) : sizeof(sockaddr_in6));


   if(hs->m_piRealSelfIP[0] != 0 || hs->m_piRealSelfIP[1] != 0 || hs->m_piRealSelfIP[2] != 0 || hs->m_piRealSelfIP[3] != 0 && hs->m_iRealSelfPort > 0)
   {
	   //..
	   m_pRealSelfAddr = (AF_INET == m_iIPversion) ? (sockaddr*)new sockaddr_in : (sockaddr*)new sockaddr_in6;
	   m_pRealPeerAddr = (AF_INET == m_iIPversion) ? (sockaddr*)new sockaddr_in : (sockaddr*)new sockaddr_in6;
	   CIPAddress::pton(m_pRealPeerAddr, hs->m_piRealSelfIP, m_iIPversion);
	   CIPAddress::pton(m_pRealSelfAddr, hs->m_piRealPeerIP, m_iIPversion);
	   ((sockaddr_in*)m_pRealPeerAddr)->sin_port = htons(hs->m_iRealSelfPort);
	   ((sockaddr_in*)m_pRealSelfAddr)->sin_port = htons(hs->m_iRealPeerPort);
   }

   // And of course, it is connected.
   m_bConnected = true;

   // register this socket for receiving data packets
   m_pRNode->m_bOnList = true;
   m_pRcvQueue->setNewEntry(this);
}

void CUDT::close()
{
   if (!m_bOpened)
      return;

   if (!m_bConnected)
      m_bClosing = true;

   if (0 != m_Linger.l_onoff)
   {
      uint64_t entertime = CTimer::getTime();

//      while (!m_bBroken && m_bConnected && (m_pSndBuffer->getCurrBufSize() > 0) && (CTimer::getTime() - entertime < m_Linger.l_linger * 1000000ULL))
	  while (!m_bBroken && m_bConnected && (m_pSndBuffer->getCurrBufSize() > 0) && (CTimer::getTime() - entertime < (uint64_t)m_Linger.l_linger * 1000000L))
      {
         #ifndef WIN32
            timespec ts;
            ts.tv_sec = 0;
            ts.tv_nsec = 1000000;
            nanosleep(&ts, NULL);
         #else
            Sleep(1);
         #endif
      }
   }

   // remove this socket from the snd queue
   if (m_bConnected)
      m_pSndQueue->m_pSndUList->remove(this);

   CGuard cg(m_ConnectionLock);

   // Inform the threads handler to stop.
   m_bClosing = true;

   // Signal the sender and recver if they are waiting for data.
   releaseSynch();

   if (m_bListening)
   {
      m_bListening = false;
      m_pRcvQueue->removeListener(this);
   }
   if (m_bConnected)
   {
      if (!m_bShutdown)
         sendCtrl(5);

      m_pCC->close();

      CInfoBlock ib;
      ib.m_iRTT = m_iRTT;
      ib.m_iBandwidth = m_iBandwidth;
	#ifdef WIN32
	  m_pCache->update(m_pPeerAddr, m_iIPversion, &ib);//保证DVR中释放内存屏蔽
	#endif

      m_bConnected = false;
   }

   // waiting all send and recv calls to stop
   CGuard sendguard(m_SendLock);
   CGuard recvguard(m_RecvLock);

   // CLOSED.
   m_bOpened = false;
}


int CUDT::send(const char* data, const int& len, bool bonce)
{
   if (UDT_DGRAM == m_iSockType)
   {
    #ifdef WIN32
	   throw CUDTException(5, 10, 0);
	#else
	   return -1;
	#endif
   }
   // throw an exception if not connected
   if (m_bBroken || m_bClosing)
   {
    #ifdef WIN32
	   throw CUDTException(2, 1, 0);
	#else
	   return -1;
	#endif
   }
   else if (!m_bConnected)
   {
   #ifdef WIN32
	   throw CUDTException(2, 2, 0);
	#else
	   return -1;
	#endif
   }

   if (len <= 0)
      return 0;

   CGuard sendguard(m_SendLock);

   if (m_iSndBufSize <= m_pSndBuffer->getCurrBufSize())
   {
      if (!m_bSynSending)
	  {
		  //throw CUDTException(6, 1, 0);
		  return 0;
	  }
      else
      {
         // wait here during a blocking sending
         #ifndef WIN32
            pthread_mutex_lock(&m_SendBlockLock);
            if (m_iSndTimeOut < 0) 
            { 
               while (!m_bBroken && m_bConnected && !m_bClosing && (m_iSndBufSize <= m_pSndBuffer->getCurrBufSize()))
                  pthread_cond_wait(&m_SendBlockCond, &m_SendBlockLock);
            }
            else
            {
               uint64_t exptime = CTimer::getTime() + m_iSndTimeOut * 1000ULL;
               timespec locktime; 
    
               locktime.tv_sec = exptime / 1000000;
               locktime.tv_nsec = (exptime % 1000000) * 1000;
    
               pthread_cond_timedwait(&m_SendBlockCond, &m_SendBlockLock, &locktime);
            }
            pthread_mutex_unlock(&m_SendBlockLock);
         #else
            if (m_iSndTimeOut < 0)
            {
               while (!m_bBroken && m_bConnected && !m_bClosing && (m_iSndBufSize <= m_pSndBuffer->getCurrBufSize()))
                  WaitForSingleObject(m_SendBlockCond, INFINITE);
            }
            else 
               WaitForSingleObject(m_SendBlockCond, DWORD(m_iSndTimeOut)); 
         #endif

         // check the connection status
         if (m_bBroken || m_bClosing)
		 {
		#ifdef WIN32
			 throw CUDTException(2, 1, 0);
		#else
			 return -1;
		#endif
		 }
         else if (!m_bConnected)
		 {
		#ifdef WIN32
			 throw CUDTException(2, 2, 0);
		#else
			 return -1;
		#endif
		 }
      }
   }

   if (m_iSndBufSize <= m_pSndBuffer->getCurrBufSize())
      return 0; 

   int size = (m_iSndBufSize - m_pSndBuffer->getCurrBufSize()) * m_iPayloadSize;
   if (size > len)
   {//缓存比实际数据大
	   size = len;
   }
   else
   {//缓存比实际数据小
	   if(bonce)
	   {//不允许填充一部分，直接结束
		   return 0;
	   }
   }

   // record total time used for sending
   if (0 == m_pSndBuffer->getCurrBufSize())
      m_llSndDurationCounter = CTimer::getTime();

   // insert the user buffer into the sening list
   m_pSndBuffer->addBuffer(data, size);

   // insert this socket to snd list if it is not on the list yet
   m_pSndQueue->m_pSndUList->update(this, false);

   return size;
}

int CUDT::recv(char* data, const int& len)
{
   if (UDT_DGRAM == m_iSockType)
   {
	#ifdef WIN32
	   throw CUDTException(5, 10, 0);
	#else
	   return -1;
	#endif
   }

   // throw an exception if not connected
   if (!m_bConnected)
   {
	#ifdef WIN32
	   throw CUDTException(2, 2, 0);
	#else
	   return -1;
	#endif
   }
   else if ((m_bBroken || m_bClosing) && (0 == m_pRcvBuffer->getRcvDataSize()))
   {
	#ifdef WIN32
	   throw CUDTException(2, 1, 0);
	#else
	   return -1;
	#endif
   }

   if (len <= 0)
      return 0;

   CGuard recvguard(m_RecvLock);

   if (0 == m_pRcvBuffer->getRcvDataSize())
   {
      if (!m_bSynRecving)
	  {
		  return 0;//add 20100127
//         throw CUDTException(6, 2, 0);
	  }
      else
      {
         #ifndef WIN32
            pthread_mutex_lock(&m_RecvDataLock);
            if (m_iRcvTimeOut < 0) 
            { 
               while (!m_bBroken && m_bConnected && !m_bClosing && (0 == m_pRcvBuffer->getRcvDataSize()))
                  pthread_cond_wait(&m_RecvDataCond, &m_RecvDataLock);
            }
            else
            {
               uint64_t exptime = CTimer::getTime() + m_iRcvTimeOut * 1000ULL; 
               timespec locktime; 
    
               locktime.tv_sec = exptime / 1000000;
               locktime.tv_nsec = (exptime % 1000000) * 1000;

               while (!m_bBroken && m_bConnected && !m_bClosing && (0 == m_pRcvBuffer->getRcvDataSize()))
               {
                  pthread_cond_timedwait(&m_RecvDataCond, &m_RecvDataLock, &locktime); 
                  if (CTimer::getTime() >= exptime)
                     break;
               }
            }
            pthread_mutex_unlock(&m_RecvDataLock);
         #else
            if (m_iRcvTimeOut < 0)
            {
               while (!m_bBroken && m_bConnected && !m_bClosing && (0 == m_pRcvBuffer->getRcvDataSize()))
                  WaitForSingleObject(m_RecvDataCond, INFINITE);
            }
            else
            {
               uint64_t enter_time = CTimer::getTime();

               while (!m_bBroken && m_bConnected && !m_bClosing && (0 == m_pRcvBuffer->getRcvDataSize()))
               {
                  int diff = int(CTimer::getTime() - enter_time) / 1000;
                  if (diff >= m_iRcvTimeOut)
                      break;
                  WaitForSingleObject(m_RecvDataCond, DWORD(m_iRcvTimeOut - diff ));
               }
            }
         #endif
      }
   }

   // throw an exception if not connected
   if (!m_bConnected)
   {
	#ifdef WIN32
	   throw CUDTException(2, 2, 0);
	#else
	   return -1;
	#endif
   }
   else if ((m_bBroken || m_bClosing) && (0 == m_pRcvBuffer->getRcvDataSize()))
   {
	#ifdef WIN32
	   throw CUDTException(2, 1, 0);
	#else
	   return -1;
	#endif
   }

   return m_pRcvBuffer->readBuffer(data, len);
}

//注：这个消息发送纯粹是为了在兼容的前提下实现节制性的重传而做，并非是模拟udp的消息模式
int CUDT::sendmsg(const char* data, const int& len, int nttl, bool border, unsigned int unregion)
{
   if (UDT_DGRAM == m_iSockType)
   {
    #ifdef WIN32
	   throw CUDTException(5, 10, 0);
	#else
	   return -1;
	#endif
   }
   // throw an exception if not connected
   if (m_bBroken || m_bClosing)
   {
    #ifdef WIN32
	   throw CUDTException(2, 1, 0);
	#else
	   return -1;
	#endif
   }
   else if (!m_bConnected)
   {
   #ifdef WIN32
	   throw CUDTException(2, 2, 0);
	#else
	   return -1;
	#endif
   }

   if (len <= 0)
      return 0;

   if (len > m_iSndBufSize * m_iPayloadSize)
   {
	   return 0;
   }

   CGuard sendguard(m_SendLock);

   //if (m_iSndBufSize <= m_pSndBuffer->getCurrBufSize())
   if ((m_iSndBufSize - m_pSndBuffer->getCurrBufSize()) * m_iPayloadSize < len)
   {
      if (!m_bSynSending)
	  {
		  //throw CUDTException(6, 1, 0);
		  return 0;
	  }
      else
      {
         // wait here during a blocking sending
         #ifndef WIN32
            pthread_mutex_lock(&m_SendBlockLock);
            if (m_iSndTimeOut < 0) 
            { 
               while (!m_bBroken && m_bConnected && !m_bClosing && (m_iSndBufSize <= m_pSndBuffer->getCurrBufSize()))
                  pthread_cond_wait(&m_SendBlockCond, &m_SendBlockLock);
            }
            else
            {
               uint64_t exptime = CTimer::getTime() + m_iSndTimeOut * 1000ULL;
               timespec locktime; 
    
               locktime.tv_sec = exptime / 1000000;
               locktime.tv_nsec = (exptime % 1000000) * 1000;
    
               pthread_cond_timedwait(&m_SendBlockCond, &m_SendBlockLock, &locktime);
            }
            pthread_mutex_unlock(&m_SendBlockLock);
         #else
            if (m_iSndTimeOut < 0)
            {
               while (!m_bBroken && m_bConnected && !m_bClosing && (m_iSndBufSize <= m_pSndBuffer->getCurrBufSize()))
                  WaitForSingleObject(m_SendBlockCond, INFINITE);
            }
            else 
               WaitForSingleObject(m_SendBlockCond, DWORD(m_iSndTimeOut)); 
         #endif

         // check the connection status
         if (m_bBroken || m_bClosing)
		 {
		#ifdef WIN32
			 throw CUDTException(2, 1, 0);
		#else
			 return -1;
		#endif
		 }
         else if (!m_bConnected)
		 {
		#ifdef WIN32
			 throw CUDTException(2, 2, 0);
		#else
			 return -1;
		#endif
		 }
      }
   }

   if (m_iSndBufSize <= m_pSndBuffer->getCurrBufSize())
      return 0; 

   int size = (m_iSndBufSize - m_pSndBuffer->getCurrBufSize()) * m_iPayloadSize;
   if (size > len)
   {//缓存比实际数据大
	   size = len;
   }
   else
   {//缓存比实际数据小
	   //不允许填充一部分，直接结束
	   return 0;
   }

   // record total time used for sending
   if (0 == m_pSndBuffer->getCurrBufSize())
      m_llSndDurationCounter = CTimer::getTime();

   // insert the user buffer into the sening list
   m_pSndBuffer->addBuffer(data, size, nttl, border, unregion);

   // insert this socket to snd list if it is not on the list yet
   m_pSndQueue->m_pSndUList->update(this, false);

   return size;
}

int CUDT::recvmsg(char* data, const int& len)
{
   if (UDT_DGRAM == m_iSockType)
   {
	#ifdef WIN32
	   throw CUDTException(5, 10, 0);
	#else
	   return -1;
	#endif
   }

   if (len <= 0)
	   return 0;
   
   // throw an exception if not connected
   if (!m_bConnected)
   {
	#ifdef WIN32
	   throw CUDTException(2, 2, 0);
	#else
	   return -1;
	#endif
   }

   CGuard recvguard(m_RecvLock);

   if ((m_bBroken || m_bClosing) && (0 == m_pRcvBuffer->readMsg(data, len)))
   {
	#ifdef WIN32
	   throw CUDTException(2, 1, 0);
	#else
	   return -1;
	#endif
   }
/*
   if (0 == m_pRcvBuffer->getRcvDataSize())
   {
      if (!m_bSynRecving)
	  {
		  return 0;//add 20100127
//         throw CUDTException(6, 2, 0);
	  }
      else
      {
         #ifndef WIN32
            pthread_mutex_lock(&m_RecvDataLock);
            if (m_iRcvTimeOut < 0) 
            { 
               while (!m_bBroken && m_bConnected && !m_bClosing && (0 == m_pRcvBuffer->getRcvDataSize()))
                  pthread_cond_wait(&m_RecvDataCond, &m_RecvDataLock);
            }
            else
            {
               uint64_t exptime = CTimer::getTime() + m_iRcvTimeOut * 1000ULL; 
               timespec locktime; 
    
               locktime.tv_sec = exptime / 1000000;
               locktime.tv_nsec = (exptime % 1000000) * 1000;

               while (!m_bBroken && m_bConnected && !m_bClosing && (0 == m_pRcvBuffer->getRcvDataSize()))
               {
                  pthread_cond_timedwait(&m_RecvDataCond, &m_RecvDataLock, &locktime); 
                  if (CTimer::getTime() >= exptime)
                     break;
               }
            }
            pthread_mutex_unlock(&m_RecvDataLock);
         #else
            if (m_iRcvTimeOut < 0)
            {
               while (!m_bBroken && m_bConnected && !m_bClosing && (0 == m_pRcvBuffer->getRcvDataSize()))
                  WaitForSingleObject(m_RecvDataCond, INFINITE);
            }
            else
            {
               uint64_t enter_time = CTimer::getTime();

               while (!m_bBroken && m_bConnected && !m_bClosing && (0 == m_pRcvBuffer->getRcvDataSize()))
               {
                  int diff = int(CTimer::getTime() - enter_time) / 1000;
                  if (diff >= m_iRcvTimeOut)
                      break;
                  WaitForSingleObject(m_RecvDataCond, DWORD(m_iRcvTimeOut - diff ));
               }
            }
         #endif
      }
   }
*/
   // throw an exception if not connected
   if (!m_bConnected)
   {
	#ifdef WIN32
	   throw CUDTException(2, 2, 0);
	#else
	   return -1;
	#endif
   }
   else if ((m_bBroken || m_bClosing) && (0 == m_pRcvBuffer->getRcvDataSize()))
   {
	#ifdef WIN32
	   throw CUDTException(2, 1, 0);
	#else
	   return -1;
	#endif
   }

   int res = m_pRcvBuffer->readMsg(data, len);
   return res;
//  return m_pRcvBuffer->readBuffer(data, len);
}

void CUDT::sample(CPerfMon* perf, bool clear)
{
   if (!m_bConnected)
      throw CUDTException(2, 2, 0);
   if (m_bBroken || m_bClosing)
      throw CUDTException(2, 1, 0);

   uint64_t currtime = CTimer::getTime();
   perf->msTimeStamp = (currtime - m_StartTime) / 1000;

   m_llSentTotal += m_llTraceSent;
   m_llRecvTotal += m_llTraceRecv;
   m_iSndLossTotal += m_iTraceSndLoss;
   m_iRcvLossTotal += m_iTraceRcvLoss;
   m_iRetransTotal += m_iTraceRetrans;
   m_iSentACKTotal += m_iSentACK;
   m_iRecvACKTotal += m_iRecvACK;
   m_iSentNAKTotal += m_iSentNAK;
   m_iRecvNAKTotal += m_iRecvNAK;
   m_llSndDurationTotal += m_llSndDuration;

   perf->pktSentTotal = m_llSentTotal;
   perf->pktRecvTotal = m_llRecvTotal;
   perf->pktSndLossTotal = m_iSndLossTotal;
   perf->pktRcvLossTotal = m_iRcvLossTotal;
   perf->pktRetransTotal = m_iRetransTotal;
   perf->pktSentACKTotal = m_iSentACKTotal;
   perf->pktRecvACKTotal = m_iRecvACKTotal;
   perf->pktSentNAKTotal = m_iSentNAKTotal;
   perf->pktRecvNAKTotal = m_iRecvNAKTotal;
   perf->usSndDurationTotal = m_llSndDurationTotal;

   perf->pktSent = m_llTraceSent;
   perf->pktRecv = m_llTraceRecv;
   perf->pktSndLoss = m_iTraceSndLoss;
   perf->pktRcvLoss = m_iTraceRcvLoss;
   perf->pktRetrans = m_iTraceRetrans;
   perf->pktSentACK = m_iSentACK;
   perf->pktRecvACK = m_iRecvACK;
   perf->pktSentNAK = m_iSentNAK;
   perf->pktRecvNAK = m_iRecvNAK;
   perf->usSndDuration = m_llSndDuration;

   double interval = double(currtime - m_LastSampleTime);

   perf->mbpsSendRate = double(m_llTraceSent) * m_iPayloadSize * 8.0 / interval;
   perf->mbpsRecvRate = double(m_llTraceRecv) * m_iPayloadSize * 8.0 / interval;

   perf->usPktSndPeriod = m_ullInterval / double(m_ullCPUFrequency);
   perf->pktFlowWindow = m_iFlowWindowSize;
   perf->pktCongestionWindow = (int)m_dCongestionWindow;
   perf->pktFlightSize = CSeqNo::seqlen(const_cast<int32_t&>(m_iSndLastAck), CSeqNo::incseq(m_iSndCurrSeqNo)) - 1;
   perf->msRTT = m_iRTT/1000.0;
   perf->mbpsBandwidth = m_iBandwidth * m_iPayloadSize * 8.0 / 1000000.0;

   #ifndef WIN32
      if (0 == pthread_mutex_trylock(&m_ConnectionLock))
   #else
      if (WAIT_OBJECT_0 == WaitForSingleObject(m_ConnectionLock, 0))
   #endif
   {
      perf->byteAvailSndBuf = (NULL == m_pSndBuffer) ? 0 : (m_iSndBufSize - m_pSndBuffer->getCurrBufSize()) * m_iMSS;
      perf->byteAvailRcvBuf = (NULL == m_pRcvBuffer) ? 0 : m_pRcvBuffer->getAvailBufSize() * m_iMSS;

      #ifndef WIN32
         pthread_mutex_unlock(&m_ConnectionLock);
      #else
         ReleaseMutex(m_ConnectionLock);
      #endif
   }
   else
   {
      perf->byteAvailSndBuf = 0;
      perf->byteAvailRcvBuf = 0;
   }

   if (clear)
   {
      m_llTraceSent = m_llTraceRecv = m_iTraceSndLoss = m_iTraceRcvLoss = m_iTraceRetrans = m_iSentACK = m_iRecvACK = m_iSentNAK = m_iRecvNAK = 0;
      m_llSndDuration = 0;
      m_LastSampleTime = currtime;
   }
}

void CUDT::initSynch()
{
   #ifndef WIN32
      pthread_mutex_init(&m_SendBlockLock, NULL);
      pthread_cond_init(&m_SendBlockCond, NULL);
      pthread_mutex_init(&m_RecvDataLock, NULL);
      pthread_cond_init(&m_RecvDataCond, NULL);
      pthread_mutex_init(&m_SendLock, NULL);
      pthread_mutex_init(&m_RecvLock, NULL);
      pthread_mutex_init(&m_AckLock, NULL);
      pthread_mutex_init(&m_ConnectionLock, NULL);
   #else
      m_SendBlockLock = CreateMutex(NULL, false, NULL);
      m_SendBlockCond = CreateEvent(NULL, false, false, NULL);
      m_RecvDataLock = CreateMutex(NULL, false, NULL);
      m_RecvDataCond = CreateEvent(NULL, false, false, NULL);
      m_SendLock = CreateMutex(NULL, false, NULL);
      m_RecvLock = CreateMutex(NULL, false, NULL);
      m_AckLock = CreateMutex(NULL, false, NULL);
      m_ConnectionLock = CreateMutex(NULL, false, NULL);
   #endif
}

void CUDT::destroySynch()
{
   #ifndef WIN32
      pthread_mutex_destroy(&m_SendBlockLock);
      pthread_cond_destroy(&m_SendBlockCond);
      pthread_mutex_destroy(&m_RecvDataLock);
      pthread_cond_destroy(&m_RecvDataCond);
      pthread_mutex_destroy(&m_SendLock);
      pthread_mutex_destroy(&m_RecvLock);
      pthread_mutex_destroy(&m_AckLock);
      pthread_mutex_destroy(&m_ConnectionLock);
   #else
      CloseHandle(m_SendBlockLock);
      CloseHandle(m_SendBlockCond);
      CloseHandle(m_RecvDataLock);
      CloseHandle(m_RecvDataCond);
      CloseHandle(m_SendLock);
      CloseHandle(m_RecvLock);
      CloseHandle(m_AckLock);
      CloseHandle(m_ConnectionLock);
   #endif
}

void CUDT::releaseSynch()
{
   #ifndef WIN32
      // wake up user calls
      pthread_mutex_lock(&m_SendBlockLock);
      pthread_cond_signal(&m_SendBlockCond);
      pthread_mutex_unlock(&m_SendBlockLock);

      pthread_mutex_lock(&m_SendLock);
      pthread_mutex_unlock(&m_SendLock);

      pthread_mutex_lock(&m_RecvDataLock);
      pthread_cond_signal(&m_RecvDataCond);
      pthread_mutex_unlock(&m_RecvDataLock);

      pthread_mutex_lock(&m_RecvLock);
      pthread_mutex_unlock(&m_RecvLock);
   #else
      SetEvent(m_SendBlockCond);
      WaitForSingleObject(m_SendLock, INFINITE);
      ReleaseMutex(m_SendLock);
      SetEvent(m_RecvDataCond);
      WaitForSingleObject(m_RecvLock, INFINITE);
      ReleaseMutex(m_RecvLock);
   #endif
}

void CUDT::sendCtrl(const int& pkttype, void* lparam, void* rparam, const int& size)
{
   CPacket ctrlpkt;

   switch (pkttype)
   {
   case 2: //010 - Acknowledgement
      {
      int32_t ack;

      // If there is no loss, the ACK is the current largest sequence number plus 1;
      // Otherwise it is the smallest sequence number in the receiver loss list.
      if (0 == m_pRcvLossList->getLossLength())
         ack = CSeqNo::incseq(m_iRcvCurrSeqNo);
      else
         ack = m_pRcvLossList->getFirstLostSeq();

      if (ack == m_iRcvLastAckAck)
         break;

      // send out a lite ACK
      // to save time on buffer processing and bandwidth/AS measurement, a lite ACK only feeds back an ACK number
      if (4 == size)
      {
         ctrlpkt.pack(2, NULL, &ack, size);
         ctrlpkt.m_iID = m_PeerID;
         m_pSndQueue->sendto(m_pPeerAddr, ctrlpkt, m_pRealPeerAddr, 0, "");

         break;
      }

      uint64_t currtime;
      CTimer::rdtsc(currtime);

      // There are new received packets to acknowledge, update related information.
      if (CSeqNo::seqcmp(ack, m_iRcvLastAck) > 0)
      {
         int acksize = CSeqNo::seqoff(m_iRcvLastAck, ack);

         m_iRcvLastAck = ack;

         m_pRcvBuffer->ackData(acksize);

         // signal a waiting "recv" call if there is any data available
         #ifndef WIN32
            pthread_mutex_lock(&m_RecvDataLock);
            if (m_bSynRecving)
               pthread_cond_signal(&m_RecvDataCond);
            pthread_mutex_unlock(&m_RecvDataLock);
         #else
            if (m_bSynRecving)
               SetEvent(m_RecvDataCond);
         #endif
      }
      else if (ack == m_iRcvLastAck)
      {
         if ((currtime - m_ullLastAckTime) < ((m_iRTT + 4 * m_iRTTVar) * m_ullCPUFrequency))
            break;
      }
      else
         break;

      // Send out the ACK only if has not been received by the sender before
      if (CSeqNo::seqcmp(m_iRcvLastAck, m_iRcvLastAckAck) > 0)
      {
         int32_t data[6];

         m_iAckSeqNo = CAckNo::incack(m_iAckSeqNo);
         data[0] = m_iRcvLastAck;
         data[1] = m_iRTT;
         data[2] = m_iRTTVar;
         data[3] = m_pRcvBuffer->getAvailBufSize();
         // a minimum flow window of 2 is used, even if buffer is full, to break potential deadlock
         if (data[3] < 2)
            data[3] = 2;

         if (currtime - m_ullLastAckTime > m_ullSYNInt)
         {
            data[4] = m_pRcvTimeWindow->getPktRcvSpeed();
            data[5] = m_pRcvTimeWindow->getBandwidth();
            ctrlpkt.pack(2, &m_iAckSeqNo, data, 24);

            CTimer::rdtsc(m_ullLastAckTime);
         }
         else
         {
            ctrlpkt.pack(2, &m_iAckSeqNo, data, 16);
         }

         ctrlpkt.m_iID = m_PeerID;
         m_pSndQueue->sendto(m_pPeerAddr, ctrlpkt, m_pRealPeerAddr, 0, "");

         m_pACKWindow->store(m_iAckSeqNo, m_iRcvLastAck);

         ++ m_iSentACK;
         ++ m_iSentACKTotal;
      }

      break;
      }

   case 6: //110 - Acknowledgement of Acknowledgement
      ctrlpkt.pack(6, lparam);
      ctrlpkt.m_iID = m_PeerID;
      m_pSndQueue->sendto(m_pPeerAddr, ctrlpkt, m_pRealPeerAddr, 0, "");

      break;

   case 3: //011 - Loss Report
      if (NULL != rparam)
      {
         if (1 == size)
         {
            // only 1 loss packet
            ctrlpkt.pack(3, NULL, (int32_t *)rparam + 1, 4);
         }
         else
         {
            // more than 1 loss packets
            ctrlpkt.pack(3, NULL, rparam, 8);
         }

         ctrlpkt.m_iID = m_PeerID;
         m_pSndQueue->sendto(m_pPeerAddr, ctrlpkt, m_pRealPeerAddr, 0, "");

         ++ m_iSentNAK;
         ++ m_iSentNAKTotal;
      }
      else if (m_pRcvLossList->getLossLength() > 0)
      {
         // this is periodically NAK report

         // read loss list from the local receiver loss list
         int32_t* data = new int32_t[m_iPayloadSize / 4];
         int losslen;
         m_pRcvLossList->getLossArray(data, losslen, m_iPayloadSize / 4, m_iRTT + 4 * m_iRTTVar);

         if (0 < losslen)
         {
            ctrlpkt.pack(3, NULL, data, losslen * 4);
            ctrlpkt.m_iID = m_PeerID;
            m_pSndQueue->sendto(m_pPeerAddr, ctrlpkt, m_pRealPeerAddr, 0, "");

            ++ m_iSentNAK;
            ++ m_iSentNAKTotal;
         }

         delete[] data;
		 data = NULL;
      }

      break;

   case 4: //100 - Congestion Warning
      ctrlpkt.pack(4);
      ctrlpkt.m_iID = m_PeerID;
      m_pSndQueue->sendto(m_pPeerAddr, ctrlpkt, m_pRealPeerAddr, 0, "");

      CTimer::rdtsc(m_ullLastWarningTime);

      break;

   case 1: //001 - Keep-alive
      ctrlpkt.pack(1);
      ctrlpkt.m_iID = m_PeerID;
      m_pSndQueue->sendto(m_pPeerAddr, ctrlpkt, m_pRealPeerAddr, 0, "");
 
      break;

   case 0: //000 - Handshake
      ctrlpkt.pack(0, NULL, rparam, sizeof(CHandShake));
      ctrlpkt.m_iID = m_PeerID;
      m_pSndQueue->sendto(m_pPeerAddr, ctrlpkt, m_pRealPeerAddr, 0, "");

      break;

   case 5: //101 - Shutdown
      ctrlpkt.pack(5);
      ctrlpkt.m_iID = m_PeerID;
      m_pSndQueue->sendto(m_pPeerAddr, ctrlpkt, m_pRealPeerAddr, 0, "");

      break;

   case 7: //111 - Msg drop request
      ctrlpkt.pack(7, lparam, rparam, 8);
      ctrlpkt.m_iID = m_PeerID;
      m_pSndQueue->sendto(m_pPeerAddr, ctrlpkt, m_pRealPeerAddr, 0, "");

      break;

   case 32767: //0x7FFF - Resevered for future use
      break;

   default:
      break;
   }
}

void CUDT::processCtrl(CPacket& ctrlpkt)
{
   // Just heard from the peer, reset the expiration count.
   m_iEXPCount = 1;
   m_llLastRspTime = CTimer::getTime();
   m_ullMinEXPInt = (m_iRTT + 4 * m_iRTTVar) * m_ullCPUFrequency + m_ullSYNInt;
   if (m_ullMinEXPInt < 100000 * m_ullCPUFrequency)
       m_ullMinEXPInt = 100000 * m_ullCPUFrequency;
   
   if ((CSeqNo::incseq(m_iSndCurrSeqNo) == m_iSndLastAck) || (2 == ctrlpkt.getType()) || (3 == ctrlpkt.getType()))
   {
      CTimer::rdtsc(m_ullNextEXPTime);
      m_ullNextEXPTime += m_ullMinEXPInt;
   }

   switch (ctrlpkt.getType())
   {
   case 2: //010 - Acknowledgement
      {
      int32_t ack;

      // process a lite ACK
      if (4 == ctrlpkt.getLength())
      {
         ack = *(int32_t *)ctrlpkt.m_pcData;
         if (CSeqNo::seqcmp(ack, const_cast<int32_t&>(m_iSndLastAck)) >= 0)
         {
            m_iFlowWindowSize -= CSeqNo::seqoff(const_cast<int32_t&>(m_iSndLastAck), ack);
            m_iSndLastAck = ack;
         }

         break;
      }

       // read ACK seq. no.
      ack = ctrlpkt.getAckSeqNo();

      // send ACK acknowledgement
      // ACK2 can be much less than ACK
      uint64_t currtime = CTimer::getTime();
      if ((currtime - m_ullSndLastAck2Time > (uint64_t)m_iSYNInterval) || (ack == m_iSndLastAck2))
      {
         sendCtrl(6, &ack);
         m_iSndLastAck2 = ack;
         m_ullSndLastAck2Time = currtime;
      }

      // Got data ACK
      ack = *(int32_t *)ctrlpkt.m_pcData;

      // check the validation of the ack
      if (CSeqNo::seqcmp(ack, CSeqNo::incseq(m_iSndCurrSeqNo)) > 0)
      {
         //this should not happen: attack or bug
         m_bBroken = true;
//         m_iBrokenCounter = 0;
         break;
      }

      if (CSeqNo::seqcmp(ack, const_cast<int32_t&>(m_iSndLastAck)) >= 0)
      {
         // Update Flow Window Size, must update before and together with m_iSndLastAck
         m_iFlowWindowSize = *((int32_t *)ctrlpkt.m_pcData + 3);
         m_iSndLastAck = ack;
      }

      // protect packet retransmission
      CGuard::enterCS(m_AckLock);

      int offset = CSeqNo::seqoff((int32_t&)m_iSndLastDataAck, ack);
      if (offset <= 0)
      {
         // discard it if it is a repeated ACK
         CGuard::leaveCS(m_AckLock);
         break;
      }

      // acknowledge the sending buffer
      m_pSndBuffer->ackData(offset);

      // record total time used for sending
      m_llSndDuration += currtime - m_llSndDurationCounter;
      m_llSndDurationTotal += currtime - m_llSndDurationCounter;
      m_llSndDurationCounter = currtime;

      // update sending variables
      m_iSndLastDataAck = ack;
      m_pSndLossList->remove(CSeqNo::decseq((int32_t&)m_iSndLastDataAck));

      CGuard::leaveCS(m_AckLock);

      #ifndef WIN32
         pthread_mutex_lock(&m_SendBlockLock);
         if (m_bSynSending)
            pthread_cond_signal(&m_SendBlockCond);
         pthread_mutex_unlock(&m_SendBlockLock);
      #else
         if (m_bSynSending)
            SetEvent(m_SendBlockCond);
      #endif

      // insert this socket to snd list if it is not on the list yet
	#ifdef WIN32
		 m_pSndQueue->m_pSndUList->update(this, false);
	#else
		 m_pSndQueue->m_pSndUList->update(this, false, 0);
	#endif

      // Update RTT
      //m_iRTT = *((int32_t *)ctrlpkt.m_pcData + 1);
      //m_iRTTVar = *((int32_t *)ctrlpkt.m_pcData + 2);
      int rtt = *((int32_t *)ctrlpkt.m_pcData + 1);
//      m_iRTTVar = (m_iRTTVar * 3 + abs(rtt - m_iRTT)) >> 2;
//      m_iRTT = (m_iRTT * 7 + rtt) >> 3;
	  if(rtt>0)
      {
		  m_iRTTVar = (m_iRTTVar * 3 + abs(rtt - m_iRTT)) >> 2;
		  m_iRTT = (m_iRTT * 7 + rtt) >> 3;
      }

      m_pCC->setRTT(m_iRTT);

      m_ullMinEXPInt = (m_iRTT + 4 * m_iRTTVar) * m_ullCPUFrequency + m_ullSYNInt;
      if (m_ullMinEXPInt < 100000 * m_ullCPUFrequency)
          m_ullMinEXPInt = 100000 * m_ullCPUFrequency;

      if (ctrlpkt.getLength() > 16)
      {
         // Update Estimated Bandwidth and packet delivery rate
         if (*((int32_t *)ctrlpkt.m_pcData + 4) > 0)
            m_iDeliveryRate = (m_iDeliveryRate * 7 + *((int32_t *)ctrlpkt.m_pcData + 4)) >> 3;

         if (*((int32_t *)ctrlpkt.m_pcData + 5) > 0)
            m_iBandwidth = (m_iBandwidth * 7 + *((int32_t *)ctrlpkt.m_pcData + 5)) >> 3;

         m_pCC->setRcvRate(m_iDeliveryRate);
         m_pCC->setBandwidth(m_iBandwidth);
      }

      m_pCC->onACK(ack);
      // update CC parameters
      m_ullInterval = (uint64_t)(m_pCC->m_dPktSndPeriod * m_ullCPUFrequency);
      m_dCongestionWindow = m_pCC->m_dCWndSize;

      ++ m_iRecvACK;
      ++ m_iRecvACKTotal;

      break;
      }

   case 6: //110 - Acknowledgement of Acknowledgement
      {
      int32_t ack;
      int rtt = -1;

      // update RTT
      rtt = m_pACKWindow->acknowledge(ctrlpkt.getAckSeqNo(), ack);

      if (rtt <= 0)
         break;

      //if increasing delay detected...
      //   sendCtrl(4);

      // RTT EWMA
      m_iRTTVar = (m_iRTTVar * 3 + abs(rtt - m_iRTT)) >> 2;
      m_iRTT = (m_iRTT * 7 + rtt) >> 3;

      m_pCC->setRTT(m_iRTT);

      m_ullMinEXPInt = (m_iRTT + 4 * m_iRTTVar) * m_ullCPUFrequency + m_ullSYNInt;
      if (m_ullMinEXPInt < 100000 * m_ullCPUFrequency)
          m_ullMinEXPInt = 100000 * m_ullCPUFrequency;

      // update last ACK that has been received by the sender
      if (CSeqNo::seqcmp(ack, m_iRcvLastAckAck) > 0)
         m_iRcvLastAckAck = ack;

      break;
      }

   case 3: //011 - Loss Report
      {
      int32_t* losslist = (int32_t *)(ctrlpkt.m_pcData);

      m_pCC->onLoss(losslist, ctrlpkt.getLength() / 4);
      // update CC parameters
      m_ullInterval = (uint64_t)(m_pCC->m_dPktSndPeriod * m_ullCPUFrequency);
      m_dCongestionWindow = m_pCC->m_dCWndSize;

      bool secure = true;

      // decode loss list message and insert loss into the sender loss list
      for (int i = 0, n = (int)(ctrlpkt.getLength() / 4); i < n; ++ i)
      {
         if (0 != (losslist[i] & 0x80000000))
         {
            if ((CSeqNo::seqcmp(losslist[i] & 0x7FFFFFFF, losslist[i + 1]) > 0) || (CSeqNo::seqcmp(losslist[i + 1], const_cast<int32_t&>(m_iSndCurrSeqNo)) > 0))
            {
               // seq_a must not be greater than seq_b; seq_b must not be greater than the most recent sent seq
               secure = false;
               break;
            }

            int num = 0;
            if (CSeqNo::seqcmp(losslist[i] & 0x7FFFFFFF, const_cast<int32_t&>(m_iSndLastAck)) >= 0)
               num = m_pSndLossList->insert(losslist[i] & 0x7FFFFFFF, losslist[i + 1]);
            else if (CSeqNo::seqcmp(losslist[i + 1], const_cast<int32_t&>(m_iSndLastAck)) >= 0)
               num = m_pSndLossList->insert(const_cast<int32_t&>(m_iSndLastAck), losslist[i + 1]);

            m_iTraceSndLoss += num;
            m_iSndLossTotal += num;

            ++ i;
         }
         else if (CSeqNo::seqcmp(losslist[i], const_cast<int32_t&>(m_iSndLastAck)) >= 0)
         {
            if (CSeqNo::seqcmp(losslist[i], const_cast<int32_t&>(m_iSndCurrSeqNo)) > 0)
            {
               //seq_a must not be greater than the most recent sent seq
               secure = false;
               break;
            }

            int num = m_pSndLossList->insert(losslist[i], losslist[i]);

            m_iTraceSndLoss += num;
            m_iSndLossTotal += num;
         }
      }

      if (!secure)
      {
         //this should not happen: attack or bug
         m_bBroken = true;
//         m_iBrokenCounter = 0;
         break;
      }

      // the lost packet (retransmission) should be sent out immediately
      m_pSndQueue->m_pSndUList->update(this);

      ++ m_iRecvNAK;
      ++ m_iRecvNAKTotal;

      break;
      }

   case 4: //100 - Delay Warning
      // One way packet delay is increasing, so decrease the sending rate
      m_ullInterval = (uint64_t)ceil(m_ullInterval * 1.125);
      m_iLastDecSeq = m_iSndCurrSeqNo;

      break;

   case 1: //001 - Keep-alive
      // The only purpose of keep-alive packet is to tell that the peer is still alive
      // nothing needs to be done.

      break;

   case 0: //000 - Handshake
      if ((((CHandShake*)(ctrlpkt.m_pcData))->m_iReqType > 0) || (m_bRendezvous && (((CHandShake*)(ctrlpkt.m_pcData))->m_iReqType != -2)))
      {
         // The peer side has not received the handshake message, so it keeps querying
         // resend the handshake packet

         CHandShake initdata;
         initdata.m_iISN = m_iISN;
         initdata.m_iMSS = m_iMSS;
         initdata.m_iFlightFlagSize = m_iFlightFlagSize;
         initdata.m_iReqType = (!m_bRendezvous) ? -1 : -2;
         initdata.m_iID = m_SocketID;
         sendCtrl(0, NULL, (char *)&initdata, sizeof(CHandShake));
      }

      break;

   case 5: //101 - Shutdown
      m_bShutdown = true;
      m_bClosing = true;
      m_bBroken = true;
//      m_iBrokenCounter = 60;

      // Signal the sender and recver if they are waiting for data.
      releaseSynch();

      CTimer::triggerEvent();

      break;

   case 7: //111 - Msg drop request
      m_pRcvBuffer->dropMsg(ctrlpkt.getMsgSeq());
      m_pRcvLossList->remove(*(int32_t*)ctrlpkt.m_pcData, *(int32_t*)(ctrlpkt.m_pcData + 4));

	  // move forward with current recv seq no.
      if ((CSeqNo::seqcmp(*(int32_t*)ctrlpkt.m_pcData, CSeqNo::incseq(m_iRcvCurrSeqNo)) <= 0)
		  && (CSeqNo::seqcmp(*(int32_t*)(ctrlpkt.m_pcData + 4), m_iRcvCurrSeqNo) > 0))
      {
		  m_iRcvCurrSeqNo = *(int32_t*)(ctrlpkt.m_pcData + 4);
      }
      break;

   case 32767: //0x7FFF - reserved and user defined messages
      m_pCC->processCustomMsg(&ctrlpkt);
      // update CC parameters
      m_ullInterval = (uint64_t)(m_pCC->m_dPktSndPeriod * m_ullCPUFrequency);
      m_dCongestionWindow = m_pCC->m_dCWndSize;

      break;

   default:
      break;
   }
}

int CUDT::packData(CPacket& packet, uint64_t& ts)
{
   int payload = 0;
   bool probe = false;

   uint64_t entertime;
   CTimer::rdtsc(entertime);

   if ((0 != m_ullTargetTime) && (entertime > m_ullTargetTime))
      m_ullTimeDiff += entertime - m_ullTargetTime;

   // Loss retransmission always has higher priority.
   if ((packet.m_iSeqNo = m_pSndLossList->getLostSeq()) >= 0)
   {
      // protect m_iSndLastDataAck from updating by ACK processing
      CGuard ackguard(m_AckLock);

      int offset = CSeqNo::seqoff((int32_t&)m_iSndLastDataAck, packet.m_iSeqNo);
      if (offset < 0)
         return 0;

      int msglen;

      payload = m_pSndBuffer->readData(&(packet.m_pcData), offset, packet.m_iMsgNo, msglen);

      if (-1 == payload)
      {
         int32_t seqpair[2];
         seqpair[0] = packet.m_iSeqNo;
         //seqpair[1] = CSeqNo::incseq(seqpair[0], msglen);
		 seqpair[1] = CSeqNo::incseq(seqpair[0], ((msglen>1)?(msglen-1):0));
         sendCtrl(7, &packet.m_iMsgNo, seqpair, 8);

         // only one msg drop request is necessary
         m_pSndLossList->remove(seqpair[1]);

		 // skip all dropped packets
         if (CSeqNo::seqcmp(const_cast<int32_t&>(m_iSndCurrSeqNo), CSeqNo::incseq(seqpair[1])) < 0)//
             m_iSndCurrSeqNo = seqpair[1];//CSeqNo::incseq(seqpair[1]);

         return 0;
      }
      else if (0 == payload)
         return 0;

      ++ m_iTraceRetrans;
      ++ m_iRetransTotal;
   }
   else
   {
      // If no loss, pack a new packet.

      // check congestion/flow window limit
      int cwnd = (m_iFlowWindowSize < (int)m_dCongestionWindow) ? m_iFlowWindowSize : (int)m_dCongestionWindow;
      if (cwnd >= CSeqNo::seqlen(const_cast<int32_t&>(m_iSndLastAck), CSeqNo::incseq(m_iSndCurrSeqNo)))
      {
         if (0 != (payload = m_pSndBuffer->readData(&(packet.m_pcData), packet.m_iMsgNo)))
         {
            m_iSndCurrSeqNo = CSeqNo::incseq(m_iSndCurrSeqNo);
            m_pCC->setSndCurrSeqNo((int32_t&)m_iSndCurrSeqNo);

            packet.m_iSeqNo = m_iSndCurrSeqNo;

            // every 16 (0xF) packets, a packet pair is sent
            if (0 == (packet.m_iSeqNo & 0xF))
               probe = true;
         }
         else
         {
            m_ullTargetTime = 0;
            m_ullTimeDiff = 0;
            ts = 0;
            return 0;
         }
      }
      else
      {
         m_ullTargetTime = 0;
         m_ullTimeDiff = 0;
         ts = 0;
         return 0;
      }
   }

   packet.m_iTimeStamp = int(CTimer::getTime() - m_StartTime);
   //m_pSndTimeWindow->onPktSent(packet.m_iTimeStamp);

   packet.m_iID = m_PeerID;

   m_pCC->onPktSent(&packet);

   ++ m_llTraceSent;
   ++ m_llSentTotal;

   if (probe)
   {
      // sends out probing packet pair
      ts = entertime;
      probe = false;
   }
   else
   {
      #ifndef NO_BUSY_WAITING
         ts = entertime + m_ullInterval;
      #else
         if (m_ullTimeDiff >= m_ullInterval)
         {
            ts = entertime;
            m_ullTimeDiff -= m_ullInterval;
         }
         else
         {
            ts = entertime + m_ullInterval - m_ullTimeDiff;
            m_ullTimeDiff = 0;
         }
      #endif
   }

   m_ullTargetTime = ts;

   packet.m_iID = m_PeerID;
   packet.setLength(payload);

   return payload;
}

int CUDT::processData(CUnit* unit)
{
   CPacket& packet = unit->m_Packet;

   // Just heard from the peer, reset the expiration count.
   m_iEXPCount = 1;
   m_llLastRspTime = CTimer::getTime();
   m_ullMinEXPInt = (m_iRTT + 4 * m_iRTTVar) * m_ullCPUFrequency + m_ullSYNInt;
   if (m_ullMinEXPInt < 100000 * m_ullCPUFrequency)
       m_ullMinEXPInt = 100000 * m_ullCPUFrequency;

   if (CSeqNo::incseq(m_iSndCurrSeqNo) == m_iSndLastAck)
   {
      CTimer::rdtsc(m_ullNextEXPTime);
      if (!m_pCC->m_bUserDefinedRTO)
         m_ullNextEXPTime += m_ullMinEXPInt;
      else
         m_ullNextEXPTime += m_pCC->m_iRTO * m_ullCPUFrequency;
   }

   m_pCC->onPktReceived(&packet);

   ++ m_iPktCount;

   // update time information
   m_pRcvTimeWindow->onPktArrival();

   // check if it is probing packet pair
   if (0 == (packet.m_iSeqNo & 0xF))
      m_pRcvTimeWindow->probe1Arrival();
   else if (1 == (packet.m_iSeqNo & 0xF))
      m_pRcvTimeWindow->probe2Arrival();

   ++ m_llTraceRecv;
   ++ m_llRecvTotal;

   int32_t offset = CSeqNo::seqoff(m_iRcvLastAck, packet.m_iSeqNo);
   if ((offset < 0) || (offset >= m_pRcvBuffer->getAvailBufSize()))
   {
	   m_ullNextACKTime = 0;//
      return -1;
   }

   if (m_pRcvBuffer->addData(unit, offset) < 0)
   {
	   m_ullNextACKTime = 0;//
	   return -1;
   }
   // Loss detection.
   if (CSeqNo::seqcmp(packet.m_iSeqNo, CSeqNo::incseq(m_iRcvCurrSeqNo)) > 0)
   {
      // If loss found, insert them to the receiver loss list
      m_pRcvLossList->insert(CSeqNo::incseq(m_iRcvCurrSeqNo), CSeqNo::decseq(packet.m_iSeqNo));

      // pack loss list for NAK
      int32_t lossdata[2];
      lossdata[0] = CSeqNo::incseq(m_iRcvCurrSeqNo) | 0x80000000;
      lossdata[1] = CSeqNo::decseq(packet.m_iSeqNo);

      // Generate loss report immediately.
      sendCtrl(3, NULL, lossdata, (CSeqNo::incseq(m_iRcvCurrSeqNo) == CSeqNo::decseq(packet.m_iSeqNo)) ? 1 : 2);

      int loss = CSeqNo::seqlen(m_iRcvCurrSeqNo, packet.m_iSeqNo) - 2;
      m_iTraceRcvLoss += loss;
      m_iRcvLossTotal += loss;
   }

   // This is not a regular fixed size packet...   
   //an irregular sized packet usually indicates the end of a message, so send an ACK immediately   
   if (packet.getLength() != m_iPayloadSize)   
      CTimer::rdtsc(m_ullNextACKTime); 

   // Update the current largest sequence number that has been received.
   // Or it is a retransmitted packet, remove it from receiver loss list.
   if (CSeqNo::seqcmp(packet.m_iSeqNo, m_iRcvCurrSeqNo) > 0)
      m_iRcvCurrSeqNo = packet.m_iSeqNo;
   else
      m_pRcvLossList->remove(packet.m_iSeqNo);

   return 0;
}

int CUDT::listen(sockaddr* addr, CPacket& packet, int nrecvlen)
{
   CGuard cg(m_ConnectionLock);
   if (m_bClosing)
      return 1002;

   if(nrecvlen > 0 && nrecvlen < sizeof(CHandShake))
  {
	   memset(&packet.m_pcData[nrecvlen],0, sizeof(CHandShake) - nrecvlen);
   }

   CHandShake* hs = (CHandShake *)packet.m_pcData;

   // SYN cookie
//   char clienthost[NI_MAXHOST];
//   char clientport[NI_MAXSERV];
//   getnameinfo(addr, (AF_INET == m_iVersion) ? sizeof(sockaddr_in) : sizeof(sockaddr_in6), clienthost, sizeof(clienthost), clientport, sizeof(clientport), NI_NUMERICHOST|NI_NUMERICSERV);
   int64_t timestamp = (CTimer::getTime() - m_StartTime) / 60000000; // secret changes every one minute
   char cookiestr[1024];
//   sprintf(cookiestr, "%s:%s:%lld", clienthost, clientport, (long long int)timestamp);
   unsigned char cookie[16];
   CMD5::compute(cookiestr, cookie);

   m_nChannelID = hs->m_nChannelID;//,,,,
   m_nPTLinkID = hs->m_nPTLinkID;//,,,,
   m_nPTYSTNO = hs->m_nPTYSTNO;//,,,,
   m_nPTYSTADDR = hs->m_nPTYSTADDR;//,,,,
   m_uchVirtual = hs->m_uchVirtual;//,,,,
   m_nVChannelID = hs->m_nVChannelID;//,,,,

   m_nYSTFV = hs->m_nYSTLV;
   m_nYSTFV_2 = hs->m_nYSTLV_2;
   m_bFTCP = (hs->m_uchLTCP==1?true:false);

   if (1 == hs->m_iReqType)
   {
      hs->m_iCookie = *(int*)cookie;
      packet.m_iID = hs->m_iID;
	  
	  hs->m_nYSTFV = m_nYSTLV;//
	  hs->m_nYSTFV_2 = m_nYSTLV_2;//
	  hs->m_uchFTCP = (m_bLTCP?1:0);
	  /*判断对方是否有real地址，有则加头，没有直接发送*/
//	  OutputDebug("CUDT::listen ... ");
	  if(hs->m_piRealPeerIP[0] == 0 && hs->m_piRealPeerIP[1] == 0 && hs->m_piRealPeerIP[2] == 0 && hs->m_piRealPeerIP[3] == 0 && hs->m_iRealPeerPort <= 0)
	  {
		  m_pSndQueue->sendto(addr, packet, NULL, 0, "");
	  }
	  else
	  {
		  //更新real地址
          uint32_t piaddrtemp[4]={0};
		  int32_t porttemp=0;
		  memcpy(piaddrtemp, hs->m_piRealPeerIP,16);
		  memcpy(hs->m_piRealPeerIP, hs->m_piRealSelfIP,16);
		  memcpy(hs->m_piRealSelfIP, piaddrtemp, 16);
		  porttemp = hs->m_iRealPeerPort;
		  hs->m_iRealPeerPort = hs->m_iRealSelfPort;
		  hs->m_iRealSelfPort = porttemp;
		  //目的地址
		  sockaddr realaddr;
		  CIPAddress::pton(&realaddr, hs->m_piRealPeerIP, m_iIPversion);
		  ((sockaddr_in *)&realaddr)->sin_port = htons(hs->m_iRealPeerPort);

		  //加头发送
		  m_pSndQueue->sendto(addr, packet, &realaddr, 0, "");
	  }

      return 0;
   }
   else
   {
      if (hs->m_iCookie != *(int*)cookie)
      {
         timestamp --;
//         sprintf(cookiestr, "%s:%s:%lld", clienthost, clientport, (long long int)timestamp);
         CMD5::compute(cookiestr, cookie);

         if (hs->m_iCookie != *(int*)cookie)
            return -1;
      }
	  if(m_nCheckYSTNO > 0 && strlen(m_chCheckGroup) > 0)//注：这里为了兼容一些旧产品，严格判断应该是以来方号码为准
	  {//本地有云视通号码，需要验证号码是不是匹配，防止内网探测中的误连，本地没有号码或是对方没有发号码则跳过不需验证
		  if(hs->m_nCheckYSTNO > 0 && strlen(hs->m_chCheckGroup) > 0)
		  {
#ifdef WIN32
			  if(hs->m_nCheckYSTNO != m_nCheckYSTNO || stricmp(hs->m_chCheckGroup, m_chCheckGroup) != 0)
#else
			  if(hs->m_nCheckYSTNO != m_nCheckYSTNO || strcasecmp(hs->m_chCheckGroup, m_chCheckGroup) != 0)
#endif
			  {
				  return -1;
			  }
		  }
	  }
   }

   int32_t id = hs->m_iID;

   // When a peer side connects in...
   if ((1 == packet.getFlag()) && (0 == packet.getType()))
   {
      if ((hs->m_iVersion != m_iVersion) || (hs->m_iType != m_iSockType) || (-1 == s_UDTUnited.newConnection(m_SocketID, addr, hs)))
      {
         // couldn't create a new connection, reject the request
         hs->m_iReqType = 1002;
      }

      packet.m_iID = id;

	  hs->m_nYSTFV = m_nYSTLV;
	  hs->m_nYSTFV_2 = m_nYSTLV_2;

	  hs->m_uchFTCP = (m_bLTCP?1:0);
      /*判断对方是否有real地址，有则加头，没有直接发送*/
	  if(hs->m_piRealPeerIP[0] == 0 && hs->m_piRealPeerIP[1] == 0 && hs->m_piRealPeerIP[2] == 0 && hs->m_piRealPeerIP[3] == 0 && hs->m_iRealPeerPort <= 0)
	  {
		  m_pSndQueue->sendto(addr, packet, NULL, 0, "");
	  }
	  else
	  {
		  //更新real地址
          uint32_t piaddrtemp[4]={0};
		  int32_t porttemp=0;
		  memcpy(piaddrtemp, hs->m_piRealPeerIP,16);
		  memcpy(hs->m_piRealPeerIP, hs->m_piRealSelfIP,16);
		  memcpy(hs->m_piRealSelfIP, piaddrtemp, 16);
		  porttemp = hs->m_iRealPeerPort;
		  hs->m_iRealPeerPort = hs->m_iRealSelfPort;
		  hs->m_iRealSelfPort = porttemp;
		  //目的地址
		  sockaddr realaddr;
		  CIPAddress::pton(&realaddr, hs->m_piRealPeerIP, m_iIPversion);
		  ((sockaddr_in*)&realaddr)->sin_port = htons(hs->m_iRealPeerPort);
		  
		  //加头发送
		  m_pSndQueue->sendto(addr, packet, &realaddr, 0, "");
	  }
   }

   return hs->m_iReqType;
}

void CUDT::checkTimers()
{
   // update CC parameters
   m_ullInterval = (uint64_t)(m_pCC->m_dPktSndPeriod * m_ullCPUFrequency);
   m_dCongestionWindow = m_pCC->m_dCWndSize;
   //uint64_t minint = (uint64_t)(m_ullCPUFrequency * m_pSndTimeWindow->getMinPktSndInt() * 0.9);
   //if (m_ullInterval < minint)
   //   m_ullInterval = minint;

   uint64_t currtime;
   CTimer::rdtsc(currtime);
   int32_t loss = m_pRcvLossList->getFirstLostSeq();

   if ((currtime > m_ullNextACKTime) || ((m_pCC->m_iACKInterval > 0) && (m_pCC->m_iACKInterval <= m_iPktCount)))
   {
      // ACK timer expired or ACK interval reached

      sendCtrl(2);
      CTimer::rdtsc(currtime);
      if (m_pCC->m_iACKPeriod > 0)
         m_ullNextACKTime = currtime + m_pCC->m_iACKPeriod * m_ullCPUFrequency;
      else
         m_ullNextACKTime = currtime + m_ullACKInt;

      m_iPktCount = 0;
      m_iLightACKCount = 1;
   }
   else if (m_iSelfClockInterval * m_iLightACKCount <= m_iPktCount)
   {
      //send a "light" ACK
      sendCtrl(2, NULL, NULL, 4);
      ++ m_iLightACKCount;
   }

   if ((loss >= 0) && (currtime > m_ullNextNAKTime))
   {
      // NAK timer expired, and there is loss to be reported.
      sendCtrl(3);

      CTimer::rdtsc(currtime);
      m_ullNextNAKTime = currtime + m_ullNAKInt;
   }

   if (currtime > m_ullNextEXPTime)
   {
      // Haven't receive any information from the peer, is it dead?!
      // timeout: at least 16 expirations and must be greater than 3 seconds and be less than 30 seconds
//      if ((m_iEXPCount > 16) && (CTimer::getTime() - m_llLastRspTime > 10000000))
	  int64_t llcurtime = CTimer::getTime();
	  if (( (m_iEXPCount > 16) && ((llcurtime < m_llLastRspTime) || ((llcurtime >= m_llLastRspTime) && llcurtime > m_llLastRspTime + 15000000)))//次数够了，时间也够了 
		|| ((m_iEXPCount > 1) && ((llcurtime >= m_llLastRspTime) && (llcurtime < m_llLastRspTime + 900000000) && llcurtime > m_llLastRspTime + 90000000)))//次数不够，时间够了
      {
         //
         // Connection is broken. 
         // UDT does not signal any information about this instead of to stop quietly.
         // Apllication will detect this when it calls any UDT methods next time.
         //
         m_bClosing = true;
         m_bBroken = true;
//         m_iBrokenCounter = 30;

         // update snd U list to remove this socket
         m_pSndQueue->m_pSndUList->update(this);

         releaseSynch();

         CTimer::triggerEvent();

         return;
      }

      // sender: Insert all the packets sent after last received acknowledgement into the sender loss list.
      // recver: Send out a keep-alive packet
      if (m_pSndBuffer->getCurrBufSize() > 0)
      {
		  if (CSeqNo::incseq(m_iSndCurrSeqNo) != m_iSndLastAck)
		  {
			  int32_t csn = m_iSndCurrSeqNo;
			  int num = m_pSndLossList->insert(const_cast<int32_t&>(m_iSndLastAck), csn);
			  m_iTraceSndLoss += num;
			  m_iSndLossTotal += num;
		  }
		  
		  m_pCC->onTimeout();
		  // update CC parameters
		  m_ullInterval = (uint64_t)(m_pCC->m_dPktSndPeriod * m_ullCPUFrequency);
		  m_dCongestionWindow = m_pCC->m_dCWndSize;
		  
		  // immediately restart transmission
		  m_pSndQueue->m_pSndUList->update(this);
      }
      else
		  sendCtrl(1);
	  
      ++ m_iEXPCount;
      m_ullMinEXPInt = (m_iEXPCount * (m_iRTT + 4 * m_iRTTVar) + m_iSYNInterval) * m_ullCPUFrequency;
      if (m_ullMinEXPInt < m_iEXPCount * 100000 * m_ullCPUFrequency)
		  m_ullMinEXPInt = m_iEXPCount * 100000 * m_ullCPUFrequency;
      CTimer::rdtsc(m_ullNextEXPTime);
      m_ullNextEXPTime += m_ullMinEXPInt;
   }
}







