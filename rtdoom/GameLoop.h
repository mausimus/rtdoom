#pragma once

#include <SDL.h>
#include <SDL_render.h>

#include "rtdoom.h"
#include "Viewport.h"
#include "GameState.h"
#include "SoftwareRenderer.h"
#include "GLRenderer.h"
#include "GLViewport.h"
#include "MapRenderer.h"

namespace rtdoom
{
class GameLoop
{
protected:
    GameState               m_gameState;
    SoftwareRenderer        m_softwareRenderer;
    GLRenderer              m_glRenderer;
    GLViewport              m_glViewport;
    Viewport                m_playerViewport;
    MapRenderer             m_mapRenderer;
    Viewport                m_mapViewport;
    bool                    m_isRunning;
    bool                    m_stepFrame;
    int                     m_moveDirection;
    int                     m_rotateDirection;
    Renderer::RenderingMode m_renderingMode;
    SDL_Renderer*           m_sdlRenderer;

    constexpr int ViewScale(int windowSize) const;
    constexpr int MapScale(int windowSize) const;

public:
    void Start(const MapStore& mapStore);
    void Stop();
    bool isRunning() const
    {
        return m_isRunning;
    }

    void Move(int moveDirection);
    void Rotate(int rotateDirection);

    const Frame* RenderFrame();
    void         ClipPlayer();
    void         StepFrame();
    void         Tick(float seconds);
    void         ResizeWindow(int width, int height);
    void         SetRenderingMode(Renderer::RenderingMode renderingMode);

    const Thing& Player() const
    {
        return m_gameState.m_player;
    }

    GameLoop(SDL_Renderer* sdlRenderer, SDL_Window* window, const WADFile& wadFile);
    ~GameLoop();
};
} // namespace rtdoom
