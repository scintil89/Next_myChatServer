#include "../../Common/Packet.h"
#include "../ServerNetLib/ILog.h"
#include "../ServerNetLib/TcpNetwork.h"
#include "../../Common/ErrorCode.h"
#include "User.h"
#include "UserManager.h"
#include "LobbyManager.h"
#include "RoomManager.h"
#include "Lobby.h"
#include "Room.h"
#include "Game.h"
#include "PacketProcess.h"

using PACKET_ID = NCommon::PACKET_ID;
using LOG_TYPE = NServerNetLib::LOG_TYPE;

namespace NLogicLib
{
	ERROR_CODE PacketProcess::RoomEnter(PacketInfo packetInfo)
	{
	CHECK_START
		auto reqPkt = (NCommon::PktRoomEnterReq*)packetInfo.pRefData;
		NCommon::PktRoomEnterRes resPkt;

		auto pUserRet = m_pRefUserMgr->GetUser(packetInfo.SessionIndex);
		auto errorCode = std::get<0>(pUserRet);

		if (errorCode != ERROR_CODE::NONE) {
			CHECK_ERROR(errorCode);
		}

		auto pUser = std::get<1>(pUserRet);

		if (pUser->IsCurDomainInLobby() == false) {
			CHECK_ERROR(ERROR_CODE::ROOM_ENTER_INVALID_DOMAIN);
		}

		auto lobbyIndex = pUser->GetLobbyIndex();
		auto pLobby = m_pRefLobbyMgr->GetLobby(lobbyIndex);
		if (pLobby == nullptr) {
			CHECK_ERROR(ERROR_CODE::ROOM_ENTER_INVALID_LOBBY_INDEX);
		}
		
// 		auto pRoom = pLobby->GetRoom(reqPkt->RoomIndex);
// 
// 		if (pRoom == nullptr) 
// 		{
// 			CHECK_ERROR(ERROR_CODE::ROOM_ENTER_INVALID_ROOM_INDEX);
// 		}
// 
// 		// ���� ����� ����� ���� �����
// 		if (reqPkt->IsCreate) 
// 		{
// 			pRoom = pLobby->CreateRoom();
//			if (pRoom == nullptr) 
//			{
//				CHECK_ERROR(ERROR_CODE::ROOM_ENTER_EMPTY_ROOM);
//			}
// 
// 			auto ret = pRoom->CreateRoom(reqPkt->RoomTitle);
// 			if (ret != ERROR_CODE::NONE) {
// 				CHECK_ERROR(ret);
// 			}
// 		}
// 
// 		auto enterRet = pRoom->EnterUser(pUser);
// 		if (enterRet != ERROR_CODE::NONE) {
// 			CHECK_ERROR(enterRet);
// 		}

		Room* pRoom = nullptr;

		// ���� ����� ����� ���� �����
		if (reqPkt->IsCreate)
		{
			pRoom = pLobby->CreateRoom();
			if (pRoom == nullptr) {
				CHECK_ERROR(ERROR_CODE::ROOM_ENTER_EMPTY_ROOM);
			}

			auto ret = pRoom->CreateRoom(reqPkt->RoomTitle);
			if (ret != ERROR_CODE::NONE) {
				CHECK_ERROR(ret);
			}

			m_pRefRoomMgr->CreateRoom(pRoom);
		}
		else
		{
			pRoom = pLobby->GetRoom(reqPkt->RoomIndex);
			if (pRoom == nullptr) {
				CHECK_ERROR(ERROR_CODE::ROOM_ENTER_INVALID_ROOM_INDEX);
			}
		}

		auto enterRet = pRoom->EnterUser(pUser);
		if (enterRet != ERROR_CODE::NONE) {
			CHECK_ERROR(enterRet);
		}		
		// ���� ������ �뿡 ���Դٰ� �����Ѵ�.
		pUser->EnterRoom(lobbyIndex, pRoom->GetIndex());

		// �κ� ������ �������� �˸���
		pLobby->NotifyLobbyLeaveUserInfo(pUser);
		
		// �κ� �� ������ �뺸�Ѵ�.
		pLobby->NotifyChangedRoomInfo(pRoom->GetIndex());

		// �뿡 �� ���� ���Դٰ� �˸���
		pRoom->NotifyEnterUserInfo(pUser->GetIndex(), pUser->GetID().c_str());
		
		m_pRefNetwork->SendData(packetInfo.SessionIndex, (short)PACKET_ID::ROOM_ENTER_RES, sizeof(resPkt), (char*)&resPkt);
		return ERROR_CODE::NONE;

	CHECK_ERR:
		resPkt.SetError(__result);
		m_pRefNetwork->SendData(packetInfo.SessionIndex, (short)PACKET_ID::ROOM_ENTER_RES, sizeof(resPkt), (char*)&resPkt);
		return (ERROR_CODE)__result;
	}

