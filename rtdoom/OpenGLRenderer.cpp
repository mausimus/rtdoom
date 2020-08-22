#include "pch.h"
#include "rtdoom.h"
#include "OpenGLRenderer.h"
#include "MathCache.h"
#include "WireframePainter.h"
#include "SolidPainter.h"
#include "TexturePainter.h"

#include "glad/glad.h"
#include <SDL.h>
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

namespace rtdoom
{

const char* vertexShaderSource = "#version 330 core\n"
                                 "layout (location = 0) in vec3 aPos;\n"
                                 "layout (location = 1) in vec3 aTexCoord;\n"
                                 "layout (location = 2) in float aTexNo;\n"
                                 "layout (location = 3) in float aLight;\n"
                                 "out vec3 TexCoord;\n"
                                 "out float TexNo;\n"
                                 "out float Light;\n"
                                 "uniform mat4 view;\n"
                                 "uniform mat4 projection;\n"
                                 "void main()\n"
                                 "{\n"
                                 "   gl_Position = projection * view * vec4(aPos.x, aPos.y, aPos.z, 1.0);\n"
                                 "   TexCoord = aTexCoord;"
                                 "   TexNo = aTexNo;"
                                 "   Light = aLight;"
                                 "}\0";
const char* fragmentShaderSource =
    "#version 330 core\n"
    "out vec4 FragColor;\n"
    "in vec3 TexCoord;\n"
    "in float TexNo;\n"
    "in float Light;\n"
    "uniform sampler2DArray ourTexture[32];\n"
    "void main()\n"
    "{\n"
    "       FragColor = texture(ourTexture[int(TexNo)], TexCoord);\n"
    //                                   "       FragColor = 0.5f + FragColor * 4.0f * (1.0f - gl_FragCoord.z);\n"
    "       FragColor = FragColor * Light;\n"
    "       FragColor.w = 1.0f;\n"
    "}\n\0";

struct TextureUnit
{
    int                                           n;
    short                                         w;
    short                                         h;
    std::vector<std::shared_ptr<rtdoom::Texture>> textures;
};

int shaderProgram, vertexShader, fragmentShader;

unsigned int                                    VBO, VAO, EBO;
int                                             gl_init   = 0;
int                                             map_ready = 0;
int                                             ii        = 0;
int                                             vi        = 0;
int                                             vc        = 0;
float                                           vertices[250000];
unsigned int                                    indices[150000];
SDL_Window*                                     Window;
std::vector<TextureUnit>                        textureUnits;
unsigned int                                    textures[200];
std::vector<std::vector<std::shared_ptr<Line>>> sectorLines;

void CompileShaders()
{
    vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);
    int  success;
    char infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if(!success)
    {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
        abort();
    }

    fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if(!success)
    {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
        abort();
    }

    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if(!success)
    {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
        abort();
    }
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
}

void AddVertex(float x, float y, float z, float tx, float ty, float tn, float tz, float l)
{
    vertices[vi++] = x;
    vertices[vi++] = y;
    vertices[vi++] = z;
    vertices[vi++] = tx;
    vertices[vi++] = ty;
    vertices[vi++] = tz;
    vertices[vi++] = tn;
    vertices[vi++] = l;
}

void LinkTriangle(int a, int b, int c)
{
    indices[ii++] = a;
    indices[ii++] = b;
    indices[ii++] = c;
}

// wall section
void AddWallSegment(
    float x0, float y0, float z0, float x1, float y1, float z1, float tx0, float ty0, float tx1, float ty1, int tn, int ti, float l)
{
    AddVertex(x0, y0, z0, tx0, ty0, tn, ti, l);
    AddVertex(x1, y1, z0, tx1, ty0, tn, ti, l);
    AddVertex(x1, y1, z1, tx1, ty1, tn, ti, l);
    AddVertex(x0, y0, z1, tx0, ty1, tn, ti, l);
    LinkTriangle(vc, vc + 1, vc + 2);
    LinkTriangle(vc, vc + 2, vc + 3);
    vc += 4;
}

