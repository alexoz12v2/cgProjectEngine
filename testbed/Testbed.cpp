#include "Testbed.h"

#include "Core/Event.h"
#include "Core/Events.h"
#include "Core/KeyboardKeys.h"
#include "Core/StringUtils.h"
#include "Core/Type.h"
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

#include <fstream>

CGE_DECLARE_STARTUP_MODULE(cge, TestbedModule, "TestbedModule");
// TODO scene and world not global. Also refactor them, they suck

namespace cge
{
nlohmann::json json;

TestbedModule::~TestbedModule()
{
    if (m_init)
    {
        for (auto const &pair : m_listeners.arr)
        { //
            g_eventQueue.removeListener(pair);
        }

        if (m_bgm)
        { //
            m_bgm->stop();
            m_bgm->drop();
        }
        g_soundEngine()->removeSoundSource(m_bgmSource);

        g_scene.removeNode(*m_cubeHandle);
    }
}

// boilerplate ----------------------------------------------------------------
void TestbedModule::onInit(ModuleInitParams params)
{
    Sid_t const mId = "TestbedModule"_sid;
    CGE_SID("TestbedModule");
    Char8_t const *str = CGE_DBG_STRLOOKUP(mId);
    printf("Hello World!! %s\n", str);
#if defined(CGE_DEBUG)
    printf("DebugMode!\n");
#endif

    m_bgmSource = g_soundEngine()->addSoundSourceFromFile("../assets/bgm0.mp3");
    m_bgm       = g_soundEngine()->play2D(m_bgmSource, true);

    // register to all relevant events pressed
    EventArg_t listenerData{};
    listenerData.idata.p    = (Byte_t *)this;
    m_listeners.keyListener = g_eventQueue.addListener(
      evKeyPressed, KeyCallback<TestbedModule>, listenerData);
    m_listeners.mouseMovementListener = g_eventQueue.addListener(
      evMouseMoved, mouseMovementCallback<TestbedModule>, listenerData);
    m_listeners.mouseButtonListener = g_eventQueue.addListener(
      evMouseButtonPressed, mouseButtonCallback<TestbedModule>, listenerData);
    m_listeners.framebufferSizeListener = g_eventQueue.addListener(
      evFramebufferSize, framebufferSizeCallback<TestbedModule>, listenerData);
    m_listeners.gameOverListener = g_eventQueue.addListener(
      evGameOver, gameOverCallback<TestbedModule>, listenerData);

    // open mesh file
    printf("Opening Scene file\n");

    // lightTestScene.obj has 1 mesh called Cube
    g_handleTable.loadFromObj("../assets/lightTestScene.obj");
    m_cubeHandle = g_scene.addChild(CGE_SID("Cube"));

    // plane.obj has 1 mesh called Plane01
    g_handleTable.loadFromObj("../assets/plane.obj");
    auto planeSid = CGE_SID("Plane01");
    m_pieces.resize(10);
    std::fill(m_pieces.begin(), m_pieces.end(), planeSid);

    // prop.obj has 1 mesh called Obstacle
    g_handleTable.loadFromObj("../assets/prop.obj");
    auto obstacleSid = CGE_SID("Obstacle");
    m_obstacles.push_back(obstacleSid);

    m_scrollingTerrain.init(g_scene, m_pieces);

    // add light to the scene
    Light_t const sunLight{ 
        .sid   = CGE_SID("SUN LIGHT"),
        .props = { 
            .isEnabled = true,
            .isLocal = false,
            .isSpot = false,
            .ambient   = {0,0,0},
            .color = {2.f, 2.f, 2.f},
            .position  = {-0.4f, 0.3f, -1.f},
            .halfVector = {-0.4f, 0.3f, -1.f},
            .coneDirection = {0,0,0},
            .spotCosCutoff = 0,
            .spotExponent = 0,
            .constantAttenuation = 0,
            .linearAttenuation = 0,
            .quadraticAttenuation = 0,
        },
    };

    g_handleTable.insertLight(sunLight.sid, sunLight);

    // setup player
    Camera_t camera{};
    camera.position = glm::vec3(0, 0, 10);
    camera.right    = glm::vec3(1.f, 0.f, 0.f);
    camera.up       = glm::vec3(0.f, 0.f, 1.f);
    camera.forward  = glm::vec3(0.f, 1.f, 0.f);
    m_player.spawn(camera, cubeMeshSid);

    // background, terrain, terrain collisions
    stbi_set_flip_vertically_on_load(true);
    const char    *imagePath = "../assets/background.png";
    int            width, height, channels;
    unsigned char *image =
      stbi_load(imagePath, &width, &height, &channels, STBI_rgb);
    getBackgroundRenderer().init(image, width, height);
    stbi_image_free(image);

    // load saves, if existent, otherwise create new ones
    std::ifstream file{ "../assets/saves.json" };
    if (file)
    { //
        json = nlohmann::json::parse(file);
    }
    else
    { //
        json = nlohmann::json::object({ { "bestScore", 0ULL } });
    }

    m_init = true;
}

void TestbedModule::onKey(I32_t key, I32_t action)
{
    if (action == action::PRESS)
    {
        if (key == key::_1)
        { //
            switchToModule(CGE_SID("MenuModule"));
        }
        else if (key == key::_2)
        { //
            switchToModule(CGE_SID("non existent"));
        }
    }
}

void TestbedModule::onMouseButton(I32_t key, I32_t action) {}

void TestbedModule::onMouseMovement(F32_t xpos, F32_t ypos) {}

void TestbedModule::onFramebufferSize(I32_t width, I32_t height)
{
    m_framebufferSize.x = width;
    m_framebufferSize.y = height;
    printf(
      "width: %u, height: %u, aspectRatio: %f\n",
      m_framebufferSize.x,
      m_framebufferSize.y,
      aspectRatio());
}

void TestbedModule::onGameOver(U64_t score)
{
    U64_t curr = json["bestScore"].template get<U64_t>();
    if (score > curr)
    { //
        json["bestScore"] = score;
    }

    std::ofstream file{ "../assets/saves.json",
                        std::ios::out | std::ios::trunc };
    file << json.dump();
    switchToModule(CGE_SID("MenuModule"));
}

void TestbedModule::onTick(float deltaTime)
{
    getBackgroundRenderer().renderBackground(
      m_player.getCamera(), aspectRatio(), CLIPDISTANCE, RENDERDISTANCE);

    m_player.onTick(deltaTime);

    auto const p      = m_player.lastDisplacement();
    auto const camera = m_player.getCamera();
    auto const center = m_player.getCentroid();
    auto       obsVec =
      m_scrollingTerrain.updateTilesFromPosition(center, m_pieces, m_obstacles);
    Hit_t hit{};
    m_player.intersectPlayerWith(obsVec, hit);

    g_renderer.renderScene(
      g_scene,
      camera.viewTransform(),
      glm::perspective(FOV, aspectRatio(), CLIPDISTANCE, RENDERDISTANCE),
      camera.forward);
    FixedString str = fixedStringWithNumber<FixedString("Best Score")>(
      json["bestScore"].template get<U64_t>());
    g_renderer2D.renderText(
      str.cStr(), glm::vec3(25.0f, 25.0f, 1.0f), glm::vec3(0.5, 0.8f, 0.2f));
}

[[nodiscard]] F32_t TestbedModule::aspectRatio() const
{
    return (F32_t)m_framebufferSize.x / (F32_t)m_framebufferSize.y;
}

} // namespace cge
