#include <stdio.h>
#include <vector>
#include <deque>

#include "ILog.h"
#include "TcpNetwork.h"


namespace NServerNetLib
{
	TcpNetwork::TcpNetwork() {}
	
	TcpNetwork::~TcpNetwork() 
	{
		for (auto& client : m_ClientSessionPool)
		{
			if (client.pRecvBuffer) {
				delete[] client.pRecvBuffer;
			}
			
			if (client.pSendBuffer) {
				delete[] client.pSendBuffer;
			}
		}
	}

	NET_ERROR_CODE TcpNetwork::Init(const ServerConfig* pConfig, ILog* pLogger)
	{
		// 서버 셋팅 불러오기
		memcpy(&m_Config, pConfig, sizeof(ServerConfig));

		// 로거 셋팅
		m_pRefLogger = pLogger;

		// 서버소켓 생성 후 초기화
		auto initRet = InitServerSocket();
		if (initRet != NET_ERROR_CODE::NONE)
		{
			return initRet;
		}
		
		// bind하고 listen한다.
		auto bindListenRet = BindListen(pConfig->Port, pConfig->BackLogCount);
		if (bindListenRet != NET_ERROR_CODE::NONE)
		{
			return bindListenRet;
		}
		
		FD_ZERO(&m_Readfds);
		FD_SET(m_ServerSockfd, &m_Readfds);
		
		CreateSessionPool(pConfig->MaxClientCount + pConfig->ExtraClientCount);
				
		return NET_ERROR_CODE::NONE;
	}

	RecvPacketInfo TcpNetwork::GetPacketInfo()
	{
		RecvPacketInfo packetInfo;

		if (m_PacketQueue.empty() == false)
		{
			packetInfo = m_PacketQueue.front();
			m_PacketQueue.pop_front();
		}
				
		return packetInfo;
	}
	
	// 메인 루프
	void TcpNetwork::Run()
	{
		/*
			readFD는 수신할 데이터가 있는지 알아보고 싶은 소켓들
			writeFD는 블로킹되지 않고 바로 데이터를 보낼 수 있는지 알아보고 싶은 소켓들
			exceptionFD는 예외가 발생했는지 알아보고 싶은 소켓들
		*/
		auto read_set = m_Readfds;
		auto write_set = m_Readfds;
		auto exc_set = m_Readfds;

		// 함수 호출 후 무한 대기상태에 빠지지 않도록 timeout을 설정해둔다
		timeval timeout{ 0, 1000 }; //tv_sec, tv_usec

		// 읽을게 있나, 보낼 수 있는게 있나 1ms동안 알아봐라~
		auto selectResult = select(0, &read_set, &write_set, &exc_set, &timeout);

		// 읽을 게 있거나 보낼 수 있는게 있는지 확인
		auto isFDSetChanged = RunCheckSelectResult(selectResult);
		if (isFDSetChanged == false)
		{
			return;
		}

		// 읽을 게 있다는 의미는 누군가 접속했다는 의미이므로 NewSession한다.
		// 왜냐하면 m_serverSockfd는 listen하고 있는 포트 하나만 담고있기 때문
		if (FD_ISSET(m_ServerSockfd, &read_set))
			NewSession();
		else // clients
			RunCheckSelectClients(exc_set, read_set, write_set);
	}

	bool TcpNetwork::RunCheckSelectResult(const int result)
	{
		if (result == 0)
		{
			return false;
		}
		else if (result == -1)
		{
			// TODO:로그 남기기
			return false;
		}

		return true;
	}
	
	void TcpNetwork::RunCheckSelectClients(fd_set& exc_set, fd_set& read_set, fd_set& write_set)
	{
		for (int i = 0; i < m_ClientSessionPool.size(); ++i)
		{
			auto& session = m_ClientSessionPool[i];

			if (session.IsConnected() == false) {
				continue;
			}

			SOCKET fd = session.SocketFD;
			auto sessionIndex = session.Index;

			// check error
			if (FD_ISSET(fd, &exc_set))
			{
				CloseSession(SOCKET_CLOSE_CASE::SELECT_ERROR, fd, sessionIndex);
				continue;
			}

			// check read
			auto retReceive = RunProcessReceive(sessionIndex, fd, read_set);
			if (retReceive == false) {
				continue;
			}

			// check write
			RunProcessWrite(sessionIndex, fd, write_set);
		}
	}

