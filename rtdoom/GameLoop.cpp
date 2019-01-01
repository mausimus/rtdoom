#include "pch.h"
#include "GameLoop.h"

namespace rtdoom
{
	GameLoop::GameLoop(SDL_Renderer* sdlRenderer) :
		m_gameState{},
		m_moveDirection{ 0 },
		m_rotateDirection{ 0 },
		m_isRunning{ false },
		m_viewRenderer{ m_gameState },
		m_playerViewport{ sdlRenderer, m_viewRenderer, ViewScale(s_displayX), ViewScale(s_displayY), true },
		m_mapRenderer{ m_gameState },
		m_mapViewport{ sdlRenderer, m_mapRenderer, MapScale(s_displayX), MapScale(s_displayY), false }
	{
	}

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

	void GameLoop::RenderFrame()
	{
		m_playerViewport.Draw();
		m_mapViewport.Draw();
	}

	void GameLoop::Tick(float seconds)
	{
		m_gameState.Move(m_moveDirection, m_rotateDirection, seconds);
		m_gameState.m_player.z = m_viewRenderer.GetLastFrame()->m_drawnSectors[0].floorHeight + 45;
	}

	void GameLoop::ResizeWindow(int width, int height)
	{
		m_playerViewport.Resize(ViewScale(width), ViewScale(height));
		m_mapViewport.Resize(MapScale(width), MapScale(height));
	}

	void GameLoop::SetRenderingMode(ViewRenderer::RenderingMode renderingMode)
	{
		m_viewRenderer.SetMode(renderingMode);
	}

	void GameLoop::Start(const std::string& mapFolder)
	{
		m_gameState.NewGame(mapFolder);
		m_isRunning = true;
	}

	void GameLoop::Stop()
	{
		m_isRunning = false;
	}

	GameLoop::~GameLoop()
	{
	}
}