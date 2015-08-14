// CChannel.cpp: implementation of the CCStream class.
//
//////////////////////////////////////////////////////////////////////

//注：该类已停用，如需使用必须重新调试修改20140319

#include "CStream.h"

#include "CWorker.h"
#include "JVN_DBG.h"
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
extern int JVC_MSS;
CCStream::CCStream()
{
}
CCStream::CCStream(int nChannel,int nLocalChannel,int nProtocolType,CCWorker *pWorker,CCOldChannel *pParent)
{
	m_nProtocolType = nProtocolType;
	m_bExit = FALSE;

	m_nOCount = 0;
	
	m_bAcceptChat = FALSE;
	m_bAcceptText = FALSE;
	m_bDAndP = FALSE;

	m_nLocalChannel = nLocalChannel;
	m_pWorker = pWorker;
	m_bPass = FALSE;

	m_idMyID.idH=0;
	m_idMyID.idL=0;
	this->m_idSServerID=0;
	this->m_nSServerIP=0;
	this->m_nSServerPort=0;
	this->m_pParent=pParent;

	m_hSRecvThread = 0;
	m_SServerSocket = 0;
	m_nChannel = nChannel;

	m_pBuffer = new CCOldBufferCtrl(5000*1024, nLocalChannel);//pParent->m_pBuffer;

	m_nPACKLEN = 20000;
}

CCStream::~CCStream()
{
	m_bExit = TRUE;

	if(m_SServerSocket > 0)
	{
		if(m_nProtocolType == TYPE_PC_UDP)// || m_nProtocolType == TYPE_MO_UDP)
		{
			UDT::close(m_SServerSocket);
		}
		else
		{
			closesocket(m_SServerSocket);
		}
	}
	m_SServerSocket = 0;

	if(m_pBuffer != NULL)
	{
		delete m_pBuffer;
		m_pBuffer = NULL;
	}
}

BOOL CCStream::SendData(int sock,BYTE uchType, BYTE *pBuffer,int nSize)
{
	if(m_nProtocolType == TYPE_PC_TCP || m_nProtocolType == TYPE_MO_TCP)
	{//TCP 
		return SendDataTCP(sock,uchType, pBuffer,nSize);
	}
	if(sock > 0)
	{
		switch(uchType) 
		{
		case JVN_REQ_CHECK://请求录像检索
			{
				if(pBuffer != NULL && nSize == 2*JVN_ABCHECKPLEN)
				{
					BYTE data[5 + 2*JVN_ABCHECKPLEN]={0};
					data[0] = uchType;
					memcpy(&data[1], &nSize, 4);
					memcpy(&data[5], pBuffer, nSize);
					UDT::send(sock, (char *)data,5 + nSize, 0);
				}
			}
			break;
		case JVN_REQ_DOWNLOAD://请求录像下载
		case JVN_REQ_PLAY://请求远程回放
			{
				if(pBuffer != NULL && nSize < MAX_PATH && nSize > 0)
				{
					BYTE data[5 + MAX_PATH]={0};
					data[0] = uchType;
					memcpy(&data[1], &nSize, 4);
					memcpy(&data[5], pBuffer, nSize);

					int nLen = nSize + 5;
					int ss=0;
					int ssize=0;
					while(ssize < nLen)
					{
						if(0 < (ss = UDT::send(sock, (char *)data + ssize, nLen - ssize, 0)))
						{
							ssize += ss;
						}
						else if(ss == 0)
						{
							CCWorker::jvc_sleep(1);
							continue;
						}
						else
						{	
							if(m_pWorker->m_bNeedLog)
							{
								if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
								{
									m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "发送数据失败 详细:", __FILE__,__LINE__,(char *)UDT::getlasterror().getErrorMessage());
								}
								else
								{
									m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "SendData faild,INFO:", __FILE__,__LINE__,(char *)UDT::getlasterror().getErrorMessage());
								}
							}
							
							return FALSE;
						}
					}
					m_bDAndP = TRUE;
				}
			}
			break;
		
		case JVN_CMD_YTCTRL://云台控制
			{
				if(pBuffer != NULL && nSize == 4)
				{
					BYTE data[9]={0};
					data[0] = uchType;
					memcpy(&data[1], &nSize, 4);
					memcpy(&data[5], pBuffer, nSize);
					UDT::send(sock, (char *)data,9, 0);
				}
			}
			break;
		case JVN_RSP_CHATACCEPT://同意语音
			m_bAcceptChat = TRUE;
			goto DOSEND;
		case JVN_RSP_TEXTACCEPT://同意文本
			m_bAcceptText = TRUE;
			goto DOSEND;
			break;
		case JVN_RSP_ID:
		case JVN_REQ_CHAT://请求语音聊天
		case JVN_REQ_TEXT://请求文本聊天
		case JVN_CMD_DISCONN://断开连接
		case JVN_CMD_PLAYSTOP://暂停播放
		case JVN_CMD_DOWNLOADSTOP://停止下载数据
		case JVN_CMD_CHATSTOP://停止语音聊天
		case JVN_CMD_TEXTSTOP://停止文本聊天
		case JVN_CMD_VIDEO://请求实时监控数据
		case JVN_CMD_VIDEOPAUSE://请求实时监控数据
		case JVN_CMD_PLAYUP://快进
		case JVN_CMD_PLAYDOWN://慢放
		case JVN_CMD_PLAYDEF://原速播放
		case JVN_CMD_PLAYPAUSE://暂停播放
		case JVN_CMD_PLAYGOON://继续播放
		case JVN_DATA_DANDP://下载回放确认
			{
DOSEND:
				int nLen = 5;
				BYTE data[5]={0};
				data[0] = uchType;

				int ss=0;
				int ssize=0;
				while(ssize < nLen)
				{
					if(0 < (ss = UDT::send(sock, (char *)data + ssize, nLen - ssize, 0)))
					{
						ssize += ss;
					}
					else if(ss == 0)
					{
						CCWorker::jvc_sleep(1);
						continue;
					}
					else
					{
						if(m_pWorker->m_bNeedLog)
						{
							if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
							{
								m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "发送数据失败 详细:", __FILE__,__LINE__,(char *)UDT::getlasterror().getErrorMessage());
							}
							else
							{
								m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "SendData faild,INFO:", __FILE__,__LINE__,(char *)UDT::getlasterror().getErrorMessage());
							}
						}
						
						return FALSE;
					}
				}
			}
			break;
		case JVN_DATA_OK://视频帧确认
			{
				int nLen = 9;
				BYTE data[9]={0};
				data[0] = uchType;
				memcpy(&data[1], &nSize, 4);
				if(pBuffer != NULL)
				{
					memcpy(&data[5], pBuffer, nSize);
				}
				
				int ss=0;
				int ssize=0;
				while(ssize < nLen)
				{
					if(0 < (ss = UDT::send(sock, (char *)data + ssize, nLen - ssize, 0)))
					{
						ssize += ss;
					}
					else if(ss == 0)
					{
						CCWorker::jvc_sleep(1);
						continue;
					}
					else
					{
						if(m_pWorker->m_bNeedLog)
						{
							if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
							{
								m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "发送数据失败 详细:", __FILE__,__LINE__,(char *)UDT::getlasterror().getErrorMessage());
							}
							else
							{
								m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "SendData faild,INFO:", __FILE__,__LINE__,(char *)UDT::getlasterror().getErrorMessage());
							}
						}
						
						return FALSE;
					}
				}
			}
			break;
		case JVN_RSP_TEXTDATA://文本数据
			{
				if(m_bAcceptText)
				{
					if(pBuffer != NULL && nSize < JVN_BAPACKDEFLEN && nSize > 0)
					{
						int nLen = 5+nSize;
						BYTE data[JVN_BAPACKDEFLEN]={0};
						data[0] = uchType;
						memcpy(&data[1], &nSize, 4);
						memcpy(&data[5], pBuffer, nSize);
						
						int ss=0;
						int ssize=0;
						while(ssize < nLen)
						{
							if(0 < (ss = UDT::send(sock, (char *)data + ssize, nLen - ssize, 0)))
							{
								ssize += ss;
							}
							else if(ss == 0)
							{
								CCWorker::jvc_sleep(1);
								continue;
							}
							else
							{
								if(m_pWorker->m_bNeedLog)
								{
									if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
									{
										m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "发送数据失败 详细:", __FILE__,__LINE__,(char *)UDT::getlasterror().getErrorMessage());
									}
									else
									{
										m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "SendData faild,INFO:", __FILE__,__LINE__,(char *)UDT::getlasterror().getErrorMessage());
									}
								}
								
								return FALSE;
							}
						}
					}
				}
			}
			break;
		case JVN_RSP_CHATDATA://语音数据
			{
				if(m_bAcceptChat)
				{
					if(pBuffer != NULL && nSize < JVN_BAPACKDEFLEN-5 && nSize > 0)
					{
						int nLen = 5+nSize;
						BYTE data[JVN_BAPACKDEFLEN]={0};
						data[0] = uchType;
						memcpy(&data[1], &nSize, 4);
						memcpy(&data[5], pBuffer, nSize);
						
						int ss=0;
						int ssize=0;
						while(ssize < nLen)
						{
							if(0 < (ss = UDT::send(sock, (char *)data + ssize, jvs_min(nLen - ssize, m_nPACKLEN), 0)))
							{
								ssize += ss;
							}
							else if(ss == 0)
							{
								CCWorker::jvc_sleep(1);
								continue;
							}
							else
							{
								if(m_pWorker->m_bNeedLog)
								{
									if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
									{
										m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "发送数据失败 详细:", __FILE__,__LINE__,(char *)UDT::getlasterror().getErrorMessage());
									}
									else
									{
										m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "SendData faild,INFO:", __FILE__,__LINE__,(char *)UDT::getlasterror().getErrorMessage());
									}
								}
								
								return FALSE;
							}
						}
					}
				}
			}
			break;
		case JVN_CMD_ONLYI://只要关键帧
		case JVN_CMD_FULL://回复满帧发送
			{
				int nLen = 5+nSize;
				BYTE data[JVN_BAPACKDEFLEN]={0};
				data[0] = uchType;

				if(pBuffer != NULL && nSize < JVN_BAPACKDEFLEN-5 && nSize > 0)
				{
					nLen = 5+nSize;
					memcpy(&data[1], &nSize, 4);
					memcpy(&data[5], pBuffer, nSize);
				}
				else
				{
					nLen = 5;
				}

				int ss=0;
				int ssize=0;
				while(ssize < nLen)
				{
					if(0 < (ss = UDT::send(sock, (char *)data + ssize, nLen - ssize, 0)))
					{
						ssize += ss;
					}
					else if(ss == 0)
					{
						CCWorker::jvc_sleep(1);
						continue;
					}
					else
					{
						if(m_pWorker->m_bNeedLog)
						{
							if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
							{
								m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "发送数据失败 详细:", __FILE__,__LINE__,(char *)UDT::getlasterror().getErrorMessage());
							}
							else
							{
								m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "SendData faild,INFO:", __FILE__,__LINE__,(char *)UDT::getlasterror().getErrorMessage());
							}
						}
						
						return FALSE;
					}
				}
			}
			break;
		case JVN_REQ_RATE:		
			{
				int nLen = 5+nSize;
				BYTE data[JVN_BAPACKDEFLEN]={0};
				data[0] = uchType;

				if(pBuffer != NULL && nSize < JVN_BAPACKDEFLEN-5 && nSize > 0)
				{
					nLen = 5+nSize;
					memcpy(&data[1], &nSize, 4);
					memcpy(&data[5], pBuffer, nSize);
				}
				else
				{
					break;
				}

				int ss=0;
				int ssize=0;
				while(ssize < nLen)
				{
					if(0 < (ss = UDT::send(sock, (char *)data + ssize, nLen - ssize, 0)))
					{
						ssize += ss;
					}
					else if(ss == 0)
					{
						CCWorker::jvc_sleep(1);
						continue;
					}
					else
					{
						if(m_pWorker->m_bNeedLog)
						{
							if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
							{
								m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "发送数据失败 详细:", __FILE__,__LINE__,(char *)UDT::getlasterror().getErrorMessage());
							}
							else
							{
								m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "SendData faild,INFO:", __FILE__,__LINE__,(char *)UDT::getlasterror().getErrorMessage());
							}
						}
						
						return FALSE;
					}
				}
			}
			break;
		case JVN_DATA_O://自定义数据
			{
				if(pBuffer != NULL && nSize < JVN_BAPACKDEFLEN-5 && nSize > 0)
				{
					int nLen = 5+nSize;
					BYTE data[JVN_BAPACKDEFLEN]={0};
					data[0] = uchType;
					memcpy(&data[1], &nSize, 4);
					memcpy(&data[5], pBuffer, nSize);
					
					int ss=0;
					int ssize=0;
					while(ssize < nLen)
					{
						if(0 < (ss = UDT::send(sock, (char *)data + ssize, nLen - ssize, 0)))
						{
							ssize += ss;
						}
						else if(ss == 0)
						{
							CCWorker::jvc_sleep(1);
							continue;
						}
						else
						{
							if(m_pWorker->m_bNeedLog)
							{
								if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
								{
									m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "发送数据失败 详细:", __FILE__,__LINE__,(char *)UDT::getlasterror().getErrorMessage());
								}
								else
								{
									m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "SendData faild,INFO:", __FILE__,__LINE__,(char *)UDT::getlasterror().getErrorMessage());
								}
							}
							
							return FALSE;
						}
					}
				}
			}
			break;
		default:
			break;
		}
	}
	return TRUE;
}

