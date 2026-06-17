#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_LEFT_HANDED
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "App.h"

#include "Shaders/Depth.h"
#include "Shaders/Sky.h"
#include "Shaders/Simple.h"
#include "Shaders/Normal.h"
#include "Shaders/Lighting.h"
#include "Shaders/Effects.h"

#include <filesystem>

App::App(SDL_Texture* frameBuffer)
: m_frameBuffer(frameBuffer)
, m_depthBuffer(Render::PixelFormat::F32, frameBuffer->w, frameBuffer->h)
, m_shadow(Render::PixelFormat::F32, 1024, 1024)
, m_shadowShader(reinterpret_cast<Render::VertexShaderPtr>(ShadowVertexShader),
                 reinterpret_cast<Render::PixelShaderPtr>(ShadowPixelShader),
                 6)
, m_depthPrepassShader(reinterpret_cast<Render::VertexShaderPtr>(DepthVertexShader),
                       reinterpret_cast<Render::PixelShaderPtr>(DepthPixelShader),
                       6)
, m_skyShader(reinterpret_cast<Render::VertexShaderPtr>(SkyVertexShader),
              reinterpret_cast<Render::PixelShaderPtr>(SkyPixelShader),
              6)
, m_simpleShader(reinterpret_cast<Render::VertexShaderPtr>(SimpleVertexShader),
                 reinterpret_cast<Render::PixelShaderPtr>(SimplePixelShader),
                 6)
, m_normalShader(reinterpret_cast<Render::VertexShaderPtr>(NormalVertexShader),
                 reinterpret_cast<Render::PixelShaderPtr>(NormalPixelShader),
                 15)
, m_lightShader(reinterpret_cast<Render::VertexShaderPtr>(LightingVertexShader),
                reinterpret_cast<Render::PixelShaderPtr>(LightingPixelShader),
                18)
, m_fireShader(reinterpret_cast<Render::VertexShaderPtr>(FireVertexShader),
               reinterpret_cast<Render::PixelShaderPtr>(FirePixelShader),
               6)
, m_skyBox("textures/env/neg")
, m_keys(0)
, m_debugDraw(false)
, m_renderMode(RenderMode::Simple)
, m_viewUpdate(true)
, m_speed(10.0f)
, m_animTime(0.0f)
{
    LoadObj("models/courtyard.obj", m_scene);
    //LoadObj("models/sponza.obj", m_scene, 0.01f);

    m_sceneData.materials = m_scene.materials().data();
    m_sceneData.faceData = m_scene.faces();

    for (size_t i = 0; i < 6; i++) m_skyData.faces[i] = m_skyBox.face(i);

    setupEffects();
    setupLight(glm::vec3(0.3f, 1.0f, 0.3f));
    drawShadow();
}

void App::setupEffects()
{
    std::vector<SimpleVertex> vertices = { {{-0.7f, 1.0f, 0.0f}, {0.2f, 1.0f}},
                                           {{0.7f, 1.0f, 0.0f}, {0.8f, 1.0f}},
                                           {{-0.7f, -1.0f, 0.0f}, {0.2f, 0.0f}}, 
                                           {{0.7f, -1.0f, 0.0f}, {0.8f, 0.0f}} };

    std::vector<Render::IndexType> indices = { 0, 1, 2, 1, 3, 2 };

    m_flameMesh.reset(vertices, indices);

    std::string texname = "textures/effects/fire";
    
    for (uint32_t frame = 1; ; frame++)
    {
        std::string filename = texname + std::to_string(frame) + ".png";
        if (!std::filesystem::exists(filename)) break;

        m_fire.push_back(LoadPng(filename.c_str()));
    }

    m_animFrames = m_fire.size() - 1;

    m_flameData = { {&m_effectsData, {6.0f, 2.0f, 6.0f}, {1.0f, 1.0f, 1.0f} },
                    {&m_effectsData, {-6.0f, 2.0f, 6.0f}, {1.0f, 0.0f, 1.0f} },
                    {&m_effectsData, {6.0f, 2.0f, -6.0f}, {0.0f, 1.0f, 0.0f} },
                    {&m_effectsData, {-6.0f, 2.0f, -6.0f}, {0.0f, 0.3f, 1.0f} } 
                  };
}

