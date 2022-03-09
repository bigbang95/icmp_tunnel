/****************************************************
*
*		稳定 ICMP 通讯
*
*		客户端
*
*		上传/下载 文件
*
*		Release x64 VS2019
*
*****************************************************/


#include "..\icmp\icmpClientSocket.h"
#pragma comment(lib,"ws2_32.lib")


int main()
{
	// 1.11.1.11：vps 公网 ip 
	CicmpClientSocket icmp(L"1.11.1.11");

	FILE* pf;
	if (fopen_s(&pf, "test.exe", "rb") != 0)
	{
		char sz[1024];
		strerror_s(sz, errno);
		MessageBoxA(0, sz, 0, 0);
		return -1;
	}

	fseek(pf, 0, SEEK_END);
	long nLen = ftell(pf);
	fseek(pf, 0, SEEK_SET);

	char* buf = new char[nLen];
	fread(buf, 1, nLen, pf);
	fclose(pf);
	std::string str(buf, nLen);
	delete[] buf;

	int start = GetTickCount();

	int ret;

	ret = icmp.send(std::string((char*)&nLen, 4));
	if (ret != ERROR_SUCCESS)
	{
		MessageBoxA(0, "send len error code", std::to_string(ret).c_str(), 0);
		return -1;
	}

	ret = icmp.send(str);
	if (ret != ERROR_SUCCESS)
	{
		MessageBoxA(0, "send data error code", std::to_string(ret).c_str(), 0);
		return -1;
	}

	printf("client send 用时：%d 毫秒\n", GetTickCount() - start);

	printf("client 发送完成\n");



	std::string strRecv;

	ret = icmp.recv(strRecv, 4);
	if (ret != ERROR_SUCCESS)
	{
		MessageBoxA(0, "recv len error code", std::to_string(ret).c_str(), 0);
		return -1;
	}

	int buf_len = *(int*)strRecv.c_str();

	ret = icmp.recv(strRecv, buf_len);
	if (ret != ERROR_SUCCESS)
	{
		MessageBoxA(0, "recv data error code", std::to_string(ret).c_str(), 0);
		return -1;
	}

	FILE* pf1;
	if (fopen_s(&pf1, "1.exe", "wb") != 0)
	{
		char sz[1024];
		strerror_s(sz, errno);
		MessageBoxA(0, sz, 0, 0);
		return -1;
	}

	fwrite(strRecv.c_str(), 1, strRecv.length(), pf1);
	fclose(pf1);

	printf("client 接收完成\n");

	system("pause");
	return 0;
}
