#include "pch.h"
#include "GLContext.h"

#include "glad/glad.h"
#include "glm/gtc/type_ptr.hpp"

namespace rtdoom
{

GLContext::GLContext(const WADFile& wadFile) :
    m_wadFile {wadFile}, m_EBO {}, m_VBO {}, m_VAO {}, m_fragmentShader {}, m_shaderProgram {}, m_vertexShader {}
{ }

void GLContext::Reset()
{
    m_vertices.clear();
    m_indices.clear();
    m_vertexCounter = 0;
    m_textures.clear();
    m_textureUnits.clear();
    m_subSectorOffsets.clear();
}

void GLContext::AddVertex(float x, float y, float z, float tx, float ty, float tn, float tz, float l)
{
    m_vertices.push_back(x);
    m_vertices.push_back(y);
    m_vertices.push_back(z);
    m_vertices.push_back(tx);
    m_vertices.push_back(ty);
    m_vertices.push_back(tz);
    m_vertices.push_back(tn);
    m_vertices.push_back(l);
}

void GLContext::LinkTriangle(int a, int b, int c)
{
    m_indices.push_back(a);
    m_indices.push_back(b);
    m_indices.push_back(c);
}

// wall section
void GLContext::AddWallSegment(
    float x0, float y0, float z0, float x1, float y1, float z1, float tx0, float ty0, float tx1, float ty1, int tn, int ti, float l)
{
    AddVertex(x0, y0, z0, tx0, ty0, tn, ti, l);
    AddVertex(x1, y1, z0, tx1, ty0, tn, ti, l);
    AddVertex(x1, y1, z1, tx1, ty1, tn, ti, l);
    AddVertex(x0, y0, z1, tx0, ty1, tn, ti, l);
    LinkTriangle(m_vertexCounter, m_vertexCounter + 1, m_vertexCounter + 2);
    LinkTriangle(m_vertexCounter, m_vertexCounter + 2, m_vertexCounter + 3);
    m_vertexCounter += 4;
}

// floor/ceil triangle
void GLContext::AddFloorCeiling(
    float x0, float y0, float x1, float y1, float x2, float y2, float z, float tw, float th, int tn, int ti, float l)
{
    AddVertex(x0, y0, z, x0 / tw, y0 / th, tn, ti, l);
    AddVertex(x1, y1, z, x1 / tw, y1 / th, tn, ti, l);
    AddVertex(x2, y2, z, x2 / tw, y2 / th, tn, ti, l);
    LinkTriangle(m_vertexCounter, m_vertexCounter + 1, m_vertexCounter + 2);
    m_vertexCounter += 3;
}

void GLContext::LoadTexture(std::shared_ptr<Texture> wtex, int i)
{
    int            x, y;
    unsigned char* data = new unsigned char[wtex->width * wtex->height * 3];
    for(y = 0; y < wtex->height; y++)
        for(x = 0; x < wtex->width; x++)
        {
            const auto& c                       = m_wadFile.m_palette.colors[wtex->pixels[x + y * wtex->width]];
            data[3 * (x + y * wtex->width)]     = c.r;
            data[3 * (x + y * wtex->width) + 1] = c.g;
            data[3 * (x + y * wtex->width) + 2] = c.b;
        }

    glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, i, wtex->width, wtex->height, 1, GL_RGB, GL_UNSIGNED_BYTE, data);
    delete[] data;
}

void GLContext::LoadTextures()
{
    printf("Texture units (%d):\n", m_textureUnits.size());
    m_textures.resize(m_textureUnits.size());
    glGenTextures(m_textureUnits.size(), m_textures.data());
    for(int tu = 0; tu < m_textureUnits.size(); tu++)
    {
        TextureUnit& unit = m_textureUnits.at(tu);

        printf("  %d = %d x %d (%d):\n", tu, unit.w, unit.h, unit.textures.size());
        glBindTexture(GL_TEXTURE_2D_ARRAY, m_textures[tu]);
        glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, GL_RGB, unit.w, unit.h, unit.textures.size(), 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);

        for(int tn = 0; tn < unit.textures.size(); tn++)
        {
            LoadTexture(unit.textures[tn], tn);
        }

        glGenerateMipmap(GL_TEXTURE_2D_ARRAY);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    }

    glUseProgram(m_shaderProgram);

    std::vector<int> textureNos;
    textureNos.resize(m_textureUnits.size());
    for(int i = 0; i < textureNos.size(); i++)
    {
        textureNos[i] = i;
    }
    glUniform1iv(glGetUniformLocation(m_shaderProgram, "ourTexture"), m_textureUnits.size(), textureNos.data());
}