	ERROR_CODE PacketProcess::RoomLeave(PacketInfo packetInfo)
	{
	CHECK_START
		NCommon::PktRoomLeaveRes resPkt;

		auto pUserRet = m_pRefUserMgr->GetUser(packetInfo.SessionIndex);
		auto errorCode = std::get<0>(pUserRet);

		if (errorCode != ERROR_CODE::NONE) {
			CHECK_ERROR(errorCode);
		}

		auto pUser = std::get<1>(pUserRet);
		auto userIndex = pUser->GetIndex();

		if (pUser->IsCurDomainInRoom() == false) {
			CHECK_ERROR(ERROR_CODE::ROOM_LEAVE_INVALID_DOMAIN);
		}

		auto lobbyIndex = pUser->GetLobbyIndex();
		auto pLobby = m_pRefLobbyMgr->GetLobby(lobbyIndex);
		if (pLobby == nullptr) {
			CHECK_ERROR(ERROR_CODE::ROOM_ENTER_INVALID_LOBBY_INDEX);
		}

		auto pRoom = pLobby->GetRoom(pUser->GetRoomIndex());
		if (pRoom == nullptr) {
			CHECK_ERROR(ERROR_CODE::ROOM_ENTER_INVALID_ROOM_INDEX);
		}

		auto leaveRet = pRoom->LeaveUser(userIndex);
		if (leaveRet != ERROR_CODE::NONE) {
			CHECK_ERROR(leaveRet);
		}
		
		// ���� ������ �κ�� ����
		pUser->EnterLobby(lobbyIndex);

		// �뿡 ������ �������� �뺸
		pRoom->NotifyLeaveUserInfo(pUser->GetID().c_str());

		// �κ� ���ο� ������ �������� �뺸
		pLobby->NotifyLobbyEnterUserInfo(pUser);

		// �κ� �ٲ� �� ������ �뺸
		pLobby->NotifyChangedRoomInfo(pRoom->GetIndex());
		
		m_pRefNetwork->SendData(packetInfo.SessionIndex, (short)PACKET_ID::ROOM_LEAVE_RES, sizeof(resPkt), (char*)&resPkt);
		return ERROR_CODE::NONE;

	CHECK_ERR:
		resPkt.SetError(__result);
		m_pRefNetwork->SendData(packetInfo.SessionIndex, (short)PACKET_ID::ROOM_LEAVE_RES, sizeof(resPkt), (char*)&resPkt);
		return (ERROR_CODE)__result;
	}