	bool TcpNetwork::RunProcessReceive(const int sessionIndex, const SOCKET fd, fd_set& read_set)
	{
		if (!FD_ISSET(fd, &read_set))
		{
			return true;
		}

		auto ret = RecvSocket(sessionIndex);
		if (ret != NET_ERROR_CODE::NONE)
		{
			CloseSession(SOCKET_CLOSE_CASE::SOCKET_RECV_ERROR, fd, sessionIndex);
			return false;
		}

		ret = RecvBufferProcess(sessionIndex);
		if (ret != NET_ERROR_CODE::NONE)
		{
			CloseSession(SOCKET_CLOSE_CASE::SOCKET_RECV_BUFFER_PROCESS_ERROR, fd, sessionIndex);
			return false;
		}

		return true;
	}

	NET_ERROR_CODE TcpNetwork::SendData(const int sessionIndex, const short packetId, const short size, const char* pMsg)
	{
		auto& session = m_ClientSessionPool[sessionIndex];

		auto pos = session.SendSize;

		if ((pos + size + PACKET_HEADER_SIZE) > m_Config.MaxClientSendBufferSize ) {
			return NET_ERROR_CODE::CLIENT_SEND_BUFFER_FULL;
		}
				
		PacketHeader pktHeader{ packetId, size };
		memcpy(&session.pSendBuffer[pos], (char*)&pktHeader, PACKET_HEADER_SIZE);
		memcpy(&session.pSendBuffer[pos + PACKET_HEADER_SIZE], pMsg, size);
		session.SendSize += (size + PACKET_HEADER_SIZE);

		return NET_ERROR_CODE::NONE;
	}

	// 세션 풀을 미리 다 만들어두고 재활용할 것이다.
	void TcpNetwork::CreateSessionPool(const int maxClientCount)
	{
		for (int i = 0; i < maxClientCount; ++i)
		{
			ClientSession session;
			ZeroMemory(&session, sizeof(session));
			session.Index = i;
			session.pRecvBuffer = new char[m_Config.MaxClientRecvBufferSize];
			session.pSendBuffer = new char[m_Config.MaxClientSendBufferSize];
			
			m_ClientSessionPool.push_back(session);
			m_ClientSessionPoolIndex.push_back(session.Index);			
		}
	}

	int TcpNetwork::AllocClientSessionIndex()
	{
		if (m_ClientSessionPoolIndex.empty()) {
			return -1;
		}

		int index = m_ClientSessionPoolIndex.front();
		m_ClientSessionPoolIndex.pop_front();
		return index;
	}

	void TcpNetwork::ReleaseSessionIndex(const int index)
	{
		m_ClientSessionPoolIndex.push_back(index);
		m_ClientSessionPool[index].Clear();
	}

	// 서버 소켓을 생성하고 초기화한다
	NET_ERROR_CODE TcpNetwork::InitServerSocket()
	{
		// winsock 버전 셋팅
		WORD wVersionRequested = MAKEWORD(2, 2);
		WSADATA wsaData;
		WSAStartup(wVersionRequested, &wsaData);

		// 소켓을 생성해서 소켓 접근자를 얻어온다.
		m_ServerSockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (m_ServerSockfd < 0)
		{
			return NET_ERROR_CODE::SERVER_SOCKET_CREATE_FAIL;
		}

		// 소켓 옵션 설정
		auto n = 1;
		if (setsockopt(m_ServerSockfd, SOL_SOCKET, SO_REUSEADDR, (char*)&n, sizeof(n)) < 0)
		{
			return NET_ERROR_CODE::SERVER_SOCKET_SO_REUSEADDR_FAIL;
		}

		return NET_ERROR_CODE::NONE;
	}

	NET_ERROR_CODE TcpNetwork::BindListen(short port, int backlogCount)
	{
		// ip, 포트 셋팅.
		// ip는 아무거나 상관없으니 INADDR_ANY
		SOCKADDR_IN server_addr;
		ZeroMemory(&server_addr, sizeof(server_addr));
		server_addr.sin_family = AF_INET;
		server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
		server_addr.sin_port = htons(port);

		// bind()
		if (bind(m_ServerSockfd, (SOCKADDR*)&server_addr, sizeof(server_addr)) < 0)
		{
			return NET_ERROR_CODE::SERVER_SOCKET_BIND_FAIL;
		}
		
		// listen()
		if (listen(m_ServerSockfd, backlogCount) == SOCKET_ERROR)
		{
			return NET_ERROR_CODE::SERVER_SOCKET_LISTEN_FAIL;
		}
		
		//로그
		m_pRefLogger->Write(LOG_TYPE::L_INFO, "%s | Listen. ServerSockfd(%d)", __FUNCTION__, m_ServerSockfd);
		return NET_ERROR_CODE::NONE;
	}