// floor/ceil triangle
void AddFloorCeiling(float x0, float y0, float x1, float y1, float x2, float y2, float z, float tw, float th, int tn, int ti, float l)
{
    AddVertex(x0, y0, z, x0 / tw, y0 / th, tn, ti, l);
    AddVertex(x1, y1, z, x1 / tw, y1 / th, tn, ti, l);
    AddVertex(x2, y2, z, x2 / tw, y2 / th, tn, ti, l);
    LinkTriangle(vc, vc + 1, vc + 2);
    vc += 3;
}

void OpenGLRenderer::LoadTexture(std::shared_ptr<Texture> wtex, int i)
{
    int            x, y;
    unsigned char* data = new unsigned char[wtex->width * wtex->height * 3];
    for(y = 0; y < wtex->height; y++)
        for(x = 0; x < wtex->width; x++)
        {
            const auto& c                                                  = m_wadFile.m_palette.colors[wtex->pixels[x + y * wtex->width]];
            data[3 * (x + /*(wtex->height - y - 1)*/ y * wtex->width)]     = c.r;
            data[3 * (x + /*(wtex->height - y - 1)*/ y * wtex->width) + 1] = c.g;
            data[3 * (x + /*(wtex->height - y - 1)*/ y * wtex->width) + 2] = c.b;
        }

    glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, i, wtex->width, wtex->height, 1, GL_RGB, GL_UNSIGNED_BYTE, data);
    delete[] data;
}

