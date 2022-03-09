#include "icmpServerSocket.h"
#include "..\log\wLog.h"
#include <mstcpip.h>

CicmpServerSocket::CicmpServerSocket(LPCWSTR wsClientIp, LPCWSTR wsBindIp)
{
	m_sock = socket(AF_INET, SOCK_RAW, IPPROTO_IP);
	if (m_sock == INVALID_SOCKET)
	{
		WriteLog(L"socket error code: " + std::to_wstring(WSAGetLastError()), __FILEW__, __LINE__);
		return;
	}

	// sendto 需要 自己 填充 IP 头
	int optval = TRUE;
	int ret = setsockopt(m_sock, IPPROTO_IP, IP_HDRINCL, (char*)&optval, sizeof(optval));
	if (ret == SOCKET_ERROR)
	{
		WriteLog(L"setsockopt set IP_HDRINCL error code: " + std::to_wstring(WSAGetLastError()), __FILEW__, __LINE__);
		return;
	}

	m_event = WSACreateEvent(); // 创建 一个 初始状态 为 失信的 事件对象

	if (m_event == WSA_INVALID_EVENT)
	{
		WriteLog(L"WSACreateEvent error code: " + std::to_wstring(WSAGetLastError()), __FILEW__, __LINE__);
		return;
	}

	if (SOCKET_ERROR == WSAEventSelect(m_sock, m_event, FD_READ))
	{
		WriteLog(L"WSAEventSelect error code: " + std::to_wstring(WSAGetLastError()), __FILEW__, __LINE__);
		return;
	}

	// wsBindIp 不能是 0.0.0.0，必须是 具体的 IP 地址，如：192.168.1.100
	in_addr in_addr1;
	ret = InetPtonW(AF_INET, wsBindIp, &in_addr1);
	if (ret != 1)
	{
		// 失败
		WriteLog(L"bind ip 地址无效", __FILEW__, __LINE__);
		return;
	}

	m_addr.sin_addr = in_addr1;
	m_addr.sin_family = AF_INET;
	m_addr.sin_port = htons(0);

	if (SOCKET_ERROR == bind(m_sock, (sockaddr*)&m_addr, sizeof(sockaddr)))
	{
		WriteLog(L"bind error code: " + std::to_wstring(WSAGetLastError()), __FILEW__, __LINE__);
		return;
	}


	// 必须在 bind 之后，执行
	// 开启 混杂模式
	ULONG optval1 = RCVALL_ON, cbret, opt1;
	ret = WSAIoctl(m_sock, SIO_RCVALL, &optval1, sizeof(optval1), &opt1, sizeof(opt1), &cbret, NULL, NULL);
	if (ret == SOCKET_ERROR)
	{
		WriteLog(L"WSAIoctl error code: " + std::to_wstring(WSAGetLastError()), __FILEW__, __LINE__);
		return;
	}

	printf("ICMP server start success.\n");

	if (1 != InetPtonW(AF_INET, wsClientIp, &m_addrClient))
	{
		// 失败
		WriteLog(L"客户端 公网 ip 地址无效", __FILEW__, __LINE__);
		return;
	}


	m_dwSeq = 0;
}


CicmpServerSocket::~CicmpServerSocket()
{
	if (m_sock != INVALID_SOCKET)
		closesocket(m_sock);

	if (m_event != WSA_INVALID_EVENT)
		WSACloseEvent(m_event);
}

int CicmpServerSocket::bind(SOCKET s, const struct sockaddr* name, int namelen)
{
	return ::bind(s, name, namelen);
}


///////////////////////////////////////////////////////////////////


// 返回 0、5、ret
int CicmpServerSocket::recv(SOCKET s, std::string& buf, size_t buf_len)
{
	buf = "";

	char sz1[2048];

	ICMP_DATA_HEAD h1, h2;

	const DWORD dwTimeout = INFINITE;

	int j = 5; // 执行 5 次 循环
	while (j--)
	{
		int len1, addr_len = sizeof(sockaddr);
		int ret = recvfrom(s, sz1, &len1, dwTimeout, (sockaddr*)&m_addr, &addr_len);
		if (ret == WSA_WAIT_TIMEOUT)
		{
			// 不可能事件
		}
		else if (ret != ERROR_SUCCESS)
		{
			return ret;
		}
		else
		{
			char* sz11 = sz1 + sizeof(IPHeader) + sizeof(ICMP_HEAD);
			int len11 = len1 - sizeof(IPHeader) - sizeof(ICMP_HEAD);

			if (len11 == sizeof(h1))
			{
				h1 = *(ICMP_DATA_HEAD*)sz11;

				if (h1.flags == ICMP_SYN)
				{
					m_dwSeq = h1.seq;

					// 重置 while 参数
					j = 5;
				}
			}
			else if (len11 > sizeof(h1))
			{
				h1 = *(ICMP_DATA_HEAD*)sz11;

				if ((h1.seq == m_dwSeq + 1)
					&& (h1.flags == ICMP_PSH))
				{
					// 一次 recv 成功
					buf += std::string(sz11 + sizeof(ICMP_DATA_HEAD), len11 - sizeof(ICMP_DATA_HEAD));

					m_dwSeq++;

					// 重置 while 参数
					j = 5;
				}
			}
			else
			{
				continue;
			}
		}


		h2.seq = m_dwSeq + 1;
		h2.flags = ICMP_ACK;

		if (sendto(s, sz1, (char*)&h2, sizeof(h2), (sockaddr*)&m_addr, sizeof(sockaddr)) == SOCKET_ERROR)
		{
			return WSAGetLastError();
		}

		if (buf.length() == buf_len)
		{
			break;
		}
	}

	if (j == -1)
	{
		// 5 次 数据包 全 接收失败
		return 5;
	}

	return ERROR_SUCCESS;
}