	NET_ERROR_CODE TcpNetwork::NewSession()
	{
		SOCKADDR_IN client_addr;
		auto client_len = static_cast<int>(sizeof(client_addr));
		auto client_sockfd = accept(m_ServerSockfd, (SOCKADDR*)&client_addr, &client_len);

		if (client_sockfd < 0)
		{
			m_pRefLogger->Write(LOG_TYPE::L_ERROR, "%s | Wrong socket %d cannot accept", __FUNCTION__, client_sockfd);
			return NET_ERROR_CODE::ACCEPT_API_ERROR;
		}

		auto newSessionIndex = AllocClientSessionIndex();
		if (newSessionIndex < 0)
		{
			m_pRefLogger->Write(LOG_TYPE::L_WARN, "%s | client_sockfd(%d)  >= MAX_SESSION", __FUNCTION__, client_sockfd);
			
			// 더 이상 수용할 수 없으므로 바로 짜른다.
			CloseSession(SOCKET_CLOSE_CASE::SESSION_POOL_EMPTY, client_sockfd, -1);
			return NET_ERROR_CODE::ACCEPT_MAX_SESSION_COUNT;
		}


		char clientIP[MAX_IP_LEN] = { 0, };
		inet_ntop(AF_INET, &(client_addr.sin_addr), clientIP, MAX_IP_LEN - 1);
		
		SetSockOption(client_sockfd);
		
		FD_SET(client_sockfd, &m_Readfds);
		
		ConnectedSession(newSessionIndex, (int)client_sockfd, clientIP);
		
		return NET_ERROR_CODE::NONE;
	}
	
	void TcpNetwork::ConnectedSession(const int sessionIndex, const int fd, const char* pIP)
	{
		++m_ConnectSeq;

		auto& session = m_ClientSessionPool[sessionIndex];
		session.Seq = m_ConnectSeq;
		session.SocketFD = fd;
		memcpy(session.IP, pIP, MAX_IP_LEN - 1);

		++m_ConnectedSessionCount;

		m_pRefLogger->Write(LOG_TYPE::L_INFO, "%s | New Session. FD(%d), m_ConnectSeq(%d), IP(%s)", __FUNCTION__, fd, m_ConnectSeq, pIP);
	}

	void TcpNetwork::SetSockOption(const SOCKET fd)
	{
		linger ling;
		ling.l_onoff = 0;
		ling.l_linger = 0;
		setsockopt(fd, SOL_SOCKET, SO_LINGER, (char*)&ling, sizeof(ling));

		int size1 = m_Config.MaxClientSockOptRecvBufferSize;
		int size2 = m_Config.MaxClientSockOptSendBufferSize;
		setsockopt(fd, SOL_SOCKET, SO_RCVBUF, (char*)&size1, sizeof(size1));
		setsockopt(fd, SOL_SOCKET, SO_SNDBUF, (char*)&size2, sizeof(size2));
	}

	void TcpNetwork::CloseSession(const SOCKET_CLOSE_CASE closeCase, const SOCKET sockFD, const int sessionIndex)
	{
		if (closeCase == SOCKET_CLOSE_CASE::SESSION_POOL_EMPTY)
		{
			closesocket(sockFD);
			FD_CLR(sockFD, &m_Readfds);
			return;
		}

		if (m_ClientSessionPool[sessionIndex].IsConnected() == false) {
			return;
		}

		closesocket(sockFD);
		FD_CLR(sockFD, &m_Readfds);

		m_ClientSessionPool[sessionIndex].Clear();
		--m_ConnectedSessionCount;
		ReleaseSessionIndex(sessionIndex);

		AddPacketQueue(sessionIndex, (short)PACKET_ID::NTF_SYS_CLOSE_SESSION, 0, nullptr);
	}

	NET_ERROR_CODE TcpNetwork::RecvSocket(const int sessionIndex)
	{
		auto& session = m_ClientSessionPool[sessionIndex];
		auto fd = static_cast<SOCKET>(session.SocketFD);

		if (session.IsConnected() == false)
		{
			return NET_ERROR_CODE::RECV_PROCESS_NOT_CONNECTED;
		}

		// 받은 데이터를 버퍼의 어디서부터 채울건지
		int recvPos = 0;
		
		if (session.RemainingDataSize > 0) // 아직 receiveBuffer에 남아있는 데이터가 있을 경우
			recvPos += session.RemainingDataSize; // 새로 receive한 데이터는 이미 있는 데이터 뒤에다가 채울 것이다.

		// 받아서 버퍼에 채운다.
		auto recvSize = recv(fd, &session.pRecvBuffer[recvPos], (MAX_PACKET_SIZE*2), 0);

		// 만약 받은 게 없으면
		if (recvSize == 0)
		{
			return NET_ERROR_CODE::RECV_REMOTE_CLOSE;
		}

		// 받으려다 에러가 나버리면
		if (recvSize < 0)
		{
			auto error = WSAGetLastError();
			if (error != WSAEWOULDBLOCK)
			{
				return NET_ERROR_CODE::RECV_API_ERROR; 
			}
			else 
			{
				return NET_ERROR_CODE::NONE;
			}
		}

		// 해당 세션에 남아있는 총 데이터 양에 새로 받은 데이터 양을 더해준다.
		session.RemainingDataSize += recvSize;
		return NET_ERROR_CODE::NONE;
	}

