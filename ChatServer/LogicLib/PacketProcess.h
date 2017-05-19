#pragma once

#include <memory>
#include "../../Common/Packet.h"
#include "../ServerNetLib/Define.h"
#include "../../Common/ErrorCode.h"

using ERROR_CODE = NCommon::ERROR_CODE;

namespace NServerNetLib
{
	class ITcpNetwork;
	class ILog;
}

namespace NLogicLib
{	
	class ConnectedUserManager;
	class UserManager;
	class LobbyManager;
	class RoomManager;

	#define CHECK_START  ERROR_CODE __result=ERROR_CODE::NONE;
	#define CHECK_ERROR(f) __result=f; goto CHECK_ERR;

	class PacketProcess
	{
		using PacketInfo = NServerNetLib::RecvPacketInfo;
		typedef ERROR_CODE(PacketProcess::*PacketFunc)(PacketInfo);
		PacketFunc PacketFuncArray[(int)NCommon::PACKET_ID::MAX];

		using TcpNet = NServerNetLib::ITcpNetwork;
		using ILog = NServerNetLib::ILog;

	public:
		PacketProcess();
		~PacketProcess();

		void Init(TcpNet* pNetwork, UserManager* pUserMgr, LobbyManager* pLobbyMgr, RoomManager* pRoomMgr, ILog* pLogger);
		void Process(PacketInfo packetInfo);

		void StateCheck();
	
	private:
		ILog* m_pRefLogger;
		TcpNet* m_pRefNetwork;
				
		UserManager* m_pRefUserMgr;
		LobbyManager* m_pRefLobbyMgr;
		RoomManager* m_pRefRoomMgr;

		std::unique_ptr<ConnectedUserManager> m_pConnectedUserManager;
						
	private:
		ERROR_CODE NtfSysConnctSession(PacketInfo packetInfo);
		ERROR_CODE NtfSysCloseSession(PacketInfo packetInfo);
		
		ERROR_CODE Login(PacketInfo packetInfo);
		
		ERROR_CODE LobbyList(PacketInfo packetInfo);

		ERROR_CODE LobbyEnter(PacketInfo packetInfo);

		ERROR_CODE LobbyRoomList(PacketInfo packetInfo);

		ERROR_CODE LobbyUserList(PacketInfo packetInfo);

		ERROR_CODE LOBBYChat(PacketInfo packetInfo);

		ERROR_CODE LOBBYWisperChat(PacketInfo packetinfo);

		ERROR_CODE LobbyLeave(PacketInfo packetInfo);

		ERROR_CODE RoomEnter(PacketInfo packetInfo);

		ERROR_CODE RoomLeave(PacketInfo packetInfo);

		ERROR_CODE RoomChat(PacketInfo packetInfo);

		ERROR_CODE RoomUserList(PacketInfo packetInfo);

		ERROR_CODE RoomMasterGameStart(PacketInfo packetInfo);

		ERROR_CODE RoomGameStart(PacketInfo packetInfo);
	};
}