void OpenGLRenderer::LoadTextures()
{
    printf("Texture units (%d):\n", textureUnits.size());
    glGenTextures(textureUnits.size(), textures);
    for(int tu = 0; tu < textureUnits.size(); tu++)
    {
        TextureUnit& unit = textureUnits.at(tu);

        printf("  %d = %d x %d (%d):\n", tu, unit.w, unit.h, unit.textures.size());
        glBindTexture(GL_TEXTURE_2D_ARRAY, textures[tu]);
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
    glUseProgram(shaderProgram);

    int tus[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
    glUniform1iv(glGetUniformLocation(shaderProgram, "ourTexture"), textureUnits.size(), tus); // set it manually
}

std::shared_ptr<Texture> OpenGLRenderer::AllocateTexture(std::string name, int& textureUnit, int& textureNo)
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
    for(tui = 0; tui < textureUnits.size(); tui++)
    {
        if(textureUnits[tui].w == wtex->width && textureUnits[tui].h == wtex->height)
        {
            textureUnit = tui;
            for(int tn = 0; tn < textureUnits[tui].textures.size(); tn++)
            {
                if(textureUnits[tui].textures[tn]->name == name)
                {
                    textureNo = tn;
                    return wtex;
                }
            }
            textureNo = textureUnits[tui].textures.size();
            textureUnits[tui].textures.push_back(wtex);
            return wtex;
        }
    }

    // create and retry
    TextureUnit tu;
    tu.w = wtex->width;
    tu.h = wtex->height;
    tu.n = textureUnits.size();
    textureUnits.push_back(tu);
    return AllocateTexture(name, textureUnit, textureNo);
}

void OpenGLRenderer::Resize(int w, int h)
{
    glViewport(0, 0, w, h);
    m_renderW = w;
    m_renderH = h;
}

std::vector<Point> FindOutline(std::set<Point>& points, std::vector<std::shared_ptr<Line>>& lines)
{
    std::vector<Point> outline;

    // find a point with most extreme coordinates
    Point start = *points.begin();
    for(const auto& p : points)
    {
        if(p.x + p.y > start.x + start.y)
        {
            start = p;
        }
    }

    Point current = start;
    outline.push_back(current);
    points.erase(current);

    while(!points.empty())
    {
        // find next point connected still in the set
        Point next(current);
        for(const auto& line : lines)
        {
            if(current == line->s && points.find(line->e) != points.end())
            {
                next = line->e;
                break;
            }
            else if(current == line->e && points.find(line->s) != points.end())
            {
                next = line->s;
                break;
            }
        }
        if(next == current)
        {
            // haven't found any more connected points
            return outline;
        }

        outline.push_back(next);
        points.erase(next);
        current = next;
    }
    return outline;
}

std::vector<std::vector<Point>> ExtractOutline(std::vector<std::shared_ptr<Line>> lines)
{
    // need to identify points that have more than 2 connectors

    // sort points by connections
    std::set<Point>                 points;
    std::vector<Point>              outline;
    std::vector<std::vector<Point>> holes;

    // add all points
    for(const auto& line : lines)
    {
        points.insert(line->s);
        points.insert(line->e);
    }

    std::vector<std::vector<Point>> result;
    result.push_back(FindOutline(points, lines));
    while(!points.empty())
    {
        // holes
        result.push_back(FindOutline(points, lines));
    }
    return result;
}

void OpenGLRenderer::GenerateMap()
{
    int q = 0;
    sectorLines.resize(m_gameState.m_mapDef->m_sectors.size());
    for(const auto& segment : m_gameState.m_mapDef->m_segments)
    {
        const auto&              frontSector = segment->frontSide.sector;
        int                      textureUnit = 0, textureNo = 0;
        float                    lightness = frontSector.lightLevel;
        std::shared_ptr<Texture> wtex;
        // auto-shade 90-degree edges
        if(segment->isVertical)
        {
            lightness *= 1.1f;
        }
        else if(segment->isHorizontal)
        {
            lightness *= 0.9f;
        }
        if(segment->isSolid)
        {
            wtex                  = AllocateTexture(segment->frontSide.middleTexture, textureUnit, textureNo);
            const auto texXOffset = (segment->xOffset + segment->frontSide.xOffset) / (float)wtex->width;
            const auto texYOffset = (segment->frontSide.yOffset) / (float)wtex->height;
            const auto segW       = Projection::Distance(segment->s, segment->e) / (float)wtex->width;
            const auto segH       = (frontSector.ceilingHeight - frontSector.floorHeight) / (float)wtex->height;

            AddWallSegment(segment->s.x,
                           segment->s.y,
                           frontSector.floorHeight,
                           segment->e.x,
                           segment->e.y,
                           frontSector.ceilingHeight,
                           texXOffset,
                           texYOffset + segH,
                           texXOffset + segW,
                           texYOffset,
                           textureUnit,
                           textureNo,
                           lightness);
        }
        else
        {
            const auto& backSector = segment->backSide.sector;

            if(segment->frontSide.lowerTexture != "-" && segment->frontSide.lowerTexture != "")
            {
                wtex                  = AllocateTexture(segment->frontSide.lowerTexture, textureUnit, textureNo);
                const auto texXOffset = (segment->xOffset + segment->frontSide.xOffset) / (float)wtex->width;
                auto       texYOffset = (segment->frontSide.yOffset) / (float)wtex->height;
                const auto segW       = Projection::Distance(segment->s, segment->e) / (float)wtex->width;
                const auto segH       = abs(frontSector.floorHeight - backSector.floorHeight) / (float)wtex->height;

                if(segment->lowerUnpegged)
                {
                    // peg to ceiling
                    texYOffset += (frontSector.ceilingHeight - backSector.floorHeight) / (float)wtex->height;
                }

                AddWallSegment(segment->s.x,
                               segment->s.y,
                               frontSector.floorHeight,
                               segment->e.x,
                               segment->e.y,
                               backSector.floorHeight,
                               texXOffset,
                               texYOffset + segH,
                               texXOffset + segW,
                               texYOffset,
                               textureUnit,
                               textureNo,
                               lightness);
            }

            if(segment->frontSide.upperTexture != "-" && segment->frontSide.lowerTexture != "")
            {
                wtex                  = AllocateTexture(segment->frontSide.upperTexture, textureUnit, textureNo);
                const auto texXOffset = (segment->xOffset + segment->frontSide.xOffset) / (float)wtex->width;
                auto       texYOffset = (segment->frontSide.yOffset) / (float)wtex->height;
                const auto segW       = Projection::Distance(segment->s, segment->e) / (float)wtex->width;
                const auto segH       = abs(backSector.ceilingHeight - frontSector.ceilingHeight) / (float)wtex->height;

                if(segment->upperUnpegged)
                {
                    // peg to ceiling - not tested
                    //texYOffset += (frontSector.ceilingHeight - backSector.floorHeight) / (float)wtex->height;
                }

                AddWallSegment(segment->s.x,
                               segment->s.y,
                               backSector.ceilingHeight,
                               segment->e.x,
                               segment->e.y,
                               frontSector.ceilingHeight,
                               texXOffset,
                               texYOffset + segH,
                               texXOffset + segW,
                               texYOffset,
                               textureUnit,
                               textureNo,
                               lightness);
            }
        }

        if(segment->frontSide.sector.sectorId >= 0 && segment->frontSide.sector.sectorId != segment->backSide.sector.sectorId)
        {
            sectorLines[segment->frontSide.sector.sectorId].push_back(segment);
        }

        /*        if(segment->backSide.sector.sectorId >= 0)
        {
            sectorLines[segment->backSide.sector.sectorId].push_back(segment);
        }*/
    }

    for(const auto& ss : m_gameState.m_mapDef->m_subSectors)
    {
        const auto& sector = m_gameState.m_mapDef->m_sectors[ss->sectorId];
        if(ss->segments.size())
        {
            // add triangle fan
            auto   curr = ss->segments.begin();
            Vertex fanStart((*curr)->s);
            while(++curr != ss->segments.end())
            {
                std::shared_ptr<Texture> wtex;
                int                      textureUnit, textureNo;
                if(sector.floorTexture != "-")
                {
                    wtex = AllocateTexture(sector.floorTexture, textureUnit, textureNo);
                    AddFloorCeiling((*curr)->e.x,
                                    (*curr)->e.y,
                                    (*curr)->s.x,
                                    (*curr)->s.y,
                                    fanStart.x,
                                    fanStart.y,
                                    sector.floorHeight,
                                    wtex->width,
                                    wtex->height,
                                    textureUnit,
                                    textureNo,
                                    sector.lightLevel);
                }
                if(sector.ceilingTexture != "-")
                {
                    wtex = AllocateTexture(sector.ceilingTexture, textureUnit, textureNo);
                    AddFloorCeiling(fanStart.x,
                                    fanStart.y,
                                    (*curr)->s.x,
                                    (*curr)->s.y,
                                    (*curr)->e.x,
                                    (*curr)->e.y,
                                    sector.ceilingHeight,
                                    wtex->width,
                                    wtex->height,
                                    textureUnit,
                                    textureNo,
                                    sector.lightLevel);
                }
            }
        }
    }

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);
    // bind the Vertex Array Object first, then bind and set vertex buffer(s), and then configure vertex attributes(s).
    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * vi, vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(int) * ii, indices, GL_STATIC_DRAW);

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
}

