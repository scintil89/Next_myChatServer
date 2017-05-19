#include "Game.h"



NLogicLib::CGame::CGame()
{
}


NLogicLib::CGame::~CGame()
{
}

void NLogicLib::CGame::Clear()
{
	m_state = GameState::NONE;
}

NLogicLib::GameState NLogicLib::CGame::NowState()
{
	return m_state;
}

void NLogicLib::CGame::SetState(const GameState state)
{
	m_state = state;
}
