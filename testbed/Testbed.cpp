#include "Testbed.h"

#include "Core/Event.h"
#include "Core/Events.h"
#include "Core/KeyboardKeys.h"
#include "Core/StringUtils.h"
#include "Core/Type.h"
#include "Entity/CollisionWorld.h"
#include "Launch/Entry.h"
#include "Render/Renderer.h"
#include "Render/Renderer2d.h"
#include "RenderUtils/GLutils.h"
#include "Resource/HandleTable.h"
#include "Resource/Rendering/cgeMesh.h"
#include "Resource/Rendering/cgeScene.h"

#include "ConstantsAndStructs.h"

#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/geometric.hpp>
#include <glm/matrix.hpp>
#include <glm/trigonometric.hpp>
#include <stb/stb_image.h>

CGE_DECLARE_STARTUP_MODULE(cge, TestbedModule, "TestbedModule");
// TODO scene and world not global. Also refactor them, they suck

namespace cge
{

TestbedModule::~TestbedModule()
{
    if (m_init)
    {
        m_soundEngine->drop();
        for (auto const &pair : m_listeners.arr)
        { //
            g_eventQueue.removeListener(pair);
        }
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

    m_soundEngine = irrklang::createIrrKlangDevice();
    if (!m_soundEngine)
    { //
        printf("[TestBed] error creating sound engine");
    }

    irrklang::ISoundSource *sound =
      m_soundEngine->addSoundSourceFromFile("../assets/ophelia.mp3");

    m_soundEngine->play2D(sound);

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

    // open mesh file
    printf("Opening Scene file\n");
    g_scene         = Scene_s::fromObj("../assets/lightTestScene.obj");
    auto planeScene = Scene_s::fromObj("../assets/plane.obj");
    auto planeSid   = *planeScene.names();
    m_pieces        = std::pmr::vector<Sid_t>(10);
    std::fill(m_pieces.begin(), m_pieces.end(), planeSid);

    auto obstaclesScene = Scene_s::fromObj("../assets/prop.obj");
    auto obstacleSid    = *obstaclesScene.names();
    m_obstacles.push_back(obstacleSid);

    m_scrollingTerrain.init(g_scene, m_pieces.begin(), m_pieces.end());

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

void TestbedModule::onTick(float deltaTime)
{
    getBackgroundRenderer().renderBackground(
      m_player.getCamera(), aspectRatio(), CLIPDISTANCE, RENDERDISTANCE);
    // TODO place randomly m_obstacles with a seed by reading back the height of
    // the mesh
    // TODO astronave che spara su ostacoli!

    m_player.onTick(deltaTime);

    auto const p      = m_player.lastDisplacement();
    auto const camera = m_player.getCamera();
    auto const center = m_player.getCentroid();
    auto       obsVec =
      m_scrollingTerrain.updateTilesFromPosition(center, m_pieces, m_obstacles);
    Hit_t hit{};
    m_player.intersectPlayerWith(obsVec, hit);

    // worldSpawner.transformTerrain(glm::translate(glm::mat4(1.F),
    // glm::vec3(p.x, p.y, 0)));

    // worldSpawner.renderTerrain(camera); // TODO deltaTime is broken
    // printf(
    //  "Testbed::onTick camera.position = { %f %f %f }\n",
    //  camera.position.x,
    //  camera.position.y,
    //  camera.position.z);

    g_renderer.renderScene(
      g_scene,
      camera.viewTransform(),
      glm::perspective(FOV, aspectRatio(), CLIPDISTANCE, RENDERDISTANCE));
    g_renderer2D.renderText(
      "Text", glm::vec3(25.0f, 25.0f, 1.0f), glm::vec3(0.5, 0.8f, 0.2f));

    // TODO transparency when
    // Disable depth buffer writes
    // glDepthMask(GL_FALSE);

    // worldSpawner.detectTerrainCollisions(camera.viewTransform());
}

[[nodiscard]] F32_t TestbedModule::aspectRatio() const
{
    return (F32_t)m_framebufferSize.x / (F32_t)m_framebufferSize.y;
}

} // namespace cge