void InitGL()
{
    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 5);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 5);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 5);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

    SDL_GLContext Context = SDL_GL_CreateContext(Window);
    SDL_GL_SetSwapInterval(1);

    if(!gladLoadGL())
    {
        printf("Error initializing GL!\n");
        abort();
    }

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    CompileShaders();
}

void DestroyGL()
{
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteProgram(shaderProgram);
}

OpenGLRenderer::OpenGLRenderer(const GameState& gameState, const WADFile& wadFile, SDL_Window* window, int renderW, int renderH) :
    Renderer {gameState}, m_wadFile {wadFile}, m_window {window}, m_renderW {renderW}, m_renderH {renderH}
{
    Window = m_window;
}

void OpenGLRenderer::Reset()
{
    map_ready = 0;
    ii        = 0;
    vi        = 0;
    vc        = 0;
    textureUnits.clear();
    sectorLines.clear();
}

// entry method for rendering a frame
void OpenGLRenderer::RenderFrame()
{
    if(!gl_init)
    {
        InitGL();
        gl_init = 1;
    }
    if(!map_ready)
    {
        GenerateMap();
        LoadTextures();
        map_ready = 1;
    }
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glm::mat4 viewMat       = glm::mat4(1.0f);
    viewMat                 = glm::rotate(viewMat, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    viewMat                 = glm::scale(viewMat, glm::vec3(0.001f, 0.001f, 0.001f));
    viewMat                 = glm::rotate(viewMat, -m_gameState.m_player.a + glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    viewMat                 = glm::translate(viewMat, glm::vec3(-m_gameState.m_player.x, -m_gameState.m_player.y, -m_gameState.m_player.z));
//    glm::mat4 projectionMat = glm::perspective(glm::radians(48.5f) / 1.15f, 1.15f * m_renderW / (float)m_renderH, 0.01f, 100.0f);
    glm::mat4 projectionMat = glm::perspective(glm::radians(53.75f) / 1.27f, 1.27f * m_renderW / (float)m_renderH, 0.01f, 100.0f);

    unsigned int viewLoc = glGetUniformLocation(shaderProgram, "view");
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(viewMat));

    unsigned int projectionLoc = glGetUniformLocation(shaderProgram, "projection");
    glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, glm::value_ptr(projectionMat));

    for(int tu = 0; tu < textureUnits.size(); tu++)
    {
        glActiveTexture(GL_TEXTURE0 + tu);
        glBindTexture(GL_TEXTURE_2D_ARRAY, textures[tu]);
    }

    glUseProgram(shaderProgram);
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, ii, GL_UNSIGNED_INT, 0);

    SDL_GL_SwapWindow(Window);
}

