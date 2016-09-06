#pragma once

#include <time.h>
#include <chrono>
#include <vector>
#include "../ServerNetLib/ILog.h"
#include "../ServerNetLib/TcpNetwork.h"

namespace NLogicLib
{
	struct ConnectedUser
	{
		void Clear()
		{
			m_IsLoginSuccess = false;
			m_ConnectedTime = 0;
		}

		bool m_IsLoginSuccess = false;
		time_t m_ConnectedTime = 0;
	};

	//접속 후 로그인 안하는 유저를 강제로 정리함
	class ConnectedUserManager
	{
		using TcpNet = NServerNetLib::ITcpNetwork;
		using ILog = NServerNetLib::ILog;
	public:
		ConnectedUserManager() {}
		virtual ~ConnectedUserManager() {}

		void Init(const int maxSessionCount, TcpNet* pNetwork, ILog* pLogger)
		{
			m_pRefLogger = pLogger;
			m_pRefNetwork = pNetwork;

			for (int i = 0; i < maxSessionCount; ++i)
			{
				ConnectedUserList.emplace_back(ConnectedUser());
			}
		}

		void SetConnectSession(const int sessionIndex)
		{
			time(&ConnectedUserList[sessionIndex].m_ConnectedTime);
		}

		void SetLogin(const int sessionIndex)
		{
			ConnectedUserList[sessionIndex].m_IsLoginSuccess = true;
		}

		void SetDisConnectSession(const int sessionIndex)
		{
			ConnectedUserList[sessionIndex].Clear();
		}

		void LoginCheck()
		{
			//현재시간 체크
			auto curTime = std::chrono::system_clock::now();
			auto diffTime = std::chrono::duration_cast<std::chrono::milliseconds>(curTime - m_LatestLoginCheckTime);

			// 60밀리초 마다 검사
			if (diffTime.count() < 60)
			{
				return;
			}
			else
			{
				m_LatestLoginCheckTime = curTime;
			}

			auto curSecTime = std::chrono::system_clock::to_time_t(curTime);

			
			const auto maxSessionCount = (int)ConnectedUserList.size();
			
			if (m_LatestLogincheckIndex >= maxSessionCount) {
				m_LatestLogincheckIndex = -1;
			}

			++m_LatestLogincheckIndex;

			auto lastCheckIndex = m_LatestLogincheckIndex + 100;
			if (lastCheckIndex > maxSessionCount) {
				lastCheckIndex = maxSessionCount;
			}

			for (; m_LatestLogincheckIndex < lastCheckIndex; ++m_LatestLogincheckIndex)
			{
				auto i = m_LatestLogincheckIndex;

				if (ConnectedUserList[i].m_ConnectedTime == 0 ||
					ConnectedUserList[i].m_IsLoginSuccess)
				{
					continue;
				}

				auto diff = curSecTime - ConnectedUserList[i].m_ConnectedTime;
				//접속 후 3초동안 로그인 안하면 악의적 유저로 간주
				if (diff >= 20) //3초는 너무 짧아서 20초로 변경
				{
					m_pRefLogger->Write(NServerNetLib::LOG_TYPE::L_WARN, "%s | Login Wait Time Over. sessionIndex(%d).", __FUNCTION__, i);
					m_pRefNetwork->ForcingClose(i);
				}
			}
		}

	private:
		ILog* m_pRefLogger;
		TcpNet* m_pRefNetwork;

		std::vector<ConnectedUser> ConnectedUserList;

		std::chrono::system_clock::time_point m_LatestLoginCheckTime = std::chrono::system_clock::now();
		int m_LatestLogincheckIndex = -1;
	};
}