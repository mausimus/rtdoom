#include "pch.h"

#include "WADFile.h"
#include "MapStructs.h"
#include "glm/glm.hpp"

namespace rtdoom
{
class GLContext
{
private:
#pragma pack(1)
    struct VertexInfo
    {
        float x; // 3d coordinates
        float y;
        float z;
        float tx; // 2d texture coordinates
        float ty;
        float tz; // texture z coordinate (texture number within unit)
        float tn; // texture unit number
        float l; // lightness

        VertexInfo(float x, float y, float z, float tx, float ty, float tz, float tn, float l) :
            x {x}, y {y}, z {z}, tx {tx}, ty {ty}, tz {tz}, tn {tn}, l {l}
        { }
    };
#pragma pack()

    struct TextureUnit
    {
        int                                           n;
        short                                         w;
        short                                         h;
        std::vector<std::shared_ptr<rtdoom::Texture>> textures;
    };

    int                       m_maxTextureUnits;
    int                       m_vertexCounter;
    std::vector<TextureUnit>  m_textureUnits;
    std::vector<unsigned int> m_textures;
    std::vector<VertexInfo>   m_vertices;
    int                       m_shaderProgram;
    int                       m_vertexShader;
    int                       m_fragmentShader;
    unsigned int              m_VBO;
    unsigned int              m_VAO;
    unsigned int              m_EBO;
    const WADFile&            m_wadFile;

    void LoadTextures();
    void AddVertex(float x, float y, float z, float tx, float ty, float tn, float tz, float l);
    void LinkTriangle(int a, int b, int c);
    void LoadTexture(std::shared_ptr<Texture> wtex, int i);

public:
    void AddWallSegment(
        const Vertex& v0, float z0, const Vertex& v1, float z1, float tx0, float ty0, float tx1, float ty1, int tn, int ti, float l);
    void AddFloorCeiling(const Vertex& v0, const Vertex& v1, const Vertex& v2, float z, float tw, float th, int tn, int ti, float l);

    std::vector<unsigned int>          m_indices;
    std::map<int, std::pair<int, int>> m_subSectorOffsets;

    std::shared_ptr<Texture> AllocateTexture(std::string name, int& textureNo, int& textureLayer);
    void                     Initialize();
    void                     CompileShaders(const char* vertexShaderSource, const char* fragmentShaderSource);
    void                     BindMap();
    void                     BindView(glm::f32* viewMat, glm::f32* projectionMat);
    void                     Reset();

    GLContext(const WADFile& wadFile);
    ~GLContext();
};
} // namespace rtdoom