void GLContext::BindView(glm::f32 *viewMat, glm::f32 *projectionMat) {
    unsigned int viewLoc = glGetUniformLocation(m_shaderProgram, "view");
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, viewMat);

    unsigned int projectionLoc = glGetUniformLocation(m_shaderProgram, "projection");
    glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, projectionMat);

    for(int tu = 0; tu < m_textureUnits.size(); tu++)
    {
        glActiveTexture(GL_TEXTURE0 + tu);
        glBindTexture(GL_TEXTURE_2D_ARRAY, m_textures[tu]);
    }

    glUseProgram(m_shaderProgram);
    glBindVertexArray(m_VAO);
}

std::shared_ptr<Texture> GLContext::AllocateTexture(std::string name, int& textureUnit, int& textureNo)
{
    // decide which texture unit and number the texture will go to, but create them later?
    if(name == "-")
    {
        textureUnit = 0;
        textureNo   = 0;
        return NULL;
    }
    auto wtex = m_wadFile.m_textures.find(name)->second;
    int  tui;
    for(tui = 0; tui < m_textureUnits.size(); tui++)
    {
        if(m_textureUnits[tui].w == wtex->width && m_textureUnits[tui].h == wtex->height)
        {
            textureUnit = tui;
            for(int tn = 0; tn < m_textureUnits[tui].textures.size(); tn++)
            {
                if(m_textureUnits[tui].textures[tn]->name == name)
                {
                    textureNo = tn;
                    return wtex;
                }
            }
            textureNo = m_textureUnits[tui].textures.size();
            m_textureUnits[tui].textures.push_back(wtex);
            return wtex;
        }
    }

    // create and retry
    TextureUnit tu;
    tu.w = wtex->width;
    tu.h = wtex->height;
    tu.n = m_textureUnits.size();
    m_textureUnits.push_back(tu);
    return AllocateTexture(name, textureUnit, textureNo);
}

void GLContext::CompileShaders(const char* vertexShaderSource, const char* fragmentShaderSource)
{
    m_vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(m_vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(m_vertexShader);
    int  success;
    char infoLog[512];
    glGetShaderiv(m_vertexShader, GL_COMPILE_STATUS, &success);
    if(!success)
    {
        glGetShaderInfoLog(m_vertexShader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
        abort();
    }

    m_fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(m_fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(m_fragmentShader);
    glGetShaderiv(m_fragmentShader, GL_COMPILE_STATUS, &success);
    if(!success)
    {
        glGetShaderInfoLog(m_fragmentShader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
        abort();
    }

    m_shaderProgram = glCreateProgram();
    glAttachShader(m_shaderProgram, m_vertexShader);
    glAttachShader(m_shaderProgram, m_fragmentShader);
    glLinkProgram(m_shaderProgram);
    glGetProgramiv(m_shaderProgram, GL_LINK_STATUS, &success);
    if(!success)
    {
        glGetProgramInfoLog(m_shaderProgram, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
        abort();
    }
    glDeleteShader(m_vertexShader);
    glDeleteShader(m_fragmentShader);
}

GLContext::~GLContext()
{
    glDeleteVertexArrays(1, &m_VAO);
    glDeleteBuffers(1, &m_VBO);
    glDeleteBuffers(1, &m_EBO);
    glDeleteProgram(m_shaderProgram);
}

void GLContext::BindMap()
{
    glGenVertexArrays(1, &m_VAO);
    glGenBuffers(1, &m_VBO);
    glGenBuffers(1, &m_EBO);
    // bind the Vertex Array Object first, then bind and set vertex buffer(s), and then configure vertex attributes(s).
    glBindVertexArray(m_VAO);

    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * m_vertices.size(), m_vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(int) * m_indices.size(), m_indices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(7 * sizeof(float)));
    glEnableVertexAttribArray(3);

    // note that this is allowed, the call to glVertexAttribPointer registered VBO as the vertex attribute's bound vertex buffer object so afterwards we can safely unbind
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // remember: do NOT unbind the EBO while a VAO is active as the bound element buffer object IS stored in the VAO; keep the EBO bound.
    //glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    // You can unbind the VAO afterwards so other VAO calls won't accidentally modify this VAO, but this rarely happens. Modifying other
    // VAOs requires a call to glBindVertexArray anyways so we generally don't unbind VAOs (nor VBOs) when it's not directly necessary.
    glBindVertexArray(0);

    LoadTextures();
}
} // namespace rtdoom