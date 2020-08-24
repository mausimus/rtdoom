#include "pch.h"
#include "Viewport.h"
#include "FrameBuffer32.h"

namespace rtdoom
{
Viewport::Viewport(SDL_Renderer* sdlRenderer, Renderer& renderer, int width, int height, const Palette& palette, bool fillTarget) :
    m_sdlRenderer(sdlRenderer), m_renderer(renderer), m_palette(palette), m_width(width), m_height(height), m_fillTarget(fillTarget)
{
    Initialize();
}

void Viewport::Initialize()
{
    m_screenTexture = SDL_CreateTexture(m_sdlRenderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, m_width, m_height);

    if(m_screenTexture == nullptr)
    {
        throw std::runtime_error("Unable to create texture");
    }

    m_frameBuffer.reset(new FrameBuffer32(m_width, m_height, m_palette));

    if(!m_fillTarget)
    {
        m_targetRect.reset(new SDL_Rect());
        m_targetRect->x = 0;
        m_targetRect->y = 0;
        m_targetRect->w = m_width;
        m_targetRect->h = m_height;
    }
    else
    {
        m_targetRect.reset();
    }
}

void Viewport::Uninitialize()
{
    SDL_DestroyTexture(m_screenTexture);
}

void Viewport::Resize(int width, int height)
{
    Uninitialize();
    m_width  = width;
    m_height = height;
    Initialize();
}

void Viewport::Draw()
{
    void* pixelBuffer;
    int   pitch;

    if(SDL_LockTexture(m_screenTexture, NULL, &pixelBuffer, &pitch))
    {
        throw std::runtime_error("Unable to lock texture");
    }

    m_frameBuffer->Attach(pixelBuffer, []() {});

    m_renderer.RenderFrame(*m_frameBuffer);

    SDL_UnlockTexture(m_screenTexture);

    if(SDL_RenderCopy(m_sdlRenderer, m_screenTexture, NULL, m_targetRect.get()))
    {
        throw std::runtime_error("Unable to render texture");
    }
}

void Viewport::DrawSteps()
{
    void* pixelBuffer = new int[m_width * m_height];
    memset(pixelBuffer, 0x00ffffff, sizeof(int) * m_width * m_height);
    int knt = 0;

    m_frameBuffer->Attach(pixelBuffer, [&]() {
        knt++;
        knt %= 2;
        if(knt != 1)
        {
            return;        
		}
        void* sdlBuffer;
        int   pitch;

        if(SDL_LockTexture(m_screenTexture, NULL, &sdlBuffer, &pitch))
        {
            throw std::runtime_error("Unable to lock texture");
        }

        memcpy(sdlBuffer, pixelBuffer, sizeof(int) * m_width * m_height);

        SDL_UnlockTexture(m_screenTexture);

        if(SDL_RenderCopy(m_sdlRenderer, m_screenTexture, NULL, m_targetRect.get()))
        {
            throw std::runtime_error("Unable to render texture");
        }

        SDL_RenderPresent(m_sdlRenderer);
    });

    m_renderer.RenderFrame(*m_frameBuffer);

    delete[] pixelBuffer;
}

Viewport::~Viewport()
{
    Uninitialize();
}
} // namespace rtdoom
