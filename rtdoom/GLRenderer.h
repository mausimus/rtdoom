#pragma once

#include "Renderer.h"
#include "WADFile.h"
#include "Projection.h"
#include "Frame.h"
#include "Painter.h"

namespace rtdoom
{
class GLRenderer : public Renderer
{
protected:
    void Initialize();
    void GenerateMap();
    void LoadTextures();
    std::shared_ptr<Texture> AllocateTexture(std::string name, int& textureNo, int& textureLayer);
    void LoadTexture(std::shared_ptr<Texture> wtex, int i);

    void RenderSegments() const;
    void RenderPlanes() const;
    void RenderSprites() const;
    void RenderOverlay() const;
    void RenderMapSegment(const Segment& segment) const;

    const WADFile& m_wadFile;

public:
    GLRenderer(const GameState& gameState, const WADFile& wadFile);
    ~GLRenderer();

    virtual void RenderFrame();
    virtual void RenderFrame(FrameBuffer& frameBuffer) override;
};
} // namespace rtdoom
