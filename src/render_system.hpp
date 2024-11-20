#pragma once

#include <array>
#include <utility>

#include "common.hpp"
#include "components.hpp"
#include "tiny_ecs.hpp"

#include "../ext/freetype/include/ft2build.h"
#include FT_FREETYPE_H
#include <map>

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
        textures_path("ui/continue-button.png"),
        textures_path("health-bar.png"),
        sprite_sheets_path("boss-enemy-sprite-sheet.png")
    };

    std::array<GLuint, effect_count> effects;
    // Make sure these paths remain in sync with the associated enumerators.
    const std::array<std::string, effect_count> effect_paths = {
        shader_path("textured"),
        shader_path("water"),
        shader_path("light"),
    };

    std::array<GLuint, geometry_count> vertex_buffers;
    std::array<GLuint, geometry_count> index_buffers;
    std::array<Mesh, geometry_count> meshes;

public:
    // Initialize the window
    bool init(GLFWwindow *window);

    bool fontInit(GLFWwindow* window, const std::string& font_filename, unsigned int font_default_size);

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

    bool mouseGestureInit();

    int getActiveScreen() const;
    void setActiveScreen(int activeScreen);

    Entity getHoverEntity() {return hoverEntity;};

    Entity createButton(vec2 position, int screenTiedTo, int screenGoTo, int textureID, bool isActive);

    void flipActiveButtions(int activeScreen);

    Entity createHoverEffect();

    void drawButtons();

    bool doesSaveFileExist();

    void initLight();
    std::vector<vec2> getEntityCurrentVertices(Entity& e);
    int getCurrentFrame(Entity& e);
    std::vector<Ray> generateWallRays();
    std::vector<Ray> generateRays(std::vector<vec2>& vertices);
    std::vector<LineSegment> generateWallSegments();
    std::vector<LineSegment> generateLineSegments(std::vector<vec2>& vertices);
    float getRayIntersectionDist(Ray& ray, LineSegment& segment);
    std::vector<std::array<float, 2>> getLitArea(std::vector<Ray>& rays, std::vector<LineSegment>& segments);
    void lightScreen();

    // Destroy resources associated to one or all entities created by the system
    ~RenderSystem();

    // Draw all entities
    void draw(float elapsed_ms, bool isPaused);

    void drawMouseGestures();

    mat3 createProjectionMatrix();

