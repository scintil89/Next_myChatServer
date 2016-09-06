// #include <algorithm>
// 
// #include "../ServerNetLib/ILog.h"
// #include "../ServerNetLib/TcpNetwork.h"
// #include "../../Common/Packet.h"
// #include "../../Common/ErrorCode.h"
// 
// #include "User.h"
// #include "Room.h"
// 
// using PACKET_ID = NCommon::PACKET_ID;
// 
// namespace NLogicLib
// {
// 	Room::Room() {}
// 
// 	Room::~Room() {}
// 
// 
// 	void Room::Init(const short index, const short maxUserCount)
// 	{
// 		m_Index = index;
// 		m_MaxUserCount = maxUserCount;
// 		m_pGame = std::make_unique<CGame>();
// 	}
// 
// 	void Room::SetNetwork(TcpNet* pNetwork, ILog* pLogger)
// 	{
// 		m_pRefLogger = pLogger;
// 		m_pRefNetwork = pNetwork;
// 	}
// 
// 	void Room::Clear()
// 	{
// 		m_IsUsed = false;
// 		m_Title = L"";
// 		m_UserList.clear();
// 	}
// 
// 	ERROR_CODE Room::CreateRoom(const wchar_t* pRoomTitle)
// 	{
// 		if (m_IsUsed) 
// 		{
// 			return ERROR_CODE::ROOM_ENTER_CREATE_FAIL;
// 		}
// 
// 		m_IsUsed = true;
// 		m_Title = pRoomTitle;
// 
// 		return ERROR_CODE::NONE;
// 	}
// 
// 	ERROR_CODE Room::EnterUser(User* pUser)
// 	{
// 		if (m_IsUsed == false) 
// 		{
// 			return ERROR_CODE::ROOM_ENTER_NOT_CREATED;
// 		}
// 
// 		//printf_s("================ siztest ==============\n");
// 		if (GetUserCount() == m_MaxUserCount) 
// 		{
// 			return ERROR_CODE::ROOM_ENTER_MEMBER_FULL;
// 		}
// 
// 		m_UserList.push_back(pUser);
// 		return ERROR_CODE::NONE;
// 	}
// 
// 	ERROR_CODE Room::LeaveUser(const short userIndex)
// 	{
// 		if (m_IsUsed == false) 
// 		{
// 			return ERROR_CODE::ROOM_ENTER_NOT_CREATED;
// 		}
// 
// 		auto iter = std::find_if(std::begin(m_UserList), std::end(m_UserList), [userIndex](auto pUser) { return pUser->GetIndex() == userIndex; });
// 		if (iter == std::end(m_UserList)) 
// 		{
// 			return ERROR_CODE::ROOM_LEAVE_NOT_MEMBER;
// 		}
// 		
// 		m_UserList.erase(iter);
// 
// 		if (m_UserList.empty()) 
// 		{
// 			Clear();
// 		}
// 
// 		return ERROR_CODE::NONE;
// 	}
// 
// 	//룸 유저 리스트
// 	ERROR_CODE Room::SendUserList(const int sessionId, const short startUserIndex)
// 	{
// 		if (m_IsUsed == false)
// 		{
// 			printf_s("false\n");
// 			return ERROR_CODE::ROOM_ENTER_NOT_CREATED;
// 		}
// 
// 		//printf_s("================ %d, %d ==============\n", sessionId, startUserIndex);
// 
// 		if ( startUserIndex < 0)
// 		{
// 			//printf_s("================ ROOM_USER_LIST_INVALID_START_USER_INDEX ==============\n");
// 			return ERROR_CODE::ROOM_USER_LIST_INVALID_START_USER_INDEX;
// 		}
// 		
// 		//printf_s("!!!!!!!!!!!!!!! %d !!!!!!!!!!!!!!!\n", GetUserCount());
// 
// 		if ( startUserIndex >= GetUserCount() ) //인덱스가 유저 수보다 클때 에러.
// 		{
// 			printf_s("================ ROOM_USER_LIST_INVALID_START_USER_INDEX %d ==============\n", GetUserCount() );
// 			return ERROR_CODE::ROOM_USER_LIST_INVALID_START_USER_INDEX;
// 		}
// 				
// 		int lastCheckedIndex = 0;
// 		NCommon::PktRoomUserListRes pktRes;
// 		short userCount = 0;
// 
// 
// 		for (int i = startUserIndex; i < GetUserCount(); ++i)
// 		{
// 			auto& roomUser = m_UserList[i];
// 			lastCheckedIndex = i;
// 
// 			//유저가 없거나 룸에 있지 않으면 continue
// 			if (roomUser == nullptr /*|| roomUser->IsCurDomainInRoom() == false*/) 
// 			{
// 				continue;
// 			}
// 
// 			//유저 리스트에 자신 id도 표시하고싶어서 지움
// // 			if (roomUser->GetSessioIndex() == sessionId)
// // 				continue;
// 
// 
// 			pktRes.UserInfo[userCount].UserIndex = (short)i;
// 			strncpy_s(pktRes.UserInfo[userCount].UserID, NCommon::MAX_USER_ID_SIZE + 1, roomUser->GetID().c_str(), NCommon::MAX_USER_ID_SIZE);
// 
// 			++userCount;
// 			if (userCount >= NCommon::MAX_SEND_ROOM_USER_LIST_COUNT)
// 			{
// 				break;
// 			}
// 		}
// 		//printf_s("================ end for loop ==============\n");
// 
// 		pktRes.Count = userCount;
// 
// 		if (userCount <= 0 || (lastCheckedIndex + 1) == m_UserList.size()) 
// 		{
// 			pktRes.IsEnd = true;
// 		}
// 
// 		//printf_s("================ start Send Data ==============\n");
// 		m_pRefNetwork->SendData(sessionId, (short)PACKET_ID::ROOM_ENTER_USER_LIST_RES, sizeof(pktRes), (char*)&pktRes);
// 		//printf_s("================ end Send Data ==============\n");
// 
// 		return ERROR_CODE::NONE;
// 	}
// 
// 	void Room::SendToAllUser(const short packetId, const short dataSize, char* pData, const int passUserindex)
// 	{
// 		for (auto pUser : m_UserList)
// 		{
// 			//자신한테는 안보냄.
// 			if (pUser->GetIndex() == passUserindex) 
// 			{
// 				continue;
// 			}
// 
// 			m_pRefNetwork->SendData(pUser->GetSessioIndex(), packetId, dataSize, pData);
// 		}
// 	}
// 
// 	void Room::NotifyEnterUserInfo(const int userIndex, const char* pszUserID)
// 	{
// 		NCommon::PktRoomEnterUserInfoNtf pkt;
// 		strncpy_s(pkt.UserID, _countof(pkt.UserID), pszUserID, NCommon::MAX_USER_ID_SIZE);
// 
// 		SendToAllUser((short)PACKET_ID::ROOM_ENTER_USER_NTF, sizeof(pkt), (char*)&pkt, userIndex);
// 	}
// 
// 	void Room::NotifyLeaveUserInfo(const char* pszUserID)
// 	{
// 		if (m_IsUsed == false) 
// 		{
// 			return;
// 		}
// 
// 		NCommon::PktRoomLeaveUserInfoNtf pkt;
// 		strncpy_s(pkt.UserID, _countof(pkt.UserID), pszUserID, NCommon::MAX_USER_ID_SIZE);
// 
// 		SendToAllUser((short)PACKET_ID::ROOM_LEAVE_USER_NTF, sizeof(pkt), (char*)&pkt);
// 	}
// 
// 	void Room::NotifyChat(const int sessionIndex, const char* pszUserID, const wchar_t* pszMsg)
// 	{
// 		NCommon::PktRoomChatNtf pkt;
// 		strncpy_s(pkt.UserID, _countof(pkt.UserID), pszUserID, NCommon::MAX_USER_ID_SIZE);
// 		wcsncpy_s(pkt.Msg, NCommon::MAX_ROOM_CHAT_MSG_SIZE + 1, pszMsg, NCommon::MAX_ROOM_CHAT_MSG_SIZE);
// 
// 		SendToAllUser((short)PACKET_ID::ROOM_CHAT_NTF, sizeof(pkt), (char*)&pkt, sessionIndex);
// 	}
// 
// 	bool Room::IsMaster(const short userIndex)
// 	{
// 		return m_UserList[0]->GetIndex() == userIndex ? true : false;
// 	}
// 
// 	NLogicLib::CGame* Room::GetGameObj()
// 	{
// 		return m_pGame.get();
// 	}
// 
// }