BOOL CCStream::SendData(BYTE uchType, BYTE *pBuffer,int nSize)
{
	switch(uchType) 
	{
	case JVN_REQ_CHECK://请求录像检索
		{
			if(pBuffer != NULL && nSize == 2*JVN_ABCHECKPLEN)
			{
				BYTE data[13 + 2*JVN_ABCHECKPLEN]={0};
				data[0] = uchType;
				int len=nSize+8;
				memcpy(&data[1],&len,4);
				memcpy(&data[5], &this->m_idMyID, 8);
				memcpy(&data[13], pBuffer, nSize);
				//UDT::send(this->m_SServerSocket, (char *)data,13 + nSize, 0);

				int ss=0;
				int ssize=0;
				while(ssize < len+5)
				{
					if(0 < (ss = UDT::send(this->m_SServerSocket, (char *)data + ssize, len+5 - ssize, 0)))
					{
						ssize += ss;
					}
					else if(ss == 0)
					{
						CCWorker::jvc_sleep(1);
						continue;
					}
					else
					{	
						if(m_pWorker->m_bNeedLog)
						{
							if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
							{
								m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "发送数据失败 详细:", __FILE__,__LINE__,(char *)UDT::getlasterror().getErrorMessage());
							}
							else
							{
								m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "SendData faild,INFO:", __FILE__,__LINE__,(char *)UDT::getlasterror().getErrorMessage());
							}
						}
						
						return FALSE;
					}
				}
			}
		}
		break;
	case JVN_REQ_DOWNLOAD://请求录像下载
	case JVN_REQ_PLAY://请求远程回放
		{
			if(pBuffer != NULL && nSize < MAX_PATH && nSize > 0)
			{
				BYTE data[13 + MAX_PATH]={0};
				data[0] = uchType;
				int len=nSize+8;
				memcpy(&data[1],&len,4);
				memcpy(&data[5], &this->m_idMyID, 8);
				memcpy(&data[13], pBuffer, nSize);

				int ss=0;
				int ssize=0;
				while(ssize < len+5)
				{
					if(0 < (ss = UDT::send(this->m_SServerSocket, (char *)data + ssize,len+5 - ssize, 0)))
					{
						ssize += ss;
					}
					else if(ss == 0)
					{
						CCWorker::jvc_sleep(1);
						continue;
					}
					else
					{	
						if(m_pWorker->m_bNeedLog)
						{
							if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
							{
								m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "发送数据失败 详细:", __FILE__,__LINE__,(char *)UDT::getlasterror().getErrorMessage());
							}
							else
							{
								m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "SendData faild,INFO:", __FILE__,__LINE__,(char *)UDT::getlasterror().getErrorMessage());
							}
						}
						
						return FALSE;
					}
				}
				m_bDAndP = TRUE;
			}
		}
		break;
	
	case JVN_CMD_YTCTRL://云台控制
		{
			if(pBuffer != NULL && nSize == 4)
			{
				BYTE data[17]={0};
				data[0] = uchType;
				int len=nSize+8;
				memcpy(&data[1],&len,4);
				memcpy(&data[5], &this->m_idMyID, 8);
				memcpy(&data[13], pBuffer, nSize);
				//UDT::send(this->m_SServerSocket, (char *)data,17, 0);

				int ss=0;
				int ssize=0;
				while(ssize < len+5)
				{
					if(0 < (ss = UDT::send(this->m_SServerSocket, (char *)data + ssize, len+5 - ssize, 0)))
					{
						ssize += ss;
					}
					else if(ss == 0)
					{
						CCWorker::jvc_sleep(1);
						continue;
					}
					else
					{	
						if(m_pWorker->m_bNeedLog)
						{
							if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
							{
								m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "发送数据失败 详细:", __FILE__,__LINE__,(char *)UDT::getlasterror().getErrorMessage());
							}
							else
							{
								m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "SendData faild,INFO:", __FILE__,__LINE__,(char *)UDT::getlasterror().getErrorMessage());
							}
						}
						
						return FALSE;
					}
				}
			}
		}
		break;
	case JVN_RSP_CHATACCEPT://同意语音
		m_bAcceptChat = TRUE;
		goto DOSEND;
	case JVN_RSP_TEXTACCEPT://同意文本
		m_bAcceptText = TRUE;
		goto DOSEND;
		break;
	case JVN_RSP_ID:
	case JVN_REQ_CHAT://请求语音聊天
	case JVN_REQ_TEXT://请求文本聊天
	case JVN_CMD_DISCONN://断开连接
	case JVN_CMD_PLAYSTOP://暂停播放
	case JVN_CMD_DOWNLOADSTOP://停止下载数据
	case JVN_CMD_CHATSTOP://停止语音聊天
	case JVN_CMD_TEXTSTOP://停止文本聊天
	case JVN_CMD_VIDEO://请求实时监控数据
	case JVN_CMD_VIDEOPAUSE://请求实时监控数据
	case JVN_CMD_PLAYUP://快进
	case JVN_CMD_PLAYDOWN://慢放
	case JVN_CMD_PLAYDEF://原速播放
	case JVN_CMD_PLAYPAUSE://暂停播放
	case JVN_CMD_PLAYGOON://继续播放
	case JVN_DATA_DANDP://下载回放确认
		{
DOSEND:
			int nLen = 13;
			BYTE data[13]={0};
			data[0] = uchType;
			int len=8;
			memcpy(&data[1],&len,4);
			memcpy(&data[5],&this->m_idMyID,8);
			int ss=0;
			int ssize=0;

			while(ssize < nLen)
			{
				if(0 < (ss = UDT::send(this->m_SServerSocket, (char *)data + ssize, nLen - ssize, 0)))
				{
					ssize += ss;
				}
				else if(ss == 0)
				{
					CCWorker::jvc_sleep(1);
					continue;
				}
				else
				{
					if(m_pWorker->m_bNeedLog)
					{
						if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
						{
							m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "发送数据失败 详细:", __FILE__,__LINE__,(char *)UDT::getlasterror().getErrorMessage());
						}
						else
						{
							m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "SendData faild,INFO:", __FILE__,__LINE__,(char *)UDT::getlasterror().getErrorMessage());
						}
					}
					
					return FALSE;
				}
			}
		}
		break;
	case JVN_RSP_TEXTDATA://文本数据
		{
			if(m_bAcceptText)
			{
				if(pBuffer != NULL && nSize < JVN_BAPACKDEFLEN-13 && nSize > 0)
				{
					int nLen = 13+nSize;
					BYTE data[JVN_BAPACKDEFLEN]={0};
					data[0] = uchType;
					int len=nSize+8;
					memcpy(&data[1],&len,4);
					memcpy(&data[5], &this->m_idMyID, 8);
					memcpy(&data[13], pBuffer, nSize);
					
					int ss=0;
					int ssize=0;
					while(ssize < nLen)
					{
						if(0 < (ss = UDT::send(this->m_SServerSocket, (char *)data + ssize, nLen - ssize, 0)))
						{
							ssize += ss;
						}
						else if(ss == 0)
						{
							CCWorker::jvc_sleep(1);
							continue;
						}
						else
						{
							if(m_pWorker->m_bNeedLog)
							{
								if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
								{
									m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "发送数据失败 详细:", __FILE__,__LINE__,(char *)UDT::getlasterror().getErrorMessage());
								}
								else
								{
									m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "SendData faild,INFO:", __FILE__,__LINE__,(char *)UDT::getlasterror().getErrorMessage());
								}
							}
							
							return FALSE;
						}
					}
				}
			}
		}
		break;
	case JVN_RSP_CHATDATA://语音数据
		{
			if(m_bAcceptChat)
			{
				if(pBuffer != NULL && nSize < JVN_BAPACKDEFLEN-13 && nSize > 0)
				{
					int nLen = 13+nSize;
					BYTE data[JVN_BAPACKDEFLEN]={0};
					data[0] = uchType;
					int len=nSize+8;
					memcpy(&data[1],&len,4);
					memcpy(&data[5], &this->m_idMyID, 8);
					memcpy(&data[13], pBuffer, nSize);
					
					int ss=0;
					int ssize=0;
					while(ssize < nLen)
					{
						if(0 < (ss = UDT::send(this->m_SServerSocket, (char *)data + ssize, jvs_min(nLen - ssize, m_nPACKLEN), 0)))
						{
							ssize += ss;
						}
						else if(ss == 0)
						{
							CCWorker::jvc_sleep(1);
							continue;
						}
						else
						{
							if(m_pWorker->m_bNeedLog)
							{
								if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
								{
									m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "发送数据失败 详细:", __FILE__,__LINE__,(char *)UDT::getlasterror().getErrorMessage());
								}
								else
								{
									m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "SendData faild,INFO:", __FILE__,__LINE__,(char *)UDT::getlasterror().getErrorMessage());
								}
							}
							
							return FALSE;
						}
					}
				}
			}
		}
		break;
	case JVN_DATA_O://自定义数据
		{
			if(pBuffer != NULL && nSize < JVN_BAPACKDEFLEN-13 && nSize > 0)
			{
				int nLen = 13+nSize;
				BYTE data[JVN_BAPACKDEFLEN]={0};
				data[0] = uchType;
				int len=nSize+8;
				memcpy(&data[1],&len,4);
				memcpy(&data[5], &this->m_idMyID, 8);
				memcpy(&data[13], pBuffer, nSize);
				
				int ss=0;
				int ssize=0;
				while(ssize < nLen)
				{
					if(0 < (ss = UDT::send(this->m_SServerSocket, (char *)data + ssize, nLen - ssize, 0)))
					{
						ssize += ss;
					}
					else if(ss == 0)
					{
						CCWorker::jvc_sleep(1);
						continue;
					}
					else
					{
						if(m_pWorker->m_bNeedLog)
						{
							if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
							{
								m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "发送数据失败 详细:", __FILE__,__LINE__,(char *)UDT::getlasterror().getErrorMessage());
							}
							else
							{
								m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "SendData faild,INFO:", __FILE__,__LINE__,(char *)UDT::getlasterror().getErrorMessage());
							}
						}
						
						return FALSE;
					}
				}
			}
		}
		break;
	default:
		break;
	}
	return TRUE;
}

