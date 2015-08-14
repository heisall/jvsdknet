//----------------------------------------------------------------------
// AmfSerializer.h
// Amf序列化工具
//
// 作者：程行通 Copyright (C) Jovision 2014
//----------------------------------------------------------------------

#pragma once

#include "JRCDef.h"

namespace JMS
{
	enum AmfValueType
	{
		JMSAMF_NUMBER = 0,
		JMSAMF_BOOLEAN,
		JMSAMF_STRING,
		JMSAMF_OBJECT,
		JMSAMF_MOVIECLIP,
		JMSAMF_NULL,
		JMSAMF_UNDEFINED,
		JMSAMF_REFERENCE,
		JMSAMF_ECMA_ARRAY,
		JMSAMF_OBJECT_END,
		JMSAMF_STRICT_ARRAY,
		JMSAMF_DATE,
		JMSAMF_LONG_STRING,
		JMSAMF_UNSUPPORTED,
		JMSAMF_RECORDSET,
		JMSAMF_XML_DOC,
		JMSAMF_TYPED_OBJECT,
		JMSAMF_AVMPLUS,
		JMSAMF_INVALID = 0xff
	};

	class AmfSerializer
	{
	public:
		static uint8_t ReadByte(uint8_t *pBuf);
		static void WriteByte(uint8_t *pBuf, uint8_t value);

		static uint16_t ReadWord(uint8_t *pBuf);
		static void WriteWord(uint8_t *pBuf, uint16_t value);

		static uint32_t Read3Byte(uint8_t *pBuf);
		static void Write3Byte(uint8_t *pBuf, uint32_t value);

		static uint32_t ReadDWord(uint8_t *pBuf);
		static void WriteDWord(uint8_t *pBuf, uint32_t value);

		static uint64_t ReadQWord(uint8_t *pBuf);
		static void WriteQWord(uint8_t *pBuf, uint64_t value);

		static int SkeepValue(uint8_t *pBuf, int nSize);

		static double ReadDouble(uint8_t *pBuf);
		static void WriteDouble(uint8_t *pBuf, double value);

		static int ReadNumber(uint8_t *pBuf, int nSize, double& dbValue, bool bWithHeader);
		static int WriteNumber(uint8_t *pBuf, int nSize, double dbValue, bool bWithHeader);

		static int ReadBool(uint8_t *pBuf, int nSize, bool& bValue, bool bWithHeader);
		static int WriteBool(uint8_t *pBuf, int nSize, bool bValue, bool bWithHeader);

		static int ReadString(uint8_t *pBuf, int nSize, std::string& strValue, bool bWithHeader);
		static int WriteString(uint8_t *pBuf, int nSize, const char *szValue, bool bWithHeader);

		static int WriteObjectHeader(uint8_t *pBuf, int nSize);
		static int WriteObjectEnd(uint8_t *pBuf, int nSize);
	};
}
