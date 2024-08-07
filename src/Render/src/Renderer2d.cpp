#include "Renderer2d.h"

#include "Window.h"

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
#version 460 core
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
#version 460 core
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

static Char8_t const *const buttonVS = R"a(
#version 460 core
layout (location = 0) in vec2 vPos;

uniform mat4 projection;

void main() 
{
    gl_Position = projection * vec4(vPos, -0.5f, 1.f);
}
)a";

static Char8_t const *const buttonFS = R"a(
#version 460 core
out vec4 color;

uniform vec2 resolution;     // framebuffer size
uniform vec2 buttonSize;     // normalized with respect to window size
uniform vec3 fillColor;
uniform vec3 borderColor;
uniform float borderWidth;   // normalized with respect to window size
uniform vec2 buttonCenter;   // normalized with respect to window size
uniform vec2 buttonPosition;   // normalized with respect to window size

void main()
{
    // Calculate UV coordinates
    vec2 ratioVec = resolution * buttonSize;
    vec2 uv = (gl_FragCoord.xy / resolution - buttonPosition) / buttonSize;
    vec2 border = borderWidth * vec2(1.f, ratioVec.x / ratioVec.y);
    
    // Determine if the fragment is in the border area
    bool inBorder = (uv.x < border.x || uv.x > 1.f - border.x || uv.y < border.y || uv.y > 1.f -border.y);
    
    if (inBorder) {
        color = vec4(borderColor, 1.0);
    } else {
        color = vec4(fillColor, 1.0);
    }
}
)a";

void Renderer2D::init()
{
    // requires initialization of focused window before renderer. See main
    m_windowSize = g_focusedWindow()->getFramebufferSize();

    static_assert(std::is_standard_layout_v<Renderer2D::U>);
    std::construct_at<GpuProgram_s>(
      reinterpret_cast<GpuProgram_s *>(&m_textProgram));
    std::construct_at<GpuProgram_s>(
      reinterpret_cast<GpuProgram_s *>(&m_buttonProgram));

    m_textProgram.toggleInit();
    m_buttonProgram.toggleInit();
    auto *p = reinterpret_cast<FT_Library *>(&m_freeType);
    if (FT_Init_FreeType(p) || fillCharaterMap(48))
    { //
        printf("[Renderer2D] ERROR: couldn't initialize Renderer2D\n");
    }

    static U32_t constexpr stagesCount{ 2 };
    char const *sources[stagesCount]{ textVS, textFS };
    U32_t       stages[stagesCount]{ GL_VERTEX_SHADER, GL_FRAGMENT_SHADER };
    m_textProgram.p.build({ .sid{ CGE_SID("TEXT_PROGRAM") },
                            .pSources{ sources },
                            .pStages{ stages },
                            .sourcesCount{ stagesCount } });

    Char8_t const *bSources[stagesCount]{ buttonVS, buttonFS };
    m_buttonProgram.p.build({ .sid{ CGE_SID("BUTTON PROGRAM") },
                              .pSources{ bSources },
                              .pStages{ stages },
                              .sourcesCount{ stagesCount } });

    // configure VAO/VBO for texture quads
    glGenVertexArrays(1, &m_textVAO);
    glGenVertexArrays(1, &m_buttonVAO);
    glGenBuffers(1, &m_textVBO);
    glGenBuffers(1, &m_buttonVBO);

    /// text configuration
    glBindVertexArray(m_textVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_textVBO);

    // allocate 6 vertices of 4 floats each
    glBufferData(GL_ARRAY_BUFFER, sizeof(F32_t) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(F32_t), 0);

    /// button configuration
    glBindVertexArray(m_buttonVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_buttonVBO);

    // allocate 4 vertices (triangle strip) of 2 floats each
    glBufferData(GL_ARRAY_BUFFER, sizeof(F32_t) * 4 * 2, NULL, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(F32_t), 0);

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
        glDeleteBuffers(1, &m_textVBO);
        glDeleteBuffers(1, &m_buttonVBO);
        glDeleteVertexArrays(1, &m_textVAO);
        glDeleteVertexArrays(1, &m_buttonVAO);
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

    // default, who cares
    m_projection = glm::ortho(0.0f, F32_t(width), 0.0f, F32_t(height));
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

    m_points = points;
    return false;
}