BOOL CCStream::DisConnectFromStream()
{
	SendData(this->m_SServerSocket,JVN_CMD_DISCONN, NULL, 0);
	m_bExit = TRUE;
	CCWorker::jvc_sleep(10);	
	
	if(m_SServerSocket > 0)
	{
		if(m_nProtocolType == TYPE_PC_TCP || m_nProtocolType == TYPE_MO_TCP)
		{//TCP
			closesocket(m_SServerSocket);
		}
		else
		{
			UDT::close(m_SServerSocket);
		}
	}
	m_SServerSocket = 0;
	return TRUE;
}

void CCStream::ClearBuffer()
{
	if(m_pBuffer != NULL)
	{
		m_pBuffer->ClearBuffer();
	}
}

BOOL CCStream::SendDataTCP(int sock,BYTE uchType, BYTE *pBuffer,int nSize)
{
	if(sock > 0)
	{
		switch(uchType) 
		{
		case JVN_REQ_CHECK://请求录像检索
			{
				if(pBuffer != NULL && nSize == 2*JVN_ABCHECKPLEN)
				{
					BYTE data[5 + 2*JVN_ABCHECKPLEN]={0};
					data[0] = uchType;
					memcpy(&data[1], &nSize, 4);
					memcpy(&data[5], pBuffer, nSize);
				#ifndef WIN32
					send(sock, (char *)data,5 + nSize, MSG_NOSIGNAL);
				#else
					send(sock, (char *)data,5 + nSize, 0);
				#endif
				}
			}
			break;
		case JVN_REQ_DOWNLOAD://请求录像下载
		case JVN_REQ_PLAY://请求远程回放
			{
				if(pBuffer != NULL && nSize < MAX_PATH && nSize > 0)
				{
					BYTE data[5 + MAX_PATH]={0};
					data[0] = uchType;
					memcpy(&data[1], &nSize, 4);
					memcpy(&data[5], pBuffer, nSize);

					int nLen = nSize + 5;
					int ss=0;
					int ssize=0;
					while(ssize < nLen)
					{
					#ifndef WIN32
						if ((ss = send(sock, (char *)data + ssize,nLen - ssize, MSG_NOSIGNAL)) <= 0)
						{
							if(ss < 0 && (errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN))
							{
								CCWorker::jvc_sleep(1);
								continue;
							}
					#else
						if (SOCKET_ERROR == (ss = send(sock, (char *)data + ssize, nLen - ssize, 0)))
						{
							int kkk = WSAGetLastError();
							if(kkk == WSAEINTR || kkk == WSAEWOULDBLOCK || kkk == WSAENOBUFS)
							{
								CCWorker::jvc_sleep(1);
								continue;
							}
					#endif	
							if(m_pWorker->m_bNeedLog)
							{
								if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
								{
									m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "发送数据失败 详细:", __FILE__,__LINE__,(char *)UDT::getlasterror().getErrorMessage());
								}
								else
								{
									m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "SendData faild,INFO:", __FILE__,__LINE__,(char *)UDT::getlasterror().getErrorMessage());
								}
							}
							
							return FALSE;
						}
						ssize += ss;
					}
					m_bDAndP = TRUE;
				}
			}
			break;
		
		case JVN_CMD_YTCTRL://云台控制
			{
				if(pBuffer != NULL && nSize == 4)
				{
					BYTE data[9]={0};
					data[0] = uchType;
					memcpy(&data[1], &nSize, 4);
					memcpy(&data[5], pBuffer, nSize);
				#ifndef WIN32
					send(sock, (char *)data,9, MSG_NOSIGNAL);
				#else
					send(sock, (char *)data,9, 0);
				#endif
				}
			}
			break;
		case JVN_RSP_CHATACCEPT://同意语音
			m_bAcceptChat = TRUE;
			goto DOSEND;
		case JVN_RSP_TEXTACCEPT://同意文本
			m_bAcceptText = TRUE;
			goto DOSEND;
		case JVN_REQ_CHAT://请求语音聊天
		case JVN_REQ_TEXT://请求文本聊天
		case JVN_CMD_DISCONN://断开连接
		case JVN_CMD_PLAYSTOP://暂停播放
		case JVN_CMD_DOWNLOADSTOP://停止下载数据
		case JVN_CMD_CHATSTOP://停止语音聊天
		case JVN_CMD_TEXTSTOP://停止文本聊天
		case JVN_CMD_VIDEO://请求实时监控数据
		case JVN_CMD_VIDEOPAUSE://请求实时监控数据
		case JVN_CMD_PLAYUP://快进
		case JVN_CMD_PLAYDOWN://慢放
		case JVN_CMD_PLAYDEF://原速播放
		case JVN_CMD_PLAYPAUSE://暂停播放
		case JVN_CMD_PLAYGOON://继续播放
		case JVN_DATA_DANDP://下载回放确认
			{
DOSEND:
				int nLen = 5;
				BYTE data[5]={0};
				data[0] = uchType;

				int ss=0;
				int ssize=0;
				while(ssize < nLen)
				{
				#ifndef WIN32
					if ((ss = send(sock, (char *)data + ssize,nLen - ssize, MSG_NOSIGNAL)) <= 0)
					{
						if(ss < 0 && (errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN))
						{
							CCWorker::jvc_sleep(1);
							continue;
						}
				#else
					if (SOCKET_ERROR == (ss = send(sock, (char *)data + ssize, nLen - ssize, 0)))
					{
						int kkk = WSAGetLastError();
						if(kkk == WSAEINTR || kkk == WSAEWOULDBLOCK || kkk == WSAENOBUFS)
						{
							CCWorker::jvc_sleep(1);
							continue;
						}
				#endif
						if(m_pWorker->m_bNeedLog)
						{
							if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
							{
								m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "发送数据失败 详细:", __FILE__,__LINE__,(char *)UDT::getlasterror().getErrorMessage());
							}
							else
							{
								m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "SendData faild,INFO:", __FILE__,__LINE__,(char *)UDT::getlasterror().getErrorMessage());
							}
						}
						
						return FALSE;
					}
					ssize += ss;
				}
			}
			break;
		case JVN_DATA_OK://视频帧确认
			{
				int nLen = 9;
				BYTE data[9]={0};
				data[0] = uchType;
				memcpy(&data[1], &nSize, 4);
				if(pBuffer != NULL)
				{
					memcpy(&data[5], pBuffer, nSize);
				}
				
				int ss=0;
				int ssize=0;
				while(ssize < nLen)
				{
				#ifndef WIN32
					if ((ss = send(sock, (char *)data + ssize,nLen - ssize, MSG_NOSIGNAL)) <= 0)
					{
						if(ss < 0 && (errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN))
						{
							CCWorker::jvc_sleep(1);
							continue;
						}
				#else
					if (SOCKET_ERROR == (ss = send(sock, (char *)data + ssize, nLen - ssize, 0)))
					{
						int kkk = WSAGetLastError();
						if(kkk == WSAEINTR || kkk == WSAEWOULDBLOCK || kkk == WSAENOBUFS)
						{
							CCWorker::jvc_sleep(1);
							continue;
						}
				#endif
						if(m_pWorker->m_bNeedLog)
						{
							if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
							{
								m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "发送数据失败 详细:", __FILE__,__LINE__,(char *)UDT::getlasterror().getErrorMessage());
							}
							else
							{
								m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "SendData faild,INFO:", __FILE__,__LINE__,(char *)UDT::getlasterror().getErrorMessage());
							}
						}
						
						return FALSE;
					}
					ssize += ss;
				}
			}
			break;
		case JVN_RSP_TEXTDATA://文本数据
			{
				if(m_bAcceptText)
				{
					if(pBuffer != NULL && nSize < JVN_BAPACKDEFLEN-5 && nSize > 0)
					{
						int nLen = 5+nSize;
						BYTE data[JVN_BAPACKDEFLEN]={0};
						data[0] = uchType;
						memcpy(&data[1], &nSize, 4);
						memcpy(&data[5], pBuffer, nSize);
						
						int ss=0;
						int ssize=0;
						while(ssize < nLen)
						{
						#ifndef WIN32
							if ((ss = send(sock, (char *)data + ssize,nLen - ssize, MSG_NOSIGNAL)) <= 0)
							{
								if(ss < 0 && (errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN))
								{
									CCWorker::jvc_sleep(1);
									continue;
								}
						#else
							if (SOCKET_ERROR == (ss = send(sock, (char *)data + ssize, nLen - ssize, 0)))
							{
								int kkk = WSAGetLastError();
								if(kkk == WSAEINTR || kkk == WSAEWOULDBLOCK || kkk == WSAENOBUFS)
								{
									CCWorker::jvc_sleep(1);
									continue;
								}
						#endif
								if(m_pWorker->m_bNeedLog)
								{
									if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
									{
										m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "发送数据失败 详细:", __FILE__,__LINE__,(char *)UDT::getlasterror().getErrorMessage());
									}
									else
									{
										m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "SendData faild,INFO:", __FILE__,__LINE__,(char *)UDT::getlasterror().getErrorMessage());
									}
								}
								
								return FALSE;
							}
							ssize += ss;
						}
					}
				}
			}
			break;
		case JVN_RSP_CHATDATA://语音数据
			{
				if(m_bAcceptChat)
				{
					if(pBuffer != NULL && nSize < JVN_BAPACKDEFLEN-5 && nSize > 0)
					{
						int nLen = 5+nSize;
						BYTE data[JVN_BAPACKDEFLEN]={0};
						data[0] = uchType;
						memcpy(&data[1], &nSize, 4);
						memcpy(&data[5], pBuffer, nSize);
						
						int ss=0;
						int ssize=0;
						while(ssize < nLen)
						{
						#ifndef WIN32
							if ((ss = send(sock, (char *)data + ssize,jvs_min(nLen - ssize, m_nPACKLEN), MSG_NOSIGNAL)) <= 0)
							{
								if(ss < 0 && (errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN))
								{
									CCWorker::jvc_sleep(1);
									continue;
								}
						#else
							if (SOCKET_ERROR == (ss = send(sock, (char *)data + ssize, jvs_min(nLen - ssize, m_nPACKLEN), 0)))
							{
								int kkk = WSAGetLastError();
								if(kkk == WSAEINTR || kkk == WSAEWOULDBLOCK || kkk == WSAENOBUFS)
								{
									CCWorker::jvc_sleep(1);
									continue;
								}
						#endif
								if(m_pWorker->m_bNeedLog)
								{
									if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
									{
										m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "发送数据失败 详细:", __FILE__,__LINE__,(char *)UDT::getlasterror().getErrorMessage());
									}
									else
									{
										m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "SendData faild,INFO:", __FILE__,__LINE__,(char *)UDT::getlasterror().getErrorMessage());
									}
								}
								
								return FALSE;
							}
							ssize += ss;
						}
					}
				}
			}
			break;
		case JVN_CMD_ONLYI://只要关键帧
		case JVN_CMD_FULL://回复满帧发送
			{
				int nLen = 5+nSize;
				BYTE data[JVN_BAPACKDEFLEN]={0};
				data[0] = uchType;

				if(pBuffer != NULL && nSize < JVN_BAPACKDEFLEN-5 && nSize > 0)
				{
					nLen = 5+nSize;
					memcpy(&data[1], &nSize, 4);
					memcpy(&data[5], pBuffer, nSize);
				}
				else
				{
					nLen = 5;
				}
					
				int ss=0;
				int ssize=0;
				while(ssize < nLen)
				{
				#ifndef WIN32
					if ((ss = send(sock, (char *)data + ssize,nLen - ssize, MSG_NOSIGNAL)) <= 0)
					{
						if(ss < 0 && (errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN))
						{
							CCWorker::jvc_sleep(1);
							continue;
						}
				#else
					if (SOCKET_ERROR == (ss = send(sock, (char *)data + ssize, nLen - ssize, 0)))
					{
						int kkk = WSAGetLastError();
						if(kkk == WSAEINTR || kkk == WSAEWOULDBLOCK || kkk == WSAENOBUFS)
						{
							CCWorker::jvc_sleep(1);
							continue;
						}
				#endif
						if(m_pWorker->m_bNeedLog)
						{
							if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
							{
								m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "发送数据失败 详细:", __FILE__,__LINE__,(char *)UDT::getlasterror().getErrorMessage());
							}
							else
							{
								m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "SendData faild,INFO:", __FILE__,__LINE__,(char *)UDT::getlasterror().getErrorMessage());
							}
						}
								
						return FALSE;
					}
					ssize += ss;
				}
			}
			break;
		case JVN_REQ_RATE:
			{
				int nLen = 5+nSize;
				BYTE data[JVN_BAPACKDEFLEN]={0};
				data[0] = uchType;

				if(pBuffer != NULL && nSize < JVN_BAPACKDEFLEN-5 && nSize > 0)
				{
					nLen = 5+nSize;
					memcpy(&data[1], &nSize, 4);
					memcpy(&data[5], pBuffer, nSize);
				}
				else
				{
					break;
				}
					
				int ss=0;
				int ssize=0;
				while(ssize < nLen)
				{
				#ifndef WIN32
					if ((ss = send(sock, (char *)data + ssize,nLen - ssize, MSG_NOSIGNAL)) <= 0)
					{
						if(ss < 0 && (errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN))
						{
							CCWorker::jvc_sleep(1);
							continue;
						}
				#else
					if (SOCKET_ERROR == (ss = send(sock, (char *)data + ssize, nLen - ssize, 0)))
					{
						int kkk = WSAGetLastError();
						if(kkk == WSAEINTR || kkk == WSAEWOULDBLOCK || kkk == WSAENOBUFS)
						{
							CCWorker::jvc_sleep(1);
							continue;
						}
				#endif
						if(m_pWorker->m_bNeedLog)
						{
							if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
							{
								m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "发送数据失败 详细:", __FILE__,__LINE__,(char *)UDT::getlasterror().getErrorMessage());
							}
							else
							{
								m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "SendData faild,INFO:", __FILE__,__LINE__,(char *)UDT::getlasterror().getErrorMessage());
							}
						}
								
						return FALSE;
					}
					ssize += ss;
				}
			}
			break;
		case JVN_DATA_O://自定义数据
			{
				if(pBuffer != NULL && nSize < JVN_BAPACKDEFLEN-5 && nSize > 0)
				{
					int nLen = 5+nSize;
					BYTE data[JVN_BAPACKDEFLEN]={0};
					data[0] = uchType;
					memcpy(&data[1], &nSize, 4);
					memcpy(&data[5], pBuffer, nSize);
					
					int ss=0;
					int ssize=0;
					while(ssize < nLen)
					{
					#ifndef WIN32
						if ((ss = send(sock, (char *)data + ssize,nLen - ssize, MSG_NOSIGNAL)) <= 0)
						{
							if(ss < 0 && (errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN))
							{
								CCWorker::jvc_sleep(1);
								continue;
							}
					#else
						if (SOCKET_ERROR == (ss = send(sock, (char *)data + ssize, nLen - ssize, 0)))
						{
							int kkk = WSAGetLastError();
							if(kkk == WSAEINTR || kkk == WSAEWOULDBLOCK || kkk == WSAENOBUFS)
							{
								CCWorker::jvc_sleep(1);
								continue;
							}
					#endif
							if(m_pWorker->m_bNeedLog)
							{
								if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
								{
									m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "发送数据失败 详细:", __FILE__,__LINE__,(char *)UDT::getlasterror().getErrorMessage());
								}
								else
								{
									m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "SendData faild,INFO:", __FILE__,__LINE__,(char *)UDT::getlasterror().getErrorMessage());
								}
							}
							
							return FALSE;
						}
						ssize += ss;
					}
				}
			}
			break;
		default:
			break;
		}
	}
	return TRUE;
}

