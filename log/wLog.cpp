#include<Windows.h>
#include<time.h>
#include<direct.h>
#include "wLog.h"

std::wstring GetCurrentStringTime(BOOL bIsTime = TRUE)
{
	// 获取 时间
	time_t t = time(0);
	wchar_t buff[80];
	tm tm;
	localtime_s(&tm, &t);
	wcsftime(buff, 80, bIsTime ? L"%Y-%m-%d %H:%M:%S" : L"%Y-%m-%d %H-%M-%S", &tm);

	return buff;
}

std::wstring GetLogFileName()
{
	_wmkdir(L"logs");
	return L"logs\\record_error_log.txt";
}

void WriteLog(std::wstring info, std::wstring currentFileName, int nLineNumber)
{
	FILE* fp = NULL;
	if (_wfopen_s(&fp, GetLogFileName().c_str(), L"a, ccs=utf-8") != 0)
	{
		// 失败
		wchar_t szErr[1024];
		_wcserror_s(szErr, errno);
		MessageBoxW(0, szErr, L"日志写入失败", 0);
		return;
	}

	std::wstring wstr = L"<" + GetCurrentStringTime() + L"> <文件 " + currentFileName + L" 第 " + std::to_wstring(nLineNumber) + L" 行> : " + info + L"\n";

	fwrite(wstr.c_str(), sizeof(wchar_t), wstr.length(), fp);
	fclose(fp);
}