void App::setupLight(const glm::vec3& lightDir)
{
    static constexpr float eps = 0.0001f;

    static constexpr glm::vec3 WorldX = { 1.0f, 0.0f, 0.0f };
    static constexpr glm::vec3 WorldZ = { 0.0f, 1.0f, 0.0f };

    glm::vec3 dir = -glm::normalize(lightDir);

    glm::vec3 yaxis = WorldZ - dir * glm::dot(dir, WorldZ);
    glm::vec3 xaxis = glm::cross(yaxis, dir);

    xaxis = glm::normalize(xaxis);
    yaxis = glm::normalize(yaxis);

    glm::mat4 lightMat = { {xaxis.x, yaxis.x, dir.x, 0.0f},
                           {xaxis.y, yaxis.y, dir.y, 0.0f},
                           {xaxis.z, yaxis.z, dir.z, 0.0f},
                           {0.0f, 0.0f, 0.0f, 1.0f} };

    glm::mat4 orthoMat = glm::ortho(-18.0f, 18.0f, -18.0f, 18.0f, -50.0f, 50.0f);

    m_sceneData.lightDir = glm::normalize(lightDir);
    m_sceneData.shadowMat = glm::mat4(orthoMat) * lightMat;
    m_sceneData.shadow = m_shadow;

    m_lights = { {{6.0f, 2.0f, 6.0f}, 2.5f, 1.0f, {1.0f, 0.8f, 0.5f}},
                 {{-6.0f, 2.0f, 6.0f}, 2.5f, 1.0f, {1.0f, 0.0f, 1.0f}},
                 {{6.0f, 2.0f, -6.0f}, 2.5f, 1.0f, {0.0f, 1.0f, 0.0f}},
                 {{-6.0f, 2.0f, -6.0f}, 2.5f, 1.0f, {0.0f, 0.7f, 1.0f}} };

    m_sceneData.lights = m_lights.data();
    m_ltgridData.lights = m_lights.data();
}

void App::resize(uint32_t width, uint32_t height)
{
    m_width = width;
    m_height = height;

    m_projMat = glm::perspective(glm::radians(90.0f), m_width / float(m_height), ZNear, ZFar);

    m_ltgridx = m_width / LtGridXSz;
    m_ltgridy = m_height / LtGridYSz;

    m_lightGrid.resize(m_ltgridx * m_ltgridy * GridMaxLights);

    m_sceneData.ltgridx = m_ltgridx;
    m_sceneData.ltgridy = m_ltgridy;
    m_sceneData.ltgrid = m_lightGrid.data();

    m_ltgridData.depth = m_depthBuffer;
    m_ltgridData.xblocks = m_width / 4;
    m_ltgridData.zfactor = ZNear / (ZFar - ZNear);
    m_ltgridData.ltgridx = m_ltgridx;
    m_ltgridData.ltgridy = m_ltgridy;
    m_ltgridData.ltgrid = m_lightGrid.data();
}

void App::input(const SDL_Event& event)
{
        switch (event.type)
    {
        case SDL_EVENT_MOUSE_MOTION:
            m_camera.rotate(event.motion.yrel * 0.5f, event.motion.xrel * 0.5f);
        break;

        case SDL_EVENT_KEY_DOWN:
            if (event.key.key == SDLK_W) m_keys |= key_up;
            if (event.key.key == SDLK_S) m_keys |= key_down;
            if (event.key.key == SDLK_A) m_keys |= key_left;
            if (event.key.key == SDLK_D) m_keys |= key_right;
        break;

        case SDL_EVENT_KEY_UP:
            if (event.key.key == SDLK_W) m_keys &= ~key_up;
            if (event.key.key == SDLK_S) m_keys &= ~key_down;
            if (event.key.key == SDLK_A) m_keys &= ~key_left;
            if (event.key.key == SDLK_D) m_keys &= ~key_right;
            if (event.key.key == SDLK_1) m_renderMode = RenderMode::Wireframe;
            if (event.key.key == SDLK_2) m_renderMode = RenderMode::Simple;
            if (event.key.key == SDLK_3) m_renderMode = RenderMode::Normals;
            if (event.key.key == SDLK_4) m_renderMode = RenderMode::Lighting;
            if (event.key.key == SDLK_V) m_viewUpdate = !m_viewUpdate;
        break;
    }
}

