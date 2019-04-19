#pragma once

#include "MapDef.h"

namespace rtdoom
{
class GameState
{
public:
    std::unique_ptr<MapDef> m_mapDef;
    float                   m_step;
    Thing                   m_player;

    void Move(int m, int r, float step);
    void NewGame(const MapStore& mapStore);

    GameState();
    ~GameState();
};
} // namespace rtdoom
