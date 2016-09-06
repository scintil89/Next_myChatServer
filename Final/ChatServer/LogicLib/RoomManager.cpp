// #include "../ServerNetLib/ILog.h"
// #include "../ServerNetLib/TcpNetwork.h"
// #include "../../Common/Packet.h"
// #include "../../Common/ErrorCode.h"
// 
// #include "Room.h"
// #include "RoomManager.h"
// 
// using ERROR_CODE = NCommon::ERROR_CODE;
// using PACKET_ID = NCommon::PACKET_ID;
// 
// namespace NLogicLib
// {
// 	RoomManager::RoomManager() {}
// 
// 	RoomManager::~RoomManager() {}
// 
// 
// 	void RoomManager::Init(const RoomManagerConfig config, TcpNet* pNetwork, ILog* pLogger)
// 	{
// 		m_pRefLogger = pLogger;
// 		m_pRefNetwork = pNetwork;
// 	}
// 
// 	Room* RoomManager::GetRoom(short RoomId)
// 	{
// 		if (RoomId < 0 || RoomId >= (short)m_RoomList.size()) 
// 		{
// 			return nullptr;
// 		}
// 
// 		return m_RoomList[RoomId];
// 	}
// 
// 	void RoomManager::CreateRoom(Room* room)
// 	{
// 		room->SetNetwork(m_pRefNetwork, m_pRefLogger);
// 		m_RoomList.push_back(room);
// 	}
// }