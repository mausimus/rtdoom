#include "pch.h"
#include "rtdoom.h"
#include "ViewRenderer.h"
#include "MathCache.h"
#include "WireframePainter.h"
#include "SolidPainter.h"
#include "TexturePainter.h"

namespace rtdoom
{
ViewRenderer::ViewRenderer(const GameState& gameState, const WADFile& wadFile) :
    Renderer {gameState}, m_wadFile {wadFile}, m_renderingMode {RenderingMode::Textured}
{}

// entry method for rendering a frame
void ViewRenderer::RenderFrame(FrameBuffer& frameBuffer)
{
    Initialize(frameBuffer);

    // walls
    RenderSegments();

    // floors and ceilings
    RenderPlanes();

    // things/objects
    RenderSprites();

    // HUD
    RenderOverlay();
}

void ViewRenderer::Initialize(FrameBuffer& frameBuffer)
{
    m_frameBuffer = &frameBuffer;
    m_projection  = std::make_unique<Projection>(m_gameState.m_player, frameBuffer);
    m_frame       = std::make_unique<Frame>(frameBuffer);
    switch(m_renderingMode)
    {
    case RenderingMode::Wireframe:
        m_painter = std::make_unique<WireframePainter>(frameBuffer);
        break;
    case RenderingMode::Solid:
        m_painter = std::make_unique<SolidPainter>(frameBuffer, *m_projection);
        break;
    case RenderingMode::Textured:
        m_painter = std::make_unique<TexturePainter>(frameBuffer, m_gameState.m_player, *m_projection, m_wadFile);
        break;
    default:
        throw std::runtime_error("Unsupported rendering mode");
    }
}

void ViewRenderer::RenderSegments() const
{
    // iterate through all segments (map lines) in visibility order returned by traversing the map's BSP tree
    for(const auto& segment : m_gameState.m_mapDef->GetSegmentsToDraw(m_gameState.m_player))
    {
        // only draw segments that are facing the player
        if(MapDef::IsInFrontOf(m_gameState.m_player, *segment))
        {
            RenderMapSegment(*segment);

            // stop drawing once the frame has been fully occluded with solid walls or vertical spans
            if(m_frame->IsOccluded())
            {
                break;
            }
        }
    }
}

void ViewRenderer::RenderMapSegment(const Segment& segment) const
{
    VisibleSegment vs {segment};

    // calculate relative view angles of the mapSegment and skip rendering if they are not within the field of view
    vs.startAngle = m_projection->ProjectionAngle(segment.s);
    vs.endAngle   = m_projection->ProjectionAngle(segment.e);
    if(!Projection::NormalizeViewAngleSpan(vs.startAngle, vs.endAngle))
    {
        return;
    }

    // project edges of the mapSegment onto the screen from left to right
    vs.startX = m_projection->ViewX(vs.startAngle);
    vs.endX   = m_projection->ViewX(vs.endAngle);
    if(vs.startX > vs.endX)
    {
        std::swap(vs.startX, vs.endX);
        std::swap(vs.startAngle, vs.endAngle);
    }

    // clip the mapSegment against already drawn solid walls (horizontal occlusion)
    const auto visibleSpans = m_frame->ClipHorizontalSegment(vs.startX, vs.endX, segment.isSolid);
    if(visibleSpans.empty())
    {
        return;
    }

    // draw all visible spans
    vs.normalVector = m_projection->NormalVector(segment); // normal vector from the mapSegment towards the player
    vs.normalOffset = m_projection->NormalOffset(segment); // offset of the normal vector from the start of the line (for texturing)
    for(const auto& span : visibleSpans)
    {
        RenderMapSegmentSpan(span, vs);
    }

    m_frame->m_sectors.emplace(segment.frontSide.sector.sectorId);
    m_frame->m_sectors.emplace(segment.backSide.sector.sectorId);
    m_frame->m_numSegments++;
}

// draw a visible span of a mapSegment on the frame buffer
void ViewRenderer::RenderMapSegmentSpan(const Frame::Span& span, const VisibleSegment& visibleSegment) const
{
    const auto& mapSegment  = visibleSegment.mapSegment;
    const auto& frontSector = mapSegment.frontSide.sector;
    Frame::Clip lowerClip {span}, middleClip {span}, upperClip {span};

    // iterate through all vertical columns from left to right
    for(auto x = span.s; x <= span.e; x++)
    {
        // calculate the relative viewAngle and distance to the mapSegment from our viewpoint
        auto viewAngle = GetViewAngle(x, visibleSegment);
        auto projectionDistance  = m_projection->Distance(visibleSegment.normalVector, viewAngle);
        if(projectionDistance < s_minDistance)
        {
            lowerClip.Add(x, Frame::PainterContext(), 0, 0);
            middleClip.Add(x, Frame::PainterContext(), 0, 0);
            upperClip.Add(x, Frame::PainterContext(), 0, 0);
            continue;
        }

        // calculate on-screen vertical start and end positions for the column, based on the front (outer) side
        const auto outerTopY    = m_projection->ViewY(projectionDistance, frontSector.ceilingHeight - m_gameState.m_player.z);
        const auto outerBottomY = m_projection->ViewY(projectionDistance, frontSector.floorHeight - m_gameState.m_player.z);

        Frame::PainterContext outerTexture;
        outerTexture.yScale      = m_projection->TextureScale(projectionDistance);
        outerTexture.yPegging    = mapSegment.lowerUnpegged ? outerBottomY : outerTopY;
        outerTexture.textureName = mapSegment.frontSide.middleTexture;
        outerTexture.yOffset     = mapSegment.frontSide.yOffset;
        // texel x position is the offset from player to normal vector plus offset from normal vector to view, plus static mapSegment and linedef offsets
        outerTexture.texelX = visibleSegment.normalOffset + m_projection->Offset(visibleSegment.normalVector, viewAngle) +
                              mapSegment.xOffset + mapSegment.frontSide.xOffset;
        outerTexture.isEdge    = x == visibleSegment.startX || x == visibleSegment.endX;
        outerTexture.lightness = m_projection->Lightness(projectionDistance, &mapSegment) * frontSector.lightLevel;

        // clip the column based on what we've already have drawn (vertical occlusion)
        const auto  ceilingHeight = frontSector.isSky ? s_skyHeight : (frontSector.ceilingHeight - m_gameState.m_player.z);
        const auto  floorHeight   = frontSector.floorHeight - m_gameState.m_player.z;
        const auto& outerSpan     = m_frame->ClipVerticalSegment(x,
                                                             outerTopY,
                                                             outerBottomY,
                                                             mapSegment.isSolid,
                                                             &ceilingHeight,
                                                             &floorHeight,
                                                             frontSector.ceilingTexture,
                                                             frontSector.floorTexture,
                                                             frontSector.lightLevel);

        if(mapSegment.isSolid)
        {
            middleClip.Add(x, outerTexture, 0, m_frameBuffer->m_height - 1);
            if(outerSpan.isVisible())
            {
                m_painter->PaintWall(x, outerSpan, outerTexture);
            }
        }
        else
        {
            // if the mapSegment is not a solid wall but a pass-through portal clip its back (inner) side
            const auto& backSector = mapSegment.backSide.sector;

            // recalculate column size for the back side
            const auto innerTopY    = m_projection->ViewY(projectionDistance, backSector.ceilingHeight - m_gameState.m_player.z);
            const auto innerBottomY = m_projection->ViewY(projectionDistance, backSector.floorHeight - m_gameState.m_player.z);

            // segments that connect open sky sectors should not have their top sections drawn
            const auto isSky = frontSector.isSky && backSector.isSky;

            // clip the inner span against what's already been drawn
            const auto& innerSpan = m_frame->ClipVerticalSegment(x,
                                                                 innerTopY,
                                                                 innerBottomY,
                                                                 mapSegment.isSolid,
                                                                 isSky ? &s_skyHeight : nullptr,
                                                                 nullptr,
                                                                 frontSector.ceilingTexture,
                                                                 frontSector.floorTexture,
                                                                 frontSector.lightLevel);

            upperClip.Add(x, outerTexture, 0, std::max(innerTopY, outerTopY));
            lowerClip.Add(x, outerTexture, std::min(innerBottomY, outerBottomY), m_frameBuffer->m_height - 1);

            if(innerSpan.isVisible())
            {
                // render the inner section (floor/ceiling height change)
                if(!isSky)
                {
                    Frame::Span           upperSpan {outerSpan.s, innerSpan.s};
                    Frame::PainterContext upperTexture {outerTexture};
                    upperTexture.textureName = mapSegment.frontSide.upperTexture;
                    upperTexture.yPegging    = mapSegment.upperUnpegged ? outerTopY : innerTopY;
                    m_painter->PaintWall(x, upperSpan, upperTexture);
                }

                Frame::Span           lowerSpan {innerSpan.e, outerSpan.e};
                Frame::PainterContext lowerTexture {outerTexture};
                lowerTexture.textureName = mapSegment.frontSide.lowerTexture;
                lowerTexture.yPegging    = mapSegment.lowerUnpegged ? outerTopY : innerBottomY;
                m_painter->PaintWall(x, lowerSpan, lowerTexture);

                // if there's a middle texture paint it as sprite (could be semi-transparent)
                if(outerTexture.textureName != "-")
                {
                    m_frame->m_sprites.push_back(std::make_unique<Frame::SpriteWall>(x, innerSpan, outerTexture, projectionDistance));
                }
            }
        }
    }

    if(mapSegment.isSolid)
    {
        m_frame->m_clips.push_back(middleClip);
    }
    else
    {
        m_frame->m_clips.push_back(upperClip);
        m_frame->m_clips.push_back(lowerClip);
    }
}

// render floors and ceilings based on data collected during drawing walls
void ViewRenderer::RenderPlanes() const
{
    for(const auto& floorPlane : m_frame->m_floorPlanes)
    {
        m_painter->PaintPlane(floorPlane);
        m_frame->m_numFloorPlanes++;
    }
    for(const auto& ceilingPlane : m_frame->m_ceilingPlanes)
    {
        m_painter->PaintPlane(ceilingPlane);
        m_frame->m_numCeilingPlanes++;
    }
}

// render sprites (things or semi-transparent walls)
void ViewRenderer::RenderSprites() const
{
    // add things in visited sectors to list of sprites
    for(const auto s : m_frame->m_sectors)
    {
        if(s >= 0)
        {
            for(const auto t : m_gameState.m_mapDef->m_things[s])
            {
                const auto viewAngle = m_projection->ProjectionAngle(t);
                const auto projectionDistance = Projection::Distance(t, m_gameState.m_player) * MathCache::instance().Cos(viewAngle);
                m_frame->m_sprites.push_back(std::make_unique<Frame::SpriteThing>(t, projectionDistance));
            }
        }
    }

    // sort and draw sprites farthest to nearest
    std::sort(m_frame->m_sprites.begin(),
              m_frame->m_sprites.end(),
              [](const std::unique_ptr<Frame::Sprite>& a, const std::unique_ptr<Frame::Sprite>& b) { return (a->distance > b->distance); });

    for(const auto& sprite : m_frame->m_sprites)
    {
        if(sprite->IsThing())
        {
            RenderSpriteThing(dynamic_cast<Frame::SpriteThing* const>(sprite.get()));
        }
        else if(sprite->IsWall())
        {
            RenderSpriteWall(dynamic_cast<Frame::SpriteWall* const>(sprite.get()));
        }
    }
}

// render things
void ViewRenderer::RenderSpriteThing(Frame::SpriteThing* const thing) const
{
    std::string textureName(thing->textureName);
    if(textureName.length() > 5 && textureName[5] == '1')
    {
        // check for angle frame
        const auto angleDiff = Projection::NormalizeAngle(thing->a + PI - m_gameState.m_player.a) + 2 * PI;
        const char frame     = static_cast<char>((angleDiff + PI4 / 2.0f) / PI4) % 8;
        textureName[5]       = '1' + frame;
        textureName[4]       = 'A' + static_cast<int>(m_gameState.m_step * 2) % 4;
    }
    else if(textureName.length() > 4 && textureName[4] == 'A')
    {
        // check for animated frame
        std::string alternateTexture(textureName);
        alternateTexture[4] = 'A' + static_cast<int>(m_gameState.m_step * 2) % 2;
        if(m_wadFile.m_sprites.find(alternateTexture) != m_wadFile.m_sprites.end())
        {
            textureName = alternateTexture;
        }
    }

    const auto viewAngle = m_projection->ProjectionAngle(*thing);
    if(thing->distance < s_minDistance || textureName.empty() || viewAngle < -PI4 || viewAngle > PI4)
    {
        return;
    }

    const auto spritePatch = m_wadFile.m_sprites.find(textureName);
    if(spritePatch == m_wadFile.m_sprites.end())
    {
        return;
    }

    const auto scale          = m_projection->TextureScale(thing->distance);
    if(scale < s_minScale)
    {
        return;
    }

    const auto  midDistance  = MathCache::instance().Tan(viewAngle) / PI4;
    const auto  centerX      = static_cast<int>((m_frameBuffer->m_width / 2) * (1 + midDistance));
    const auto  centerY      = m_projection->ViewY(thing->distance, thing->z - m_gameState.m_player.z);
    const auto& texture      = spritePatch->second;
    const auto  spriteWidth  = static_cast<int>(texture->width / scale);
    const auto  spriteHeight = static_cast<int>(texture->height / scale);
    const auto  startY       = static_cast<int>(centerY - texture->top / scale);
    const auto  startX       = static_cast<int>(centerX - texture->left / scale);

    // clip sprite against already drawn walls
    const auto& occlusionMatrix = ClipSprite(startX, startY, spriteWidth, spriteHeight, scale);

    // draw sprite column by column
    Frame::PainterContext spriteContext;
    spriteContext.textureName = textureName;
    spriteContext.yScale      = scale;
    spriteContext.lightness   = m_gameState.m_mapDef->m_sectors[thing->sectorId].lightLevel * m_projection->Lightness(thing->distance);
    for(int x = 0; x < spriteWidth; x++)
    {
        const auto screenX = startX + x;
        if(screenX >= 0 && screenX < m_frameBuffer->m_width)
        {
            spriteContext.texelX = static_cast<float>(x) * texture->width / spriteWidth;
            m_painter->PaintSprite(screenX, startY, occlusionMatrix[x], spriteContext);
        }
    }
}

// build sprite occlusion matrix
// our sprite spans from [startX, startX + spriteWidth] and [startY, startY + spriteHeight]
std::vector<std::vector<bool>>
ViewRenderer::ClipSprite(int startX, int startY, int spriteWidth, int spriteHeight, float spriteScale) const
{
    std::vector<std::vector<bool>> occlusion(spriteWidth);
    for(auto& o : occlusion)
    {
        o.resize(spriteHeight);
    }
    for(const auto& clip : m_frame->m_clips)
    {
        // for every clip that's in front of the sprite and overlaps it, add it to occlusion matrix
        if((clip.yScaleStart < spriteScale || clip.yScaleEnd < spriteScale) && clip.xSpan.s < startX + spriteWidth && clip.xSpan.e > startX)
        {
            for(auto x = startX; x < startX + spriteWidth; x++)
            {
                if(x < clip.xSpan.s || x > clip.xSpan.e)
                {
                    continue;
                }
                const auto clipX = x - clip.xSpan.s;
                for(auto y = startY; y < startY + spriteHeight; y++)
                {
                    if(y >= clip.topClips[clipX] && y <= clip.bottomClips[clipX] && clip.yScales[clipX] < spriteScale)
                    {
                        occlusion[x - startX][y - startY] = true;
                    }
                }
            }
        }
    }
    return occlusion;
}

// render semi-transparent walls
void ViewRenderer::RenderSpriteWall(Frame::SpriteWall* const wall) const
{
    m_painter->PaintWall(wall->x, wall->span, wall->textureContext);
}

void ViewRenderer::RenderOverlay() const
{
    // TODO: render HUD etc.
}

// return the view viewAngle for vertical screen column
Angle ViewRenderer::GetViewAngle(int x, const VisibleSegment& visibleSegment) const
{
    auto viewAngle = m_projection->ViewAngle(x);

    // due to limited resolution we have to reduce bleeding from converting angles to columns and back
    if(x == visibleSegment.startX)
    {
        viewAngle = visibleSegment.startAngle;
    }
    else if(x == visibleSegment.endX)
    {
        viewAngle = visibleSegment.endAngle;
    }
    return viewAngle;
}

void ViewRenderer::SetMode(RenderingMode renderingMode)
{
    m_renderingMode = renderingMode;
}

Frame* ViewRenderer::GetLastFrame() const
{
    return m_frame.get();
}

ViewRenderer::~ViewRenderer() {}
} // namespace rtdoom
