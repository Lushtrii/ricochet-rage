#pragma once

#include <array>
#include <utility>

#include "common.hpp"
#include "components.hpp"
#include "tiny_ecs.hpp"

// System responsible for setting up OpenGL and for rendering all the
// visual entities in the game
class RenderSystem
{
    /**
     * The following arrays store the assets the game will use. They are loaded
     * at initialization and are assumed to not be modified by the render loop.
     *
     * Whenever possible, add to these lists instead of creating dynamic state
     * it is easier to debug and faster to execute for the computer.
     */
    std::array<GLuint, texture_count> texture_gl_handles;
    std::array<ivec2, texture_count> texture_dimensions;

    // Make sure these paths remain in sync with the associated enumerators.
    // Associated id with .obj path
    const std::vector<std::pair<GEOMETRY_BUFFER_ID, std::string>> mesh_paths ={
        {GEOMETRY_BUFFER_ID::PROJECTILE, mesh_path("projectile.obj")},
    };

    // Make sure these paths remain in sync with the associated enumerators.
    const std::array<std::string, texture_count> texture_paths = {
        sprite_sheets_path("player-sprite-sheet.png"),
        sprite_sheets_path("melee-enemy-sprite-sheet.png"),
        sprite_sheets_path("ranged-enemy-sprite-sheet.png"),
        textures_path("projectile.png"),
        textures_path("charged-projectile.png"),
        textures_path("supercharged-projectile.png"),
        textures_path("wall.png"),
        textures_path("invincibility.png"),
        textures_path("ui/play-button.png"),
        textures_path("ui/tutorial-button.png"),
        textures_path("ui/exit-button.png"),
        textures_path("ui/button-border.png"),
        textures_path("ui/resume-button.png"),
        textures_path("ui/title-screen-button.png"),
        textures_path("ui/save-quit-button.png"),
        textures_path("ui/play-again-button.png"),
        textures_path("ui/continue-button.png")
    };

    std::array<GLuint, effect_count> effects;
    // Make sure these paths remain in sync with the associated enumerators.
    const std::array<std::string, effect_count> effect_paths = {
        shader_path("textured"),
        shader_path("water")};

    std::array<GLuint, geometry_count> vertex_buffers;
    std::array<GLuint, geometry_count> index_buffers;
    std::array<Mesh, geometry_count> meshes;

public:
    // Initialize the window
    bool init(GLFWwindow *window);

    template <class T>
    void bindVBOandIBO(GEOMETRY_BUFFER_ID gid, std::vector<T> vertices, std::vector<uint16_t> indices);

    void initializeGlTextures();

    void initializeGlEffects();

    void initializeGlMeshes();
    Mesh &getMesh(GEOMETRY_BUFFER_ID id) { return meshes[(int)id]; };

    void initializeGlGeometryBuffers();
    // Initialize the screen texture used as intermediate render target
    // The draw loop first renders to this texture, then it is used for the wind
    // shader
    bool initScreenTexture();

    bool initMainMenu(bool saveFileExists);
    void drawMainMenu();

    bool initTutorial();
    void drawTutorial();

    bool initPauseMenu();
    void drawPauseMenu();

    bool initDeathScreen();
    void drawDeathScreen();

    bool initWinScreen();
    void drawWinScreen();

    int getActiveScreen() const;
    void setActiveScreen(int activeScreen);

    Entity getHoverEntity() {return hoverEntity;};

    Entity createButton(vec2 position, int screenTiedTo, int screenGoTo, int textureID, bool isActive);

    void flipActiveButtions(int activeScreen);

    Entity createHoverEffect();

    void drawButtons();

    bool doesSaveFileExist();

    // Destroy resources associated to one or all entities created by the system
    ~RenderSystem();

    // Draw all entities
    void draw(float elapsed_ms, bool isPaused);

    mat3 createProjectionMatrix();

private:
    void updateAnimations(float elapsed_ms);
    void drawTexturedMeshWithAnim(Entity entity, const mat3& projection, const Animation& anim);

    // Internal drawing functions for each entity type
    void drawTexturedMesh(Entity entity, const mat3 &projection);
    void drawToScreen();

    // Window handle
    GLFWwindow *window;

    // Screen texture handles
    GLuint frame_buffer;
    GLuint off_screen_render_buffer_color;
    GLuint off_screen_render_buffer_depth;

    Entity screen_state_entity;
    Entity hoverEntity;

    
    GLuint mainMenuTexture;
    const std::string mainMenuImgPath = textures_path("ui/main-menu.png");

    GLuint tutorialTexture;
    const std::string tutorialImgPath = textures_path("ui/tutorial.png");

    GLuint pauseMenuTexture;
    const std::string pauseMenuImgPath = textures_path("ui/pause-menu.png");

    GLuint deathScreenTexture;
    const std::string deathScreenImgPath = textures_path("ui/death-screen.png");

    GLuint winScreenTexture;
    const std::string winScreenImgPath = textures_path("ui/win-screen.png");

    const float MENU_BUTTON_HEIGHT = 90.f;
    const float MENU_BUTTON_WIDTH = 380.f;

    bool saveFileExists;
};

bool loadEffectFromFile(
    const std::string &vs_path, const std::string &fs_path, GLuint &out_program);
