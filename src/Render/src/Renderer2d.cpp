#include "Renderer2d.h"

#include "ft2build.h"
#include FT_FREETYPE_H

#include <Core/Event.h>
#include <Core/Events.h>
#include <glad/gl.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace cge
{
Renderer2D g_renderer2D;

static char const *const textVS = R"a(
#version 330 core
layout (location = 0) in vec4 vertex; // <vec2 pos, vec2 tex>
out vec2 TexCoords;

uniform mat4 projection;

void main()
{
    gl_Position = projection * vec4(vertex.xy, 0.0, 1.0);
    TexCoords = vertex.zw;
} 
)a";

static char const *const textFS = R"a(
#version 330 core
in vec2 TexCoords;
out vec4 color;

uniform sampler2D text;
uniform vec3 textColor;

void main()
{    
    vec4 sampled = vec4(1.0, 1.0, 1.0, texture(text, TexCoords).r);
    color = vec4(textColor, 1.0) * sampled;
}  
)a";

void Renderer2D::init()
{
    static_assert(std::is_standard_layout_v<Renderer2D::U>);
    std::construct_at<GpuProgram_s>(
      reinterpret_cast<GpuProgram_s *>(&m_textProgram));
    m_textProgram.toggleInit();
    auto *p = reinterpret_cast<FT_Library *>(&m_freeType);
    if (FT_Init_FreeType(p) || fillCharaterMap(48))
    { //
        printf("[Renderer2D] ERROR: couldn't initialize Renderer2D\n");
    }

    static U32_t constexpr stagesCount = 2;
    char const *sources[stagesCount]   = { textVS, textFS };
    U32_t       stages[stagesCount] = { GL_VERTEX_SHADER, GL_FRAGMENT_SHADER };
    m_textProgram.p.build({ .sid          = CGE_SID("TEXT_PROGRAM"),
                            .pSources     = sources,
                            .pStages      = stages,
                            .sourcesCount = stagesCount });

    // configure VAO/VBO for texture quads
    glGenVertexArrays(1, &m_textVAO);
    glGenBuffers(1, &m_textVBO);
    glBindVertexArray(m_textVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_textVBO);

    // allocate 6 vertices of 4 floats each
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // register to callback such that you can get the window size
    EventArg_t listenerData{};
    listenerData.idata.p = (Byte_t *)this;
    g_eventQueue.addListener(
      evFramebufferSize, framebufferSizeCallback<Renderer2D>, listenerData);
    m_init = true;
}

Renderer2D::~Renderer2D()
{
    if (m_init)
    {
        if (FT_Done_FreeType(reinterpret_cast<FT_Library>(m_freeType)))
        { //
            printf("[Renderer2D] ERROR: couldn't destroy FreeType library\n");
        };
    }
}

void Renderer2D::onFramebufferSize(I32_t width, I32_t height)
{
    m_windowSize.x = width;
    m_windowSize.y = height;
}

bool Renderer2D::fillCharaterMap(U32_t points)
{
    FT_Face face;
    if (FT_New_Face(
          reinterpret_cast<FT_Library>(m_freeType),
          "../assets/comic.ttf",
          0,
          &face))
    {
        printf("[Renderer2D] ERROR: Failed to load Comic Sans Font\n");
        return true;
    }

    m_characterMap.clear();

    // set which pixel size you want to retrieve
    // width = 0 -> computed dynamically from height
    FT_Set_Pixel_Sizes(face, 0, points);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1); // disable byte-alignment restriction

    for (unsigned char c = 0; c < 128; c++)
    {
        // load character glyph
        if (FT_Load_Char(face, c, FT_LOAD_RENDER))
        {
            printf("[Renderer2D] ERROR: Failed to load Glyph %c\n", c);
            continue;
        }

        auto &glyph = *face->glyph;

        // generate texture
        U32_t texture;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);

        // both format and interalFormat to GL_RED to generate
        glTexImage2D(
          GL_TEXTURE_2D,
          0,
          GL_RED,
          glyph.bitmap.width,
          glyph.bitmap.rows,
          0,
          GL_RED,
          GL_UNSIGNED_BYTE,
          glyph.bitmap.buffer);

        // set texture options
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        // now store character for later use
        m_characterMap.emplace(
          std::piecewise_construct,
          std::forward_as_tuple(c),
          std::forward_as_tuple(
            texture,
            glm::ivec2(glyph.bitmap.width, glyph.bitmap.rows),
            glm::ivec2(glyph.bitmap_left, glyph.bitmap_top),
            glyph.advance.x));
    }

    FT_Done_Face(face);
    glBindTexture(GL_TEXTURE_2D, 0);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

    return false;
}

Renderer2D::Character::~Character()
{ //
    glDeleteTextures(1, &textureID);
}

static void prepare()
{
    glEnable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void Renderer2D::renderText(
  Char8_t const *text,
  glm::vec3      xyScale,
  glm::vec3      color) const
{
    if (m_windowSize.x == 0 || m_windowSize.y == 0)
    { //
        return;
    }

    prepare();
    std::string_view const str{ text };

    // activate corresponding render state
    m_textProgram.p.bind();
    glUniform3f(
      glGetUniformLocation(m_textProgram.p.id(), "textColor"),
      color.x,
      color.y,
      color.z);
    glm::mat4 projection = glm::ortho(
      0.0f,
      static_cast<F32_t>(m_windowSize.x),
      0.0f,
      static_cast<F32_t>(m_windowSize.y));
    glUniformMatrix4fv(
      glGetUniformLocation(m_textProgram.p.id(), "projection"),
      1,
      GL_FALSE,
      glm::value_ptr(projection));

    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(m_textVAO);

    // iterate through all characters
    for (auto const c : str)
    {
        Character const &ch = m_characterMap.at(c);

        F32_t const xpos = xyScale.x + ch.bearing.x * xyScale.z;
        F32_t const ypos = xyScale.y - (ch.size.y - ch.bearing.y) * xyScale.z;

        F32_t const w = ch.size.x * xyScale.z;
        F32_t const h = ch.size.y * xyScale.z;
        // update VBO for each character
        F32_t const vertices[6][4] = { { xpos, ypos + h, 0.0f, 0.0f },
                                       { xpos, ypos, 0.0f, 1.0f },
                                       { xpos + w, ypos, 1.0f, 1.0f },

                                       { xpos, ypos + h, 0.0f, 0.0f },
                                       { xpos + w, ypos, 1.0f, 1.0f },
                                       { xpos + w, ypos + h, 1.0f, 0.0f } };
        // render glyph texture over quad
        glBindTexture(GL_TEXTURE_2D, ch.textureID);
        // update content of VBO memory
        glBindBuffer(GL_ARRAY_BUFFER, m_textVBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        // render quad
        glDrawArrays(GL_TRIANGLES, 0, 6);
        // now advance cursors for next glyph (note that advance is number of
        // 1/64 pixels)
        xyScale.x +=
          (ch.advance >> 6)
          * xyScale.z; // bitshift by 6 to get value in pixels (2^6 = 64)
    }
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

} // namespace cge