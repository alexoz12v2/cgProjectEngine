#include "Testbed.h"

#include "Core/Event.h"
#include "Core/Events.h"
#include "Core/KeyboardKeys.h"
#include "Core/StringUtils.h"
#include "Core/Type.h"
#include "Core/Utility.h"
#include "Launch/Entry.h"
#include "Render/Renderer.h"
#include "Render/Renderer2d.h"
#include "RenderUtils/GLutils.h"
#include "Resource/HandleTable.h"
#include "Resource/Rendering/cgeMesh.h"
#include "Resource/Rendering/cgeScene.h"

#include "ConstantsAndStructs.h"
#include "SoundEngine.h"

#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/geometric.hpp>
#include <glm/matrix.hpp>
#include <glm/trigonometric.hpp>
#include <nlohmann/json.hpp>
#include <stb/stb_image.h>

#include <Utils.h>
#include <fstream>
#include <vector>

CGE_DECLARE_STARTUP_MODULE(cge, TestbedModule, "TestbedModule");

// TODOs:
// - camera shaking
// - malus speed no invincibility
// - invincibility timer rendered
// - renderer2D.renderRectangle({.position, .size})
// - UI
// - 2x main menu renderings
// - handle an array of best scores in json file and display them in the extras menu
// - background renderer no getter
// ? difficulty
// - depth buffer is not properly used by 3D scene

