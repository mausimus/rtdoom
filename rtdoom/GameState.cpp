#include "pch.h"
#include "rtdoom.h"
#include "GameState.h"
#include "Projection.h"

using std::string;

namespace rtdoom
{
	GameState::GameState() :
		m_player{ 0, 0, 0, 0 },
		m_mapDef{ nullptr }
	{
	}

	void GameState::NewGame(const string& mapFolder)
	{
		m_mapDef = std::make_unique<MapDef>(mapFolder);
		m_player = m_mapDef->GetStartingPosition();
	}

	void GameState::Move(int m, int r, float step)
	{
		m_player.a += step * 25 * 0.15f * r;
		m_player.x += step * 25 * 10.0f * m * cos(m_player.a);
		m_player.y += step * 25 * 10.0f * m * sin(m_player.a);

		m_player.a = Projection::NormalizeAngle(m_player.a);
	}

	GameState::~GameState()
	{
	}
}