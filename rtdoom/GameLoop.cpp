#include "pch.h"
#include "GameLoop.h"

namespace rtdoom
{
GameLoop::GameLoop(SDL_Renderer* sdlRenderer, SDL_Window* window, const WADFile& wadFile) :
    m_gameState {}, m_moveDirection {0}, m_rotateDirection {0}, m_isRunning {false}, m_stepFrame {false}, m_softwareRenderer {m_gameState,
                                                                                                                              wadFile},
    m_glRenderer {m_gameState, wadFile, s_displayX, s_displayY}, m_glViewport {window, m_glRenderer},
    m_playerViewport {sdlRenderer, m_softwareRenderer, ViewScale(s_displayX), ViewScale(s_displayY), wadFile.m_palette, true},
    m_mapRenderer {m_gameState},
    m_mapViewport {sdlRenderer, m_mapRenderer, MapScale(s_displayX), MapScale(s_displayY), wadFile.m_palette, false},
    m_sdlRenderer(sdlRenderer)
{ }

constexpr int GameLoop::ViewScale(int windowSize) const
{
    return static_cast<int>(windowSize * s_multisamplingLevel);
}

constexpr int GameLoop::MapScale(int windowSize) const
{
    return static_cast<int>(windowSize * 0.25f);
}

void GameLoop::Move(int moveDirection)
{
    m_moveDirection = moveDirection;
}

void GameLoop::Rotate(int rotateDirection)
{
    m_rotateDirection = rotateDirection;
}

const Frame* GameLoop::RenderFrame()
{
    if(m_renderingMode == Renderer::RenderingMode::OpenGL)
    {
        m_glViewport.Draw();
        return NULL;
    }
    else
    {
        if(m_stepFrame)
        {
            m_playerViewport.DrawSteps();
            StepFrame(); // disable stepping
        }
        else
        {
            m_playerViewport.Draw();
            m_mapViewport.Draw();
        }
        SDL_RenderPresent(m_sdlRenderer);
        return m_softwareRenderer.GetLastFrame();
    }
}

void GameLoop::ClipPlayer()
{
    const auto& sector = m_gameState.m_mapDef->GetSector(Point(m_gameState.m_player.x, m_gameState.m_player.y));
    if(sector.has_value())
    {
        m_gameState.m_player.z = sector.value().floorHeight + 45;
    }
}

void GameLoop::StepFrame()
{
    m_stepFrame = !m_stepFrame;
}

void GameLoop::Tick(float seconds)
{
    m_gameState.Move(m_moveDirection, m_rotateDirection, seconds);
    ClipPlayer();
}

void GameLoop::ResizeWindow(int width, int height)
{
    m_playerViewport.Resize(ViewScale(width), ViewScale(height));
    m_mapViewport.Resize(MapScale(width), MapScale(height));
    m_glViewport.Resize(width, height);
}

void GameLoop::SetRenderingMode(Renderer::RenderingMode renderingMode)
{
    m_renderingMode = renderingMode;
    if(m_renderingMode != Renderer::RenderingMode::OpenGL)
    {
        m_softwareRenderer.SetMode(renderingMode);
    }
}

void GameLoop::Start(const MapStore& mapStore)
{
    m_gameState.NewGame(mapStore);
    ClipPlayer();
    m_isRunning = true;
    m_glViewport.Reset();
}

void GameLoop::Stop()
{
    m_isRunning = false;
}

GameLoop::~GameLoop() { }
} // namespace rtdoom
