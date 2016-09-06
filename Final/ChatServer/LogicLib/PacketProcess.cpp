#include "../ServerNetLib/ILog.h"
#include "../ServerNetLib/TcpNetwork.h"
#include "ConnectedUserManager.h"
#include "User.h"
#include "UserManager.h"
//#include "Room.h"
#include "Lobby.h"
#include "LobbyManager.h"
#include "PacketProcess.h"

using LOG_TYPE = NServerNetLib::LOG_TYPE;

namespace NLogicLib
{	
	PacketProcess::PacketProcess() {}
	PacketProcess::~PacketProcess() {}

	void PacketProcess::Init(TcpNet* pNetwork, UserManager* pUserMgr, LobbyManager* pLobbyMgr, ILog* pLogger) //pRoomMgr 삭제
	{
		m_pRefLogger = pLogger;
		m_pRefNetwork = pNetwork;
		m_pRefUserMgr = pUserMgr;
		m_pRefLobbyMgr = pLobbyMgr;
		//m_pRefRoomMgr = pRoomMgr;

		m_pConnectedUserManager = std::make_unique<ConnectedUserManager>();
		m_pConnectedUserManager->Init(pNetwork->ClientSessionPoolSize(), pNetwork, pLogger);

		using netLibPacketId = NServerNetLib::PACKET_ID;
		using commonPacketId = NCommon::PACKET_ID;
		for (int i = 0; i < (int)commonPacketId::MAX; ++i)
		{
			PacketFuncArray[i] = nullptr;
		}

		PacketFuncArray[(int)netLibPacketId::NTF_SYS_CONNECT_SESSION] = &PacketProcess::NtfSysConnctSession;
		PacketFuncArray[(int)netLibPacketId::NTF_SYS_CLOSE_SESSION] = &PacketProcess::NtfSysCloseSession;

		PacketFuncArray[(int)commonPacketId::LOGIN_IN_REQ] = &PacketProcess::Login;
		PacketFuncArray[(int)commonPacketId::LOBBY_LIST_REQ] = &PacketProcess::LobbyList;
		PacketFuncArray[(int)commonPacketId::LOBBY_ENTER_REQ] = &PacketProcess::LobbyEnter;

		PacketFuncArray[(int)commonPacketId::LOBBY_ENTER_ROOM_LIST_REQ] = &PacketProcess::LobbyRoomList;
		PacketFuncArray[(int)commonPacketId::LOBBY_ENTER_USER_LIST_REQ] = &PacketProcess::LobbyUserList;

		PacketFuncArray[(int)commonPacketId::LOBBY_CHAT_REQ] = &PacketProcess::LOBBYChat;
		PacketFuncArray[(int)commonPacketId::LOBBY_WHISPER_CHAT_REQ] = &PacketProcess::LOBBYWisperChat;
		PacketFuncArray[(int)commonPacketId::LOBBY_LEAVE_REQ] = &PacketProcess::LobbyLeave;

// 		PacketFuncArray[(int)commonPacketId::ROOM_ENTER_REQ] = &PacketProcess::RoomEnter;
// 		PacketFuncArray[(int)commonPacketId::ROOM_LEAVE_REQ] = &PacketProcess::RoomLeave;
// 		PacketFuncArray[(int)commonPacketId::ROOM_CHAT_REQ] = &PacketProcess::RoomChat;
// 		PacketFuncArray[(int)commonPacketId::ROOM_ENTER_USER_LIST_REQ] = &PacketProcess::RoomUserList;
// 
// 		PacketFuncArray[(int)commonPacketId::ROOM_MASTER_GAME_STATE_REQ] = &PacketProcess::RoomMasterGameStart;
// 		PacketFuncArray[(int)commonPacketId::ROOM_GAME_STATE_REQ] = &PacketProcess::RoomGameStart;
	}
	
	void PacketProcess::Process(PacketInfo packetInfo)
	{
		auto packetId = packetInfo.PacketId;

		if (PacketFuncArray[packetId] == nullptr)
		{
			//TODO: 로그 남긴다
			m_pRefLogger->Write(LOG_TYPE::L_ERROR, "%s | PacketID == nullptr(%d).", __FUNCTION__, packetId);
			return;
		}

		m_pRefLogger->Write(LOG_TYPE::L_INFO, "%s | Start Process PacketID(%d).", __FUNCTION__, packetId);

		(this->*PacketFuncArray[packetId])(packetInfo);
	}

	void PacketProcess::StateCheck()
	{
		m_pConnectedUserManager->LoginCheck();
	}

	ERROR_CODE PacketProcess::NtfSysConnctSession(PacketInfo packetInfo)
	{
		m_pConnectedUserManager->SetConnectSession(packetInfo.SessionIndex);
		return ERROR_CODE::NONE;
	}

	ERROR_CODE PacketProcess::NtfSysCloseSession(PacketInfo packetInfo)
	{
		auto pUser = std::get<1>(m_pRefUserMgr->GetUser(packetInfo.SessionIndex));

		if (pUser) 
		{
			auto pLobby = m_pRefLobbyMgr->GetLobby(pUser->GetLobbyIndex());
			if (pLobby)
			{
// 				auto pRoom = pLobby->GetRoom(pUser->GetRoomIndex());
// 
// 				if (pRoom)
// 				{
// 					pRoom->LeaveUser(pUser->GetIndex());
// 					pRoom->NotifyLeaveUserInfo(pUser->GetID().c_str());
// 					pLobby->NotifyChangedRoomInfo(pRoom->GetIndex());
// 
// 					m_pRefLogger->Write(LOG_TYPE::L_INFO, "%s | NtfSysCloseSesson. sessionIndex(%d). Room Out", __FUNCTION__, packetInfo.SessionIndex);
// 				}
// 
// 				pLobby->LeaveUser(pUser->GetIndex());
// 
// 				if (pRoom == nullptr) {
// 					pLobby->NotifyLobbyLeaveUserInfo(pUser);
// 				}

				m_pRefLogger->Write(LOG_TYPE::L_INFO, "%s | NtfSysCloseSesson. sessionIndex(%d). Lobby Out", __FUNCTION__, packetInfo.SessionIndex);
			}
			
			m_pRefUserMgr->RemoveUser(packetInfo.SessionIndex);		
		}
		
		m_pConnectedUserManager->SetDisConnectSession(packetInfo.SessionIndex);

		m_pRefLogger->Write(LOG_TYPE::L_INFO, "%s | NtfSysCloseSesson. sessionIndex(%d)", __FUNCTION__, packetInfo.SessionIndex);
		return ERROR_CODE::NONE;
	}
}