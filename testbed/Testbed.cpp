#include "Testbed.h"

#include "Launch/Entry.h"
#include "RenderUtils/GLutils.h"
#include "Resource/HandleTable.h"
#include "Resource/Rendering/Buffer.h"
#include "Resource/Rendering/cgeMesh.h"
#include "Resource/Rendering/cgeScene.h"

#define STB_IMAGE_IMPLEMENTATION
#include <fmt/core.h>
#include <glad/gl.h>
#include <stb/stb_image.h>

#include <glm/gtc/matrix_transform.hpp>

#include <Entity/CollisionWorld.h>

CGE_DECLARE_STARTUP_MODULE(cge, TestbedModule, "TestbedModule");

namespace cge
{

void TestbedModule::onInit(ModuleInitParams params)
{
    Sid_t mId = "TestbedModule"_sid;
    CGE_DBG_SID("TestbedModule");
    Char8_t const *str = CGE_DBG_STRLOOKUP(mId);
    fmt::print("Hello World!! {}\n", str);
#if defined(CGE_DEBUG)
    fmt::print("DebugMode!\n");
#endif

    // register to event key pressed
    EventArg_t listenerData{};
    listenerData.idata.p = (Byte_t *)this;
    g_eventQueue.addListener(evKeyPressed, KeyCallback, listenerData);
    g_eventQueue.addListener(evMouseMoved, mouseCallback, listenerData);
    g_eventQueue.addListener(
      evMouseButtonPressed, mouseButtonCallback, listenerData);
    g_eventQueue.addListener(
      evFramebufferSize, framebufferSizeCallback, listenerData);

    // setup camera
    this->camera.position = glm::vec3(0.f);
    this->camera.right    = glm::vec3(1.f, 0.f, 0.f);
    this->camera.up       = glm::vec3(0.f, 0.f, 1.f);
    this->camera.forward  = glm::vec3(0.f, 1.f, 0.f);

    // open mesh file
    fmt::print("Opening Scene file\n");
    g_scene = *Scene_s::fromObj("../assets/lightTestScene.obj");

    // scene setup
    for (Sid_t sid : { cubeMeshSid, planeMeshSid })
    {
        Mesh_s const &mesh = g_handleTable.get(sid).getAsMesh();

        glm::mat4 &transform = g_scene.getNodeBySid(sid)->absoluteTransform;
        transform = glm::translate(transform, glm::vec3(0.f, 0.f, 2.f));

        box               = computeAABB(mesh);
        AABB_t gSpaceAABB = { .min = transform * glm::vec4(box.min, 1.f),
                              .max = transform * glm::vec4(box.max, 1.f) };
        CollisionObj_t cubeCollisionMesh{ .ebox = gSpaceAABB,
                                          .sid  = cubeMeshSid };

        g_world.addObject(cubeCollisionMesh);
    }
}

void TestbedModule::onKey(I32_t key, I32_t action, F32_t deltaTime)
{
    I32_t dirMult = 0;
    if (action == GLFW_PRESS) { dirMult = 1; }
    if (action == GLFW_RELEASE) { dirMult = -1; }

    switch (key)
    {
    case GLFW_KEY_W:
        keyPressed[0] += dirMult;
        break;
    case GLFW_KEY_A:
        keyPressed[1] += dirMult;
        break;
    case GLFW_KEY_S:
        keyPressed[0] -= dirMult;
        break;
    case GLFW_KEY_D:
        keyPressed[1] -= dirMult;
        break;
    default:
        break;
    }
}

void TestbedModule::onMouseButton(I32_t key, I32_t action, F32_t deltaTime)
{
    if (action == GLFW_PRESS)
    {
        disableCursor();
        isCursorDisabled = true;
    }
    else if (action == GLFW_RELEASE)
    {
        enableCursor();
        isCursorDisabled = false;
    }
}

void TestbedModule::onMouseMovement(F32_t xpos, F32_t ypos)
{
    static F32_t yaw = 0, pitch = 0;
    if (!isCursorDisabled)
    {
        lastCursorPosition = { xpos, ypos };
        return;
    }

    F32_t deltaX = lastCursorPosition.x - xpos;
    F32_t deltaY = ypos - lastCursorPosition.y;
    yaw += deltaX * mouseSensitivity;
    pitch += deltaY * mouseSensitivity;

    yawPitchRotate(yaw, pitch);

    lastCursorPosition = { xpos, ypos };
}

void TestbedModule::onFramebufferSize(I32_t width, I32_t height)
{
    framebufferSize.x = width;
    framebufferSize.y = height;
    fmt::print("width: {}, height: {}\n", framebufferSize.x, framebufferSize.y);
}

void TestbedModule::onTick(float deltaTime)
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    Mesh_s const    &cubeMesh = g_handleTable.get(cubeMeshSid).getAsMesh();
    glm::mat4 const &cubeMeshTransform =
      g_scene.getNodeBySid(cubeMeshSid)->absoluteTransform;
    AABB_t    gSpaceAABB = { .min = cubeMeshTransform * glm::vec4(box.min, 1.f),
                             .max = cubeMeshTransform * glm::vec4(box.max, 1.f) };
    glm::vec3 cubePos    = centroid(gSpaceAABB);

