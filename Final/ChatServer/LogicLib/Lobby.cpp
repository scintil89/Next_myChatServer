#include <algorithm>

#include "../ServerNetLib/ILog.h"
#include "../ServerNetLib/TcpNetwork.h"
#include "../../Common/Packet.h"
#include "../../Common/ErrorCode.h"
#include "User.h"
#include "Lobby.h"

using PACKET_ID = NCommon::PACKET_ID;

namespace NLogicLib
{
	Lobby::Lobby() {}

	Lobby::~Lobby() {}

	void Lobby::Init(const short lobbyIndex, const short maxLobbyUserCount, const short maxRoomCountByLobby, const short maxRoomUserCount)
	{
		m_LobbyIndex = lobbyIndex;
		m_MaxUserCount = (short)maxLobbyUserCount;

		for (int i = 0; i < maxLobbyUserCount; ++i)
		{
			LobbyUser lobbyUser;
			lobbyUser.Index = (short)i;
			lobbyUser.pUser = nullptr;

			m_UserList.push_back(lobbyUser);
		}

// 		for (int i = 0; i < maxRoomCountByLobby; ++i)
// 		{
// 			m_RoomList.emplace_back(Room());
// 			m_RoomList[i].Init((short)i, maxRoomUserCount);
// 		}
	}

	void Lobby::SetNetwork(TcpNet* pNetwork, ILog* pLogger)
	{
		m_pRefLogger = pLogger;
		m_pRefNetwork = pNetwork;

// 		for (auto& room : m_RoomList)
// 		{
// 			room.SetNetwork(pNetwork, pLogger);
// 		}
	}

	ERROR_CODE Lobby::EnterUser(User* pUser)
	{
		if (m_UserIndexDic.size() >= m_MaxUserCount) {
			return ERROR_CODE::LOBBY_ENTER_MAX_USER_COUNT;
		}

		if (FindUser(pUser->GetIndex()) != nullptr) {
			return ERROR_CODE::LOBBY_ENTER_USER_DUPLICATION;
		}

		auto addRet = AddUser(pUser);
		if (addRet != ERROR_CODE::NONE) {
			return addRet;
		}

		pUser->EnterLobby(m_LobbyIndex);

		m_UserIndexDic.insert({ pUser->GetIndex(), pUser });
		m_UserIDDic.insert({ pUser->GetID().c_str(), pUser });

		return ERROR_CODE::NONE;
	}

	ERROR_CODE Lobby::LeaveUser(const int userIndex)
	{
		RemoveUser(userIndex);

		auto pUser = FindUser(userIndex);

		if (pUser == nullptr) {
			return ERROR_CODE::LOBBY_LEAVE_USER_NVALID_UNIQUEINDEX;
		}

		pUser->LeaveLobby();

		m_UserIndexDic.erase(pUser->GetIndex());
		m_UserIDDic.erase(pUser->GetID().c_str());
		
		return ERROR_CODE::NONE;
	}
		
	User* Lobby::FindUser(const int userIndex)
	{
		auto findIter = m_UserIndexDic.find(userIndex);

		if (findIter == m_UserIndexDic.end()) {
			return nullptr;
		}

		return (User*)findIter->second;
	}

	ERROR_CODE Lobby::AddUser(User* pUser)
	{
		auto findIter = std::find_if(std::begin(m_UserList), std::end(m_UserList), [](auto& lobbyUser) { return lobbyUser.pUser == nullptr; });
		
		if (findIter == std::end(m_UserList)) {
			return ERROR_CODE::LOBBY_ENTER_EMPTY_USER_LIST;
		}

		findIter->pUser = pUser;
		return ERROR_CODE::NONE;
	}

	void Lobby::RemoveUser(const int userIndex)
	{
		auto findIter = std::find_if(std::begin(m_UserList), std::end(m_UserList), [userIndex](auto& lobbyUser) { return lobbyUser.pUser != nullptr && lobbyUser.pUser->GetIndex() == userIndex; });

		if (findIter != std::end(m_UserList)) {
			return;
		}

		findIter->pUser = nullptr;
	}

	short Lobby::GetUserCount()
	{ 
		return static_cast<short>(m_UserIndexDic.size()); 
	}


	auto Lobby::GetUser(char* UserId)
	{
		
	}

