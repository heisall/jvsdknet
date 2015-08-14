//----------------------------------------------------------------------
// FIFO.h
// 先进先出队列
//
// 作者：程行通 Copyright (C) Jovision 2014
//----------------------------------------------------------------------

#pragma once

#include <stdint.h>
#include <vector>
#include <cassert>
#include "Thread.h"

namespace JMS
{
	class FIFO
	{
	public:
		FIFO();
		~FIFO();

		typedef uint8_t *BlockPtr_t;

	public:
		bool Create(int nSize);
		void Release();
		void Clear();
		BlockPtr_t Push(int nBlockSize);
		void Pop();
		bool GetFrontBlock(BlockPtr_t& pBuf, int& nSize);
		
	public:
		int m_nBufferSize;
		BlockPtr_t m_pBuffer;
		int m_nDataSize;
		int m_nWritePos;
		int m_nReadPos;
		std::vector<int> m_lstBlockSizes;
	};
}
