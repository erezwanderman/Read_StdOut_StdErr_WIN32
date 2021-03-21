#include <algorithm>
#include <iostream>
#include <list>
#include <sstream>
#include <vector>

#include <tchar.h>
#include <Windows.h>

#include "Shared.h"
#include "MyCreatePipeEx.h"

static std::pair<std::vector<char>, std::vector<char>> CreateProcessAndGetOutput();

int main()
{
	try
	{
		std::pair<std::vector<char>, std::vector<char>> outputVectors;
		outputVectors = CreateProcessAndGetOutput();
		std::cout.imbue(std::locale(""));
		std::cout << "stdout size: " << outputVectors.first.size() << std::endl;
		std::cout << "stderr size: " << outputVectors.second.size() << std::endl;
	}
	catch (const std::exception& e)
	{
		std::cout << "Sorry..." << std::endl << e.what() << std::endl;
		return 1;
	}
	return 0;
}

static std::pair<std::vector<char>, std::vector<char>> CreateProcessAndGetOutput()
{
	SECURITY_ATTRIBUTES saAttr;
	saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
	saAttr.bInheritHandle = TRUE;
	saAttr.lpSecurityDescriptor = nullptr;

	// Create a pipe for the child process's STDIN, used to write to the child
	HANDLE g_hChildStd_IN_Rd = nullptr;
	HANDLE g_hChildStd_IN_Wr = nullptr;
	CHECK_WIN32_ERROR(CreatePipe(&g_hChildStd_IN_Rd, &g_hChildStd_IN_Wr, &saAttr, 0));
	CHECK_WIN32_ERROR(SetHandleInformation(g_hChildStd_IN_Wr, HANDLE_FLAG_INHERIT, 0));

	// Create a pipe for the child process's STDOUT, used to read from the child
	HANDLE g_hChildStd_OUT_Rd = nullptr;
	HANDLE g_hChildStd_OUT_Wr = nullptr;
	CHECK_WIN32_ERROR(MyCreatePipeEx(&g_hChildStd_OUT_Rd, &g_hChildStd_OUT_Wr, &saAttr, 0, FILE_FLAG_OVERLAPPED, 0));
	CHECK_WIN32_ERROR(SetHandleInformation(g_hChildStd_OUT_Rd, HANDLE_FLAG_INHERIT, 0));

	// Create a pipe for the child process's STDERR, used to read from the child
	HANDLE g_hChildStd_ERR_Rd = nullptr;
	HANDLE g_hChildStd_ERR_Wr = nullptr;
	CHECK_WIN32_ERROR(MyCreatePipeEx(&g_hChildStd_ERR_Rd, &g_hChildStd_ERR_Wr, &saAttr, 0, FILE_FLAG_OVERLAPPED, 0));
	CHECK_WIN32_ERROR(SetHandleInformation(g_hChildStd_ERR_Rd, HANDLE_FLAG_INHERIT, 0));

	STARTUPINFO siStartupInfo;
	ZeroMemory(&siStartupInfo, sizeof(siStartupInfo));
	siStartupInfo.cb = sizeof(siStartupInfo);
	siStartupInfo.hStdInput = g_hChildStd_IN_Rd;
	siStartupInfo.hStdOutput = g_hChildStd_OUT_Wr;
	siStartupInfo.hStdError = g_hChildStd_ERR_Wr;
	siStartupInfo.dwFlags |= STARTF_USESTDHANDLES;

	TCHAR commandLine[] = _TEXT("ChildProcess.exe");
	PROCESS_INFORMATION pi;
	CHECK_WIN32_ERROR(CreateProcess(
		nullptr,        // No module name (use command line)
		commandLine,    // Command line
		nullptr,        // Process handle not inheritable
		NULL,           // Thread handle not inheritable
		TRUE,           // handles are inherited
		0,              // No creation flags
		NULL,           // Use parent's environment block
		NULL,           // Use parent's starting directory 
		&siStartupInfo, // Pointer to STARTUPINFO structure
		&pi             // Pointer to PROCESS_INFORMATION structure)
	));

	CHECK_WIN32_ERROR(CloseHandle(pi.hProcess));
	CHECK_WIN32_ERROR(CloseHandle(pi.hThread));

	CHECK_WIN32_ERROR(CloseHandle(g_hChildStd_IN_Rd));
	g_hChildStd_IN_Rd = nullptr;
	CHECK_WIN32_ERROR(CloseHandle(g_hChildStd_OUT_Wr));
	g_hChildStd_OUT_Wr = nullptr;
	CHECK_WIN32_ERROR(CloseHandle(g_hChildStd_ERR_Wr));
	g_hChildStd_ERR_Wr = nullptr;

	/*const char* inputBuffer = input.data();
	DWORD dwWritten = 0;
	CHECK_WIN32_ERROR(WriteFile(g_hChildStd_IN_Wr, inputBuffer, (DWORD)input.size(), &dwWritten, nullptr));*/
	// Nothing to write
	CHECK_WIN32_ERROR(CloseHandle(g_hChildStd_IN_Wr));

	std::vector<char> tempOut, tempErr;
	std::vector<std::vector<char>> stdOutErrVecs = ReadFromMultiplePipes({ g_hChildStd_OUT_Rd, g_hChildStd_ERR_Rd });

	CHECK_WIN32_ERROR(CloseHandle(g_hChildStd_OUT_Rd));
	CHECK_WIN32_ERROR(CloseHandle(g_hChildStd_ERR_Rd));

	return std::make_pair(stdOutErrVecs[0], stdOutErrVecs[1]);
}
