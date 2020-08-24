#pragma once

#include <SDL.h>
#include "Renderer.h"
#include "WADFile.h"
#include "GLContext.h"

namespace rtdoom
{
class GLRenderer : public RendererBase
{
protected:
    int       m_width;
    int       m_height;
    GLContext m_context;

    void RenderSegments() const;

public:
    GLRenderer(const GameState& gameState, const WADFile& wadFile, int width, int height);
    ~GLRenderer();

    void Initialize();
    void Resize(int width, int height);
    void LoadMap();
    void Reset();

    void RenderFrame();
};
} // namespace rtdoom
