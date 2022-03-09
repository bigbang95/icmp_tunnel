/****************************************************
*
*		稳定 ICMP 通讯
*
*		服务端
*
*		上传/下载 文件
*
*		Release x64 VS2019
*
*****************************************************/


#include "..\icmp\icmpServerSocket.h"
#pragma comment(lib,"ws2_32.lib")


int main()
{
	// 52.182.141.64：客户端 公网 ip
	CicmpServerSocket icmp(L"52.182.141.64", L"192.168.1.100");

	int ret;

	std::string buf;

	ret = icmp.recv(buf, 4);
	if (ret != ERROR_SUCCESS)
	{
		MessageBoxA(0, "recv len error code", std::to_string(ret).c_str(), 0);
		return -1;
	}

	int buf_len = *(int*)buf.c_str();

	ret = icmp.recv(buf, buf_len);
	if (ret != ERROR_SUCCESS)
	{
		MessageBoxA(0, "recv data error code", std::to_string(ret).c_str(), 0);
		return -1;
	}

	FILE* pf;
	if (fopen_s(&pf, "11.exe", "wb") != 0)
	{
		char sz[1024];
		strerror_s(sz, errno);
		MessageBoxA(0, sz, 0, 0);
		return -1;
	}

	fwrite(buf.c_str(), 1, buf.length(), pf);
	fclose(pf);

	printf("server 接收完成\n");



	int start = GetTickCount();

	int nLen = buf.length();

	ret = icmp.send(std::string((char*)&nLen, 4));
	if (ret != ERROR_SUCCESS)
	{
		MessageBoxA(0, "send len error code", std::to_string(ret).c_str(), 0);
		return -1;
	}

	ret = icmp.send(buf);
	if (ret != ERROR_SUCCESS)
	{
		MessageBoxA(0, "send data error code", std::to_string(ret).c_str(), 0);
		return -1;
	}

	printf("server send 用时：%d 毫秒\n", GetTickCount() - start);

	printf("server 发送完成\n");

	system("pause");
	return 0;
}