	NET_ERROR_CODE TcpNetwork::RecvBufferProcess(const int sessionIndex)
	{
		auto& session = m_ClientSessionPool[sessionIndex];
		
		auto readPos = 0;
		const auto dataSize = session.RemainingDataSize;
		PacketHeader* pPktHeader;
		
		while ((dataSize - readPos) >= PACKET_HEADER_SIZE)
		{
			pPktHeader = (PacketHeader*)&session.pRecvBuffer[readPos];
			readPos += PACKET_HEADER_SIZE;

			if (pPktHeader->BodySize > 0)
			{
				// <를 >로 바꿔야 할 것 같다.
				if (pPktHeader->BodySize < (dataSize - readPos))
				{
					break;
				}

				if (pPktHeader->BodySize > MAX_PACKET_SIZE)
				{
					return NET_ERROR_CODE::RECV_CLIENT_MAX_PACKET;
				}
			}

			AddPacketQueue(sessionIndex, pPktHeader->Id, pPktHeader->BodySize, &session.pRecvBuffer[readPos]);

			readPos += pPktHeader->BodySize;
		}
		
		session.RemainingDataSize -= readPos;
		
		if (session.RemainingDataSize > 0)
		{
			memcpy(session.pRecvBuffer, &session.pRecvBuffer[readPos], session.RemainingDataSize);
		}

		return NET_ERROR_CODE::NONE;
	}

	void TcpNetwork::AddPacketQueue(const int sessionIndex, const short pktId, const short bodySize, char* pDataPos)
	{
		RecvPacketInfo packetInfo;
		packetInfo.SessionIndex = sessionIndex;
		packetInfo.PacketId = pktId;
		packetInfo.PacketBodySize = bodySize;
		packetInfo.pRefData = pDataPos;

		m_PacketQueue.push_back(packetInfo);
	}

	void TcpNetwork::RunProcessWrite(const int sessionIndex, const SOCKET fd, fd_set& write_set)
	{
		if (!FD_ISSET(fd, &write_set))
		{
			return;
		}

		auto retsend = FlushSendBuff(sessionIndex);
		if (retsend.Error != NET_ERROR_CODE::NONE)
		{
			CloseSession(SOCKET_CLOSE_CASE::SOCKET_SEND_ERROR, fd, sessionIndex);
		}
	}

	NetError TcpNetwork::FlushSendBuff(const int sessionIndex)
	{
		auto& session = m_ClientSessionPool[sessionIndex];
		auto fd = static_cast<SOCKET>(session.SocketFD);

		if (session.IsConnected() == false)
		{
			return NetError(NET_ERROR_CODE::CLIENT_FLUSH_SEND_BUFF_REMOTE_CLOSE);
		}

		auto result = SendSocket(fd, session.pSendBuffer, session.SendSize);

		if (result.Error != NET_ERROR_CODE::NONE) {
			return result;
		}

		auto sendSize = result.Vlaue;
		if (sendSize < session.SendSize)
		{
			memmove(&session.pSendBuffer[0],
				&session.pSendBuffer[sendSize],
				session.SendSize - sendSize);

			session.SendSize -= sendSize;
		}
		else
		{
			session.SendSize = 0;
		}
		return result;
	}

	NetError TcpNetwork::SendSocket(const SOCKET fd, const char* pMsg, const int size)
	{
		NetError result(NET_ERROR_CODE::NONE);
		auto rfds = m_Readfds;

		// 접속 되어 있는지 또는 보낼 데이터가 있는지
		if (size <= 0)
		{
			return result;
		}

		// 이것은 안해도 될 듯
		/*if (!FD_ISSET(fd, &rfds))
		{
			_snwprintf_s(result.Msg, _countof(result.Msg), _TRUNCATE, L"Send Msg! User %d", (int)fd);
			result.Error = NET_ERROR_CODE::SEND_CLOSE_SOCKET;
			return result;
		}*/

		result.Vlaue = send(fd, pMsg, size, 0);

		if (result.Vlaue <= 0)
		{
			result.Error = NET_ERROR_CODE::SEND_SIZE_ZERO;
		}

		return result;
	}

	
}