    F32_t velocity = baseVelocity * deltaTime;

    // Calculate the movement direction based on camera's forward vector
    glm::vec3 direction = (F32_t)keyPressed[0] * camera.forward
                          + (F32_t)keyPressed[1] * camera.right;
    if (direction != glm::vec3(0.f)) { direction = glm::normalize(direction); }

    // Update the camera position
    glm::vec3 oldPosition = camera.position;
    camera.position += velocity * direction;

    Ray_t ray{ .o = camera.position, .d = cubePos - camera.position };
    Hit_t hit;
    g_world.build();
    if (g_world.intersect(ray, 0, hit))
    {
        if (hit.t <= 0.5f) { camera.position = oldPosition; }
    }

    terrain.regenerate({.scale = 0.1F });
    terrain.draw(
      glm::mat4(1.f),
      camera.viewTransform(),
      glm::perspective(45.f, aspectRatio(), 0.1f, 100.f));

    /// g_renderer.renderScene(
    ///   g_scene,
    ///   camera.viewTransform(),
    ///   glm::perspective(45.f, aspectRatio(), 0.1f, 100.f));
}

void TestbedModule::yawPitchRotate(F32_t yaw, F32_t pitch)
{
    yaw   = glm::radians(yaw);
    pitch = glm::radians(glm::clamp(pitch, -89.f, 89.f));

    glm::vec3 direction;
    direction.x = cos(yaw) * cos(pitch);
    direction.y = sin(yaw) * cos(pitch);
    direction.z = -sin(pitch);

    camera.forward = glm::normalize(direction);

    // Assuming the initial up direction is the z-axis
    glm::vec3 worldUp = glm::vec3(0.0f, 0.0f, 1.0f);

    // Calculate the right and up vectors using cross products
    camera.right = glm::normalize(glm::cross(worldUp, camera.forward));
    camera.up    = glm::normalize(glm::cross(camera.forward, camera.right));
}

TestbedModule::TestbedModule() : terrain(MarchingCubesSpecs_t{}) {}

// boilerplate ----------------------------------------------------------------

void KeyCallback(EventArg_t eventData, EventArg_t listenerData)
{
    auto self = (TestbedModule *)listenerData.idata.p;
    self->onKey(
      eventData.idata.i32[0], eventData.idata.i32[1], eventData.fdata.f32[0]);
}

void mouseButtonCallback(EventArg_t eventData, EventArg_t listenerData)
{
    auto self = (TestbedModule *)listenerData.idata.p;
    self->onMouseButton(
      eventData.idata.i32[0], eventData.idata.i32[1], eventData.fdata.f32[0]);
};

void mouseCallback(EventArg_t eventData, EventArg_t listenerData)
{
    auto self = (TestbedModule *)listenerData.idata.p;
    self->onMouseMovement(eventData.fdata.f32[0], eventData.fdata.f32[1]);
}

void framebufferSizeCallback(EventArg_t eventData, EventArg_t listenerData)
{
    auto self = (TestbedModule *)listenerData.idata.p;
    self->onFramebufferSize(eventData.idata.i32[0], eventData.idata.i32[1]);
}

} // namespace cge