BOOL CCStream::ConnectStreamServer()
{
	//if(m_nProtocolType == TYPE_PC_TCP || m_nProtocolType == TYPE_MO_TCP)
	//{//TCP连接
	//	return ConnectStreamServerTCP();
	//}

	if(this->m_SServerSocket > 0)
	{
		UDT::close(this->m_SServerSocket);
	}
	this->m_SServerSocket = 0;

	this->m_SServerSocket = UDT::socket(AF_INET, SOCK_STREAM, 0);
	if(this->m_SServerSocket==UDT::INVALID_SOCK)
	{
		this->m_SServerSocket = 0;
		return FALSE;
	}

	sockaddr_in addrt;
	int lent=sizeof(addrt);
	UDT::getsockname(m_pParent->m_pChannel->m_ServerSocket, (sockaddr *)&addrt, &lent);

	BOOL bReuse = TRUE;
	UDT::setsockopt(m_pParent->m_pChannel->m_ServerSocket, 0, UDT_REUSEADDR, (char *)&bReuse, sizeof(BOOL));
	
	SOCKADDR_IN addr1;
#ifndef WIN32
	addr1.sin_addr.s_addr = INADDR_ANY;
#else
	addr1.sin_addr.S_un.S_addr = INADDR_ANY;
#endif
	
	addr1.sin_family = AF_INET;
	addr1.sin_port =addrt.sin_port;
	//////////////////////////////////////////////////////////////////////////
	int len1 = JVC_MSS;
	UDT::setsockopt(this->m_SServerSocket, 0, UDT_MSS, &len1, sizeof(int));
	//////////////////////////////////////////////////////////////////////////
	if(UDT::bind(this->m_SServerSocket,(sockaddr *)&addr1,sizeof(addr1))==UDT::ERROR)
	{
		addr1.sin_port=0;
		UDT::bind(this->m_SServerSocket,(sockaddr *)&addr1,sizeof(addr1));
	}

	SOCKADDR_IN addrA;
#ifndef WIN32
	addrA.sin_addr.s_addr = this->m_nSServerIP;
#else
	addrA.sin_addr.S_un.S_addr = this->m_nSServerIP;
#endif
	
	addrA.sin_family = AF_INET;
	addrA.sin_port =htons(this->m_nSServerPort);
	//将套接字置为非阻塞模式
	BOOL block = FALSE;
	UDT::setsockopt(this->m_SServerSocket, 0, UDT_SNDSYN, &block, sizeof(BOOL));
	UDT::setsockopt(this->m_SServerSocket, 0, UDT_RCVSYN, &block, sizeof(BOOL));
	LINGER linger;
	linger.l_onoff = 0;
	linger.l_linger = 0;
	UDT::setsockopt(this->m_SServerSocket, 0, UDT_LINGER, &linger, sizeof(LINGER));

	STJUDTCONN stcon;
	stcon.u = this->m_SServerSocket;
	stcon.name = (SOCKADDR *)&addrA;
	stcon.namelen = sizeof(SOCKADDR);
	stcon.nChannelID = this->m_nChannel;
	stcon.nLVer_new = JVN_YSTVER;
	stcon.nLVer_old = JVN_YSTVER1;//必须这样，否则不兼容49-67的主控
	stcon.nMinTime = 3000;
	if(UDT::ERROR == UDT::connect(stcon))
	{
		if(this->m_SServerSocket > 0)
		{
			UDT::close(this->m_SServerSocket);
		}
		this->m_SServerSocket = 0;
		if(m_pWorker->m_bNeedLog)
		{
			if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
			{
				m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "局域网连接主控失败. 连接主控失败 详细:", __FILE__,__LINE__,(char *)UDT::getlasterror().getErrorMessage());
			}
			else
			{
				m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "Local connect faild. connect op. faild. INFO:", __FILE__,__LINE__,(char *)UDT::getlasterror().getErrorMessage());
			}
		}
		return FALSE;
	}
	
	//初步连接成功 发送身份验证信息	
	
	ReqConnectPacket req;
	req.cType=JVN_REQ_CONNSTREAM_CLT;
	req.nLen=0;
	req.nProto=TYPE_PC_UDP;
	req.nChannel=this->m_nChannel;
	req.idSID=this->m_idSServerID;
	req.idCID=this->m_idMyID;
	this->m_idMyID=req.idCID;

	if(UDT::send(this->m_SServerSocket, (char *)&req, sizeof(req), 0)==UDT::ERROR)
	{
		if(this->m_SServerSocket > 0)
		{
			UDT::close(this->m_SServerSocket);
		}
		this->m_SServerSocket = 0;

		if(m_pWorker->m_bNeedLog)
		{
			if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
			{
				m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "局域网连接主控失败. 向主控发送身份验证数据失败 详细:", __FILE__,__LINE__,(char *)UDT::getlasterror().getErrorMessage());
			}
			else
			{
				m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "Local connect faild. send pass info faild. Info:", __FILE__,__LINE__,(char *)UDT::getlasterror().getErrorMessage());
			}
		}
		return FALSE;
	}
