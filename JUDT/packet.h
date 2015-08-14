/*****************************************************************************
written by
   Yunhong Gu, last updated 05/21/2009
*****************************************************************************/

#ifndef __UDT_PACKET_H__
#define __UDT_PACKET_H__


#include "udt.h"

#ifdef WIN32
   struct iovec
   {
      int iov_len;
      char* iov_base;
   };
#endif

class CChannel;

class CPacket
{
friend class CChannel;
friend class CSndQueue;
friend class CRcvQueue;

public:
   int32_t& m_iSeqNo;                   // alias: sequence number
   int32_t& m_iMsgNo;                   // alias: message number
   int32_t& m_iTimeStamp;               // alias: timestamp
   int32_t& m_iID;			// alias: socket ID
   char*& m_pcData;                     // alias: data/control information

   static const int m_iPktHdrSize;	// packet header size

public:
   CPacket();
   ~CPacket();

      // Functionality:
      //    Get the payload or the control information field length.
      // Parameters:
      //    None.
      // Returned value:
      //    the payload or the control information field length.

   int getLength() const;

      // Functionality:
      //    Set the payload or the control information field length.
      // Parameters:
      //    0) [in] len: the payload or the control information field length.
      // Returned value:
      //    None.

   void setLength(const int& len);

      // Functionality:
      //    Pack a Control packet.
      // Parameters:
      //    0) [in] pkttype: packet type filed.
      //    1) [in] lparam: pointer to the first data structure, explained by the packet type.
      //    2) [in] rparam: pointer to the second data structure, explained by the packet type.
      //    3) [in] size: size of rparam, in number of bytes;
      // Returned value:
      //    None.

   void pack(const int& pkttype, void* lparam = NULL, void* rparam = NULL, const int& size = 0);

      // Functionality:
      //    Read the packet vector.
      // Parameters:
      //    None.
      // Returned value:
      //    Pointer to the packet vector.

   iovec* getPacketVector();

      // Functionality:
      //    Read the packet flag.
      // Parameters:
      //    None.
      // Returned value:
      //    packet flag (0 or 1).

   int getFlag() const;

      // Functionality:
      //    Read the packet type.
      // Parameters:
      //    None.
      // Returned value:
      //    packet type filed (000 ~ 111).

   int getType() const;

      // Functionality:
      //    Read the extended packet type.
      // Parameters:
      //    None.
      // Returned value:
      //    extended packet type filed (0x000 ~ 0xFFF).

   int getExtendedType() const;

      // Functionality:
      //    Read the ACK-2 seq. no.
      // Parameters:
      //    None.
      // Returned value:
      //    packet header field (bit 16~31).

   int32_t getAckSeqNo() const;

      // Functionality:
      //    Read the message boundary flag bit.
      // Parameters:
      //    None.
      // Returned value:
      //    packet header field [1] (bit 0~1).

   int getMsgBoundary() const;

      // Functionality:
      //    Read the message inorder delivery flag bit.
      // Parameters:
      //    None.
      // Returned value:
      //    packet header field [1] (bit 2).

   bool getMsgOrderFlag() const;

      // Functionality:
      //    Read the message sequence number.
      // Parameters:
      //    None.
      // Returned value:
      //    packet header field [1] (bit 3~31).

   int32_t getMsgSeq() const;

      // Functionality:
      //    Clone this packet.
      // Parameters:
      //    None.
      // Returned value:
      //    Pointer to the new packet.

   CPacket* clone() const;

protected:
   uint32_t m_nHeader[3];//uint32_t m_nHeader[4];               // The 128-bit header field
   iovec m_PacketVector[3];             // The 2-demension vector of UDT packet [header, data]

   int32_t __pad;

   uint32_t m_nTimeStampTemp;//...
protected:
   CPacket& operator=(const CPacket&);
};

////////////////////////////////////////////////////////////////////////////////

struct CHandShake
{
   int32_t m_iVersion;          // UDT version
   int32_t m_iType;             // UDT socket type
   int32_t m_iISN;              // random initial sequence number
   int32_t m_iMSS;              // maximum segment size
   int32_t m_iFlightFlagSize;   // flow control window size
   int32_t m_iReqType;          // connection request type: 1: regular connection request, 0: rendezvous connection request, -1/-2: response
   int32_t m_iID;		// socket ID
   int32_t m_iCookie;		// cookie
   uint32_t m_piPeerIP[4];	// The IP address that the peer's UDP port is bound to
   int32_t m_nChannelID;//目的通道，分控连接主控时需指明要连接主控的哪个通道
   uint32_t m_piRealPeerIP[4];//转发时实际目的地址
   uint32_t m_piRealSelfIP[4];//转发时本身实际地址
   int32_t m_iRealPeerPort;
   int32_t m_iRealSelfPort;

   int32_t m_nPTLinkID;//目的ID
   int32_t m_nPTYSTNO;//目的号码，分控伙伴连接指明属于那个号码
   int32_t m_nPTYSTADDR;//目的地址，分控伙伴连接时指明属于那个地址
   
   char    m_chCheckGroup[4];//用于防误连
   int32_t m_nCheckYSTNO;//用于防误连

   int32_t m_nYSTLV;//本地协议版本号，用于确定支持何种功能，用于兼容目的
   int32_t m_nYSTFV;//远端协议版本号，用于确定支持何种功能，用于兼容目的

   unsigned char m_uchVirtual;//连接是否是虚连接
   int32_t m_nVChannelID;//虚连接时对应的实际通道

   unsigned char m_uchLTCP;//本地是否支持内网tcp连接
   unsigned char m_uchFTCP;//远端是否支持内网tcp连接

   //由于49-67版本的主控端对版本的使用出现bug，为了兼容这些版本的主控，只能重新定义版本类型
   //也就是：新版本主分控都优先采用新版本号；此之前的版本仍然使用旧的版本号；
   int32_t m_nYSTLV_2;//本地协议版本号，用于确定支持何种功能，用于兼容目的
   int32_t m_nYSTFV_2;//远端协议版本号，用于确定支持何种功能，用于兼容目的
};


#endif
