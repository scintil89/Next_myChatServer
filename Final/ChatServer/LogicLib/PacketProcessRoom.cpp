// #include "../../Common/Packet.h"
// #include "../ServerNetLib/ILog.h"
// #include "../ServerNetLib/TcpNetwork.h"
// #include "../../Common/ErrorCode.h"
// #include "User.h"
// #include "UserManager.h"
// #include "LobbyManager.h"
// #include "RoomManager.h"
// #include "Lobby.h"
// #include "Room.h"
// #include "Game.h"
// #include "PacketProcess.h"
// 
// using PACKET_ID = NCommon::PACKET_ID;
// using LOG_TYPE = NServerNetLib::LOG_TYPE;
// 
// namespace NLogicLib
// {
// 	ERROR_CODE PacketProcess::RoomEnter(PacketInfo packetInfo)
// 	{
// 	CHECK_START
// 		auto reqPkt = (NCommon::PktRoomEnterReq*)packetInfo.pRefData;
// 		NCommon::PktRoomEnterRes resPkt;
// 
// 		auto pUserRet = m_pRefUserMgr->GetUser(packetInfo.SessionIndex);
// 		auto errorCode = std::get<0>(pUserRet);
// 
// 		if (errorCode != ERROR_CODE::NONE) {
// 			CHECK_ERROR(errorCode);
// 		}
// 
// 		auto pUser = std::get<1>(pUserRet);
// 
// 		if (pUser->IsCurDomainInLobby() == false) {
// 			CHECK_ERROR(ERROR_CODE::ROOM_ENTER_INVALID_DOMAIN);
// 		}
// 
// 		auto lobbyIndex = pUser->GetLobbyIndex();
// 		auto pLobby = m_pRefLobbyMgr->GetLobby(lobbyIndex);
// 		if (pLobby == nullptr) {
// 			CHECK_ERROR(ERROR_CODE::ROOM_ENTER_INVALID_LOBBY_INDEX);
// 		}
// 		
// // 		auto pRoom = pLobby->GetRoom(reqPkt->RoomIndex);
// // 
// // 		if (pRoom == nullptr) 
// // 		{
// // 			CHECK_ERROR(ERROR_CODE::ROOM_ENTER_INVALID_ROOM_INDEX);
// // 		}
// // 
// // 		// 룸을 만드는 경우라면 룸을 만든다
// // 		if (reqPkt->IsCreate) 
// // 		{
// // 			pRoom = pLobby->CreateRoom();
// //			if (pRoom == nullptr) 
// //			{
// //				CHECK_ERROR(ERROR_CODE::ROOM_ENTER_EMPTY_ROOM);
// //			}
// // 
// // 			auto ret = pRoom->CreateRoom(reqPkt->RoomTitle);
// // 			if (ret != ERROR_CODE::NONE) {
// // 				CHECK_ERROR(ret);
// // 			}
// // 		}
// // 
// // 		auto enterRet = pRoom->EnterUser(pUser);
// // 		if (enterRet != ERROR_CODE::NONE) {
// // 			CHECK_ERROR(enterRet);
// // 		}
// 
// 		Room* pRoom = nullptr;
// 
// 		// 룸을 만드는 경우라면 룸을 만든다
// 		if (reqPkt->IsCreate)
// 		{
// 			pRoom = pLobby->CreateRoom();
// 			if (pRoom == nullptr) {
// 				CHECK_ERROR(ERROR_CODE::ROOM_ENTER_EMPTY_ROOM);
// 			}
// 
// 			auto ret = pRoom->CreateRoom(reqPkt->RoomTitle);
// 			if (ret != ERROR_CODE::NONE) {
// 				CHECK_ERROR(ret);
// 			}
// 
// 			m_pRefRoomMgr->CreateRoom(pRoom);
// 		}
// 		else
// 		{
// 			pRoom = pLobby->GetRoom(reqPkt->RoomIndex);
// 			if (pRoom == nullptr) {
// 				CHECK_ERROR(ERROR_CODE::ROOM_ENTER_INVALID_ROOM_INDEX);
// 			}
// 		}
// 
// 		auto enterRet = pRoom->EnterUser(pUser);
// 		if (enterRet != ERROR_CODE::NONE) {
// 			CHECK_ERROR(enterRet);
// 		}		
// 		// 유저 정보를 룸에 들어왔다고 변경한다.
// 		pUser->EnterRoom(lobbyIndex, pRoom->GetIndex());
// 
// 		// 로비에 유저가 나갔음을 알린다
// 		pLobby->NotifyLobbyLeaveUserInfo(pUser);
// 		
// 		// 로비에 룸 정보를 통보한다.
// 		pLobby->NotifyChangedRoomInfo(pRoom->GetIndex());
// 
// 		// 룸에 새 유저 들어왔다고 알린다
// 		pRoom->NotifyEnterUserInfo(pUser->GetIndex(), pUser->GetID().c_str());
// 		
// 		m_pRefNetwork->SendData(packetInfo.SessionIndex, (short)PACKET_ID::ROOM_ENTER_RES, sizeof(resPkt), (char*)&resPkt);
// 		return ERROR_CODE::NONE;
// 
// 	CHECK_ERR:
// 		resPkt.SetError(__result);
// 		m_pRefNetwork->SendData(packetInfo.SessionIndex, (short)PACKET_ID::ROOM_ENTER_RES, sizeof(resPkt), (char*)&resPkt);
// 		return (ERROR_CODE)__result;
// 	}
// 
// 	ERROR_CODE PacketProcess::RoomLeave(PacketInfo packetInfo)
// 	{
// 	CHECK_START
// 		NCommon::PktRoomLeaveRes resPkt;
// 
// 		auto pUserRet = m_pRefUserMgr->GetUser(packetInfo.SessionIndex);
// 		auto errorCode = std::get<0>(pUserRet);
// 
// 		if (errorCode != ERROR_CODE::NONE) {
// 			CHECK_ERROR(errorCode);
// 		}
// 
// 		auto pUser = std::get<1>(pUserRet);
// 		auto userIndex = pUser->GetIndex();
// 
// 		if (pUser->IsCurDomainInRoom() == false) {
// 			CHECK_ERROR(ERROR_CODE::ROOM_LEAVE_INVALID_DOMAIN);
// 		}
// 
// 		auto lobbyIndex = pUser->GetLobbyIndex();
// 		auto pLobby = m_pRefLobbyMgr->GetLobby(lobbyIndex);
// 		if (pLobby == nullptr) {
// 			CHECK_ERROR(ERROR_CODE::ROOM_ENTER_INVALID_LOBBY_INDEX);
// 		}
// 
// 		auto pRoom = pLobby->GetRoom(pUser->GetRoomIndex());
// 		if (pRoom == nullptr) {
// 			CHECK_ERROR(ERROR_CODE::ROOM_ENTER_INVALID_ROOM_INDEX);
// 		}
// 
// 		auto leaveRet = pRoom->LeaveUser(userIndex);
// 		if (leaveRet != ERROR_CODE::NONE) {
// 			CHECK_ERROR(leaveRet);
// 		}
// 		
// 		// 유저 정보를 로비로 변경
// 		pUser->EnterLobby(lobbyIndex);
// 
// 		// 룸에 유저가 나갔음을 통보
// 		pRoom->NotifyLeaveUserInfo(pUser->GetID().c_str());
// 
// 		// 로비에 새로운 유저가 들어왔음을 통보
// 		pLobby->NotifyLobbyEnterUserInfo(pUser);
// 
// 		// 로비에 바뀐 방 정보를 통보
// 		pLobby->NotifyChangedRoomInfo(pRoom->GetIndex());
// 		
// 		m_pRefNetwork->SendData(packetInfo.SessionIndex, (short)PACKET_ID::ROOM_LEAVE_RES, sizeof(resPkt), (char*)&resPkt);
// 		return ERROR_CODE::NONE;
// 
// 	CHECK_ERR:
// 		resPkt.SetError(__result);
// 		m_pRefNetwork->SendData(packetInfo.SessionIndex, (short)PACKET_ID::ROOM_LEAVE_RES, sizeof(resPkt), (char*)&resPkt);
// 		return (ERROR_CODE)__result;
// 	}
// 
// 	//룸채팅
// 	ERROR_CODE PacketProcess::RoomChat(PacketInfo packetInfo)
// 	{
// 	CHECK_START
// 			m_pRefLogger->Write(LOG_TYPE::L_INFO, "%s | Check Start", __FUNCTION__);
// 		auto reqPkt = (NCommon::PktRoomChatReq*)packetInfo.pRefData;
// 		NCommon::PktRoomChatRes resPkt;
// 		
// 			m_pRefLogger->Write(LOG_TYPE::L_INFO, "%s | GetUser Start", __FUNCTION__);
// 		//유저를 받아옴.
// 		auto pUserRet = m_pRefUserMgr->GetUser(packetInfo.SessionIndex);
// 		auto errorCode = std::get<0>(pUserRet);
// 
// 		if (errorCode != ERROR_CODE::NONE) 
// 		{
// 			CHECK_ERROR(errorCode);
// 		}
// 
// 		auto pUser = std::get<1>(pUserRet);
// 		
// 			m_pRefLogger->Write(LOG_TYPE::L_INFO, "%s | Check Domain Start", __FUNCTION__);
// 		//유저의 상대가 룸인지 체크
// 		if (pUser->IsCurDomainInRoom() == false) 
// 		{
// 			CHECK_ERROR(ERROR_CODE::ROOM_CHAT_INVALID_DOMAIN);
// 		}
// 
// 			m_pRefLogger->Write(LOG_TYPE::L_INFO, "%s | Check GetLobby Start", __FUNCTION__);
// 		//유저가 있는 로비가 존재하는지를 검사.
// 		auto lobbyIndex = pUser->GetLobbyIndex();
// 		auto pLobby = m_pRefLobbyMgr->GetLobby(lobbyIndex);
// 		if (pLobby == nullptr) 
// 		{
// 			CHECK_ERROR(ERROR_CODE::ROOM_CHAT_INVALID_LOBBY_INDEX);
// 		}
// 
// 			m_pRefLogger->Write(LOG_TYPE::L_INFO, "%s | Check GetRoom Start", __FUNCTION__);
// 		//유저가 있는 룸이 존재하는지 검사
// 		auto pRoom = pLobby->GetRoom(pUser->GetRoomIndex());
// 		if (pRoom == nullptr)
// 		{
// 			CHECK_ERROR(ERROR_CODE::ROOM_ENTER_INVALID_ROOM_INDEX);
// 		}
// 
// 			m_pRefLogger->Write(LOG_TYPE::L_INFO, "%s | NotifyChat Start", __FUNCTION__);
// 		//NotifyChat
// 		pRoom->NotifyChat(pUser->GetSessioIndex(), pUser->GetID().c_str(), reqPkt->Msg);
// 			
// 			m_pRefLogger->Write(LOG_TYPE::L_INFO, "%s | SendData Start", __FUNCTION__);
// 		//SendData
// 		m_pRefNetwork->SendData(packetInfo.SessionIndex, (short)PACKET_ID::ROOM_CHAT_RES, sizeof(resPkt), (char*)&resPkt);
// 		
// 			m_pRefLogger->Write(LOG_TYPE::L_INFO, "%s | return", __FUNCTION__);
// 		return ERROR_CODE::NONE;
// 
// 	CHECK_ERR:
// 		resPkt.SetError(__result);
// 		m_pRefNetwork->SendData(packetInfo.SessionIndex, (short)PACKET_ID::ROOM_CHAT_RES, sizeof(resPkt), (char*)&resPkt);
// 		return (ERROR_CODE)__result;
// 	}
// 
// 	//룸에 있는 유저의 목록을 표시해줌
// 	ERROR_CODE PacketProcess::RoomUserList(PacketInfo packetInfo)
// 	{
// 		CHECK_START
// 			// 현재 룸에 있는지 조사한다.
// 			// 유저 리스트를 보내준다.
// 			m_pRefLogger->Write(LOG_TYPE::L_INFO, "%s | Check Start", __FUNCTION__);
// 
// 		//유저를 받아옴
// 		auto pUserRet = m_pRefUserMgr->GetUser(packetInfo.SessionIndex);
// 		auto errorCode = std::get<0>(pUserRet);
// 
// 		if (errorCode != ERROR_CODE::NONE) 
// 		{
// 			CHECK_ERROR(errorCode);
// 		}
// 
// 			m_pRefLogger->Write(LOG_TYPE::L_INFO, "%s | User Check End", __FUNCTION__);
// 
// 		//유저가 룸에 있는지 검사
// 		auto pUser = std::get<1>(pUserRet);
// 		
// 		if (pUser->IsCurDomainInRoom() == false) 
// 		{
// 			CHECK_ERROR(ERROR_CODE::LOBBY_USER_LIST_INVALID_DOMAIN);
// 		}
// 			m_pRefLogger->Write(LOG_TYPE::L_INFO, "%s | Domain Check End", __FUNCTION__);
// 
// 
// 		auto reqPkt = (NCommon::PktRoomUserListReq*)packetInfo.pRefData;
// 
// 			m_pRefLogger->Write(LOG_TYPE::L_INFO, "%s | reqPkt End", __FUNCTION__);
// 
// 
// 			m_pRefLogger->Write(LOG_TYPE::L_INFO, "%s | Start GetRoom, room index %d", __FUNCTION__, pUser->GetRoomIndex());
// 
// 		auto pRoom = m_pRefRoomMgr->GetRoom(pUser->GetRoomIndex()); //error here 
// 
// 			m_pRefLogger->Write(LOG_TYPE::L_INFO, "%s | GetRoom End", __FUNCTION__);
// 
// 
// 		auto pRoomRet = pRoom->SendUserList(pUser->GetSessioIndex(), reqPkt->StartUserIndex); 
// 		
// 		if (pRoomRet != ERROR_CODE::NONE)
// 		{
// 			CHECK_ERROR(pRoomRet)
// 		}
// 			m_pRefLogger->Write(LOG_TYPE::L_INFO, "%s | SendUserList End", __FUNCTION__);
// 
// 		return ERROR_CODE::NONE;
// 
// 	CHECK_ERR:
// 		NCommon::PktLobbyUserListRes resPkt;
// 		resPkt.SetError(__result);
// 		m_pRefNetwork->SendData(packetInfo.SessionIndex, (short)PACKET_ID::ROOM_ENTER_USER_LIST_RES, sizeof(NCommon::PktBase), (char*)&resPkt);
// 		return (ERROR_CODE)__result;
// 	}
// 
// 
// 
// 
// 	//룸에 있는 유저의 목록을 표시해줌
// 	ERROR_CODE PacketProcess::RoomUserList(PacketInfo packetInfo)
// 	{
// 		CHECK_START
// 			// 현재 룸에 있는지 조사한다.
// 			// 유저 리스트를 보내준다.
// 
// 		//유저를 받아옴
// 		auto pUserRet = m_pRefUserMgr->GetUser(packetInfo.SessionIndex);
// 		auto errorCode = std::get<0>(pUserRet);
// 
// 		if (errorCode != ERROR_CODE::NONE)
// 		{
// 			CHECK_ERROR(errorCode);
// 		}
// 
// 		//유저가 룸에 있는지 검사
// 		auto pUser = std::get<1>(pUserRet);
// 
// 		if (pUser->IsCurDomainInRoom() == false)
// 		{
// 			CHECK_ERROR(ERROR_CODE::LOBBY_USER_LIST_INVALID_DOMAIN);
// 		}
// 
// 
// 		auto reqPkt = (NCommon::PktRoomUserListReq*)packetInfo.pRefData;
// 
// 		auto pRoom = m_pRefRoomMgr->GetRoom(pUser->GetRoomIndex()); //error here 
// 
// 
// 		auto pRoomRet = pRoom->SendUserList(pUser->GetSessioIndex(), reqPkt->StartUserIndex);
// 
// 		if (pRoomRet != ERROR_CODE::NONE)
// 		{
// 			CHECK_ERROR(pRoomRet)
// 		}
// 
// 		return ERROR_CODE::NONE;
// 
// 	CHECK_ERR:
// 		NCommon::PktLobbyUserListRes resPkt;
// 		resPkt.SetError(__result);
// 		m_pRefNetwork->SendData(packetInfo.SessionIndex, (short)PACKET_ID::ROOM_ENTER_USER_LIST_RES, sizeof(NCommon::PktBase), (char*)&resPkt);
// 		return (ERROR_CODE)__result;
// 	}
// 
// 
// 
// 
// 	ERROR_CODE PacketProcess::RoomMasterGameStart(PacketInfo packetInfo)
// 	{
// 		CHECK_START
// 
// 			auto reqPkt = (NCommon::PktRoomChatReq*)packetInfo.pRefData;
// 		NCommon::PktRoomChatRes resPkt;
// 
// 		//유저를 받아옴.
// 		auto pUserRet = m_pRefUserMgr->GetUser(packetInfo.SessionIndex);
// 		auto errorCode = std::get<0>(pUserRet);
// 
// 		if (errorCode != ERROR_CODE::NONE)
// 		{
// 			CHECK_ERROR(errorCode);
// 		}
// 
// 		auto pUser = std::get<1>(pUserRet);
// 
// 		//유저의 상대가 룸인지 체크
// 		if (pUser->IsCurDomainInRoom() == false)
// 		{
// 			CHECK_ERROR(ERROR_CODE::ROOM_CHAT_INVALID_DOMAIN);
// 		}
// 
// 		//유저가 있는 로비가 존재하는지를 검사.
// 		auto lobbyIndex = pUser->GetLobbyIndex();
// 		auto pLobby = m_pRefLobbyMgr->GetLobby(lobbyIndex);
// 		if (pLobby == nullptr)
// 		{
// 			CHECK_ERROR(ERROR_CODE::ROOM_CHAT_INVALID_LOBBY_INDEX);
// 		}
// 
// 		//유저가 있는 룸이 존재하는지 검사
// 		auto pRoom = pLobby->GetRoom(pUser->GetRoomIndex());
// 		if (pRoom == nullptr)
// 		{
// 			CHECK_ERROR(ERROR_CODE::ROOM_ENTER_INVALID_ROOM_INDEX);
// 		}
// 
// 		//방장이 맞는지 확인
// 		if (pRoom->IsMaster(pUser->GetIndex) == false)
// 		{
// 			CHECK_ERROR(ERROR_CODE::ROOM_GAME_USER_INVALID_MASTER)
// 		}
// 
// 		//방의 인원수가 2명인가
// 		if (pRoom->GetUserCount() != 2)
// 		{
// 			CHECK_ERROR(ERROR_CODE::ROOM_GAME_USER_COUNT)
// 		}
// 
// 		//방의 상태가 게임을 안하는중인지?
// 		if (pRoom->GetGameObj()->NowState() == GameState::NONE)
// 		{
// 			CHECK_ERROR(ERROR_CODE::ROOM_GAME_STATE)
// 		}
// 
// 		//로비의 유저에게 방의 상태 변경 통보
// 
// 
// 
// 		//방의 다른 유저에게 방장이 게임 시작 요청을 했음을 알리고
// 		pRoom->GetGameObj()->SetState(GameState::STARTING);
// 		
// 		//요청자에게 답편을 보낸다
// 		pRoom->SendToAllUser((short)PACKET_ID::ROOM_MASTER_GAME_STATE_RES, 0, nullptr, pUser->GetIndex);
// 
// 
// 
// 		//SendData
// 		m_pRefNetwork->SendData(packetInfo.SessionIndex, (short)PACKET_ID::ROOM_MASTER_GAME_STATE_RES, sizeof(resPkt), (char*)&resPkt);
// 
// 		return ERROR_CODE::NONE;
// 
// 	CHECK_ERR:
// 		resPkt.SetError(__result);
// 		m_pRefNetwork->SendData(packetInfo.SessionIndex, (short)PACKET_ID::ROOM_MASTER_GAME_STATE_RES, sizeof(resPkt), (char*)&resPkt);
// 		return (ERROR_CODE)__result;
// 	}
// 
// 
// 
// 	ERROR_CODE PacketProcess::RoomGameStart(PacketInfo packetInfo)
// 	{
// 		CHECK_START
// 
// 		auto reqPkt = (NCommon::PktRoomChatReq*)packetInfo.pRefData;
// 		NCommon::PktRoomChatRes resPkt;
// 
// 		//유저를 받아옴.
// 		auto pUserRet = m_pRefUserMgr->GetUser(packetInfo.SessionIndex);
// 		auto errorCode = std::get<0>(pUserRet);
// 
// 		if (errorCode != ERROR_CODE::NONE)
// 		{
// 			CHECK_ERROR(errorCode);
// 		}
// 
// 		auto pUser = std::get<1>(pUserRet);
// 
// 		//유저의 상대가 룸인지 체크
// 		if (pUser->IsCurDomainInRoom() == false)
// 		{
// 			CHECK_ERROR(ERROR_CODE::ROOM_CHAT_INVALID_DOMAIN);
// 		}
// 
// 		//유저가 있는 로비가 존재하는지를 검사.
// 		auto lobbyIndex = pUser->GetLobbyIndex();
// 		auto pLobby = m_pRefLobbyMgr->GetLobby(lobbyIndex);
// 		if (pLobby == nullptr)
// 		{
// 			CHECK_ERROR(ERROR_CODE::ROOM_CHAT_INVALID_LOBBY_INDEX);
// 		}
// 
// 		//유저가 있는 룸이 존재하는지 검사
// 		auto pRoom = pLobby->GetRoom(pUser->GetRoomIndex());
// 		if (pRoom == nullptr)
// 		{
// 			CHECK_ERROR(ERROR_CODE::ROOM_ENTER_INVALID_ROOM_INDEX);
// 		}
// 
// 		//방장이 맞는지 확인
// 		if (pRoom->IsMaster(pUser->GetIndex) == false)
// 		{
// 			CHECK_ERROR(ERROR_CODE::ROOM_GAME_USER_INVALID_MASTER)
// 		}
// 
// 		//방의 인원수가 2명인가
// 		if (pRoom->GetUserCount() != 2)
// 		{
// 			CHECK_ERROR(ERROR_CODE::ROOM_GAME_USER_COUNT)
// 		}
// 
// 		//방의 상태가 게임을 안하는중인지?
// 		if (pRoom->GetGameObj()->NowState() != GameState::STARTING)
// 		{
// 			CHECK_ERROR(ERROR_CODE::ROOM_GAME_STATE)
// 		}
// 
// 
// 		//이미 게임 시작 요청을 했는가?
// 
// 
// 		//방에서 게임 시작 요청한 유저 리스트에 등록
// 		
// 
// 		//로비의 유저에게 방의 상태 변경 통보
// 		pRoom->GetGameObj()->SetState(GameState::STARTING);
// 
// 
// 		//방의 다른 유저에게 방장이 게임 시작 요청을 했음을 알리고
// 	
// 		
// 		//요청자에게 답편을 보낸다
// 		m_pRefNetwork->SendData(packetInfo.SessionIndex, (short)PACKET_ID::ROOM_GAME_STATE_RES, sizeof(resPkt), (char*)&resPkt);
// 
// 		//게임 시작 가증한가
// 		//시작이면 게임 상태 변경
// 		//게임 시작 패킷 보내기
// 
// 
// 
// 
// 
// 		return ERROR_CODE::NONE;
// 
// 	CHECK_ERR:
// 		resPkt.SetError(__result);
// 		m_pRefNetwork->SendData(packetInfo.SessionIndex, (short)PACKET_ID::ROOM_GAME_STATE_RES, sizeof(resPkt), (char*)&resPkt);
// 		return (ERROR_CODE)__result;
// 	}
// }