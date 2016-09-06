#pragma once

#include <process.h>
#include <WinSock2.h>
#include <Windows.h>

#include "Define.h"
#include "ILog.h"
#include "ServerNetErrorCode.h"

#define BUFSIZE 1024

namespace NServerNetLib
{
	typedef struct
	{
		SOCKET hClntSock;
		SOCKADDR_IN clntAddr;
	} PER_HANDLE_DATA, *LPPER_HANDLE_DATA;

	typedef struct
	{
		OVERLAPPED overlapped;
		char buffer[BUFSIZE];
		WSABUF wsaBuf;
	} PER_IO_DATA, *LPPER_IO_DATA;

	class CIocpServer
	{
	public:
		CIocpServer();
		~CIocpServer();

		bool InitServer(ILog* pLogger);
		void Release();
		RecvPacketInfo GetPacketInfo();
		static UINT __stdcall WINAPI CompletionThread(LPVOID pComPort);

	private:
		HANDLE	m_CompletionPort;
		ILog* m_pRefLogger;

	public:
		const BOOL		START_IOCP();
		//const BOOL		REGISTER_CLIENT(SOCKET ClientSocket, COMMONLIB::LIB_SESSIONDATA* Player);	// 템플릿 가능할까?
		const HANDLE	GetCompletionPort();


	};

}