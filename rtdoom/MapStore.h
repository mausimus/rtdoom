#pragma once

namespace rtdoom
{
class MapStore
{
public:
    // serialized data structures from the original Doom format
#pragma pack(1)
    struct Segment
    {
        unsigned short startVertex;
        unsigned short endVertex;
        signed short   angle;
        unsigned short lineDef;
        signed short   direction;
        signed short   offset;
    };

    struct GLSegment
    {
        unsigned short startVertex;
        unsigned short endVertex;
        unsigned short lineDef;
        unsigned short side;
        unsigned short partnerSeg;
    };
    //#define VERT_IS_GL (1 << 15)

    struct SubSector
    {
        unsigned short numSegments;
        unsigned short firstSegment;
    };

/*    struct GLSubSector : SubSector
    { };*/

    struct Vertex
    {
        signed short x;
        signed short y;
    };

    struct GLVertex
    {
        signed short xf;
        signed short x;
        signed short yf;
        signed short y;
    };

    struct LineDef
    {
        unsigned short startVertex;
        unsigned short endVertex;
        unsigned short flags;
        unsigned short lineType;
        unsigned short sectorTag;
        unsigned short rightSideDef;
        unsigned short leftSideDef;
    };

    struct SideDef
    {
        SideDef() { }

        signed short   xOffset;
        signed short   yOffset;
        char           upperTexture[8];
        char           lowerTexture[8];
        char           middleTexture[8];
        unsigned short sector;
    };

    struct Sector
    {
        signed short   floorHeight;
        signed short   ceilingHeight;
        char           floorTexture[8];
        char           ceilingTexture[8];
        signed short   lightLevel;
        unsigned short sectorSpecial;
        unsigned short sectorTag;
    };

    struct Node
    {
        signed short   partitionX;
        signed short   partitionY;
        signed short   deltaX;
        signed short   deltaY;
        signed short   rightBoxTop;
        signed short   rightBoxBottom;
        signed short   rightBoxLeft;
        signed short   rightBoxRight;
        signed short   leftBoxTop;
        signed short   leftBoxBottom;
        signed short   leftBoxLeft;
        signed short   leftBoxRight;
        unsigned short rightChild;
        unsigned short leftChild;
    };
    /*
    struct GLNode : Node
    { };*/

    struct Thing
    {
        signed short   x;
        signed short   y;
        unsigned short a;
        unsigned short type;
        unsigned short flags;
    };
#pragma pack()

    std::vector<Segment>   m_segments;
    std::vector<SubSector> m_subSectors;
    std::vector<Vertex>    m_vertexes;
    std::vector<LineDef>   m_lineDefs;
    std::vector<SideDef>   m_sideDefs;
    std::vector<Sector>    m_sectors;
    std::vector<Node>      m_nodes;
    std::vector<Thing>     m_things;

    std::vector<GLVertex>    m_glVertexes;
    std::vector<GLSegment>   m_glSegments;
/*    std::vector<GLSubSector> m_glSubSectors;
    std::vector<GLNode>      m_glNodes;*/

    void Load(const std::string& mapFolder);
    void Load(const std::map<std::string, std::vector<char>>& mapLumps);
    void LoadGL(const std::map<std::string, std::vector<char>>& glLumps);
    void GetStartingPosition(signed short& x, signed short& y, unsigned short& a) const;

    MapStore();
    ~MapStore();
};
} // namespace rtdoom
