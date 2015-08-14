//----------------------------------------------------------------------
// FIFO.cpp
// 先进先出队列
//
// 作者：程行通 Copyright (C) Jovision 2014
//----------------------------------------------------------------------
#include "Def.h"
#include "FIFO.h"
#ifndef MOBILE_CLIENT
#include <malloc.h>
#endif

namespace JMS
{
	FIFO::FIFO() : m_nBufferSize(0), m_pBuffer(NULL), m_nDataSize(0),
		m_nWritePos(0), m_nReadPos(0)
	{
	}

	FIFO::~FIFO()
	{
		Release();
	}

	bool FIFO::Create(int nSize)
	{
		Release();

		m_pBuffer = (BlockPtr_t)malloc(nSize);
		if(m_pBuffer == NULL)
		{
			return false;
		}
		m_nBufferSize = nSize;

		return true;
	}

	void FIFO::Release()
	{
		if(m_pBuffer != NULL)
		{
			free(m_pBuffer);
			m_pBuffer = NULL;
		}
		m_nBufferSize = 0;
		m_nDataSize = 0;
		m_nWritePos = 0;
		m_nReadPos = 0;
		m_lstBlockSizes.clear();
	}

	void FIFO::Clear()
	{
		m_nDataSize = 0;
		m_nWritePos = 0;
		m_nReadPos = 0;
		m_lstBlockSizes.clear();
	}

	FIFO::BlockPtr_t FIFO::Push(int nBlockSize)
	{
		BlockPtr_t pRet = NULL;
		if(nBlockSize > m_nBufferSize - m_nDataSize)
		{
			return pRet;
		}

		if(nBlockSize > m_nBufferSize - m_nWritePos)
		{
			//尾部剩余空间不足
			if(nBlockSize > m_nReadPos)
			{
				//头部剩余空间不足，写入失败
				return pRet;
			}
			else
			{
				//头部剩余空间充足，尾部写入不可用块，前部写入数据
				m_lstBlockSizes.push_back(m_nWritePos - m_nBufferSize);
				pRet = m_pBuffer;
				m_nWritePos = nBlockSize;
				m_nDataSize += nBlockSize;
				m_lstBlockSizes.push_back(nBlockSize);
			}
		}
		else
		{
			if(m_nReadPos > m_nWritePos && m_nReadPos - m_nWritePos < nBlockSize)
			{
				//尾部剩余空间不足，写入失败
				return pRet;
			}
			else
			{
				//尾部剩余空间充足，继续向后写入
				pRet = m_pBuffer + m_nWritePos;
				m_nWritePos += nBlockSize;
				m_nDataSize += nBlockSize;
				m_lstBlockSizes.push_back(nBlockSize);
			}
		}
		if(m_nWritePos == m_nBufferSize)
		{
			m_nWritePos = 0;
		}

		return pRet;
	}

	void FIFO::Pop()
	{
		bool bDelete = false;
		while(m_lstBlockSizes.size() > 0)
		{
			int nSize = m_lstBlockSizes[0];
			if(nSize >= 0)
			{
				if(bDelete)
				{
					break;
				}
				else
				{
					assert(nSize <= m_nDataSize);
					m_nDataSize -= nSize;
					bDelete = true;
				}
			}
			else
			{
				nSize = -nSize;
			}

			m_lstBlockSizes.erase(m_lstBlockSizes.begin());
			m_nReadPos += nSize;
			if(m_nReadPos == m_nBufferSize)
			{
				m_nReadPos = 0;
			}
			assert(m_nReadPos < m_nBufferSize);
		}
		if(m_nBufferSize == 0)
		{
			assert(m_lstBlockSizes.empty());
			m_nWritePos = 0;
			m_nReadPos = 0;
		}
	}

	bool FIFO::GetFrontBlock(BlockPtr_t& pBuf, int& nSize)
	{
		if(m_lstBlockSizes.empty())
		{
			return false;
		}
		if(m_lstBlockSizes[0] > 0)
		{
			pBuf = m_pBuffer + m_nReadPos;
			nSize = m_lstBlockSizes[0];
		}
		else
		{
			assert(m_lstBlockSizes.size() > 1);
			assert(m_lstBlockSizes[1] > 0);
			pBuf = m_pBuffer;
			nSize = m_lstBlockSizes[1];
		}

		return true;
	}
}
