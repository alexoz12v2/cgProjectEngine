#include "Core/Module.h"
#include "Core/Type.h"
#include "Resource/Rendering/GpuProgram.h"
#include "Resource/Rendering/cgeTexture.h"

#include <array>
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

struct RectangleSpec
{
    glm::vec2 position;
    glm::vec2 size;
    glm::vec4 color;
};

enum class ETextureRenderMode
{
    Default = 0,
    ConstantSizeNoStretching,
    ConstantRatioNoStretching,
};

struct TextureSpec
{
    glm::vec2          position;
    glm::vec2          size;
    Sid_t              texture;
    ETextureRenderMode renderMode;
};

class Renderer2D
{
  public:
    using TextureMap = std::pmr::unordered_map<Sid_t, Texture_s>;

    Renderer2D()                              = default;
    Renderer2D(Renderer2D const &)            = delete;
    Renderer2D(Renderer2D &&)                 = delete;
    Renderer2D &operator=(Renderer2D const &) = delete;
    Renderer2D &operator=(Renderer2D &&)      = delete;
    ~Renderer2D();

    void init();
    bool fillCharaterMap(U32_t points);

    void renderText(Char8_t const *text, glm::vec3 xyScale, glm::vec3 color) const;
    void renderButton(ButtonSpec const &specs) const;
    void renderRectangle(RectangleSpec const &spec) const;
    void renderTexture(TextureSpec const &spec);

    void onFramebufferSize(I32_t width, I32_t height);

    glm::ivec2 letterSize() const;

  private:
    void                             prepare(GpuProgram_s const &) const;
    Renderer2D::TextureMap::iterator uploadTextureToGPU(Sid_t sid);

  private:
    struct Character
    {
        Character(U32_t textureID, glm::ivec2 size, glm::ivec2 bearing, I32_t advance);
        Character(Character const &) = delete;
        Character(Character &&) noexcept;
        Character &operator=(Character const &) = delete;
        Character &operator=(Character &&) noexcept;
        ~Character();
        U32_t      textureID; // ID handle of the glyph texture
        glm::ivec2 size;      // Size of glyph
        glm::ivec2 bearing;   // Offset from baseline to left/top of glyph
        I32_t      advance;   // Offset to advance to next glyph
    };

    // delay initialization of GPU program after glad has loaded OpenGL
    // functions
    union U
    {
        U()
        {
        }
        ~U()
        {
        }
        struct S
        {
            GpuProgram_s textProgram;
            GpuProgram_s buttonProgram;
            GpuProgram_s rectangleProgram;
            GpuProgram_s textureProgram;
        };
        S                           s;
        std::array<GpuProgram_s, 4> arr;
        static_assert(sizeof(S) == sizeof(arr));
    };

    glm::mat4                                m_projection{ glm::mat4(1.f) };
    void                                    *m_freeType{ nullptr };
    std::pmr::unordered_map<char, Character> m_characterMap{ getMemoryPool() };
    TextureMap                               m_textureMap{ getMemoryPool() };
    U32_t                                    m_textVAO{ 0 }, m_textVBO{ 0 };
    U32_t                                    m_buttonVAO{ 0 }, m_buttonVBO{ 0 }; // rectangle and button share VAO/VBO
    U                                        m_delayedCtor;
    glm::ivec2                               m_windowSize{ 0, 0 };
    bool                                     m_init{ false };
    U32_t                                    m_points{ 0 };
};

extern Renderer2D g_renderer2D;

} // namespace cge

#endif // CGE_RENDERER2D_S_H
