#pragma once

#include "FrameBuffer.h"
#include "MapDef.h"

namespace rtdoom
{
// class holding working structures used in drawing a single frame
class Frame
{
public:
    // general horizontal or vertical screen span (line)
    struct Span
    {
        constexpr Span(int s, int e) : s {s}, e {e} {}
        constexpr Span() : Span {0, 0} {}
        bool isVisible() const
        {
            return (s != 0 || e != 0);
        };
        int s;
        int e;
        int length() const
        {
            return e - s;
        }

        bool operator<(const Span& rhs) const
        {
            if(s == rhs.s)
            {
                return e < rhs.e;
            }
            else
            {
                return s < rhs.s;
            }
        }
    };

    // screen area covered by floor or ceiling
    struct Plane
    {
        Plane(float h, const std::string& textureName, float lightLevel, int height) :
            h {h}, textureName {textureName}, lightLevel {lightLevel}, spans(height)
        {}
        bool isSky() const
        {
            return isnan(h);
        }
        const std::string&             textureName;
        const float                    lightLevel;
        const float                    h;
        std::vector<std::vector<Span>> spans;

        void addSpan(int x, int sy, int ey);
    };

    // texturing information
    struct PainterContext
    {
        PainterContext() : yPegging {0}, yOffset {0} {}
        std::string textureName;
        float       yScale;
        float       texelX;
        int         yPegging;
        int         yOffset;
        bool        isEdge;
        float       lightness;
    };

    // silhouette of a drawn wall
    struct Clip
    {
        Clip(Span xSpan) : xSpan {xSpan}, topClips(), bottomClips(), texelXs() {}

        void Add(int x, const PainterContext& painterContext, int topClip, int bottomClip)
        {
            if(x == xSpan.s)
            {
                yScaleStart = painterContext.yScale;
            }
            else if(x == xSpan.e)
            {
                yScaleEnd = painterContext.yScale;
            }
            topClips.push_back(topClip);
            bottomClips.push_back(bottomClip);
            texelXs.push_back(static_cast<int>(painterContext.texelX));
        }

        Span xSpan;

        float yScaleStart;
        float yScaleEnd;

        std::vector<int> topClips;
        std::vector<int> bottomClips;
        std::vector<int> texelXs;
    };

    // overlay sprite drawn in last phase
    struct Sprite
    {
        Sprite(float distance) : distance(distance) {}

        float distance;

        virtual bool IsThing() const
        {
            return false;
        }

        virtual bool IsWall() const
        {
            return false;
        }

        virtual ~Sprite() {}
    };

    // thing/object sprite
    struct SpriteThing : public Sprite, public Thing
    {
        SpriteThing(const Thing& thing, float distance) : Thing(thing), Sprite(distance) {}

        bool IsThing() const override
        {
            return true;
        }
    };

    // semi-transparent wall sprite
    struct SpriteWall : public Sprite
    {
        SpriteWall(int x, const Span& span, PainterContext& textureContext, float distance) :
            Sprite(distance), x(x), span(span), textureContext(textureContext)
        {}

        bool IsWall() const override
        {
            return true;
        }

        int            x;
        Span           span;
        PainterContext textureContext;
    };

    // viewport
    const int m_width;
    const int m_height;

    // list of horizontal screen spans where isSolid walls have already been drawn (completely occluded)
    std::list<Span> m_occlusion;

    // drawn walls that clip anything behind them
    std::list<Clip> m_clips;

    // sprites (in-game objects and semi-transparent walls)
    std::vector<std::unique_ptr<Sprite>> m_sprites;

    // screen height where the last floor/ceilings have been drawn up to so far
    std::vector<int> m_floorClip;
    std::vector<int> m_ceilClip;

    // numer of drawn segments
    int m_numSegments           = 0;
    int m_numFloorPlanes        = 0;
    int m_numCeilingPlanes      = 0;
    int m_numVerticallyOccluded = 0;

    // spaces between walls with floors and ceilings
    std::deque<Plane> m_floorPlanes;
    std::deque<Plane> m_ceilingPlanes;

    // visible sectors
    std::unordered_set<int> m_sectors;

    // add vertical span to existing planes
    void MergeIntoPlane(std::deque<Plane>& planes, float height, const std::string& textureName, float lightLevel, int x, int sy, int ey);

    // returns horizontal screen spans where the mapSegment is visible and updates occlusion table
    std::vector<Span> ClipHorizontalSegment(int startX, int endX, bool isSolid);

    // returns the vertical screen span where the column is visible and updates occlussion table
    Span ClipVerticalSegment(int                x,
                             int                ceilingProjection,
                             int                floorProjection,
                             bool               isSolid,
                             const float*       ceilingHeight,
                             const float*       floorHeight,
                             const std::string& ceilingTexture,
                             const std::string& floorTexture,
                             float              lightLevel);

    bool IsSpanVisible(int x, int sy, int ey) const;
    bool IsOccluded() const;
    bool IsVerticallyOccluded(int x) const;

    Frame(const FrameBuffer& frameBuffer);
    ~Frame();
};
} // namespace rtdoom
