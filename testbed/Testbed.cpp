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
#include "Core/TimeUtils.h"
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

CGE_DECLARE_MODULE(cge, TestbedModule, "TestbedModule");

// TODOs:
// ? difficulty

namespace cge
{

// -- constants --
inline U32_t constexpr coinBonusScore              = 100;
inline std::string_view constexpr youDiedText      = "You Died!";
inline Char8_t const constexpr *const mainMenuText = "Main Menu";

inline F32_t constexpr CLIPDISTANCE   = 5.f;
inline F32_t constexpr RENDERDISTANCE = 500.f;
inline F32_t constexpr startFOV       = 60.f;
inline F32_t constexpr maxFOV         = 120.f;
inline F32_t constexpr baseFovDelay   = 0.00001f;

inline ButtonSpec const mainMenuButton{
    .position{ 0.3f, 0.4f },
    .size{ 0.35f, 0.23f },
    .borderColor{ 0.5098f, 0.45098f, 0.294118f },
    .borderWidth = 0.02f,
    .backgroundColor{ 0.2f },
    .text = mainMenuText,
    .textColor{ 0.7f },
};

// -- the rest of the code --

TestbedModule::TestbedModule(Sid_t id) : IModule(id), m_fov(startFOV), m_targetFov(startFOV)
{
}

TestbedModule::~TestbedModule()
{
    if (m_init)
    {
        for (auto const &pair : m_listeners.arr)
        {
            g_eventQueue.removeListener(pair);
        }

        g_soundEngine()->removeSoundSource(m_magnetPickedSource);
        g_soundEngine()->removeSoundSource(m_coinPickedSource);
        g_soundEngine()->removeSoundSource(m_woodBreakSource);
    }
}

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
        g_handleTable.loadFromObj("../assets/down.obj");
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
    Sid_t                   down{ CGE_SID("Down") };
    m_scrollingTerrain.init({ .pieces        = pieces,
                              .obstacles     = obstacles,
                              .destructables = destructables,
                              .magnetPowerUp = magnet,
                              .coin          = coin,
                              .speed         = speed,
                              .down          = down });

    // try reading difficulty to initialize acceleration
    EDifficulty difficulty;
    if (g_globalStore.contains(CGE_SID("DIFFICULTY")))
    {
        UntypedData128 data = g_globalStore.consume(CGE_SID("DIFFICULTY"));
        difficulty          = static_cast<EDifficulty>(data.u32[0]);
        g_globalStore.put(CGE_SID("DIFFICULTY"), data);
    }
    else
    {
        difficulty = EDifficulty::eNormal;
    }

    // setup player
    Camera_t camera{};
    camera.position = glm::vec3(0, 0, 10);
    camera.right    = glm::vec3(1.f, 0.f, 0.f);
    camera.up       = glm::vec3(0.f, 0.f, 1.f);
    camera.forward  = glm::vec3(0.f, 1.f, 0.f);
    m_player.spawn(camera, difficulty);

    // background, terrain, terrain collisions
    stbi_set_flip_vertically_on_load(true);
    const char    *imagePath = "../assets/background.png";
    int            width, height, channels;
    unsigned char *image = stbi_load(imagePath, &width, &height, &channels, STBI_rgb);
    getBackgroundRenderer().init(image, width, height);
    stbi_image_free(image);