private:
    void updateAnimations(float elapsed_ms);
    void drawTexturedMeshWithAnim(Entity entity, const mat3& projection, const Animation& anim);

    // Internal drawing functions for each entity type
    void drawTexturedMesh(Entity entity, const mat3 &projection);
    void drawToScreen();

    void renderTextBulk(std::vector<TextRenderRequest>& requests);

    // Window handle
    GLFWwindow *window;

    // Screen texture handles
    GLuint frame_buffer;
    GLuint off_screen_render_buffer_color;
    GLuint off_screen_render_buffer_depth_stencil;

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

    GLuint vao;

    GLuint m_light_VAO;
    GLuint m_light_VBO;

    std::map<char, Character> m_ftCharacters;
	GLuint m_font_shaderProgram;
	GLuint m_font_VAO;
	GLuint m_font_VBO;

    const std::vector<vec2> playerFrame1 = {
        vec2(0.500, 0.035),
            vec2(-0.500, -0.086),
            vec2(-0.236, 0.488),
            vec2(-0.013, -0.500),
            vec2(-0.342, -0.463),
            vec2(-0.072, 0.500),
            vec2(-0.372, 0.383),
    };
    // Made svg outlines of each sprite frame and got a python script to extract the vertices

    const std::vector<vec2> playerFrame2 = 
        {
        vec2(-0.305, -0.484),
        vec2(0.500, 0.053),
        vec2(0.045, -0.500),
        vec2(-0.378, 0.373),
        vec2(-0.142, -0.332),
        vec2(-0.240, 0.500),
        vec2(-0.061, 0.500),
        vec2(-0.439, -0.393),
        vec2(-0.500, -0.057),
    };

    const std::vector<vec2> playerFrame3 = 
        {
        vec2(-0.074, 0.500),
        vec2(-0.369, 0.384),
        vec2(0.500, 0.006),
        vec2(-0.142, -0.483),
        vec2(-0.222, 0.494),
        vec2(-0.500, -0.087),
        vec2(-0.312, -0.500),
    };

    const std::vector<vec2> playerFrame4 =
        {
        vec2(0.043, -0.500),
        vec2(-0.289, -0.484),
        vec2(-0.059, 0.500),
        vec2(-0.144, -0.333),
        vec2(-0.354, 0.374),
        vec2(-0.500, -0.053),
        vec2(0.500, 0.045),
        vec2(-0.435, -0.378),
        vec2(-0.245, 0.480),
    };

    const std::vector<vec2> playerFrame5 =
        {
        vec2(-0.239, 0.500),
        vec2(-0.500, -0.084),
        vec2(-0.295, -0.500),
        vec2(-0.062, 0.478),
        vec2(-0.125, -0.483),
        vec2(0.500, 0.011),
        vec2(-0.375, 0.337),
    };

    const std::array<std::vector<vec2>, 5> playerOutlineVertices = 
    {
        std::vector<vec2>(playerFrame1),
        std::vector<vec2>(playerFrame2),
        std::vector<vec2>(playerFrame3),
        std::vector<vec2>(playerFrame4),
        std::vector<vec2>(playerFrame5),
    };

    const std::vector<vec2> meleeEnemyFrame1 = {
        vec2(-0.261, -0.489),
        vec2(-0.303, 0.500),
        vec2(0.076, -0.462),
        vec2(0.340, -0.335),
        vec2(0.445, -0.243),
        vec2(0.500, 0.052),
        vec2(-0.424, -0.301),
        vec2(-0.500, -0.044),
        vec2(-0.059, -0.500),
        vec2(0.282, 0.489),
        vec2(-0.408, 0.351),
    };
    const std::vector<vec2> meleeEnemyFrame2 = {
        vec2(-0.025, -0.492),
        vec2(-0.224, -0.500),
        vec2(0.500, 0.088),
        vec2(-0.301, 0.500),
        vec2(0.406, -0.212),
        vec2(-0.500, -0.364),
        vec2(-0.461, 0.317),
        vec2(0.218, 0.500),
    };
    const std::vector<vec2> meleeEnemyFrame3 = {
        vec2(0.012, -0.500),
        vec2(0.264, 0.477),
        vec2(0.500, 0.053),
        vec2(0.431, -0.239),
        vec2(-0.347, -0.485),
        vec2(-0.500, 0.326),
        vec2(-0.382, 0.178),
        vec2(-0.347, 0.500),
    };
    const std::vector<vec2> meleeEnemyFrame4 = {
        vec2(-0.483, 0.370),
        vec2(0.500, 0.085),
        vec2(0.218, 0.492),
        vec2(-0.322, 0.500),
        vec2(-0.040, -0.496),
        vec2(-0.241, -0.500),
        vec2(-0.500, -0.385),
        vec2(0.397, -0.211),
    };
    const std::vector<vec2> meleeEnemyFrame5 = {
        vec2(0.500, 0.076),
        vec2(-0.375, 0.375),
        vec2(-0.282, 0.496),
        vec2(0.442, -0.249),
        vec2(-0.251, -0.492),
        vec2(-0.500, -0.045),
        vec2(0.302, 0.500),
        vec2(-0.430, -0.304),
        vec2(0.029, -0.500),
    };

    const std::array<std::vector<vec2>, 5> meleeEnemyOutlineVertices = 
    {
        std::vector<vec2>(meleeEnemyFrame1),
        std::vector<vec2>(meleeEnemyFrame2),
        std::vector<vec2>(meleeEnemyFrame3),
        std::vector<vec2>(meleeEnemyFrame4),
        std::vector<vec2>(meleeEnemyFrame5),
    };


    const std::vector<vec2> rangedEnemyFrame1 = {
        vec2(-0.037, -0.449),
        vec2(-0.359, 0.500),
        vec2(-0.500, 0.301),
        vec2(-0.347, -0.500),
        vec2(0.500, 0.062),
        vec2(-0.150, 0.477),
    };
    const std::vector<vec2> rangedEnemyFrame2 = {
        vec2(-0.500, 0.325),
        vec2(0.500, 0.048),
        vec2(-0.403, 0.500),
        vec2(-0.191, -0.477),
        vec2(-0.329, -0.500),
        vec2(-0.174, 0.472),
    };

    const std::vector<vec2> rangedEnemyFrame3 = {
        vec2(-0.386, 0.500),
        vec2(-0.045, -0.500),
        vec2(-0.347, -0.500),
        vec2(-0.142, 0.460),
        vec2(-0.187, -0.402),
        vec2(-0.500, 0.322),
        vec2(0.500, 0.052),
    };

    const std::vector<vec2> rangedEnemyFrame4 = {
        vec2(0.500, 0.029),
        vec2(-0.350, -0.471),
        vec2(-0.188, -0.500),
        vec2(-0.500, 0.305),
        vec2(-0.408, 0.500),
        vec2(-0.176, 0.477),
    };

    const std::vector<vec2> rangedEnemyFrame5 = {
        vec2(0.500, 0.063),
        vec2(-0.422, -0.477),
        vec2(-0.500, 0.494),
        vec2(-0.205, -0.408),
        vec2(-0.090, -0.500),
        vec2(-0.235, -0.477),
        vec2(-0.217, 0.500),
    };

    const std::array<std::vector<vec2>, 5> rangedEnemyOutlineVertices = 
    {
        std::vector<vec2>(rangedEnemyFrame1),
        std::vector<vec2>(rangedEnemyFrame2),
        std::vector<vec2>(rangedEnemyFrame1),
        std::vector<vec2>(rangedEnemyFrame4),
        std::vector<vec2>(rangedEnemyFrame5),
    };

    const std::vector<vec2> bossEnemyFrame1 = {
        vec2(0.020, -0.494),
        vec2(-0.246, 0.496),
        vec2(0.268, 0.362),
        vec2(0.018, 0.500),
        vec2(-0.500, 0.368),
        vec2(-0.378, -0.500),
        vec2(0.500, 0.018),
    };
    const std::vector<vec2> bossEnemyFrame2 = {
        vec2(-0.494, -0.311),
        vec2(-0.247, 0.498),
        vec2(0.020, 0.500),
        vec2(-0.500, 0.369),
        vec2(0.500, 0.002),
        vec2(-0.192, -0.500),
        vec2(0.273, 0.338),
    };
    const std::vector<vec2> bossEnemyFrame3 = {
        vec2(-0.255, 0.496),
        vec2(-0.403, -0.414),
        vec2(0.011, 0.500),
        vec2(0.500, -0.004),
        vec2(-0.113, -0.500),
        vec2(-0.500, 0.361),
        vec2(0.273, 0.350),
    };
    const std::vector<vec2> bossEnemyFrame4 = {
        vec2(-0.099, -0.500),
        vec2(-0.500, 0.368),
        vec2(0.500, -0.002),
        vec2(0.031, 0.496),
        vec2(0.274, 0.346),
        vec2(-0.235, 0.500),
        vec2(-0.490, -0.416),
    };
    const std::vector<vec2> bossEnemyFrame5 = {
        vec2(0.032, 0.490),
        vec2(0.257, 0.374),
        vec2(-0.291, -0.500),
        vec2(-0.500, 0.360),
        vec2(0.500, 0.010),
        vec2(0.042, -0.482),
        vec2(-0.243, 0.500),
    };

    const std::array<std::vector<vec2>, 5> bossEnemyOutlineVertices = 
    {
        std::vector<vec2>(bossEnemyFrame1),
        std::vector<vec2>(bossEnemyFrame2),
        std::vector<vec2>(bossEnemyFrame1),
        std::vector<vec2>(bossEnemyFrame4),
        std::vector<vec2>(bossEnemyFrame5),
    };
    
    GLuint ges_shaderProgram;
    GLuint ges_VAO;
    GLuint ges_VBO;

    bool distortColor = false;
};

bool loadEffectFromFile(
    const std::string &vs_path, const std::string &fs_path, GLuint &out_program);