///////////////////////////////////////////////////////////////////////////////////



// 返回 0、5、ret
int CicmpServerSocket::send(SOCKET s, std::string buf)
{
	char sz1[2048], sz2[2048];

	ICMP_DATA_HEAD h1, h2;

	const DWORD dwTimeout = INFINITE;

	const int one_len = 1400 - sizeof(ICMP_DATA_HEAD);

	bool b = false;

	int j = 5; // 执行 5 次 循环
	while (j--)
	{
		int len1, addr_len = sizeof(sockaddr);
		int ret = recvfrom(s, sz1, &len1, dwTimeout, (sockaddr*)&m_addr, &addr_len);
		if (ret == WSA_WAIT_TIMEOUT)
		{
			// 不可能事件
		}
		else if (ret != ERROR_SUCCESS)
		{
			return ret;
		}
		else
		{
			char* sz11 = sz1 + sizeof(IPHeader) + sizeof(ICMP_HEAD);
			int len11 = len1 - sizeof(IPHeader) - sizeof(ICMP_HEAD);

			if (len11 == sizeof(h1))
			{
				h1 = *(ICMP_DATA_HEAD*)sz11;

				if (h1.flags == ICMP_SYN)
				{
					m_dwSeq = h1.seq;

					b = false;

					// 重置 while 参数
					j = 5;
				}
				else if (h1.flags == ICMP_POP)
				{
					if (h1.seq == m_dwSeq + 1)
					{
						// 一次 send 成功

						if (b == false)
						{
							b = true;
						}
						else
						{
							buf = buf.substr(one_len);
						}

						m_dwSeq++;

						// 重置 while 参考
						j = 5;
					}
					else
					{

					}
				}
				else
				{
					continue;
				}
			}
			else
			{
				continue;
			}
		}

		std::string str = buf.substr(0, one_len);

		h2.seq = m_dwSeq + 1;
		h2.flags = ICMP_ACK;

		memcpy(sz2, &h2, sizeof(h2));
		memcpy(sz2 + sizeof(h2), str.c_str(), str.length());

		if (h1.flags == ICMP_SYN)
		{
			if (sendto(s, sz1, (char*)&h2, sizeof(h2), (sockaddr*)&m_addr, sizeof(sockaddr)) == SOCKET_ERROR)
			{
				return WSAGetLastError();
			}
		}
		else if (h1.flags == ICMP_POP)
		{
			if (sendto(s, sz1, sz2, sizeof(h2) + str.length(), (sockaddr*)&m_addr, sizeof(sockaddr)) == SOCKET_ERROR)
			{
				return WSAGetLastError();
			}

			if (str.length() < one_len)
			{
				break;
			}

			if (str.length() == buf.length())
			{
				break;
			}
		}
		else
		{
			WriteLog(L"ICMP 标志 既不是 SYN，也不是 POP", __FILEW__, __LINE__);
			exit(0);
		}
	}

	if (j == -1)
	{
		// 5 次 数据包 全 接收失败
		return 5;
	}

	return ERROR_SUCCESS;
}


/////////////////////////////////////////////////////////////////////////////////////


int CicmpServerSocket::send(std::string buf)
{
	return send(m_sock, buf);
}

int CicmpServerSocket::recv(std::string& buf, size_t buf_len)
{
	return recv(m_sock, buf, buf_len);
}


/////////////////////////////////////////////////////////////////////////////////////


