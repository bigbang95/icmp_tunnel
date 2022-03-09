#pragma once
/*****************************************
*
*		稳定的 ICMP 通讯
*
*		Author: bigbang95
*
******************************************/

#include<WinSock2.h>
#include<WS2tcpip.h>
#include<string>

#define ICMP_SYN	0b1
#define ICMP_ACK	0b10
#define ICMP_PSH	0b100
#define ICMP_POP	0b1000


#pragma pack(push)
#pragma pack(1)

struct ICMP_DATA_HEAD
{
	UINT seq;
	USHORT flags;
};

struct IPHeader
{
	// 因为是 小端 机器，所以 位域 倒着 排列 成员
	BYTE HdrLen : 4;  // IP 头长度，5 表示 20 字节
	BYTE Version : 4; // 版本

	BYTE ServiceType; // 服务类型
	WORD TotalLen;    // 总长
	WORD ID;          // 标识

	union {
		struct {
			// 因为是 小端 机器，所以 位域 倒着 排列 成员，再进行 WORD 类型的 字节小端转换
			WORD FragOff : 13; // 分段偏移
			WORD moreFragments : 1; // 表示分更多片位
			WORD noFragment : 1; // 该位在置 1 时表示不分片
			WORD Reserved : 1;   // reserve：保留		
		} sFlags;
		WORD wFlags;
	}uFlags; // 标志

	BYTE TTL;       // 生命周期，表示该 IP 数据包可以经过的路由器的最大数量
	BYTE Protocol;  // 协议
	WORD HdrChksum; // 头校验和
	DWORD SrcAddr;  // 源地址
	DWORD DstAddr;  // 目的地址
	//BYTE Options;  // 选项
};

struct ICMP_HEAD
{
	BYTE type;
	BYTE code;
	USHORT checksum;
	USHORT identifier;
	USHORT SeqNum;
};

#pragma pack(pop)


class CicmpSocket
{
protected:
	SOCKET m_sock;
	WSAEVENT m_event;
	sockaddr_in m_addr;
	int m_nMaxPacketSize;

public:
	CicmpSocket();
	~CicmpSocket();

	SOCKET GetSocket();
	void SetSocket(SOCKET sock);

	int startup();
	int cleanup();

	SOCKET socket(int af, int type, int protocol);

	USHORT CheckSum(const char* buf, size_t size);


	// 理论上 任意长度 发送
	virtual int send(SOCKET s, std::string buf) = 0;
	virtual int send(std::string buf) = 0;

	// 理论上 任意长度 接收
	virtual int recv(SOCKET s, std::string& buf, size_t buf_len) = 0;
	virtual int recv(std::string& buf, size_t buf_len) = 0;
};

DWORD GetRandomDword();
