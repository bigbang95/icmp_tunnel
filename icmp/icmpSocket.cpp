#include "icmpSocket.h"
#include "..\log\wLog.h"

CicmpSocket::CicmpSocket()
{
	m_sock = INVALID_SOCKET;
	m_event = WSA_INVALID_EVENT;
	m_nMaxPacketSize = 2048;

	srand(GetTickCount());

	startup();
}

CicmpSocket::~CicmpSocket()
{
	cleanup();
}

SOCKET CicmpSocket::GetSocket()
{
	return m_sock;
}

void CicmpSocket::SetSocket(SOCKET sock)
{
	m_sock = sock;
}

int CicmpSocket::startup()
{
	WSADATA wsadata;

	int err;

	if (err = WSAStartup(MAKEWORD(2, 2), &wsadata))
	{
		return err;
	}

	if (wsadata.wVersion != MAKEWORD(2, 2))
	{
		return -1000;
	}

	return ERROR_SUCCESS;
}

int CicmpSocket::cleanup()
{
	return WSACleanup();
}

SOCKET CicmpSocket::socket(int af, int type, int protocol)
{
	return ::socket(af, type, protocol);
}

USHORT CicmpSocket::CheckSum(const char* buf, size_t size)
{
	size_t i;
	ULONGLONG sum = 0;

	for (i = 0; i < size; i += 2) {
		sum += *(USHORT*)buf;
		buf += 2;
	}

	if (size - i > 0) {
		sum += *(BYTE*)buf;
	}

	while ((sum >> 16) != 0) {
		sum = (sum & 0xffff) + (sum >> 16);
	}

	return (USHORT)~sum;
}

DWORD GetRandomDword()
{
	//srand(GetTickCount());

	union
	{
		UINT nd;

		struct
		{
			USHORT ns1;
			USHORT ns2;
		} ns;

	} random_num;

	random_num.ns.ns1 = rand() + rand();
	random_num.ns.ns2 = rand() + rand();

	return random_num.nd;
}
