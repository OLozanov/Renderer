#pragma once

#include <SDL3/SDL.h>
#include "Render/Render.h"
#include "Resources/Resources.h"
#include "Geometry.h"
#include "Shaders/ShaderData.h"
#include "SkyBox.h"
#include "Scene.h"
#include "Mesh.h"

#include <glm/glm.hpp>

#include <memory>

enum Key
{
    key_up = 1,
    key_down = 2,
    key_left = 4,
    key_right = 8
};

enum class RenderMode
{
    Wireframe,
    Simple,
    Lighting,
    Normals
};

class App
{
public:
    App(SDL_Texture* frameBuffer, const std::string& sceneName);

    void resize(uint32_t width, uint32_t height);

    void input(const SDL_Event& event);
    void update(float dt);
    void display();

private:
    void loadScene(const std::string& scene, glm::vec3& lightdir, float& scale);
    void setupEffects();
    void setupLight(const glm::vec3& lightDir);
    void drawShadow();
    void updateView();

private:
    uint32_t m_width;
    uint32_t m_height;

    uint32_t m_ltgridx;
    uint32_t m_ltgridy;

    Render::Pipeline m_pipeline;

    SDL_Texture* m_frameBuffer;
    Render::Bitmap m_depthBuffer;

    Render::Bitmap m_shadow;

    Render::Shader m_shadowShader;
    Render::Shader m_depthPrepassShader;
    Render::Shader m_skyShader;
    Render::Shader m_simpleShader;
    Render::Shader m_normalShader;
    Render::Shader m_lightShader;
    Render::Shader m_fireShader;

    Render::CommandList m_mainCommandList;
    Render::CommandList m_sceneCommandList;
    Render::CommandList m_transformCommandList;

    SkyBox m_skyBox;
    Mesh<SimpleVertex> m_flameMesh;
    Scene m_scene;

    std::vector<Render::Image*> m_fire;

    Render::Camera m_camera;
    Render::Frustum m_frustum;
    glm::mat4 m_projMat;

    std::vector<Light> m_lights;
    std::vector<uint32_t> m_viewLights;
    std::vector<uint32_t> m_lightGrid;

    glm::vec3 m_lightDir;

    SkyData m_skyData;
    SceneData m_sceneData;
    LightGridData m_ltgridData;
    EffectsData m_effectsData;
    std::vector<FlameInstance> m_flameData;
    std::vector<const FlameInstance*> m_flameInstances;

    uint32_t m_keys;
    RenderMode m_renderMode;
    bool m_debugDraw;
    bool m_viewUpdate;

    float m_speed;

    float m_animTime;
    uint32_t m_animFrames;

    static constexpr float AnimSpeed = 20.0f;

    static constexpr float ZNear = 0.1f;
    static constexpr float ZFar = 500.0f;

    static constexpr glm::vec3 BgColor = glm::vec3(0.0f, 0.6f, 0.8f);

    static constexpr glm::vec3 FlameBBox = glm::vec3(0.7f, 1.0f, 0.7f);
};