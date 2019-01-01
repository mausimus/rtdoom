#pragma once

#include "MapDef.h"

namespace rtdoom
{
	class GameState
	{
	public:
		std::unique_ptr<MapDef> m_mapDef;
		Thing m_player;

		void Move(int m, int r, float step);
		void NewGame(const std::string& mapFolder);

		GameState();
		~GameState();
	};
}