// 返回 258、-1、0、LastError；
// buf 为 IP Head + icmp Head + icmp data
// ::recvfrom：接收 整个 IP 包
int CicmpServerSocket::recvfrom(SOCKET s, char* buf, int* len, DWORD dwTimeout, struct sockaddr* from, int* fromlen)
{
	while (1)
	{
		// 监视事件对象状态
		DWORD dwRet = WSAWaitForMultipleEvents(1, &m_event, false, dwTimeout, false);
		if (dwRet == WSA_WAIT_TIMEOUT)
		{
			return WSA_WAIT_TIMEOUT;
		}
		else if (dwRet == WSA_WAIT_FAILED)
		{
			return WSA_WAIT_FAILED;
		}
		else
		{
			// 成功 获取 recv 信号
			WSANETWORKEVENTS netEvt;
			WSAEnumNetworkEvents(m_sock, m_event, &netEvt); // 为 m_sock 查看已发生的网络事件
			if (netEvt.lNetworkEvents & FD_READ)
			{
				// 事件 含 FD_READ

				// 接收 ICMP 报文
				*len = ::recvfrom(s, buf, m_nMaxPacketSize, 0, from, fromlen);
				if (*len == SOCKET_ERROR)
				{
					if (WSAGetLastError() == WSAEMSGSIZE)
					{
						// 数据包太大，确定不是需要的 ICMP 数据包
						WriteLog(L"recv length too large\n", __FILEW__, __LINE__);
						continue;
					}
					else
					{
						return WSAGetLastError();
					}
				}
				else if (*len < sizeof(IPHeader) + sizeof(ICMP_HEAD))
				{
					WriteLog(L"recv ip 报文长度 小于 28，为 " + std::to_wstring(*len), __FILEW__, __LINE__);
					continue;
				}
				else if (*len == sizeof(IPHeader) + sizeof(ICMP_HEAD))
				{
					int r = WSAGetLastError();
					if (r != ERROR_SUCCESS)
					{
						// 发现错误
						WriteLog(L"recvfrom 返回 28 字节，没有 ICMP data 部分，错误代码：" + std::to_wstring(r), __FILEW__, __LINE__);
						return r;
					}
					else
					{
						// 再次接收
						// 条件：对端 空 icmp data 报文 不是 有效 报文，即 对端 禁止发送 空 icmp data 报文
						//wchar_t ws[20];
						//InetNtopW(AF_INET, &(((sockaddr_in*)from)->sin_addr), ws, 20);
						//WriteLog(L"recvfrom 接收 空 icmp data 报文 IP 地址为：" + std::wstring(ws), __FILEW__, __LINE__);
						continue;
					}
				}
				else
				{
					// ip 报文长度 大于 28

					if (((IPHeader*)buf)->Protocol == IPPROTO_ICMP)
					{
						if (((sockaddr_in*)from)->sin_addr.S_un.S_addr == m_addrClient.S_un.S_addr)
						{
							// 接收成功
							return ERROR_SUCCESS;
						}

						//wchar_t ws[20];
						//InetNtopW(AF_INET, &(((sockaddr_in*)from)->sin_addr), ws, 20);
						//WriteLog(L"recvfrom icmp 接收 IP 地址为：" + std::wstring(ws), __FILEW__, __LINE__);
					}

					continue;
				}
			}
			else
			{
				WriteLog(L"网络事件不含 recv 事件", __FILEW__, __LINE__);
				exit(-1);
			}
		}
	}

	return -1000; // 不可能事件
}



int CicmpServerSocket::sendto(SOCKET s, const char* request, const char* buf, int len, const struct sockaddr* to, int tolen)
{
	char szReponse[2048] = {};

	IPHeader ipHeaderRequest = *(IPHeader*)request;

	IPHeader ipHeaderResponse;
	ipHeaderResponse.Version = 4;
	ipHeaderResponse.HdrLen = 5;
	ipHeaderResponse.ServiceType = 0;
	ipHeaderResponse.TotalLen = 0; // 任意，自动填充
	ipHeaderResponse.ID = 0; // 如果为 0，则自动填充
	ipHeaderResponse.uFlags.wFlags = htons(0);
	ipHeaderResponse.TTL = 128;
	ipHeaderResponse.Protocol = IPPROTO_ICMP;
	ipHeaderResponse.HdrChksum = 0; // 任意，自动填充
	ipHeaderResponse.SrcAddr = ipHeaderRequest.DstAddr;
	ipHeaderResponse.DstAddr = ipHeaderRequest.SrcAddr;

	ICMP_HEAD icmpHeader = *(ICMP_HEAD*)(request + sizeof(IPHeader));
	icmpHeader.type = 0;
	icmpHeader.checksum = 0;

	memcpy_s(szReponse, sizeof(IPHeader), &ipHeaderResponse, sizeof(IPHeader));
	memcpy_s(szReponse + sizeof(IPHeader), sizeof(ICMP_HEAD), &icmpHeader, sizeof(ICMP_HEAD));
	memcpy_s(szReponse + sizeof(IPHeader) + sizeof(ICMP_HEAD), len, buf, len);

	icmpHeader.checksum = CheckSum(szReponse + sizeof(IPHeader), sizeof(ICMP_HEAD) + len);

	memcpy_s(szReponse + sizeof(IPHeader), sizeof(ICMP_HEAD), &icmpHeader, sizeof(ICMP_HEAD));

	int nIPPacketSize = sizeof(IPHeader) + sizeof(ICMP_HEAD) + len;

	// sendto 整个 ip 数据包
	return ::sendto(s, szReponse, nIPPacketSize, 0, to, tolen);
}
