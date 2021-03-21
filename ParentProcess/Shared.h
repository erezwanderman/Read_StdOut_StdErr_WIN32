#pragma once
#define CHECK_WIN32_ERROR(CALL) do \
{ \
	BOOL returnValue = CALL; \
	if (!returnValue) { \
		DWORD errorCode = GetLastError(); \
		LPSTR messageBuffer = nullptr; \
		size_t size = FormatMessageA( \
			FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, \
			NULL, \
			errorCode, \
			0, \
			(LPSTR)&messageBuffer, \
			0, \
			NULL \
		); \
		std::string message(messageBuffer, size); \
		LocalFree(messageBuffer); \
		std::stringstream sstm; \
		sstm << "Call " << #CALL << " at " << __FILE__ << ":" << __LINE__ << " failed.\nError " << errorCode << ": " << message << "\n"; \
		const std::string errorMessage = sstm.str(); \
		throw std::runtime_error(errorMessage.c_str()); \
	} \
} while (false)

// ReadFromMultiplePipes.cpp
std::vector<std::vector<char>> ReadFromMultiplePipes(const std::list<HANDLE>&handles);
