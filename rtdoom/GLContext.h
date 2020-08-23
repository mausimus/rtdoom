#include "pch.h"

#include "WADFile.h"
#include "glm/glm.hpp"

namespace rtdoom
{
class GLContext
{
private:
    struct TextureUnit
    {
        int                                           n;
        short                                         w;
        short                                         h;
        std::vector<std::shared_ptr<rtdoom::Texture>> textures;
    };

    int                       m_vertexCounter;
    std::vector<TextureUnit>  m_textureUnits;
    std::vector<unsigned int> m_textures;
    std::vector<float>        m_vertices;

    void LoadTextures();

    void AddVertex(float x, float y, float z, float tx, float ty, float tn, float tz, float l);
    void LinkTriangle(int a, int b, int c);
    void LoadTexture(std::shared_ptr<Texture> wtex, int i);

    int            m_shaderProgram, m_vertexShader, m_fragmentShader;
    unsigned int   m_VBO, m_VAO, m_EBO;
    const WADFile& m_wadFile;

public:
    void AddWallSegment(
        float x0, float y0, float z0, float x1, float y1, float z1, float tx0, float ty0, float tx1, float ty1, int tn, int ti, float l);
    void AddFloorCeiling(float x0, float y0, float x1, float y1, float x2, float y2, float z, float tw, float th, int tn, int ti, float l);

    std::vector<unsigned int>          m_indices;
    std::map<int, std::pair<int, int>> m_subSectorOffsets;

    std::shared_ptr<Texture> AllocateTexture(std::string name, int& textureNo, int& textureLayer);
    void                     CompileShaders(const char* vertexShaderSource, const char* fragmentShaderSource);
    void                     BindMap();
    void                     BindView(glm::f32* viewMat, glm::f32* projectionMat);
    void                     Reset();

    GLContext(const WADFile& wadFile);
    ~GLContext();
};
} // namespace rtdoom
