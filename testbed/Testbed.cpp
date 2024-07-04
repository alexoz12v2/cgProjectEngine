#include "Testbed.h"

#include "Core/Event.h"
#include "Core/Events.h"
#include "Core/KeyboardKeys.h"
#include "Core/StringUtils.h"
#include "Core/Type.h"
#include "Entity/CollisionWorld.h"
#include "Launch/Entry.h"
#include "Render/Renderer.h"
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


CGE_DECLARE_STARTUP_MODULE(cge, TestbedModule, "TestbedModule");
// TODO scene and world not global. Also refactor them, they suck

namespace cge
{

// boilerplate ----------------------------------------------------------------
void TestbedModule::onInit(ModuleInitParams params)
{
    Sid_t const mId = "TestbedModule"_sid;
    CGE_DBG_SID("TestbedModule");
    Char8_t const *str = CGE_DBG_STRLOOKUP(mId);
    printf("Hello World!! %s\n", str);
#if defined(CGE_DEBUG)
    printf("DebugMode!\n");
#endif

    // register to all relevant events pressed
    EventArg_t listenerData{};
    listenerData.idata.p = (Byte_t *)this;
    g_eventQueue.addListener(
      evKeyPressed, KeyCallback<TestbedModule>, listenerData);
    g_eventQueue.addListener(
      evMouseMoved, mouseMovementCallback<TestbedModule>, listenerData);
    g_eventQueue.addListener(
      evMouseButtonPressed, mouseButtonCallback<TestbedModule>, listenerData);
    g_eventQueue.addListener(
      evFramebufferSize, framebufferSizeCallback<TestbedModule>, listenerData);

    // open mesh file
    printf("Opening Scene file\n");
    g_scene         = Scene_s::fromObj("../assets/lightTestScene.obj");
    auto planeScene = Scene_s::fromObj("../assets/plane.obj");
    auto planeSid   = *planeScene.names();
    pieces          = std::pmr::vector<Sid_t>(10);
    std::fill(pieces.begin(), pieces.end(), planeSid);
    scrollingTerrain.init(g_scene, pieces.begin(), pieces.end());

    // setup player
    Camera_t camera{};
    camera.position = glm::vec3(0, 0, 10);
    camera.right    = glm::vec3(1.F, 0.F, 0.F);
    camera.up       = glm::vec3(0.F, 0.F, 1.F);
    camera.forward  = glm::vec3(0.F, 1.F, 0.F);
    player.spawn(camera, cubeMeshSid);

    // set up the viewport
    g_renderer.viewport(framebufferSize.x, framebufferSize.y);

    // background, terrain, terrain collisions
    worldSpawner.init();
}

void TestbedModule::onKey(I32_t key, I32_t action) {}

void TestbedModule::onMouseButton(I32_t key, I32_t action) {}

void TestbedModule::onMouseMovement(F32_t xpos, F32_t ypos) {}

void TestbedModule::onFramebufferSize(I32_t width, I32_t height)
{
    framebufferSize.x = width;
    framebufferSize.y = height;
    g_renderer.viewport(framebufferSize.x, framebufferSize.y);
    printf("width: %u, height: %u\n", framebufferSize.x, framebufferSize.y);
}

void TestbedModule::onTick(float deltaTime)
{
    g_renderer.clear();
    worldSpawner.renderBackground(player.getCamera());

    // TODO place randomly obstacles with a seed by reading back the height of
    // the mesh
    // TODO astronave che spara su ostacoli!

    player.onTick(deltaTime);
    auto const p      = player.lastDisplacement();
    auto const camera = player.getCamera();
    auto const center = player.getCentroid();
    scrollingTerrain.updateTilesFromPosition(center, pieces);

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

    // TODO transparency when
    // Disable depth buffer writes
    // glDepthMask(GL_FALSE);

    // worldSpawner.detectTerrainCollisions(camera.viewTransform());
}

} // namespace cge