void App::update(float dt)
{
    glm::vec3 dir = m_camera.direction();

    if (m_keys & key_up) m_camera.move(dir * m_speed * dt);
    if (m_keys & key_down) m_camera.move(-dir * m_speed * dt);

    m_animTime += AnimSpeed * dt;
    if (m_animTime > m_animFrames) m_animTime = fmodf(m_animTime, m_animFrames);
}

void App::drawShadow()
{
    m_mainCommandList.begin();

    m_mainCommandList.bindFrameBuffer(nullptr, m_shadow);

    m_mainCommandList.enable(Render::Pipeline::DepthTest | Render::Pipeline::DepthWrite | 
                             Render::Pipeline::DepthUnpack | Render::Pipeline::AlphaTest);
    m_mainCommandList.clearDepth();

    m_mainCommandList.setAlpha(0.25f);

    m_mainCommandList.bindConstantBuffer((uint8_t*)&m_sceneData);
    m_mainCommandList.bindShader(m_shadowShader);
    m_mainCommandList.bindVertexBuffer(m_scene.vertexBuffer());
    m_mainCommandList.bindIndexBuffer(m_scene.indexBuffer());
    m_mainCommandList.setPolygonMode(Render::PolygonMode::Fill);

    for (const SceneNode& node : m_scene.nodes())
    {
        m_mainCommandList.drawIndexed(node.num, node.offset);
    }

    m_mainCommandList.finish();
    m_pipeline.execute(m_mainCommandList);
}

void App::updateView()
{
    m_camera.update();

    float ang = m_camera.horyzontalAngle();

    float hcos = cos(ang * glm::pi<float>() / 180.0f);
    float hsin = sin(ang * glm::pi<float>() / 180.0f);

    m_skyData.projView = m_projMat * m_camera.rotation();
    m_sceneData.projView = m_projMat * m_camera.transform();
    m_sceneData.viewPos = m_camera.pos();
    m_ltgridData.projView = m_sceneData.projView;
    m_effectsData.projView = m_sceneData.projView;
    m_effectsData.x = { hcos, hsin };
    m_effectsData.z = { -hsin, hcos };
    m_effectsData.fire = m_fire[uint32_t(m_animTime)];

    m_frustum.update(m_sceneData.projView);

    if (m_renderMode == RenderMode::Lighting)
    {
        m_viewLights.clear();

        for (size_t i = 0; i < m_lights.size(); i++)
        {
            const Light& light = m_lights[i];
            glm::vec3 bbox = { light.radius, light.radius, light.radius };

            if (m_frustum.test(light.pos, bbox)) m_viewLights.push_back(i);
        }

        m_sceneData.viewLights = m_viewLights.data();
        m_sceneData.ltnum = m_viewLights.size();

        m_ltgridData.viewLights = m_viewLights.data();
        m_ltgridData.ltnum = m_viewLights.size();
    }
}