    // sound
    m_coinPickedSource   = g_soundEngine()->addSoundSourceFromFile("../assets/coin-picked.mp3");
    m_woodBreakSource    = g_soundEngine()->addSoundSourceFromFile("../assets/wood-break.mp3");
    m_magnetPickedSource = g_soundEngine()->addSoundSourceFromFile("../assets/magnet-picked.mp3");
    assert(m_coinPickedSource && m_woodBreakSource);

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
    if (m_gameState == EGameState::eDefault)
    {
        glm::vec2 ndcMousePos{ m_screenMousePos / static_cast<glm::vec2>(m_framebufferSize) };
        ndcMousePos.y = 1 - ndcMousePos.y;
        ndcMousePos -= 0.5f;
        ndcMousePos *= 2;
        if (action == action::PRESS && key == button::LMB)
        {
            printf("[Testbed] pressed at NDC { %f %f }\n", ndcMousePos.x, ndcMousePos.y);
            isAnyCoinClicked(ndcMousePos);
        }
    }
    else
    {
        glm::vec2 ndcMousePos{ m_screenMousePos / static_cast<glm::vec2>(m_framebufferSize) };
        ndcMousePos.y = 1 - ndcMousePos.y;
        if (
          ndcMousePos.x >= mainMenuButton.position.x && ndcMousePos.y >= mainMenuButton.position.y
          && ndcMousePos.x < mainMenuButton.position.x + mainMenuButton.size.x
          && ndcMousePos.y < mainMenuButton.position.y + mainMenuButton.size.y)
        {
            UntypedData128 data{};
            data.u64[0] = m_player.getCurrentScore();
            g_globalStore.put(CGE_SID("score"), data);
            switchToModule(CGE_SID("MenuModule"));
        }
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
    printf("\033[33m[Testbed] onGameOver Called, finalScore: %zu\033[0m\n", score);
    m_player.kill();
    m_gameState = EGameState::eDead;
}

void TestbedModule::onShoot(Ray const &ray)
{
    printf("[TestbedModule] BANG\n");
    B8_t intersected = m_scrollingTerrain.handleShoot(m_player.boundingBox());
    if (intersected)
    {
        g_soundEngine()->play2D(m_woodBreakSource);
    }
}

void TestbedModule::onMagnetAcquired()
{
    g_soundEngine()->play2D(m_magnetPickedSource);
    U32_t numCoins = m_scrollingTerrain.removeAllCoins();
    m_player.incrementScore(coinBonusScore, numCoins);
    m_numCoins += numCoins;
    printf("[Testbed] MAGNET POWERUP ACQUIRED\n");
}

static glm::vec2 proportions(U32_t length, F32_t base = 0.01f)
{
    return { base * length, base };
}

void TestbedModule::onTick(U64_t deltaTime)
{
    m_elapsedTime += deltaTime;

    // Game Tick
    F32_t const deltaTimeF = deltaTime / timeUnit64;
    auto const  p          = m_player.lastDisplacement();
    auto const  camera     = m_player.getCamera();
    auto const  proj       = glm::perspective(glm::radians(m_fov), aspectRatio(), CLIPDISTANCE, RENDERDISTANCE);

    if (m_gameState == EGameState::eDefault)
    {
        auto const center = m_player.getCentroid();
        m_scrollingTerrain.updateTilesFromPosition(center);
        m_player.onTick(deltaTime);
        m_scrollingTerrain.onTick(deltaTime);
        m_player.intersectPlayerWith(m_scrollingTerrain);

        F32_t startVelocity = m_player.getStartVelocity();
        F32_t interpolation = (m_player.getVelocity() - startVelocity) / (m_player.getMaxVelocity() - startVelocity);
        m_targetFov         = glm::mix(startFOV, maxFOV, interpolation);
        m_fov               = glm::mix(m_fov, m_targetFov, 1.f - glm::pow(baseFovDelay, deltaTimeF));

        shakeCamera(deltaTime);
    }

    // Rendering
    getBackgroundRenderer().renderBackground(m_player.getCamera(), proj);
    g_renderer.renderScene(g_scene, camera.viewTransform(), proj, camera.forward);

    glClear(GL_DEPTH_BUFFER_BIT);

    std::pmr::string str{ getMemoryPool() };

    str.clear();
    str.append("Velocity: ");
    str.append(std::to_string(m_player.getVelocity()));
    str.resize(str.size() - 4);
    g_renderer2D.renderButton({ .position{ 0.0f, 0.04f },
                                .size{ 0.35f, 0.07f },
                                .borderColor{ 0.5f },
                                .borderWidth = 0.01f,
                                .backgroundColor{ 0.5f },
                                .text = str.c_str(),
                                .textColor{ 0.9f } });

    str.clear();
    str.append("Coins: ");
    str.append(std::to_string(m_numCoins));
    g_renderer2D.renderButton({ .position{ 0.8f, 0.8f },
                                .size{ 0.2f, 0.07f },
                                .borderColor{ 0.5f },
                                .borderWidth = 0.01f,
                                .backgroundColor{ 0.5f },
                                .text = str.c_str(),
                                .textColor{ 0.9f } });

    str.clear();
    str.append("Score: ");
    str.append(std::to_string(m_player.getCurrentScore()));
    g_renderer2D.renderButton({ .position{ 0.8f, 0.8f + 0.07f },
                                .size{ 0.2f, 0.07f },
                                .borderColor{ 0.5f },
                                .borderWidth = 0.01f,
                                .backgroundColor{ 0.5f },
                                .text = str.c_str(),
                                .textColor{ 0.9f } });

    if (F32_t time = m_player.remainingInvincibleTime(); time > 0.f && m_gameState == EGameState::eDefault)
    {
        str.clear();
        str.append("Remaining Invincibility Time: ");
        str.append(std::to_string(time));
        F32_t xSize = g_renderer2D.letterSize().x * str.size();
        F32_t ySize = g_renderer2D.letterSize().y;
        F32_t s     = glm::min(0.8f * m_framebufferSize.x / xSize, 0.1f * m_framebufferSize.y / ySize);
        g_renderer2D.renderText(
          str.c_str(), { 0.05f * m_framebufferSize.x, 0.9f * m_framebufferSize.y, s }, glm::vec3{ 0.3f });
    }

    if (F32_t time = m_player.remainingMalusTime(); time > 0.f && m_gameState == EGameState::eDefault)
    {
        str.clear();
        str.append("Remaining Malus Time: ");
        str.append(std::to_string(time));
        F32_t xSize = g_renderer2D.letterSize().x * str.size();
        F32_t ySize = g_renderer2D.letterSize().y;
        F32_t s     = glm::min(0.8f * m_framebufferSize.x / xSize, 0.1f * m_framebufferSize.y / ySize);
        g_renderer2D.renderText(
          str.c_str(), { 0.05f * m_framebufferSize.x, 0.9f * m_framebufferSize.y, s }, glm::vec3{ 0.3f });
    }

    if (m_gameState == EGameState::eDead)
    {
        F32_t s = glm::min(
          0.8f * m_framebufferSize.x / (youDiedText.size() * m_letterSize), 0.1f * m_framebufferSize.y / m_letterSize);
        g_renderer2D.renderText(
          youDiedText.data(), { 0.1f * m_framebufferSize.x, 0.86f * m_framebufferSize.y, s }, { 0.54f, 0.09f, 0.145f });
        g_renderer2D.renderButton(mainMenuButton);
    }
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
    glm::mat4 const  projectionMatrix{ glm::perspective(
      glm::radians(m_fov), aspectRatio(), CLIPDISTANCE, RENDERDISTANCE) };

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
            ++m_numCoins;
            m_player.incrementScore(coinBonusScore);
            return true;
        }
        ++it;
    }

    return false;
}
void TestbedModule::shakeCamera(U64_t deltaTime)
{
    F32_t elapsedTimeF = m_elapsedTime / timeUnit64;
    // Compute how far the FOV is from the startFov
    float fovDifference = std::abs(m_fov - startFOV);

    // If there's no difference, no shake
    if (fovDifference < 0.01f)
        return;

    // Maximum shake intensity (tune this value as needed)
    float maxShakeIntensity = 0.5f; // Scale factor for shake

    // Skew factor for sine function to make shake feel more chaotic
    float skewFactor = 1.5f;

    // Calculate shake amount using a skewed sine function
    float shakeRight = std::sin(elapsedTimeF * skewFactor) * fovDifference * maxShakeIntensity;
    float shakeUp    = std::sin(elapsedTimeF * skewFactor * 0.9f + 1.0f) * fovDifference * maxShakeIntensity;

    // Apply shake to the camera's position (or direction) based on the right and up vectors
    glm::vec3 shakeOffset = shakeRight * m_player.getCamera().right + shakeUp * m_player.getCamera().up;

    // Option 1: Modify camera's position (camera shaking back and forth
    m_player.getCamera().position += shakeOffset;
}

} // namespace cge