	//��ä��
	ERROR_CODE PacketProcess::RoomChat(PacketInfo packetInfo)
	{
	CHECK_START
			m_pRefLogger->Write(LOG_TYPE::L_INFO, "%s | Check Start", __FUNCTION__);
		auto reqPkt = (NCommon::PktRoomChatReq*)packetInfo.pRefData;
		NCommon::PktRoomChatRes resPkt;
		
			m_pRefLogger->Write(LOG_TYPE::L_INFO, "%s | GetUser Start", __FUNCTION__);
		//������ �޾ƿ�.
		auto pUserRet = m_pRefUserMgr->GetUser(packetInfo.SessionIndex);
		auto errorCode = std::get<0>(pUserRet);

		if (errorCode != ERROR_CODE::NONE) 
		{
			CHECK_ERROR(errorCode);
		}

		auto pUser = std::get<1>(pUserRet);
		
			m_pRefLogger->Write(LOG_TYPE::L_INFO, "%s | Check Domain Start", __FUNCTION__);
		//������ ��밡 ������ üũ
		if (pUser->IsCurDomainInRoom() == false) 
		{
			CHECK_ERROR(ERROR_CODE::ROOM_CHAT_INVALID_DOMAIN);
		}

			m_pRefLogger->Write(LOG_TYPE::L_INFO, "%s | Check GetLobby Start", __FUNCTION__);
		//������ �ִ� �κ� �����ϴ����� �˻�.
		auto lobbyIndex = pUser->GetLobbyIndex();
		auto pLobby = m_pRefLobbyMgr->GetLobby(lobbyIndex);
		if (pLobby == nullptr) 
		{
			CHECK_ERROR(ERROR_CODE::ROOM_CHAT_INVALID_LOBBY_INDEX);
		}

			m_pRefLogger->Write(LOG_TYPE::L_INFO, "%s | Check GetRoom Start", __FUNCTION__);
		//������ �ִ� ���� �����ϴ��� �˻�
		auto pRoom = pLobby->GetRoom(pUser->GetRoomIndex());
		if (pRoom == nullptr)
		{
			CHECK_ERROR(ERROR_CODE::ROOM_ENTER_INVALID_ROOM_INDEX);
		}

			m_pRefLogger->Write(LOG_TYPE::L_INFO, "%s | NotifyChat Start", __FUNCTION__);
		//NotifyChat
		pRoom->NotifyChat(pUser->GetSessioIndex(), pUser->GetID().c_str(), reqPkt->Msg);
			
			m_pRefLogger->Write(LOG_TYPE::L_INFO, "%s | SendData Start", __FUNCTION__);
		//SendData
		m_pRefNetwork->SendData(packetInfo.SessionIndex, (short)PACKET_ID::ROOM_CHAT_RES, sizeof(resPkt), (char*)&resPkt);
		
			m_pRefLogger->Write(LOG_TYPE::L_INFO, "%s | return", __FUNCTION__);
		return ERROR_CODE::NONE;

	CHECK_ERR:
		resPkt.SetError(__result);
		m_pRefNetwork->SendData(packetInfo.SessionIndex, (short)PACKET_ID::ROOM_CHAT_RES, sizeof(resPkt), (char*)&resPkt);
		return (ERROR_CODE)__result;
	}

	//�뿡 �ִ� ������ ����� ǥ������
	ERROR_CODE PacketProcess::RoomUserList(PacketInfo packetInfo)
	{
		CHECK_START
			// ���� �뿡 �ִ��� �����Ѵ�.
			// ���� ����Ʈ�� �����ش�.
			m_pRefLogger->Write(LOG_TYPE::L_INFO, "%s | Check Start", __FUNCTION__);

		//������ �޾ƿ�
		auto pUserRet = m_pRefUserMgr->GetUser(packetInfo.SessionIndex);
		auto errorCode = std::get<0>(pUserRet);

		if (errorCode != ERROR_CODE::NONE) 
		{
			CHECK_ERROR(errorCode);
		}

			m_pRefLogger->Write(LOG_TYPE::L_INFO, "%s | User Check End", __FUNCTION__);

		//������ �뿡 �ִ��� �˻�
		auto pUser = std::get<1>(pUserRet);
		
		if (pUser->IsCurDomainInRoom() == false) 
		{
			CHECK_ERROR(ERROR_CODE::LOBBY_USER_LIST_INVALID_DOMAIN);
		}
			m_pRefLogger->Write(LOG_TYPE::L_INFO, "%s | Domain Check End", __FUNCTION__);


		auto reqPkt = (NCommon::PktRoomUserListReq*)packetInfo.pRefData;

			m_pRefLogger->Write(LOG_TYPE::L_INFO, "%s | reqPkt End", __FUNCTION__);


			m_pRefLogger->Write(LOG_TYPE::L_INFO, "%s | Start GetRoom, room index %d", __FUNCTION__, pUser->GetRoomIndex());

		auto pRoom = m_pRefRoomMgr->GetRoom(pUser->GetRoomIndex()); //error here 

			m_pRefLogger->Write(LOG_TYPE::L_INFO, "%s | GetRoom End", __FUNCTION__);


		auto pRoomRet = pRoom->SendUserList(pUser->GetSessioIndex(), reqPkt->StartUserIndex); 
		
		if (pRoomRet != ERROR_CODE::NONE)
		{
			CHECK_ERROR(pRoomRet)
		}
			m_pRefLogger->Write(LOG_TYPE::L_INFO, "%s | SendUserList End", __FUNCTION__);

		return ERROR_CODE::NONE;

	CHECK_ERR:
		NCommon::PktLobbyUserListRes resPkt;
		resPkt.SetError(__result);
		m_pRefNetwork->SendData(packetInfo.SessionIndex, (short)PACKET_ID::ROOM_ENTER_USER_LIST_RES, sizeof(NCommon::PktBase), (char*)&resPkt);
		return (ERROR_CODE)__result;
	}




