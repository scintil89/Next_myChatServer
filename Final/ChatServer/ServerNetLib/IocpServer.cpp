#include "IocpServer.h"
//#include <thread>

namespace NServerNetLib
{
	CIocpServer::CIocpServer() 	
	{
		m_CompletionPort = NULL;
	}

	CIocpServer::~CIocpServer()
	{
	
	}

	bool CIocpServer::InitServer(ILog* pLogger)
	{
		WSADATA wsaData;
		HANDLE hComPort;
		SYSTEM_INFO systemInfo;

		//set logger
		m_pRefLogger = pLogger;

		if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
		{
			//error
			return false;
		}

		if (START_IOCP() == false)
		{
			//error
			return false;
		}

		GetSystemInfo(&systemInfo);

		//make thread
		for (int i = 0; i < systemInfo.dwNumberOfProcessors; i++)
		{
			_beginthreadex(NULL, 0, CompletionThread, (LPVOID)hComPort, 0, NULL);

			m_pRefLogger->Write(LOG_TYPE::L_INFO, "%s | begin thread", __FUNCTION__);
		}


		return true;
	}

	void CIocpServer::Release()
	{
		WSACleanup();

		CloseHandle(m_CompletionPort);
		m_CompletionPort = INVALID_HANDLE_VALUE;
	}

	NServerNetLib::RecvPacketInfo CIocpServer::GetPacketInfo()
	{

	}

	const BOOL CIocpServer::START_IOCP()
	{
		m_CompletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);

		if (m_CompletionPort == NULL)
			return false;

		return true;
	}

	const HANDLE CIocpServer::GetCompletionPort()
	{
		return m_CompletionPort;
	}

	UINT __stdcall WINAPI CIocpServer::CompletionThread(LPVOID pComPort)
	{
		HANDLE hCompletionPort = (HANDLE)pComPort;
		DWORD BytesTransferred;
		LPPER_HANDLE_DATA PerHandleData;
		LPPER_IO_DATA PerIoData;
		DWORD flags;

		while (true) 
		{
			GetQueuedCompletionStatus(hCompletionPort, &BytesTransferred,
				(PULONG_PTR)&PerHandleData, (LPOVERLAPPED*)&PerIoData, INFINITE);

			if (BytesTransferred == 0)
			{
				closesocket(PerHandleData->hClntSock);
				free(PerHandleData);
				free(PerIoData);
				continue;
			}

			PerIoData->wsaBuf.buf[BytesTransferred] = '\0';
			//m_pRefLogger->Write(LOG_TYPE::L_INFO, "Recv\n");

			PerIoData->wsaBuf.len = BytesTransferred;
			WSASend(PerHandleData->hClntSock, &(PerIoData->wsaBuf), 1, NULL, 0, NULL, NULL);

			memset(&(PerIoData->overlapped), 0, sizeof(OVERLAPPED));
			PerIoData->wsaBuf.len = BUFSIZE;
			PerIoData->wsaBuf.buf = PerIoData->buffer;

			flags = 0;

			WSARecv(PerHandleData->hClntSock, &(PerIoData->wsaBuf), 1, NULL, &flags, &(PerIoData->overlapped), NULL);
		}

		return 0;
	}

}