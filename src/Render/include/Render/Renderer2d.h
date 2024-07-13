#include "Core/Type.h"

#include "Resource/Rendering/GpuProgram.h"

#include <unordered_map>

#ifndef CGE_RENDERER2D_S_H
#define CGE_RENDERER2D_S_H

namespace cge
{

struct ButtonSpec
{
    glm::vec2      position;        // Button position (bottom-left corner)
    glm::vec2      size;            // Button size
    glm::vec3      borderColor;     // Border color
    F32_t          borderWidth;     // Border width
    glm::vec3      backgroundColor; // Background color
    Char8_t const *text;            // Text to display on the button
    glm::vec3      textColor;       // Text color
};

class Renderer2D
{
  public:
    void init();
    ~Renderer2D();

    bool fillCharaterMap(U32_t points);

    void
      renderText(Char8_t const *text, glm::vec3 xyScale, glm::vec3 color) const;
    void renderButton(ButtonSpec const &specs) const;

    void onFramebufferSize(I32_t width, I32_t height);

  private:
    void prepare() const;

  private:
    struct Character
    {
        ~Character();
        U32_t      textureID; // ID handle of the glyph texture
        glm::ivec2 size;      // Size of glyph
        glm::ivec2 bearing;   // Offset from baseline to left/top of glyph
        I32_t      advance;   // Offset to advance to next glyph
    };

    // delay initialization of GPU program after glad has loaded OpenGL
    // functions
    struct U
    {
        U() : m_init(false) {}
        ~U()
        {
            if (m_init)
            { //
                p.~GpuProgram_s();
            }
        }
        union
        {
            GpuProgram_s p;
        };

        void toggleInit() { m_init = !m_init; }

        bool m_init;
    };

    glm::mat4                                m_projection{ glm::mat4(1.f) };
    void                                    *m_freeType{ nullptr };
    std::pmr::unordered_map<char, Character> m_characterMap;
    U32_t                                    m_textVAO{ 0 }, m_textVBO{ 0 };
    U32_t                                    m_buttonVAO{ 0 }, m_buttonVBO{ 0 };
    U                                        m_textProgram;
    U                                        m_buttonProgram;
    glm::ivec2                               m_windowSize{ 0, 0 };
    bool                                     m_init{ false };
    U32_t                                    m_points{ 0 };
};

extern Renderer2D g_renderer2D;

} // namespace cge

#endif // CGE_RENDERER2D_S_H