void OpenGLRenderer::RenderFrame(FrameBuffer& frameBuffer)
{
    /*
    Initialize(frameBuffer);

    // walls
    RenderSegments();

    // floors and ceilings
    RenderPlanes();

    // things/objects
    RenderSprites();

    // HUD
    RenderOverlay();*/
}

void OpenGLRenderer::Initialize() { }

void OpenGLRenderer::RenderSegments() const
{
    // iterate through all segments (map lines) in visibility order returned by traversing the map's BSP tree
    for(const auto& segment : m_gameState.m_mapDef->GetSegmentsToDraw(m_gameState.m_player))
    {
        // only draw segments that are facing the player
        if(MapDef::IsInFrontOf(m_gameState.m_player, *segment))
        {
            RenderMapSegment(*segment);

            // stop drawing once the frame has been fully occluded with solid walls or vertical spans
            /*if(m_frame->IsOccluded())
            {
                break;
            }*/
        }
    }
}

void OpenGLRenderer::RenderMapSegment(const Segment& segment) const
{
#if(0)
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
#endif
}

#if(0)
// draw a visible span of a mapSegment on the frame buffer
void GLRenderer::RenderMapSegmentSpan(const Frame::Span& span, const VisibleSegment& visibleSegment) const
{
    const auto& mapSegment  = visibleSegment.mapSegment;
    const auto& frontSector = mapSegment.frontSide.sector;
    Frame::Clip lowerClip {span}, middleClip {span}, upperClip {span};

    // iterate through all vertical columns from left to right
    for(auto x = span.s; x <= span.e; x++)
    {
        // calculate the relative viewAngle and distance to the mapSegment from our viewpoint
        auto viewAngle          = GetViewAngle(x, visibleSegment);
        auto projectionDistance = m_projection->Distance(visibleSegment.normalVector, viewAngle);
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
#endif

// render floors and ceilings based on data collected during drawing walls
void OpenGLRenderer::RenderPlanes() const
{
#if(0)
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
#endif
}

// render sprites (things or semi-transparent walls)
void OpenGLRenderer::RenderSprites() const
{
#if(0)
    // add things in visited sectors to list of sprites
    for(const auto s : m_frame->m_sectors)
    {
        if(s >= 0)
        {
            for(const auto t : m_gameState.m_mapDef->m_things[s])
            {
                const auto viewAngle          = m_projection->ProjectionAngle(t);
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
#endif
}

#if(0)
// render things
void GLRenderer::RenderSpriteThing(Frame::SpriteThing* const thing) const
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

    const auto scale = m_projection->TextureScale(thing->distance);
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

// render semi-transparent walls
void GLRenderer::RenderSpriteWall(Frame::SpriteWall* const wall) const
{
    m_painter->PaintWall(wall->x, wall->span, wall->textureContext);
}
#endif

void OpenGLRenderer::RenderOverlay() const
{
    // TODO: render HUD etc.
}
/*
Frame* GLRenderer::GetLastFrame() const
{
    return m_frame.get();
}
*/
OpenGLRenderer::~OpenGLRenderer() { }
} // namespace rtdoom