void App::display()
{
    updateView();

    uint32_t* data = nullptr;
    int pitch;

    SDL_LockTexture(m_frameBuffer,
                    NULL,
                    (void**)&data,
                    &pitch);

    Render::ColorBuffer surface(m_width, m_height, data);

    if (m_viewUpdate)
    {
        m_sceneCommandList.begin();

        for (const SceneNode& node : m_scene.nodes())
        {
            if (m_frustum.test(node.bbox)) m_sceneCommandList.drawIndexed(node.num, node.offset);
        }

        m_sceneCommandList.finish();
    }

    bool drawSky = m_renderMode == RenderMode::Simple || m_renderMode == RenderMode::Lighting;

    m_mainCommandList.begin();

    m_mainCommandList.bindFrameBuffer(&surface, m_depthBuffer);

    m_mainCommandList.enable(Render::Pipeline::CullFace);
    m_mainCommandList.clearDepth();

    if (!drawSky) m_mainCommandList.clearColor(BgColor.r, BgColor.g, BgColor.b);

    m_mainCommandList.enable(Render::Pipeline::DepthTest | Render::Pipeline::DepthWrite | Render::Pipeline::AlphaTest);
    m_mainCommandList.setAlpha(0.25f);
    m_mainCommandList.bindVertexBuffer(m_scene.vertexBuffer());
    m_mainCommandList.bindIndexBuffer(m_scene.indexBuffer());
    m_mainCommandList.setPolygonMode(m_renderMode == RenderMode::Wireframe ? Render::PolygonMode::Line : Render::PolygonMode::Fill);

    // Depth prepass
    if (m_renderMode == RenderMode::Lighting)
    {
        m_mainCommandList.bindConstantBuffer((uint8_t*)&m_sceneData);
        m_mainCommandList.bindShader(m_depthPrepassShader);
        m_mainCommandList.execute(m_sceneCommandList);
        m_mainCommandList.setDepthFunc(Render::Pipeline::LessOrEqual);
    }

    if (m_renderMode == RenderMode::Lighting)
    {
        m_mainCommandList.bindConstantBuffer((uint8_t*)&m_ltgridData);
        m_mainCommandList.bindComputeShader(reinterpret_cast<Render::ComputeShaderPtr>(LightGridFillShader));
        m_mainCommandList.dispatch(m_ltgridx, m_ltgridy);
    }

    const Render::Shader& shader = m_renderMode == RenderMode::Lighting ? m_lightShader : 
                                   m_renderMode == RenderMode::Normals ? m_normalShader : m_simpleShader;

    m_mainCommandList.bindConstantBuffer((uint8_t*)&m_sceneData);
    m_mainCommandList.enable(Render::Pipeline::ColorWrite);
    m_mainCommandList.bindShader(shader);
    m_mainCommandList.execute(m_sceneCommandList);

    // Draw skybox
    if (drawSky)
    {
        m_mainCommandList.disable(Render::Pipeline::DepthWrite | Render::Pipeline::AlphaTest);

        m_mainCommandList.setPolygonMode(Render::PolygonMode::Fill);
        m_mainCommandList.setDepthFunc(Render::Pipeline::EqualOne);

        m_mainCommandList.bindConstantBuffer((uint8_t*)&m_skyData);
        m_mainCommandList.bindShader(m_skyShader);
        m_mainCommandList.bindVertexBuffer(m_skyBox.vertexBuffer());
        m_mainCommandList.bindIndexBuffer(m_skyBox.indexBuffer());

        m_mainCommandList.drawIndexed(SkyBox::VertexNum);
    }

    // Effects
    if (m_renderMode != RenderMode::Wireframe)
    {
        if (m_viewUpdate)
        {
            m_flameInstances.clear();

            for (size_t i = 0; i < m_flameData.size(); i++)
            {
                if (m_frustum.test(m_flameData[i].pos, FlameBBox))
                    m_flameInstances.push_back(m_flameData.data() + i);
            }
        }

        if (!m_flameInstances.empty())
        {
            if (m_flameInstances.size() > 1)
            {
                std::sort(m_flameInstances.begin(), m_flameInstances.end(), [camdir = m_camera.direction()]
                (const FlameInstance* a, const FlameInstance* b) -> bool
                    {
                        return glm::dot(camdir, a->pos) > glm::dot(camdir, b->pos);
                    });
            }

            m_mainCommandList.enable(Render::Pipeline::Blend);
            m_mainCommandList.disable(Render::Pipeline::CullFace);
            m_mainCommandList.setDepthFunc(Render::Pipeline::Less);
            m_mainCommandList.bindConstantBuffer((uint8_t*)&m_effectsData);
            m_mainCommandList.bindShader(m_fireShader);
            m_mainCommandList.bindVertexBuffer(m_flameMesh.vertexBuffer());
            m_mainCommandList.bindIndexBuffer(m_flameMesh.indexBuffer());

            for (size_t i = 0; i < m_flameInstances.size(); i++)
            {
                m_mainCommandList.bindConstantBuffer((uint8_t*)(m_flameInstances[i]));
                m_mainCommandList.drawIndexed(m_flameMesh.size());
            }
        }
    }

    m_mainCommandList.finish();
    m_pipeline.execute(m_mainCommandList);

    SDL_UnlockTexture(m_frameBuffer);
}