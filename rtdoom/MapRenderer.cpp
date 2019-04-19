#include "pch.h"
#include "rtdoom.h"
#include "MapRenderer.h"

namespace rtdoom
{
MapRenderer::MapRenderer(const GameState& gameState) : Renderer {gameState} {}

void MapRenderer::MapToScreenCoords(float mx, float my, int& sx, int& sy, FrameBuffer& frameBuffer) const
{
    auto rx = (mx - m_gameState.m_player.x) / s_mapScale;
    auto ry = (my - m_gameState.m_player.y) / s_mapScale;
    sx      = static_cast<int>(rx + frameBuffer.m_width / 2);
    sy      = static_cast<int>(ry + frameBuffer.m_height / 2);
}

void MapRenderer::DrawMapLine(const Vertex& startVertex, const Vertex& endVertex, float lightness, FrameBuffer& frameBuffer) const
{
    int sx, sy, dx, dy;
    MapToScreenCoords(startVertex.x, startVertex.y, sx, sy, frameBuffer);
    MapToScreenCoords(endVertex.x, endVertex.y, dx, dy, frameBuffer);
    DrawLine(sx, sy, dx, dy, 3, lightness, frameBuffer);
}

void MapRenderer::RenderFrame(FrameBuffer& frameBuffer)
{
    frameBuffer.Clear();

    for(const auto& l : m_gameState.m_mapDef->m_wireframe)
    {
        DrawMapLine(l.s, l.e, 1.0f, frameBuffer);
    }

    // player position
    int px, py;
    MapToScreenCoords(m_gameState.m_player.x, m_gameState.m_player.y, px, py, frameBuffer);
    frameBuffer.SetPixel(px, py, 4, 1.0f);

    // player view cone
    int clx, cly, crx, cry;
    MapToScreenCoords(m_gameState.m_player.x + 100.0f * cos(m_gameState.m_player.a - PI4),
                      m_gameState.m_player.y + 100.0f * sin(m_gameState.m_player.a - PI4),
                      clx,
                      cly,
                      frameBuffer);
    MapToScreenCoords(m_gameState.m_player.x + 100.0f * cos(m_gameState.m_player.a + PI4),
                      m_gameState.m_player.y + 100.0f * sin(m_gameState.m_player.a + PI4),
                      crx,
                      cry,
                      frameBuffer);
    DrawLine(px, py, clx, cly, 4, 0.5f, frameBuffer);
    DrawLine(px, py, crx, cry, 4, 0.5f, frameBuffer);
}

MapRenderer::~MapRenderer() {}
} // namespace rtdoom