	void Lobby::NotifyLobbyEnterUserInfo(User* pUser)
	{
		NCommon::PktLobbyNewUserInfoNtf pkt;
		strncpy_s(pkt.UserID, _countof(pkt.UserID), pUser->GetID().c_str(), NCommon::MAX_USER_ID_SIZE);

		SendToAllUser((short)PACKET_ID::LOBBY_ENTER_USER_NTF, sizeof(pkt), (char*)&pkt, pUser->GetIndex());
	}

	void Lobby::NotifyLobbyLeaveUserInfo(User* pUser)
	{
		NCommon::PktLobbyLeaveUserInfoNtf pkt;
		strncpy_s(pkt.UserID, _countof(pkt.UserID), pUser->GetID().c_str(), NCommon::MAX_USER_ID_SIZE);

		SendToAllUser((short)PACKET_ID::LOBBY_LEAVE_USER_NTF, sizeof(pkt), (char*)&pkt, pUser->GetIndex());
	}

// 	ERROR_CODE Lobby::SendRoomList(const int sessionId, const short startRoomId)
// 	{
// 		if (startRoomId < 0 || startRoomId >= (m_RoomList.size() - 1)) {
// 			return ERROR_CODE::LOBBY_ROOM_LIST_INVALID_START_ROOM_INDEX;
// 		}
// 
// 		NCommon::PktLobbyRoomListRes pktRes;
// 		short roomCount = 0;
// 		int lastCheckedIndex = 0;
// 
// 		for (int i = startRoomId; i < m_RoomList.size(); ++i)
// 		{
// 			auto& room = m_RoomList[i];
// 			lastCheckedIndex = i;
// 
// 			if (room.IsUsed() == false) {
// 				continue;
// 			}
// 
// 			pktRes.RoomInfo[roomCount].RoomIndex = room.GetIndex();
// 			pktRes.RoomInfo[roomCount].RoomUserCount = room.GetUserCount();
// 			wcsncpy_s(pktRes.RoomInfo[roomCount].RoomTitle, NCommon::MAX_ROOM_TITLE_SIZE + 1, room.GetTitle(), NCommon::MAX_ROOM_TITLE_SIZE);
// 			
// 			++roomCount;
// 
// 			if (roomCount >= NCommon::MAX_NTF_LOBBY_ROOM_LIST_COUNT) {
// 				break;
// 			}
// 		}
// 
// 		pktRes.Count = roomCount;
// 
// 		if (roomCount <= 0 || (lastCheckedIndex + 1) == m_RoomList.size()) {
// 			pktRes.IsEnd = true;
// 		}
// 
// 		m_pRefNetwork->SendData(sessionId, (short)PACKET_ID::LOBBY_ENTER_ROOM_LIST_RES, sizeof(pktRes), (char*)&pktRes);
// 
// 		return ERROR_CODE::NONE;
// 	}

	ERROR_CODE Lobby::SendUserList(const int sessionId, const short startUserIndex)
	{
		if (startUserIndex < 0 || startUserIndex >= (m_UserList.size() - 1)) 
		{
			return ERROR_CODE::LOBBY_USER_LIST_INVALID_START_USER_INDEX;
		}

		int lastCheckedIndex = 0;
		NCommon::PktLobbyUserListRes pktRes;
		short userCount = 0;

		for (int i = startUserIndex; i < m_UserList.size(); ++i)
		{
			auto& lobbyUser = m_UserList[i];
			lastCheckedIndex = i;

			if (lobbyUser.pUser == nullptr || lobbyUser.pUser->IsCurDomainInLobby() == false) {
				continue;
			}

			pktRes.UserInfo[userCount].UserIndex = (short)i;
			strncpy_s(pktRes.UserInfo[userCount].UserID, NCommon::MAX_USER_ID_SIZE + 1, lobbyUser.pUser->GetID().c_str(), NCommon::MAX_USER_ID_SIZE);

			++userCount;

			if (userCount >= NCommon::MAX_SEND_LOBBY_USER_LIST_COUNT) {
				break;
			}
		}

		pktRes.Count = userCount;

		if (userCount <= 0 || (lastCheckedIndex + 1) == m_UserList.size()) {
			pktRes.IsEnd = true;
		}

		m_pRefNetwork->SendData(sessionId, (short)PACKET_ID::LOBBY_ENTER_USER_LIST_RES, sizeof(pktRes), (char*)&pktRes);

		return ERROR_CODE::NONE;
	}

