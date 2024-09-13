#include "MenuModule.h"

#include "Core/Event.h"
#include "Core/Events.h"
#include "Core/KeyboardKeys.h"
#include "Launch/Entry.h"
#include "Render/Renderer2d.h"
#include "Resource/HandleTable.h"

#include <fstream>
#include <iterator>
#include <memory>
#include <nlohmann/json.hpp>
#include <stb/stb_image.h>

CGE_DECLARE_MODULE(cge, MenuModule, "MenuModule");

namespace cge
{

// -- constants --
inline U32_t constexpr numMainButtons   = 3;
inline U32_t constexpr numExtrasButtons = 1;

inline Char8_t constexpr const *const mainScreen_START     = "START";
inline Char8_t constexpr const *const mainScreen_EXTRAS    = "EXTRAS";
inline Char8_t constexpr const *const mainScreen_EXIT      = "EXIT";
inline Char8_t constexpr const *const extrasScreen_GO_BACK = "GO BACK";

inline std::array<ButtonSpec, numMainButtons> const mainScreenButtons{
    // ----------------------------------------------------------------
    ButtonSpec{
      .position{ 0.05f, 0.7f },
      .size{ 0.2f, 0.1f },
      .borderColor{ 0.5098f, 0.45098f, 0.294118f },
      .borderWidth = 0.02f,
      .backgroundColor{ 0.2f },
      .text = mainScreen_START,
      .textColor{ 0.7f },
    },
    {
      .position{ 0.05f, 0.5f },
      .size{ 0.2f, 0.1f },
      .borderColor{ 0.5098f, 0.45098f, 0.294118f },
      .borderWidth = 0.02f,
      .backgroundColor{ 0.2f },
      .text = mainScreen_EXTRAS,
      .textColor{ 0.7f },
    },
    {
      .position{ 0.05f, 0.3f },
      .size{ 0.2f, 0.1f },
      .borderColor{ 0.5098f, 0.45098f, 0.294118f },
      .borderWidth = 0.02f,
      .backgroundColor{ 0.2f },
      .text = mainScreen_EXIT,
      .textColor{ 0.7f },
    }
};

inline std::array<ButtonSpec, numExtrasButtons> const extrasScreenButtons{
    // ----------------------------------------------------------------
    ButtonSpec{
      .position{ 0.7f, 0.1f },
      .size{ 0.2f, 0.1f },
      .borderColor{ 0.5098f, 0.45098f, 0.294118f },
      .borderWidth = 0.02f,
      .backgroundColor{ 0.2f },
      .text = extrasScreen_GO_BACK,
      .textColor{ 0.7f },
    }
};

inline glm::vec2 constexpr extrasScreenRectSize{ 0.4f, 0.9f };
inline F32_t constexpr extrasScreenRectBorderSizePerc = 0.05f;
inline F32_t constexpr extrasScreenRectLineLeading    = 0.02f;
inline U32_t constexpr numLines                       = 11; // 1 line for "Best Scores" followed by the top ten scores
inline U32_t constexpr maxNumCharInLine               = 13;

inline Char8_t const constexpr *const jsonScoresKey = "bestScores";
inline Char8_t const constexpr *const jsonSavePath  = "../assets/saves.json";

// -- the rest of the code --
struct STBIDeleter
{
    void operator()(Byte_t *data) const
    {
        stbi_image_free(data);
    }
};

MenuModule::~MenuModule()
{
    if (m_init)
    {
        for (auto const &pair : m_listeners.arr)
        {
            g_eventQueue.removeListener(pair);
        }

        if (m_bop)
        {
            m_bop->stop();
            m_bop->drop();
        }

        g_soundEngine()->removeSoundSource(m_bopSource);
    }
}

static void loadTexture(Sid_t const &sid, Char8_t const *CGE_restrict path)
{
    I32_t texWidth      = 0;
    I32_t texHeight     = 0;
    I32_t texChannelCnt = 0;
    auto *tex = reinterpret_cast<Byte_t *>(stbi_load(path, &texWidth, &texHeight, &texChannelCnt, STBI_rgb_alpha));
    assert(texChannelCnt == 4);
    std::shared_ptr<Byte_t> data(tex, STBIDeleter{});

    g_handleTable.insertTexture(
      sid,
      { .data{ data },
        .width  = static_cast<U32_t>(texWidth),
        .height = static_cast<U32_t>(texHeight),
        .depth  = 1,
        .format = GL_RGBA,
        .type   = GL_UNSIGNED_BYTE });
}

void MenuModule::onInit()
{
    EventArg_t listenerData{};
    listenerData.idata.p = reinterpret_cast<decltype(listenerData.idata.p)>(this);
    m_listeners.s.framebufferSizeListener =
      g_eventQueue.addListener(evFramebufferSize, framebufferSizeCallback<MenuModule>, listenerData);
    m_listeners.s.mouseMovementListener =
      g_eventQueue.addListener(evMouseMoved, mouseMovementCallback<MenuModule>, listenerData);
    m_listeners.s.mouseButtonListener =
      g_eventQueue.addListener(evMouseButtonPressed, mouseButtonCallback<MenuModule>, listenerData);

    m_bopSource = g_soundEngine()->addSoundSourceFromFile("../assets/bop.mp3");

    deserializeScoresFromFile();
    if (g_globalStore.contains(CGE_SID("score")))
    {
        U64_t score = g_globalStore.consume(CGE_SID("score")).u64[0];
        // Check if the vector is not full or the new score is larger than the smallest score
        if (m_scores.size() < 10 || score > m_scores.back())
        {   // Insert the new score in the correct position to maintain descending order
            auto it = std::lower_bound(m_scores.begin(), m_scores.end(), score, std::less<U64_t>());

            // Insert the new score at the position found
            m_scores.insert(it, score);

            // If the vector size exceeds 10, remove the smallest element
            if (m_scores.size() > 10)
            {
                m_scores.pop_back();
            }
        }
    }

    if (!initializedOnce())
    {
        stbi_set_flip_vertically_on_load(true);
        loadTexture(CGE_SID("MENU"), "../assets/menu.png");
        loadTexture(CGE_SID("EXTRAS"), "../assets/extras.png");
        loadTexture(CGE_SID("DUNE RUN"), "../assets/dune_run.png");
    }

    IModule::onInit();
    m_init = true;
}

void MenuModule::onTick(U64_t deltaTime)
{
    F32_t ratio = aspectRatio();
    switch (m_menuScreen)
    {
    case EMenuScreen::eMain:
    {
        g_renderer2D.renderTexture({ .position{ 0.f, 0.f },
                                     .size{ 1.f, 1.f },
                                     .texture{ CGE_SID("MENU") },
                                     .renderMode = ETextureRenderMode::ConstantSizeNoStretching,
                                     .depth      = 0.9f });
        g_renderer2D.renderTexture({ .position{ 0.57f, 0.57f },
                                     .size{ 0.4f, 0.4f },
                                     .texture{ CGE_SID("DUNE RUN") },
                                     .renderMode = ETextureRenderMode::ConstantRatioNoStretching,
                                     .depth      = 0.8f });
        for (auto const &button : mainScreenButtons)
        {
            g_renderer2D.renderButton(button);
        }
        break;
    }
    case EMenuScreen::eExtras:
    {
        glm::vec2 const rectPos{ (ratio > 1.f ? 0.02f : 0.05f), (ratio > 1.f ? 0.05f : 0.02f) };
        g_renderer2D.renderTexture({ .position{ 0.f, 0.f },
                                     .size{ 1.f, 1.f },
                                     .texture{ CGE_SID("EXTRAS") },
                                     .renderMode = ETextureRenderMode::ConstantSizeNoStretching,
                                     .depth      = 0.9f });
        g_renderer2D.renderRectangle({ .position{ rectPos }, .size{ extrasScreenRectSize }, .color{ 0.5f } });
        for (auto const &button : extrasScreenButtons)
        {
            g_renderer2D.renderButton(button);
        }

        F32_t yBorder    = extrasScreenRectBorderSizePerc * extrasScreenRectSize.y;
        F32_t yStart     = rectPos.y + yBorder;
        F32_t yIncrement = (extrasScreenRectSize.y - 2 * yBorder) / numLines;
        F32_t ySize      = yIncrement - extrasScreenRectLineLeading * extrasScreenRectSize.y;
        F32_t xSize      = g_renderer2D.letterSize().x * maxNumCharInLine;
        F32_t charSize   = g_renderer2D.letterSize().y;

        glm::vec3 xyScale{ (rectPos.x + extrasScreenRectBorderSizePerc * extrasScreenRectSize.x) * m_framebufferSize.x,
                           yStart * m_framebufferSize.y,
                           glm::min(ySize * m_framebufferSize.y / charSize, xSize / charSize) };
        std::pmr::string strBuf{ getMemoryPool() };

        U32_t space = 1;
        for (U32_t i = 10; i != 0; --i)
        {
            strBuf.clear();
            strBuf.append(std::to_string(i));
            strBuf.append(1, ':');
            strBuf.append(space, ' ');
            space             = 2;
            U32_t       index = 10 - i;
            std::string score = m_scores.size() > index ? std::to_string(m_scores[index]) : "empty";
            strBuf.append(score);

            g_renderer2D.renderText(strBuf.c_str(), xyScale, glm::vec3{ 0.8f });
            xyScale.y += yIncrement * m_framebufferSize.y;
        }

        g_renderer2D.renderText("Best Scores", xyScale, glm::vec3{ 0.8f });
        break;
    }
    }
}

void MenuModule::onFramebufferSize(I32_t width, I32_t height)
{
    m_framebufferSize.x = width;
    m_framebufferSize.y = height;
}

void MenuModule::onMouseMovement(F32_t x, F32_t y)
{
    m_mousePosition = glm::vec2(x, m_framebufferSize.y - y) / static_cast<glm::vec2>(m_framebufferSize);
}

void MenuModule::onMouseButton(I32_t key, I32_t action)
{
    if (action != action::PRESS)
    {
        return;
    }

    printf("[MenuModule] position: \n\t%f\n\t%f\n", m_mousePosition.x, m_mousePosition.y);
    decltype(std::begin(mainScreenButtons)) beg = nullptr;
    decltype(std::end(mainScreenButtons))   end = nullptr;

    switch (m_menuScreen)
    {
    case EMenuScreen::eMain:
        beg = std::begin(mainScreenButtons);
        end = std::end(mainScreenButtons);
        break;
    case EMenuScreen::eExtras:
        beg = std::begin(extrasScreenButtons);
        end = std::end(extrasScreenButtons);
        break;
    }

    for (auto it = beg; it != end; ++it)
    {
        assert(it);
        auto const &button = *it;
        printf(
          "[MenuModule] button: \n\t%f - %f\n\t%f - %f\n",
          button.position.x,
          button.position.x + button.size.x,
          button.position.y,
          button.position.y + button.size.y);
        if (
          m_mousePosition.x >= button.position.x && m_mousePosition.y >= button.position.y
          && m_mousePosition.x < button.position.x + button.size.x
          && m_mousePosition.y < button.position.y + button.size.y)
        {
            printf("[MenuModule] pressed button \"%s\"\n", button.text);
            m_bop = g_soundEngine()->play2D(m_bopSource);
            buttonPressed(CGE_SID(button.text));
            break;
        }
    }
}

void MenuModule::buttonPressed(Sid_t buttonSid)
{
    switch (m_menuScreen)
    {
    case EMenuScreen::eMain:
    {
        switch (buttonSid.id)
        {
        case CGE_CONSTEXPR_SID(mainScreen_START).id:
            switchToModule(CGE_SID("TestbedModule"));
            break;
        case CGE_CONSTEXPR_SID(mainScreen_EXTRAS).id:
            m_menuScreen = EMenuScreen::eExtras;
            break;
        case CGE_CONSTEXPR_SID(mainScreen_EXIT).id:
            serializeScoresToFile();
            tagForDestruction();
            break;
        default:
            break;
        }

        break;
    }

    case EMenuScreen::eExtras:
    {
        switch (buttonSid.id)
        {
        case CGE_CONSTEXPR_SID(extrasScreen_GO_BACK).id:
            m_menuScreen = EMenuScreen::eMain;
            break;
        default:
            break;
        }

        break;
    }
    }
}

[[nodiscard]] F32_t MenuModule::aspectRatio() const
{
    F32_t ratio = m_framebufferSize.x / glm::max(static_cast<F32_t>(m_framebufferSize.y), 0.1f);
    return ratio > std::numeric_limits<F32_t>::epsilon() ? ratio : 1.f;
}

void MenuModule::deserializeScoresFromFile()
{
    // attempt to open the file
    std::ifstream file{ jsonSavePath };
    if (!file)
    { // if it doesn't exist, return empty vector
        m_scores.clear();
        return;
    }

    nlohmann::json json;
    try
    { // parse json from file
        file >> json;
    }
    catch (...)
    {
        printf("[MenuModule] Error opening saves\n");
        m_scores.clear();
        return;
    }

    // if best scores exists and is an array
    if (json.contains(jsonScoresKey) && json[jsonScoresKey].is_array())
    {
        std::pmr::vector<U64_t> tempScores;
        for (auto const &item : json[jsonScoresKey])
        { // take it only if it is unsigned number
            if (item.is_number_unsigned())
            {
                tempScores.push_back(item.get<U64_t>());
            }
        }

        std::ranges::sort(tempScores, std::less<U64_t>());
        if (tempScores.size() > numLines - 1)
        {
            tempScores.resize(numLines - 1);
        }

        m_scores = std::move(tempScores);
    }
    else
    {
        m_scores.clear();
    }
}

void MenuModule::serializeScoresToFile()
{
    nlohmann::json json;
    json[jsonScoresKey] = m_scores;

    std::ofstream file{ jsonSavePath };
    if (file)
    {
        file << json.dump(4);
    }
    else
    {
        printf("[MenuModule] couldn't open save file for writing\n");
    }
}

} // namespace cge