	//�뿡 �ִ� ������ ����� ǥ������
	ERROR_CODE PacketProcess::RoomUserList(PacketInfo packetInfo)
	{
		CHECK_START
			// ���� �뿡 �ִ��� �����Ѵ�.
			// ���� ����Ʈ�� �����ش�.

		//������ �޾ƿ�
		auto pUserRet = m_pRefUserMgr->GetUser(packetInfo.SessionIndex);
		auto errorCode = std::get<0>(pUserRet);

		if (errorCode != ERROR_CODE::NONE)
		{
			CHECK_ERROR(errorCode);
		}

		//������ �뿡 �ִ��� �˻�
		auto pUser = std::get<1>(pUserRet);

		if (pUser->IsCurDomainInRoom() == false)
		{
			CHECK_ERROR(ERROR_CODE::LOBBY_USER_LIST_INVALID_DOMAIN);
		}


		auto reqPkt = (NCommon::PktRoomUserListReq*)packetInfo.pRefData;

		auto pRoom = m_pRefRoomMgr->GetRoom(pUser->GetRoomIndex()); //error here 


		auto pRoomRet = pRoom->SendUserList(pUser->GetSessioIndex(), reqPkt->StartUserIndex);

		if (pRoomRet != ERROR_CODE::NONE)
		{
			CHECK_ERROR(pRoomRet)
		}

		return ERROR_CODE::NONE;

	CHECK_ERR:
		NCommon::PktLobbyUserListRes resPkt;
		resPkt.SetError(__result);
		m_pRefNetwork->SendData(packetInfo.SessionIndex, (short)PACKET_ID::ROOM_ENTER_USER_LIST_RES, sizeof(NCommon::PktBase), (char*)&resPkt);
		return (ERROR_CODE)__result;
	}




	ERROR_CODE PacketProcess::RoomMasterGameStart(PacketInfo packetInfo)
	{
		CHECK_START

			auto reqPkt = (NCommon::PktRoomChatReq*)packetInfo.pRefData;
		NCommon::PktRoomChatRes resPkt;

		//������ �޾ƿ�.
		auto pUserRet = m_pRefUserMgr->GetUser(packetInfo.SessionIndex);
		auto errorCode = std::get<0>(pUserRet);

		if (errorCode != ERROR_CODE::NONE)
		{
			CHECK_ERROR(errorCode);
		}

		auto pUser = std::get<1>(pUserRet);

		//������ ��밡 ������ üũ
		if (pUser->IsCurDomainInRoom() == false)
		{
			CHECK_ERROR(ERROR_CODE::ROOM_CHAT_INVALID_DOMAIN);
		}

		//������ �ִ� �κ� �����ϴ����� �˻�.
		auto lobbyIndex = pUser->GetLobbyIndex();
		auto pLobby = m_pRefLobbyMgr->GetLobby(lobbyIndex);
		if (pLobby == nullptr)
		{
			CHECK_ERROR(ERROR_CODE::ROOM_CHAT_INVALID_LOBBY_INDEX);
		}

		//������ �ִ� ���� �����ϴ��� �˻�
		auto pRoom = pLobby->GetRoom(pUser->GetRoomIndex());
		if (pRoom == nullptr)
		{
			CHECK_ERROR(ERROR_CODE::ROOM_ENTER_INVALID_ROOM_INDEX);
		}

		//������ �´��� Ȯ��
		if (pRoom->IsMaster(pUser->GetIndex) == false)
		{
			CHECK_ERROR(ERROR_CODE::ROOM_GAME_USER_INVALID_MASTER)
		}

		//���� �ο����� 2���ΰ�
		if (pRoom->GetUserCount() != 2)
		{
			CHECK_ERROR(ERROR_CODE::ROOM_GAME_USER_COUNT)
		}

		//���� ���°� ������ ���ϴ�������?
		if (pRoom->GetGameObj()->NowState() == GameState::NONE)
		{
			CHECK_ERROR(ERROR_CODE::ROOM_GAME_STATE)
		}

		//�κ��� �������� ���� ���� ���� �뺸



		//���� �ٸ� �������� ������ ���� ���� ��û�� ������ �˸���
		pRoom->GetGameObj()->SetState(GameState::STARTING);
		
		//��û�ڿ��� ������ ������
		pRoom->SendToAllUser((short)PACKET_ID::ROOM_MASTER_GAME_STATE_RES, 0, nullptr, pUser->GetIndex);



		//SendData
		m_pRefNetwork->SendData(packetInfo.SessionIndex, (short)PACKET_ID::ROOM_MASTER_GAME_STATE_RES, sizeof(resPkt), (char*)&resPkt);

		return ERROR_CODE::NONE;

	CHECK_ERR:
		resPkt.SetError(__result);
		m_pRefNetwork->SendData(packetInfo.SessionIndex, (short)PACKET_ID::ROOM_MASTER_GAME_STATE_RES, sizeof(resPkt), (char*)&resPkt);
		return (ERROR_CODE)__result;
	}



