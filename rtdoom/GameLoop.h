#pragma once

#include <SDL.h>
#include <SDL_render.h>

#include "rtdoom.h"
#include "Viewport.h"
#include "GameState.h"
#include "ViewRenderer.h"
#include "MapRenderer.h"

namespace rtdoom
{
	class GameLoop
	{
	protected:
		GameState m_gameState;
		ViewRenderer m_viewRenderer;
		Viewport m_playerViewport;
		MapRenderer m_mapRenderer;
		Viewport m_mapViewport;
		bool m_isRunning;
		bool m_stepFrame;
		int m_moveDirection;
		int m_rotateDirection;

		constexpr int ViewScale(int windowSize) const;
		constexpr int MapScale(int windowSize) const;

	public:
		void Start(const MapStore& mapStore);
		void Stop();
		bool isRunning() const { return m_isRunning; }

		void Move(int moveDirection);
		void Rotate(int rotateDirection);

		const Frame* RenderFrame();
		void ClipPlayer();
		void StepFrame();
		void Tick(float seconds);
		void ResizeWindow(int width, int height);
		void SetRenderingMode(ViewRenderer::RenderingMode renderingMode);

		const Thing& Player() const { return m_gameState.m_player; }

		GameLoop(SDL_Renderer* sdlRenderer, const WADFile& wadFile);
		~GameLoop();
	};
}