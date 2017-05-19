#pragma once
#include <chrono>

namespace NLogicLib
{
	enum class GameState
	{
		NONE,
		STARTING,
		ING,
		END
	};

	class CGame
	{
	public:
		CGame();
		virtual ~CGame();

		void Clear();

		GameState NowState();

		void SetState(const GameState state);

	private:
		GameState m_state;
	};


	std::chrono::system_clock::time_point m_SelectTime;

	int m_GameSellect1;

	int m_GameSellect2;
} 