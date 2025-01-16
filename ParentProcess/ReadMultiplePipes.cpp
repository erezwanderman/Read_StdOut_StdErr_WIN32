#include <algorithm>
#include <iostream>
#include <list>
#include <sstream>
#include <vector>

#include <tchar.h>
#include <Windows.h>

#include "Shared.h"

struct ReadPipeInfo
{
	ReadPipeInfo(HANDLE handle) : readHandle(handle)
	{
		ZeroMemory(&overlapped, sizeof(OVERLAPPED));
		if (!(overlapped.hEvent = CreateEvent(nullptr, false, true, nullptr)))
			throw std::runtime_error("Call CreateEvent failed.");
	}
	~ReadPipeInfo()
	{
		CloseHandle(overlapped.hEvent);
	}
	HANDLE readHandle = nullptr;
	std::vector<char> outputBuffer;
	OVERLAPPED overlapped = {};
	UCHAR buf[1024 * 4] = {};
	// 0   - open
	// 997 - ERROR_IO_PENDING  - Overlapped I/O operation is in progress.
	// 109 - ERROR_BROKEN_PIPE - The pipe has been ended.
	DWORD state = 0;
};

static DWORD ReadAndInsertOverlappedResult(ReadPipeInfo& readPipe);

std::vector<std::vector<char>> ReadFromMultiplePipes(const std::list<HANDLE>& handles)
{
	std::vector<std::shared_ptr<ReadPipeInfo>> readPipes;
	std::transform(handles.begin(), handles.end(), std::back_inserter(readPipes), [](HANDLE x) { return std::make_shared<ReadPipeInfo>(x); });

	while (std::any_of(readPipes.begin(), readPipes.end(), [](auto& i) { return i->state != ERROR_BROKEN_PIPE; }) )
	{
		// Read until all pipes are pending or broken
		std::vector<std::shared_ptr<ReadPipeInfo>>::iterator it;
		while ((it = std::find_if(readPipes.begin(), readPipes.end(), [](auto& i) { return i->state == 0; })) != readPipes.end())
		{
			auto& currOpenPipe = **it;
			BOOL rfResult = ::ReadFile(currOpenPipe.readHandle, currOpenPipe.buf, sizeof(currOpenPipe.buf), nullptr, &currOpenPipe.overlapped);
			if (rfResult)
				CHECK_WIN32_ERROR(ReadAndInsertOverlappedResult(currOpenPipe));
			else
			{
				DWORD lastError = ::GetLastError();
				if (lastError != ERROR_IO_PENDING && lastError != ERROR_BROKEN_PIPE) // 997 109
					throw std::runtime_error("ReadFile failed");
				else
				{
					currOpenPipe.state = lastError;
					::SetLastError(0);
				}
			}
		}

		// Wait for pending IO
		std::vector<std::shared_ptr<ReadPipeInfo>> pendingReadPipes;
		std::copy_if(readPipes.begin(), readPipes.end(), std::back_inserter(pendingReadPipes), [](auto& x) { return x->state == ERROR_IO_PENDING; });
		if (pendingReadPipes.size() > 0)
		{
			std::vector<HANDLE> eventsToWait;
			std::transform(pendingReadPipes.begin(), pendingReadPipes.end(), std::back_inserter(eventsToWait), [](auto& x) { return x->overlapped.hEvent; });
			DWORD waitResult = ::WaitForMultipleObjects((DWORD)eventsToWait.size(), eventsToWait.data(), FALSE, INFINITE);
			if (waitResult >= WAIT_OBJECT_0 && waitResult < WAIT_OBJECT_0 + pendingReadPipes.size())
			{
				auto& currPendingPipe = *pendingReadPipes[waitResult];
				CHECK_WIN32_ERROR(ReadAndInsertOverlappedResult(currPendingPipe));
			}
			else
				throw std::runtime_error("WaitForMultipleObjects failed");
		}
	}

	std::vector<std::vector<char>> ret;
	std::transform(readPipes.begin(), readPipes.end(), std::back_inserter(ret), [](auto& x) { return x->outputBuffer; });
	return ret;
}

static DWORD ReadAndInsertOverlappedResult(ReadPipeInfo& readPipe)
{
	DWORD lpNumberOfBytesTransferred = 0;
	BOOL gor = ::GetOverlappedResult(readPipe.readHandle, &readPipe.overlapped, &lpNumberOfBytesTransferred, false);
	if (gor)
	{
		readPipe.state = 0;
		readPipe.outputBuffer.insert(readPipe.outputBuffer.end(), readPipe.buf, readPipe.buf + lpNumberOfBytesTransferred);
		return TRUE;
	}
	else
	{
		if (::GetLastError() != ERROR_BROKEN_PIPE) // 109
			return FALSE;
		readPipe.state = ERROR_BROKEN_PIPE;
		::SetLastError(0);
		return TRUE;
	}
}
