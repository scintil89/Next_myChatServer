#pragma once
#include <vector>
#include <unordered_map>

///////////////////////////////////////////////////////////////////////////
//LobbyManager¸¦ ¼öÁ¤

namespace NServerNetLib
{
	class TcpNetwork;
}

namespace NServerNetLib
{
	class ILog;
}

namespace NLogicLib
{
	struct RoomManagerConfig
	{
		int MaxRoomCount;
		int MaxRoomUserCount;
	};

	struct RoomSmallInfo
	{
		short Num;
		short UserCount;
	};

	class Room;

	class RoomManager
	{
		using TcpNet = NServerNetLib::ITcpNetwork;
		using ILog = NServerNetLib::ILog;

	public:
		RoomManager();
		virtual ~RoomManager();

		void Init(const RoomManagerConfig config, TcpNet* pNetwork, ILog* pLogger);

		Room* GetRoom(short RoomId);

		void CreateRoom(Room* room);

	private:
		ILog* m_pRefLogger;
		TcpNet* m_pRefNetwork;

		std::vector<Room*> m_RoomList;
	};
}