#ifndef WIN32
	pthread_attr_t attr;
    pthread_attr_t *pAttr = &attr;
    unsigned long size = LINUX_THREAD_STACK_SIZE;
    size_t stacksize = size;
    pthread_attr_init(pAttr);
    if ((pthread_attr_setstacksize(pAttr,stacksize)) != 0)
    {
        pAttr = NULL;
    }
	if (0 != pthread_create(&m_hSRecvThread, pAttr, RecvFromSServer, this))
	{
		m_hSRecvThread = 0;

		if(this->m_SServerSocket> 0)
		{
			UDT::close(this->m_SServerSocket);
		}
		this->m_SServerSocket = 0;
		
		if(m_pWorker != NULL)
		{
			if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
			{
				m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "局域网连接主控失败. 创建接收线程失败", __FILE__,__LINE__);
			}
			else
			{
				m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "Local connect faild. create receive thread faild.", __FILE__,__LINE__);
			}
		}
		return FALSE;
	}
#else
	//启动接收线程
	UINT unTheadID;
	//创建本地监听线程
	m_hSRecvThread = (HANDLE)_beginthreadex(NULL, 0,this->RecvFromSServer, (void *)this, 0, &unTheadID);
	if (m_hSRecvThread == 0)//创建线程失败
	{
		if(this->m_SServerSocket> 0)
		{
			UDT::close(this->m_SServerSocket);
		}
		this->m_SServerSocket = 0;
		
		if(m_pWorker != NULL)
		{
			if(JVN_LANGUAGE_CHINESE == m_pWorker->m_nLanguage)
			{
				m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "局域网连接主控失败. 创建接收线程失败", __FILE__,__LINE__);
			}
			else
			{
				m_pWorker->m_Log.SetRunInfo(m_nLocalChannel, "Local connect faild. create receive thread faild.", __FILE__,__LINE__);
			}
		}
		return FALSE;
	}