	ERROR_CODE PacketProcess::RoomGameStart(PacketInfo packetInfo)
	{
		CHECK_START

		auto reqPkt = (NCommon::PktRoomChatReq*)packetInfo.pRefData;
		NCommon::PktRoomChatRes resPkt;

		//������ �޾ƿ�.
		auto pUserRet = m_pRefUserMgr->GetUser(packetInfo.SessionIndex);
		auto errorCode = std::get<0>(pUserRet);

		if (errorCode != ERROR_CODE::NONE)
		{
			CHECK_ERROR(errorCode);
		}

		auto pUser = std::get<1>(pUserRet);

		//������ ��밡 ������ üũ
		if (pUser->IsCurDomainInRoom() == false)
		{
			CHECK_ERROR(ERROR_CODE::ROOM_CHAT_INVALID_DOMAIN);
		}

		//������ �ִ� �κ� �����ϴ����� �˻�.
		auto lobbyIndex = pUser->GetLobbyIndex();
		auto pLobby = m_pRefLobbyMgr->GetLobby(lobbyIndex);
		if (pLobby == nullptr)
		{
			CHECK_ERROR(ERROR_CODE::ROOM_CHAT_INVALID_LOBBY_INDEX);
		}

		//������ �ִ� ���� �����ϴ��� �˻�
		auto pRoom = pLobby->GetRoom(pUser->GetRoomIndex());
		if (pRoom == nullptr)
		{
			CHECK_ERROR(ERROR_CODE::ROOM_ENTER_INVALID_ROOM_INDEX);
		}

		//������ �´��� Ȯ��
		if (pRoom->IsMaster(pUser->GetIndex) == false)
		{
			CHECK_ERROR(ERROR_CODE::ROOM_GAME_USER_INVALID_MASTER)
		}

		//���� �ο����� 2���ΰ�
		if (pRoom->GetUserCount() != 2)
		{
			CHECK_ERROR(ERROR_CODE::ROOM_GAME_USER_COUNT)
		}

		//���� ���°� ������ ���ϴ�������?
		if (pRoom->GetGameObj()->NowState() != GameState::STARTING)
		{
			CHECK_ERROR(ERROR_CODE::ROOM_GAME_STATE)
		}


		//�̹� ���� ���� ��û�� �ߴ°�?


		//�濡�� ���� ���� ��û�� ���� ����Ʈ�� ���
		

		//�κ��� �������� ���� ���� ���� �뺸
		pRoom->GetGameObj()->SetState(GameState::STARTING);


		//���� �ٸ� �������� ������ ���� ���� ��û�� ������ �˸���
	
		
		//��û�ڿ��� ������ ������
		m_pRefNetwork->SendData(packetInfo.SessionIndex, (short)PACKET_ID::ROOM_GAME_STATE_RES, sizeof(resPkt), (char*)&resPkt);

		//���� ���� �����Ѱ�
		//�����̸� ���� ���� ����
		//���� ���� ��Ŷ ������





		return ERROR_CODE::NONE;

	CHECK_ERR:
		resPkt.SetError(__result);
		m_pRefNetwork->SendData(packetInfo.SessionIndex, (short)PACKET_ID::ROOM_GAME_STATE_RES, sizeof(resPkt), (char*)&resPkt);
		return (ERROR_CODE)__result;
	}
}