#include "Logger.h"
#include <Windows.h>
#include <cstdarg>
#include <cstdio>

#ifdef _DEBUG
void Log(const char* szFormat, ...)
{
	char szMessage[1024];

	va_list args;
	va_start(args, szFormat);
	vsprintf_s(szMessage, sizeof(szMessage), szFormat, args);

	OutputDebugStringA(szMessage);

	va_end(args);
}
#else
void Log(const char* szFormat, ...)
{
	// No logging in release mode
}
#endif