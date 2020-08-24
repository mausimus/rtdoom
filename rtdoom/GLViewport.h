#pragma once

#include <SDL.h>
#include <SDL_render.h>

#include "GLRenderer.h"

namespace rtdoom
{
class GLViewport
{
protected:
    SDL_Window*      m_window;
    GLRenderer&      m_renderer;

    bool m_glReady;
    bool m_mapReady;

    void Initialize();

public:
    GLViewport(SDL_Window* window, GLRenderer& glRenderer);
    void Resize(int width, int height);
    void Reset();
    void Draw();
    ~GLViewport();
};
} // namespace rtdoom
