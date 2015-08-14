//?//----------------------------------------------------------------------
// AmfSerializer.cpp
// Amf序列化工具
//
// 作者：程行通 Copyright (C) Jovision 2014
//----------------------------------------------------------------------

#include "AmfSerializer.h"

namespace JMS
{
	uint8_t AmfSerializer::ReadByte(uint8_t *pBuf)
	{
		return *pBuf;
	}

	void AmfSerializer::WriteByte(uint8_t *pBuf, uint8_t value)
	{
		*pBuf = value;
	}

	uint16_t AmfSerializer::ReadWord(uint8_t *pBuf)
	{
		uint16_t value = *(pBuf++);
		value <<= 8;
		value |= *(pBuf++);
		return value;
	}

	void AmfSerializer::WriteWord(uint8_t *pBuf, uint16_t value)
	{
		*(pBuf++) = (value >> 8) & 0xFF;
		*(pBuf++) = (value) & 0xFF;
	}

	uint32_t AmfSerializer::Read3Byte(uint8_t *pBuf)
	{
		uint32_t value = *(pBuf++);
		value <<= 8;
		value |= *(pBuf++);
		value <<= 8;
		value |= *(pBuf++);
		return value;
	}

	void AmfSerializer::Write3Byte(uint8_t *pBuf, uint32_t value)
	{
		assert(value <= 0xFFFFFF);
		*(pBuf++) = (value >> 16) & 0xFF;
		*(pBuf++) = (value >> 8) & 0xFF;
		*(pBuf++) = (value) & 0xFF;
	}

	uint32_t AmfSerializer::ReadDWord(uint8_t *pBuf)
	{
		uint32_t value = *(pBuf++);
		value <<= 8;
		value |= *(pBuf++);
		value <<= 8;
		value |= *(pBuf++);
		value <<= 8;
		value |= *(pBuf++);
		return value;
	}

	void AmfSerializer::WriteDWord(uint8_t *pBuf, uint32_t value)
	{
		*(pBuf++) = (value >> 24) & 0xFF;
		*(pBuf++) = (value >> 16) & 0xFF;
		*(pBuf++) = (value >> 8) & 0xFF;
		*(pBuf++) = (value) & 0xFF;
	}

	uint64_t AmfSerializer::ReadQWord(uint8_t *pBuf)
	{
		uint64_t value = *(pBuf++);
		value <<= 8;
		value |= *(pBuf++);
		value <<= 8;
		value |= *(pBuf++);
		value <<= 8;
		value |= *(pBuf++);
		value <<= 8;
		value |= *(pBuf++);
		value <<= 8;
		value |= *(pBuf++);
		value <<= 8;
		value |= *(pBuf++);
		value <<= 8;
		value |= *(pBuf++);
		return value;
	}

	void AmfSerializer::WriteQWord(uint8_t *pBuf, uint64_t value)
	{
		*(pBuf++) = (value >> 56) & 0xFF;
		*(pBuf++) = (value >> 48) & 0xFF;
		*(pBuf++) = (value >> 40) & 0xFF;
		*(pBuf++) = (value >> 32) & 0xFF;
		*(pBuf++) = (value >> 24) & 0xFF;
		*(pBuf++) = (value >> 16) & 0xFF;
		*(pBuf++) = (value >> 8) & 0xFF;
		*(pBuf++) = (value) & 0xFF;
	}

	int AmfSerializer::SkeepValue(uint8_t *pBuf, int nSize)
	{
		if(nSize < 1)
		{
			return -1;
		}

		int nLen = 1;
		switch(pBuf[0])
		{
		case JMSAMF_NULL:
		case JMSAMF_UNDEFINED:
		case JMSAMF_REFERENCE:
		case JMSAMF_UNSUPPORTED:
			nLen = 1;
			break;

		case JMSAMF_BOOLEAN:
			nLen = 2;
			break;

		case JMSAMF_NUMBER:
			nLen = 9;
			break;

		case JMSAMF_STRING:
		case JMSAMF_DATE:
			if(nSize < 3)
			{
				return -1;
			}
			nLen = ReadWord(pBuf + 1) + 3;
			break;

		case JMSAMF_LONG_STRING:
			if(nSize < 5)
			{
				return -1;
			}
			nLen = ReadDWord(pBuf + 1) + 5;
			break;

		default:
			return -1;
			break;
		}

		if(nSize < nLen)
		{
			return -1;
		}
		else
		{
			return nLen;
		}
	}