void Renderer2D::renderButton(ButtonSpec const &specs) const
{
    if (
      specs.position.x <= 0 || specs.position.y <= 0 || specs.size.x <= 0
      || specs.size.y <= 0)
    {
        return;
    }
    const std::string_view str{ specs.text };

    m_buttonProgram.p.bind();
    prepare();

    // Create quad vertices for the button
    F32_t const xLeft   = specs.position.x * m_windowSize.x;
    F32_t const yBottom = specs.position.y * m_windowSize.y;
    F32_t const xRight  = xLeft + specs.size.x * m_windowSize.x;
    F32_t const yTop    = yBottom + specs.size.y * m_windowSize.y;

    static U32_t constexpr numVertices                = 4;
    static U32_t constexpr numComponents              = 2;
    F32_t const vertices[numVertices * numComponents] = {
        xLeft,  yTop,    // top-left
        xLeft,  yBottom, // bottom-left
        xRight, yTop,    // top-right
        xRight, yBottom  // bottom-right
    };

    // activate render state
    glUniform2f(
      glGetUniformLocation(m_buttonProgram.p.id(), "resolution"),
      m_windowSize.x,
      m_windowSize.y);
    glUniform2f(
      glGetUniformLocation(m_buttonProgram.p.id(), "buttonSize"),
      specs.size.x,
      specs.size.y);
    glUniform3f(
      glGetUniformLocation(m_buttonProgram.p.id(), "fillColor"),
      specs.backgroundColor.x,
      specs.backgroundColor.y,
      specs.backgroundColor.z);
    glUniform3f(
      glGetUniformLocation(m_buttonProgram.p.id(), "borderColor"),
      specs.borderColor.x,
      specs.borderColor.y,
      specs.borderColor.z);

    glm::vec2 const center{ specs.size * 0.5f + specs.position };
    glUniform1f(
      glGetUniformLocation(m_buttonProgram.p.id(), "borderWidth"),
      specs.borderWidth);
    glUniform2f(
      glGetUniformLocation(m_buttonProgram.p.id(), "buttonCenter"),
      center.x,
      center.y);
    glUniform2f(
      glGetUniformLocation(m_buttonProgram.p.id(), "buttonPosition"),
      specs.position.x,
      specs.position.y);


    glBindVertexArray(m_buttonVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_buttonVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, numVertices);

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    Character const &c = m_characterMap.at('O');
    auto const       sizeRatioVec{
        specs.size * (1.f - specs.borderWidth)
        * static_cast<decltype(specs.size)>(m_windowSize)
        / (static_cast<decltype(specs.size)>(c.size) * glm::vec2(str.size(), 1))
    };
    F32_t const     sizeRatio = fmin(sizeRatioVec.x, sizeRatioVec.y);
    glm::vec3 const xyScale{
        specs.position.x * (1.f + specs.borderWidth) * m_windowSize.x,
        (specs.position.y + specs.size.y * 0.5f) * m_windowSize.y
          - c.size.y * sizeRatio / 2,
        sizeRatio
    };

    if (!str.empty())
    { //
        renderText(specs.text, xyScale, specs.textColor);
    }
}

Renderer2D::Character::~Character()
{ //
    glDeleteTextures(1, &textureID);
}

void Renderer2D::prepare() const
{
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);
    glEnable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glUniformMatrix4fv(
      glGetUniformLocation(m_textProgram.p.id(), "projection"),
      1,
      GL_FALSE,
      glm::value_ptr(m_projection));
}

void Renderer2D::renderText(
  Char8_t const *text,
  glm::vec3      xyScale,
  glm::vec3      color) const
{
    if (m_windowSize.x <= 0 || m_windowSize.y <= 0)
    { //
        return;
    }

    m_textProgram.p.bind();
    prepare();

    // activate corresponding render state
    glUniform3f(
      glGetUniformLocation(m_textProgram.p.id(), "textColor"),
      color.x,
      color.y,
      color.z);

    glBindVertexArray(m_textVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_textVBO);
    std::string_view const str{ text };

    glActiveTexture(GL_TEXTURE0);

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
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
        // render quad
        glDrawArrays(GL_TRIANGLES, 0, 6);
        // now advance cursors for next glyph (note that advance is number of
        // 1/64 pixels)
        xyScale.x +=
          (ch.advance >> 6)
          * xyScale.z; // bitshift by 6 to get value in pixels (2^6 = 64)
    }
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

} // namespace cge