namespace cge
{

static Sid_t constexpr cubeMeshSid    = "Cube"_sid;
static U32_t constexpr coinBonusScore = 100;

static nlohmann::json json;

TestbedModule::~TestbedModule()
{
    if (m_init)
    {
        for (auto const &pair : m_listeners.arr)
        { //
            g_eventQueue.removeListener(pair);
        }

        g_soundEngine()->removeSoundSource(m_coinPickedSource);
        g_soundEngine()->removeSoundSource(m_woodBreakSource);
    }
}

// boilerplate ----------------------------------------------------------------
void TestbedModule::onInit()
{
    Sid_t const    mId = CGE_SID("TestbedModule");
    Char8_t const *str = CGE_DBG_STRLOOKUP(mId);
    printf("Hello World!! %s\n", str);
#if defined(CGE_DEBUG)
    printf("DebugMode!\n");
#endif
    g_scene.clearSceneNodes();
    g_scene.clearSceneLights();

    // register to all relevant events pressed
    EventArg_t listenerData{};
    listenerData.idata.p      = (Byte_t *)this;
    m_listeners.s.keyListener = g_eventQueue.addListener(evKeyPressed, KeyCallback<TestbedModule>, listenerData);
    m_listeners.s.mouseMovementListener =
      g_eventQueue.addListener(evMouseMoved, mouseMovementCallback<TestbedModule>, listenerData);
    m_listeners.s.mouseButtonListener =
      g_eventQueue.addListener(evMouseButtonPressed, mouseButtonCallback<TestbedModule>, listenerData);
    m_listeners.s.framebufferSizeListener =
      g_eventQueue.addListener(evFramebufferSize, framebufferSizeCallback<TestbedModule>, listenerData);
    m_listeners.s.gameOverListener =
      g_eventQueue.addListener(evGameOver, gameOverCallback<TestbedModule>, listenerData);
    m_listeners.s.shootListener = g_eventQueue.addListener(evShoot, shootCallback<TestbedModule>, listenerData);
    m_listeners.s.magnetAcquiredListener =
      g_eventQueue.addListener(evMagnetAcquired, magnetAcquiredCallback<TestbedModule>, listenerData);

    // open mesh file
    printf("Opening Scene file\n");

    if (!initializedOnce())
    {
        g_handleTable.loadFromObj("../assets/ornithepter.obj");
        g_handleTable.loadFromObj("../assets/plane.obj");
        g_handleTable.loadFromObj("../assets/piece.obj");
        g_handleTable.loadFromObj("../assets/prop.obj");
        g_handleTable.loadFromObj("../assets/destructible.obj");
        g_handleTable.loadFromObj("../assets/magnet.obj");
        g_handleTable.loadFromObj("../assets/coin.obj");
        g_handleTable.loadFromObj("../assets/speed.obj");
    }

    // add light to the scene
    Light_t const sunLight{
        .ambient              = { 0.1f, 0.1f, 0.1f },
        .color                = { 1.f, 1.f, 1.f },
        .position             = glm::normalize(glm::vec3{ -0.4f, -0.3f, 1.f }),
        .halfVector           = glm::normalize(glm::vec3{ -0.4f, -0.3f, 1.f }),
        .coneDirection        = { 0, 0, 0 },
        .spotCosCutoff        = 0,
        .spotExponent         = 0,
        .constantAttenuation  = 0,
        .linearAttenuation    = 0,
        .quadraticAttenuation = 0,
        .isEnabled            = true,
        .isLocal              = false,
        .isSpot               = false,
    };
    g_scene.addLight(CGE_SID("SUN LIGHT"), sunLight);

    std::pmr::vector<Sid_t> pieces{ { CGE_SID("Piece") }, getMemoryPool() };
    std::pmr::vector<Sid_t> obstacles{ { CGE_SID("Obstacle") }, getMemoryPool() };
    std::pmr::vector<Sid_t> destructables{ { CGE_SID("Destructible") }, getMemoryPool() };
    Sid_t                   magnet{ CGE_SID("Magnet") };
    Sid_t                   coin{ CGE_SID("Coin") };
    Sid_t                   speed{ CGE_SID("Speed") };
    m_scrollingTerrain.init({ .pieces        = pieces,
                              .obstacles     = obstacles,
                              .destructables = destructables,
                              .magnetPowerUp = magnet,
                              .coin          = coin,
                              .speed         = speed });


    // setup player
    Camera_t camera{};
    camera.position = glm::vec3(0, 0, 10);
    camera.right    = glm::vec3(1.f, 0.f, 0.f);
    camera.up       = glm::vec3(0.f, 0.f, 1.f);
    camera.forward  = glm::vec3(0.f, 1.f, 0.f);
    m_player.spawn(camera);

    // background, terrain, terrain collisions
    stbi_set_flip_vertically_on_load(true);
    const char    *imagePath = "../assets/background.png";
    int            width, height, channels;
    unsigned char *image = stbi_load(imagePath, &width, &height, &channels, STBI_rgb);
    getBackgroundRenderer().init(image, width, height);
    stbi_image_free(image);

    // sound
    m_coinPickedSource = g_soundEngine()->addSoundSourceFromFile("../assets/coin-picked.mp3");
    m_woodBreakSource  = g_soundEngine()->addSoundSourceFromFile("../assets/wood-break.mp3");
    assert(m_coinPickedSource && m_woodBreakSource);

    // load saves, if existent, otherwise create new ones
    std::ifstream file{ "../assets/saves.json" };
    if (file)
    { //
        json = nlohmann::json::parse(file);
    }
    else
    { //
        std::vector<U64_t> empty;
        json = nlohmann::json::object({ { "bestScore", empty } });
    }

    m_letterSize = g_renderer2D.letterSize().x;
    m_init       = true;
    IModule::onInit();
}

void TestbedModule::onKey(I32_t key, I32_t action)
{
    if (action == action::PRESS)
    {
        if (key == key::ESCAPE)
        {
            onGameOver(m_player.getCurrentScore());
        }
    }
}

void TestbedModule::onMouseButton(I32_t key, I32_t action)
{
    glm::vec2 ndcMousePos{ m_screenMousePos / static_cast<glm::vec2>(m_framebufferSize) };
    ndcMousePos.y = 1 - ndcMousePos.y;
    ndcMousePos -= 0.5f;
    ndcMousePos *= 2;
    if (action == action::PRESS && key == button::LMB)
    { //
        printf("[Testbed] pressed at NDC { %f %f }\n", ndcMousePos.x, ndcMousePos.y);
        isAnyCoinClicked(ndcMousePos);
    }
}

void TestbedModule::onMouseMovement(F32_t xpos, F32_t ypos)
{
    m_screenMousePos.x = xpos;
    m_screenMousePos.y = ypos;
}

void TestbedModule::onFramebufferSize(I32_t width, I32_t height)
{
    m_framebufferSize.x = width;
    m_framebufferSize.y = height;
    printf("width: %u, height: %u, aspectRatio: %f\n", m_framebufferSize.x, m_framebufferSize.y, aspectRatio());
}

void TestbedModule::onGameOver(U64_t score)
{
    printf("\033[33m[Testbed] onGameOver Called\033[0m\n");
    //U64_t curr = json["bestScore"].get<U64_t>();
    //if (score > curr)
    //{ //
    //    json["bestScore"] = score;
    //}
    //std::ofstream file{ "../assets/saves.json", std::ios::out | std::ios::trunc };
    //file << json.dump();
    switchToModule(CGE_SID("MenuModule"));
}

void TestbedModule::onShoot(Ray const &ray)
{ //
    printf("[TestbedModule] BANG\n");
    B8_t intersected = m_scrollingTerrain.handleShoot(m_player.boundingBox());
    if (intersected)
    { //
        g_soundEngine()->play2D(m_woodBreakSource);
    }
}

void TestbedModule::onMagnetAcquired()
{ //
    U32_t numCoins = m_scrollingTerrain.removeAllCoins();
    m_player.incrementScore(coinBonusScore, numCoins);
    printf("[Testbed] MAGNET POWERUP ACQUIRED\n");
}

void TestbedModule::onTick(U64_t deltaTime)
{
    auto const p      = m_player.lastDisplacement();
    auto const camera = m_player.getCamera();
    auto const center = m_player.getCentroid();
    auto const proj = glm::perspective(glm::radians(FOV), aspectRatio(), CLIPDISTANCE, RENDERDISTANCE);

    m_scrollingTerrain.updateTilesFromPosition(center);
    m_player.onTick(deltaTime);
    m_scrollingTerrain.onTick(deltaTime);
    m_player.intersectPlayerWith(m_scrollingTerrain);

    getBackgroundRenderer().renderBackground(m_player.getCamera(), proj);
    g_renderer.renderScene(
      g_scene,
      camera.viewTransform(),
      proj,
      camera.forward);

    glClear(GL_DEPTH_BUFFER_BIT);

    std::pmr::string str{ getMemoryPool() };
    F32_t            scale = glm::mix(1.f, 0.34f, glm::clamp(10.f * m_letterSize / m_framebufferSize.y, 0.01f, 1.f));

    str.clear();
    str.append("velocity: ");
    str.append(std::to_string(m_player.getVelocity()));
    g_renderer2D.renderText(str.c_str(), { 0.1f, 0.8f, scale }, { 0.3f, 0.3f, 0.3f });

    str.clear();
    str.append(std::to_string(m_player.getCurrentScore()));
    glm::vec3 const xyScale{ m_framebufferSize.x * 0.85f, m_framebufferSize.y / 1.2f, scale };
    glm::vec3 const color{ 0.9f };

    g_renderer2D.renderButton({ .position{ m_framebufferSize.x * 0.85f, m_framebufferSize.y / 1.2f },
                                .size{ m_letterSize * 1.1f, m_letterSize * 10 / scale },
                                .borderColor{ 0.f },
                                .borderWidth = 0.f,
                                .backgroundColor{ 0.5f },
                                .text = "",
                                .textColor{ 0.f } });
    g_renderer2D.renderText(str.c_str(), xyScale, color);
}


[[nodiscard]] F32_t TestbedModule::aspectRatio() const
{
    F32_t ratio = m_framebufferSize.x / glm::max(static_cast<F32_t>(m_framebufferSize.y), 0.1f);
    return ratio > std::numeric_limits<F32_t>::epsilon() ? ratio : 1.f;
}

static B8_t isBetween(glm::vec2 const &p, glm::vec2 const &min, glm::vec2 const &max, glm::vec2 const &threshold)
{
    return p.x + threshold.x >= min.x && p.y + threshold.y >= min.y //
           && p.x - threshold.x < max.x && p.y - threshold.y < max.y;
}

B8_t TestbedModule::isAnyCoinClicked(glm::vec2 const &clickPos)
{
    glm::mat4 const &viewMatrix = m_player.getCamera().viewTransform();
    glm::mat4 const  projectionMatrix{ glm::perspective(FOV, aspectRatio(), CLIPDISTANCE, RENDERDISTANCE) };

    for (auto it = m_scrollingTerrain.getCoinMap().begin(); it != m_scrollingTerrain.getCoinMap().end();)
    {
        AABB box = g_handleTable.getMesh(g_scene.getNodeBySid(it->second).getSid()).box;
        AABB ndcBox =
          transformAABBToNDC(box, g_scene.getNodeBySid(it->second).getTransform(), viewMatrix, projectionMatrix);

        if (isBetween(
              clickPos,
              glm::vec2{ ndcBox.mm.min.x, ndcBox.mm.min.y },
              glm::vec2{ ndcBox.mm.max.x, ndcBox.mm.max.y },
              { 0.1f, 0.1f }))
        {
            g_soundEngine()->play2D(m_coinPickedSource);
            printf("[Testbed] coin clicked\n");
            m_scrollingTerrain.removeCoin(it);
            m_player.incrementScore(coinBonusScore);
            return true;
        }
        ++it;
    }

    return false;
}

} // namespace cge
