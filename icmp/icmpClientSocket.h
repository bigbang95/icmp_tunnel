#pragma once
#include "icmpSocket.h"

class CicmpClientSocket :public CicmpSocket
{
private:
	DWORD m_dwSeq; // 当前 发送/接收 序号

public:
	CicmpClientSocket(LPCWSTR wsServerIp);
	~CicmpClientSocket();


	// 发送 一次
	int sendto(SOCKET s, const char* buf, int len, const struct sockaddr* to, int tolen);

	// 返回 258、-1、0、LastError；
	int recvfrom(SOCKET s, char* buf, int* len, DWORD dwTimeout, struct sockaddr* from, int* fromlen);

	// 发送 一个 ICMP 数据报文
	int send1(SOCKET s, const char* buf, int len, bool bSyn = false);

	// 接收 一个 ICMP 数据报文
	int recv1(SOCKET s, char* buf, int* len, bool bSyn = false);

	// 理论上 任意长度 发送
	// 返回 成功(0)、失败
	int send(SOCKET s, std::string buf);
	int send(std::string buf);

	// 理论上 任意长度 接收
	// 返回 成功(0)、失败
	int recv(SOCKET s, std::string& buf, size_t buf_len);
	int recv(std::string& buf, size_t buf_len);
};