#endif
	
	return TRUE;
}

#ifndef WIN32
	void* CCStream::RecvFromSServer(void* pParam)
#else
	UINT WINAPI CCStream::RecvFromSServer(LPVOID pParam)
#endif
{
	CCStream *pWorker = (CCStream *)pParam;
	pWorker->RecvProc();
	pWorker->m_pParent->DisConnect();
	return 0;
}
int CCStream::RecvProc()
{
	int rsize = 0;
	int rs=0;

	//计时相关
	DWORD dBeginPassTime;//开始身份验证超时计时
	DWORD dBeginTime=0;//视频数据计时
	DWORD dBeginTimeDP=0;//下载回放计时
	DWORD dEnd=0;
	DWORD dTimePassUsed=0;
	DWORD dTimeUsed=0;
	DWORD dTimeUsedDP=0;
	
	//计时
	dBeginPassTime = CCWorker::JVGetTime();
	dBeginTime = CCWorker::JVGetTime();
	dBeginTimeDP = CCWorker::JVGetTime();

	BYTE recBuf[JVNC_DATABUFLEN];
	int nIRetryCount = 0;
	int nDPRetryCount = 0;

	int nFrameIndex = 0;
	BOOL bError = FALSE;//数据出错，此时视频帧必须从I帧开始继续接收，防止出现马赛克

	BOOL bRet = TRUE;
	int nLen=-1;
	BYTE uchType=0;
	int nLenRead=-1;
	BYTE uchTypeRead=0;
	while(TRUE)
	{		
		if(this->m_bExit)
		{
			this->SendData(this->m_SServerSocket,JVN_CMD_DISCONN, NULL, 0);
			CCWorker::jvc_sleep(1);
			break;
		}
		/*读缓冲区,显示*/
		uchTypeRead = 0;
		if(this->m_pBuffer->ReadBuffer(uchTypeRead, recBuf, nLenRead))
		{
			this->m_pWorker->NormalData(this->m_nLocalChannel,uchTypeRead, recBuf, nLenRead, 0,0);
		}

		uchType = 0;
		rs = 0;

		if (UDT::ERROR == (rs = UDT::recvmsg(this->m_SServerSocket, (char *)recBuf, JVNC_DATABUFLEN)))
		{
			if(this->m_bExit)
			{
				this->SendData(this->m_SServerSocket,JVN_CMD_DISCONN, NULL, 0);
				CCWorker::jvc_sleep(1);
				break;
			}
			//结束线程
			if(this->m_pWorker != NULL)
			{
				this->m_pWorker->ConnectChange(this->m_nLocalChannel,JVN_CCONNECTTYPE_DISCONNE,NULL,0,__FILE__,__LINE__,__FUNCTION__);
			}

			if(this->m_pWorker->m_bNeedLog)
			{
				if(JVN_LANGUAGE_CHINESE == this->m_pWorker->m_nLanguage)
				{
					this->m_pWorker->m_Log.SetRunInfo(this->m_nLocalChannel, "接收数据失败,接收线程退出. 详细:", __FILE__,__LINE__,(char *)UDT::getlasterror().getErrorMessage());
				}
				else
				{
					this->m_pWorker->m_Log.SetRunInfo(this->m_nLocalChannel, "receive thread over,receive faild. INFO:", __FILE__,__LINE__,(char *)UDT::getlasterror().getErrorMessage());
				}
			}
		
			break;
		}
		else if(rs == 0)
		{
			////////////////////////////////////////计时 超过40s未收到任何帧,发送响应帧,超过5分钟仍无数据,不再发送
			dEnd = CCWorker::JVGetTime();
			dTimeUsed = dEnd - dBeginTime;
			dTimeUsedDP = dEnd - dBeginTimeDP;
			dTimePassUsed = dEnd - dBeginPassTime;
			
			if(!this->m_bPass && dTimeUsed > 30000)
			{
				//结束线程
				if(this->m_SServerSocket > 0)
				{
					UDT::close(this->m_SServerSocket);
				}
				this->m_SServerSocket = 0;
				
				if(JVN_LANGUAGE_CHINESE == this->m_pWorker->m_nLanguage)
				{
					char chMsg[] = "连接超时.请重试或重新运行程序.";
					this->m_pWorker->ConnectChange(this->m_nLocalChannel,JVN_CCONNECTTYPE_CONNERR, chMsg,0,__FILE__,__LINE__,__FUNCTION__);
					this->m_pWorker->m_Log.SetRunInfo(this->m_nLocalChannel, "接收身份验证数据超时,接收线程退出.", __FILE__,__LINE__);
				}
				else
				{
					char chMsg[] = "connect time out!";
					this->m_pWorker->ConnectChange(this->m_nLocalChannel,JVN_CCONNECTTYPE_CONNERR, chMsg,0,__FILE__,__LINE__,__FUNCTION__);
					this->m_pWorker->m_Log.SetRunInfo(this->m_nLocalChannel, "receive thread over,receive pass info time out.", __FILE__,__LINE__);
				}
				
				return 0;
			}
			if((dTimePassUsed>3000)&& (!this->m_bPass))
			{
				ReqConnectPacket req;
				req.cType=JVN_REQ_CONNSTREAM_CLT;
				req.nLen=20;
				req.nProto=TYPE_PC_UDP;
				req.nChannel=this->m_nChannel;
				req.idSID=this->m_idSServerID;
				COPYID(req.idCID,this->m_idMyID);
				UDT::send(this->m_SServerSocket, (char *)&req, 25, 0);
			}
			if(dTimeUsed > 40000 && (dTimeUsed-40000 > 40000*nIRetryCount) && nIRetryCount < 10)
			{
				this->SendData(this->m_SServerSocket,JVN_DATA_OK, NULL, 0);//视频帧确认
				nIRetryCount++;
			}
			if(this->m_bDAndP && dTimeUsedDP > 40000 && (dTimeUsedDP-40000 > 40000*nDPRetryCount) && nDPRetryCount < 2)
			{
				this->SendData(JVN_DATA_DANDP, NULL, 0);//下载回放确认
				nDPRetryCount++;
			}
			CCWorker::jvc_sleep(5);
			continue;
		}  
		nLen=-1;
		uchType = recBuf[0];
		switch(uchType)
		{
		case JVN_RSP_ID:
		case JVN_DATA_B://视频B帧
		case JVN_DATA_P://视频P帧
		case JVN_DATA_I://视频I帧
		case JVN_DATA_SKIP://视频S帧
		case JVN_DATA_A://音频
		case JVN_DATA_S://帧尺寸
		case JVN_DATA_O://自定义数据
		case JVN_DATA_HEAD://解码头
		case JVN_REQ_CHAT://请求语音聊天
		case JVN_REQ_TEXT://请求文本聊天
		case JVN_RSP_CHATACCEPT://同意语音请求
		case JVN_RSP_TEXTACCEPT://同意文本请求
		case JVN_CMD_CHATSTOP://停止语音
		case JVN_CMD_TEXTSTOP://停止文本
		case JVN_RSP_CHECKDATA://检索结果
		case JVN_RSP_CHECKOVER://检索完成(无检索内容)
		case JVN_RSP_CHATDATA://语音数据
		case JVN_RSP_TEXTDATA://文本数据
		case JVN_RSP_DOWNLOADDATA://下载数据
		case JVN_RSP_DOWNLOADE://下载数据失败
		case JVN_RSP_DOWNLOADOVER://下载数据完成
		case JVN_RSP_PLAYDATA://回放数据
		case JVN_RSP_PLAYOVER://回放完成
		case JVN_RSP_PLAYE://回放失败
		case JVN_RSP_DLTIMEOUT://下载超时
		case JVN_RSP_PLTIMEOUT://回放超时
		case JVN_CMD_DISCONN://服务端断开连接
		case JVN_CMD_FRAMETIME://帧间隔
		case JVN_DATA_SPEED://获取主控码率
			{
				memcpy(&nLen, &recBuf[1], 4);
				if(nLen < 0 || nLen > JVNC_DATABUFLEN)
				{
					bError = TRUE;
					continue;
				}
			}
			break;
		default:
			continue;
		}

		switch(uchType) 
		{
		case JVN_RSP_ID:
			{
				int id=0;
				memcpy(&id,recBuf,4);
				this->m_bPass=TRUE;
				BYTE buf[5]={0};
				buf[0]=JVN_CMD_VIDEO;
				UDT::send(this->m_SServerSocket,(char *)buf,5,0);
			}
			break;
		case JVN_DATA_B://视频B帧
		case JVN_DATA_P://视频P帧
		case JVN_DATA_I://视频I帧
		case JVN_DATA_SKIP://视频S帧
			{
				/*判断收到的帧编号，I帧确认，一个序列内每10帧一个确认*/
				memcpy(&nFrameIndex, &recBuf[5], 4);
				nFrameIndex=*(int *)recBuf;
				if(uchType == JVN_DATA_I)
				{//收到I视频帧，确认
					bError = FALSE;
					this->m_nOCount = 1;
					this->SendData(this->m_SServerSocket,JVN_DATA_OK, recBuf, 4);
				}
				else
				{
					this->m_nOCount++;	
					if(this->m_nOCount%JVNC_ABFRAMERET == 0)
					{					
						this->SendData(this->m_SServerSocket,JVN_DATA_OK, recBuf, 4);
					}
				}
				dTimeUsed = CCWorker::JVGetTime() - dBeginTime;
				//计时
				dBeginTime = CCWorker::JVGetTime();
				nIRetryCount=0;
				if(this->m_pWorker != NULL && !bError)
				{
					recBuf[3] = nFrameIndex & 0xFF;
					//写入缓冲
					this->m_pBuffer->WriteBuffer(uchType, &recBuf[3], nLen-3, dTimeUsed);
				}
			}
			break;
		case JVN_DATA_A://音频
		case JVN_DATA_O://自定义数据
		case JVN_DATA_HEAD://解码头
			{			
				if(this->m_pWorker != NULL)
				{
					if(uchType == JVN_DATA_HEAD && m_pBuffer != NULL)
					{
						m_pBuffer->ClearBuffer();
					}
					this->m_pWorker->NormalData(this->m_nLocalChannel,uchType, recBuf, nLen, 0,0);
				}
			}
			break;
		case JVN_CMD_FRAMETIME://帧间隔
		case JVN_DATA_S://帧尺寸
			{
				int nWidth = 0, nHeight = 0;
				if(nLen == 8)
				{
					rsize = 0;
					rs = 0;
					memcpy(&nWidth, &recBuf[0], 4);
					memcpy(&nHeight, &recBuf[4], 4);
					if(uchType == JVN_DATA_S)
					{
						if(this->m_pWorker != NULL)
						{
							this->m_pWorker->NormalData(this->m_nLocalChannel,uchType, NULL, 0, nWidth,nHeight);
						}
					}
					else
					{
						this->m_pBuffer->m_nFrameTime = nWidth;
						this->m_pBuffer->m_nFrames = nHeight>0?nHeight:50;

						this->m_pBuffer->m_nFrameTime = jvs_max(this->m_pBuffer->m_nFrameTime, 0);
						this->m_pBuffer->m_nFrameTime = jvs_min(this->m_pBuffer->m_nFrameTime, 10000);
						break;
					}
				}
				else
				{//数据出错，应从下一个字节开始判断,丢弃下一个I帧之前的帧
					bError = TRUE;
				}
			}
			break;
		case JVN_REQ_CHAT://请求语音聊天
			{
				if(nLen == 0)
				{
					/*调用回调函数，*/
					if(this->m_pWorker != NULL)
					{
						this->m_pWorker->ChatData(this->m_nLocalChannel, uchType, NULL, 0);
					}
				}
				else
				{//数据出错，应从下一个字节开始判断,丢弃下一个I帧之前的帧
					bError = TRUE;
				}
			}
			break;
		case JVN_REQ_TEXT://请求文本聊天
			{
				if(nLen == 0)
				{
					/*调用回调函数，*/
					if(this->m_pWorker != NULL)
					{
						this->m_pWorker->TextData(this->m_nLocalChannel, uchType, NULL, 0);
					}
				}
				else
				{//数据出错，应从下一个字节开始判断,丢弃下一个I帧之前的帧
					bError = TRUE;
				}
			}
			break;
		case JVN_RSP_CHATACCEPT://同意语音请求
			{
				if(nLen == 0)
				{
					/*回调函数通知是否同意语音请求*/
					if(this->m_pWorker != NULL)
					{
						this->m_pWorker->ChatData(this->m_nLocalChannel,uchType, NULL, 0);
					}
					this->m_bAcceptChat = TRUE;
				}
				else
				{//数据出错，应从下一个字节开始判断,丢弃下一个I帧之前的帧
					bError = TRUE;
				}
			}
			break;
		case JVN_RSP_TEXTACCEPT://同意文本请求
			{
				if(nLen == 0)
				{
					/*回调函数通知是否同意语音请求*/
					if(this->m_pWorker != NULL)
					{
						this->m_pWorker->TextData(this->m_nLocalChannel,uchType, NULL, 0);
					}
					this->m_bAcceptText = TRUE;
				}
				else
				{//数据出错，应从下一个字节开始判断,丢弃下一个I帧之前的帧
					bError = TRUE;
				}
			}
			break;
		case JVN_CMD_CHATSTOP://停止语音
			{
				if(nLen == 0)
				{
					/*回调函数通知是否同意语音请求*/
					if(this->m_pWorker != NULL)
					{
						this->m_pWorker->ChatData(this->m_nLocalChannel,uchType, NULL, 0);
					}
					this->m_bAcceptChat = FALSE;
				}
				else
				{//数据出错，应从下一个字节开始判断,丢弃下一个I帧之前的帧
					bError = TRUE;
				}
			}
			break;
		case JVN_CMD_TEXTSTOP://停止文本
			{
				if(nLen == 0)
				{
					/*回调函数通知是否同意语音请求*/
					if(this->m_pWorker != NULL)
					{
						this->m_pWorker->TextData(this->m_nLocalChannel,uchType, NULL, 0);
					}
					this->m_bAcceptText = FALSE;
				}
				else
				{//数据出错，应从下一个字节开始判断,丢弃下一个I帧之前的帧
					bError = TRUE;
				}
			}
			break;
		case JVN_RSP_CHECKDATA://检索结果
		case JVN_RSP_CHATDATA://语音数据
		case JVN_RSP_TEXTDATA://文本数据
			{
				if(uchType == JVN_RSP_CHATDATA && this->m_bAcceptChat)
				{
					/*回调函数语音*/
					if(this->m_pWorker != NULL)
					{
						this->m_pWorker->ChatData(this->m_nLocalChannel,uchType, recBuf, nLen);
					}
				}
				else if(uchType == JVN_RSP_TEXTDATA && this->m_bAcceptText)
				{
					/*回调函数文本*/
					if(this->m_pWorker != NULL)
					{
						this->m_pWorker->TextData(this->m_nLocalChannel,uchType, recBuf, nLen);
					}
				}
				else if(uchType == JVN_RSP_CHECKDATA)
				{
					/*回调函数检索结果*/
					if(this->m_pWorker != NULL)
					{
						this->m_pWorker->CheckResult(this->m_nLocalChannel,recBuf, nLen);
					}
				}
			}
			break;
		case JVN_RSP_CHECKOVER://检索完成(无检索内容)
		case JVN_RSP_DOWNLOADE://下载数据失败
		case JVN_RSP_DOWNLOADOVER://下载数据完成
		case JVN_RSP_PLAYE://下载数据失败
		case JVN_RSP_DLTIMEOUT://下载超时
		case JVN_RSP_PLTIMEOUT://回放超时
		case JVN_RSP_PLAYOVER://回放完成
		case JVN_CMD_DISCONN://服务端断开连接
			{
				if(nLen == 0)
				{
					this->m_bDAndP = FALSE;
					dBeginTimeDP = 0;
					if(uchType == JVN_RSP_DOWNLOADOVER || uchType == JVN_RSP_DOWNLOADE || uchType == JVN_RSP_DLTIMEOUT)
					{
						/*回调函数下载*/
						if(this->m_pWorker != NULL)
						{
							this->m_pWorker->DownLoad(this->m_nLocalChannel,uchType, NULL, 0, 0);
							this->SendData(JVN_CMD_VIDEO,NULL,0);
						}
					}
					else if(uchType == JVN_RSP_PLAYOVER || uchType == JVN_RSP_PLAYE || uchType == JVN_RSP_PLTIMEOUT)
					{
						/*回调函数回放*/
						if(this->m_pWorker != NULL)
						{
							this->m_pWorker->PlayData(this->m_nLocalChannel,uchType, NULL, 0, 0,0,0, 0,NULL,0);
							this->SendData(JVN_CMD_VIDEO,NULL,0);
						}
					}
					else if(uchType == JVN_RSP_CHECKOVER)
					{
						/*回调函数检索结果*/
						if(this->m_pWorker != NULL)
						{
							this->m_pWorker->CheckResult(this->m_nLocalChannel,NULL, 0);
						}
					}
					else if(uchType == JVN_CMD_DISCONN)
					{
//						this->m_bPass = FALSE;
						if(this->m_pWorker != NULL)
						{
							this->m_pWorker->ConnectChange(this->m_nLocalChannel,JVN_CCONNECTTYPE_SSTOP,NULL,0,__FILE__,__LINE__,__FUNCTION__);
						}
						
						//结束线程
						if(this->m_SServerSocket > 0)
						{
							UDT::close(this->m_SServerSocket);
						}
						this->m_SServerSocket = 0;
						return 0;
					}
				}
				else
				{//数据出错，应从下一个字节开始判断,丢弃下一个I帧之前的帧
					bError = TRUE;
				}
			}
			break;
		case JVN_RSP_DOWNLOADDATA://下载数据
			{
				int nFileLen = -1;
				memcpy(&nFileLen, recBuf, 4);
				//计时
				dBeginTimeDP = CCWorker::JVGetTime();
				//确认
				this->SendData(JVN_DATA_DANDP, NULL, 0);
				nDPRetryCount=0;
				/*回调函数下载*/
				if(this->m_pWorker != NULL)
				{
					this->m_pWorker->DownLoad(this->m_nLocalChannel,uchType, recBuf+4, nLen-4, nFileLen);
				}
			}
			break;
		case JVN_RSP_PLAYDATA://回放数据
			{
				//计时
				dBeginTimeDP = CCWorker::JVGetTime();
				//确认
				this->SendData(JVN_DATA_DANDP, NULL, 0);
				nDPRetryCount=0;
				if(recBuf[0] == JVN_DATA_S)
				{
					int nWidth = 0, nHeight = 0, nTotalFrames = 0;
					memcpy(&nWidth, &recBuf[5], 4);
					memcpy(&nHeight, &recBuf[9], 4);
					memcpy(&nTotalFrames, &recBuf[13], 4);
					/*回调函数回放*/
					if(this->m_pWorker != NULL)
					{
						this->m_pWorker->PlayData(this->m_nLocalChannel,recBuf[0], NULL, 0, nWidth,nHeight,nTotalFrames, 0,NULL,0);
					}
				}
				else if(recBuf[0] == JVN_DATA_O)
				{
					this->m_pBuffer->ClearBuffer();
					int nHeadLen = 0;
					memcpy(&nHeadLen, &recBuf[1], 4);
					/*回调函数回放*/
					if(this->m_pWorker != NULL)
					{
						this->m_pWorker->PlayData(this->m_nLocalChannel,recBuf[0], &recBuf[5], nHeadLen, 0,0,0, 0,NULL,0);
					}
				}
				else
				{
					int nFrameLen;
					memcpy(&nFrameLen, &recBuf[1], 4);
					/*回调函数回放*/
					if(this->m_pWorker != NULL)
					{
						this->m_pWorker->PlayData(this->m_nLocalChannel,recBuf[0], &recBuf[13], nFrameLen, 0,0,0, 0,NULL,0);
					}
				}
			}
			break;
		case JVN_DATA_SPEED:
			{		
				if(this->m_pWorker != NULL)
				{
					this->m_pWorker->NormalData(this->m_nLocalChannel,uchType, recBuf, 4, 0, 0);
				}
			}
			break;
		default:
			break;
		}	
	}
	
	//结束线程
	if(this->m_SServerSocket > 0)
	{
		UDT::close(this->m_SServerSocket);
	}
	this->m_SServerSocket = 0; 
	return 0;
}
