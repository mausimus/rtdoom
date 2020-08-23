#include "pch.h"
#include "GLViewport.h"

#include <SDL.h>

namespace rtdoom
{
GLViewport::GLViewport(SDL_Window* sdlWindow, GLRenderer& glRenderer) :
    m_window(sdlWindow), m_renderer(glRenderer), m_glReady(false), m_mapReady(false)
{ }

void GLViewport::Initialize()
{
    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 5);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 5);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 5);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

    SDL_GL_CreateContext(m_window);
    SDL_GL_SetSwapInterval(1);
}

GLViewport::~GLViewport()
{
}

void GLViewport::Reset()
{
    m_renderer.Reset();
    m_mapReady = false;
}

void GLViewport::Draw()
{
    if(!m_glReady)
    {
        Initialize();
        m_renderer.Initialize();
        m_glReady = true;
    }
    if(!m_mapReady)
    {
        m_renderer.LoadMap();
        m_mapReady = true;
    }

    m_renderer.RenderFrame();

    SDL_GL_SwapWindow(m_window);
}

void GLViewport::Resize(int width, int height)
{
    m_renderer.Resize(width, height);
}

} // namespace rtdoom