	//모든 유저한테 패킷 전송
	void Lobby::SendToAllUser(const short packetId, const short dataSize, char* pData, const int passUserindex)
	{
		for (auto& pUser : m_UserIndexDic)
		{
			//자기 자신은 넘어감.
			if (pUser.second->GetIndex() == passUserindex) 
			{
				continue;
			}

			if (pUser.second->IsCurDomainInLobby() == false) 
			{
				continue;
			}

			m_pRefNetwork->SendData(pUser.second->GetSessioIndex(), packetId, dataSize, pData);
		}
	}

	//특정 유저한테 패킷 전송
	void Lobby::SendToUser(const short packetId, const short dataSize, char* pData, const int recvUserindex /*= -1*/)
	{
		printf_s("%d\n", recvUserindex);
		m_pRefNetwork->SendData(recvUserindex, packetId, dataSize, pData);
	}

// 	Room* Lobby::GetRoom(const short roomIndex)
// 	{
// 		if (roomIndex < 0 || roomIndex >= m_RoomList.size()) {
// 			return nullptr;
// 		}
// 
// 		return &m_RoomList[roomIndex];
// 	}

// 	void Lobby::NotifyChangedRoomInfo(const short roomIndex)
// 	{
// 		NCommon::PktChangedRoomInfoNtf pktNtf;
// 				
// 		auto& room = m_RoomList[roomIndex];
// 		
// 		pktNtf.RoomInfo.RoomIndex = room.GetIndex();
// 		pktNtf.RoomInfo.RoomUserCount = room.GetUserCount();
// 
// 		if (m_RoomList[roomIndex].IsUsed()) {
// 			wcsncpy_s(pktNtf.RoomInfo.RoomTitle, NCommon::MAX_ROOM_TITLE_SIZE + 1, room.GetTitle(), NCommon::MAX_ROOM_TITLE_SIZE);
// 		}
// 		else {
// 			pktNtf.RoomInfo.RoomTitle[0] = L'\0';
// 		}
// 
// 		SendToAllUser((short)PACKET_ID::ROOM_CHANGED_INFO_NTF, sizeof(pktNtf), (char*)&pktNtf);
// 	}

	//로비 채팅
	void Lobby::NotifyChat(const int sessionIndex, const char* pszUserID, const wchar_t* pszMsg)
	{
		NCommon::PktLOBBYChatNtf pkt;
		strncpy_s(pkt.UserID, _countof(pkt.UserID), pszUserID, NCommon::MAX_USER_ID_SIZE);
		wcsncpy_s(pkt.Msg, NCommon::MAX_LOBBY_CHAT_MSG_SIZE + 1, pszMsg, NCommon::MAX_LOBBY_CHAT_MSG_SIZE);

		SendToAllUser((short)PACKET_ID::LOBBY_CHAT_NTF, sizeof(pkt), (char*)&pkt, sessionIndex); //sessionIndex를 제외한 모든 유저한테 보내줌.
	}

	//로비 귓속말 채팅
	void Lobby::NotifyWisperChat(const int sessionIndex, const char* pszUserID, const wchar_t* pszMsg, const int recvSessionIndex)
	{
		NCommon::PktLOBBYChatNtf pkt;
		strncpy_s(pkt.UserID, _countof(pkt.UserID), pszUserID, NCommon::MAX_USER_ID_SIZE);
		wcsncpy_s(pkt.Msg, NCommon::MAX_LOBBY_CHAT_MSG_SIZE + 1, pszMsg, NCommon::MAX_LOBBY_CHAT_MSG_SIZE);

		SendToUser((short)PACKET_ID::LOBBY_WHISPER_CHAT_NTF, sizeof(pkt), (char*)&pkt, recvSessionIndex);
	}

// 	NLogicLib::Room* Lobby::CreateRoom()
// 	{
// 		for (int i = 0; i < m_RoomList.size(); ++i)
// 		{
// 			if (m_RoomList[i].IsUsed() == false) {
// 				return &m_RoomList[i];
// 			}
// 		}
// 		return nullptr;
// 	}
}