	double AmfSerializer::ReadDouble(uint8_t *pBuf)
	{
		uint64_t qwValue = ReadQWord(pBuf);
		double dbValue;
		memcpy(&dbValue, &qwValue, 8);
		return dbValue;
	}

	void AmfSerializer::WriteDouble(uint8_t *pBuf, double value)
	{
		uint64_t qwValue;
		memcpy(&qwValue, &value, 8);
		WriteQWord(pBuf, qwValue);
	}

	int AmfSerializer::ReadNumber(uint8_t *pBuf, int nSize, double& dbValue, bool bWithHeader)
	{
		if(bWithHeader)
		{
			if(nSize < 9)
			{
				return -1;
			}
			if(*pBuf != JMSAMF_NUMBER)
			{
				return -1;
			}
			++pBuf;
		}
		else
		{
			if(nSize < 8)
			{
				return -1;
			}
		}

		dbValue = ReadDouble(pBuf);

		return bWithHeader ? 9 : 8;
	}

	int AmfSerializer::WriteNumber(uint8_t *pBuf, int nSize, double dbValue, bool bWithHeader)
	{
		if(bWithHeader)
		{
			if(nSize < 9)
			{
				return -1;
			}
			*(pBuf++) = JMSAMF_NUMBER;
		}
		else
		{
			if(nSize < 8)
			{
				return -1;
			}
		}

		WriteDouble(pBuf, dbValue);

		return bWithHeader ? 9 : 8;
	}

	int AmfSerializer::ReadBool(uint8_t *pBuf, int nSize, bool& bValue, bool bWithHeader)
	{
		if(bWithHeader)
		{
			if(nSize < 2)
			{
				return -1;
			}
			if(*pBuf != JMSAMF_BOOLEAN)
			{
				return -1;
			}
			++pBuf;
		}
		else
		{
			if(nSize < 1)
			{
				return -1;
			}
		}

		bValue = ReadByte(pBuf) != 0;

		return bWithHeader ? 2 : 1;
	}

	int AmfSerializer::WriteBool(uint8_t *pBuf, int nSize, bool bValue, bool bWithHeader)
	{
		if(bWithHeader)
		{
			if(nSize < 2)
			{
				return -1;
			}
			*(pBuf++) = JMSAMF_BOOLEAN;
		}
		else
		{
			if(nSize < 1)
			{
				return -1;
			}
		}

		WriteByte(pBuf, bValue ? 1 : 0);

		return bWithHeader ? 2 : 1;
	}

	int AmfSerializer::ReadString(uint8_t *pBuf, int nSize, std::string& strValue, bool bWithHeader)
	{
		if(bWithHeader)
		{
			if(nSize < 3)
			{
				return -1;
			}
			if(*pBuf != JMSAMF_STRING)
			{
				return -1;
			}
			++pBuf;
			nSize -= 3;
		}
		else
		{
			if(nSize < 2)
			{
				return -1;
			}
			nSize -= 2;
		}

		int nLen = ReadWord(pBuf);
		if(nLen > nSize)
		{
			return -1;
		}
		strValue = std::string((char*)pBuf + 2, nLen);

		return bWithHeader ? nLen + 3 : nLen + 2;
	}

	int AmfSerializer::WriteString(uint8_t *pBuf, int nSize, const char *szValue, bool bWithHeader)
	{
		int nLen = strlen(szValue);
		int nUsed = nLen + 2;
		if(bWithHeader)
		{
			if(nSize < nUsed + 1)
			{
				return -1;
			}
			*(pBuf++) = JMSAMF_STRING;
		}
		else
		{
			if(nSize < nUsed)
			{
				return -1;
			}
		}

		WriteWord(pBuf, nLen);
		pBuf += 2;
		memcpy(pBuf, szValue, nLen);

		return bWithHeader ? (nUsed + 1) : nUsed;
	}

	int AmfSerializer::WriteObjectHeader(uint8_t *pBuf, int nSize)
	{
		if(nSize < 1)
		{
			return -1;
		}
		*pBuf = JMSAMF_OBJECT;

		return 1;
	}

	int AmfSerializer::WriteObjectEnd(uint8_t *pBuf, int nSize)
	{
		if(nSize < 3)
		{
			return -1;
		}
		*(pBuf++) = 0;
		*(pBuf++) = 0;
		*(pBuf++) = JMSAMF_OBJECT_END;

		return 3;
	}
}
