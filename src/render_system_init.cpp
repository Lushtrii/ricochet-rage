// internal
#include "common.hpp"
#include "components.hpp"
#include "render_system.hpp"

#include <GLFW/glfw3.h>
#include <array>
#include <fstream>

#include "../ext/stb_image/stb_image.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// This creates circular header inclusion, that is quite bad.
#include "tiny_ecs_registry.hpp"
#include "world_init.hpp"

// stlib
#include <iostream>
#include <sstream>


bool RenderSystem::doesSaveFileExist() {
    std::ifstream f("../Save1.data");
    saveFileExists = f.good();
    return saveFileExists;
}

// World initialization
bool RenderSystem::init(GLFWwindow* window_arg)
{
	this->window = window_arg;

	glfwMakeContextCurrent(window);
	glfwSwapInterval(1); // vsync

	// Load OpenGL function pointers
	const int is_fine = gl3w_init();
	assert(is_fine == 0);

	// Create a frame buffer
	frame_buffer = 0;
	glGenFramebuffers(1, &frame_buffer);
	glBindFramebuffer(GL_FRAMEBUFFER, frame_buffer);
	gl_has_errors();

	// For some high DPI displays (ex. Retina Display on Macbooks)
	// https://stackoverflow.com/questions/36672935/why-retina-screen-coordinate-value-is-twice-the-value-of-pixel-value
	int frame_buffer_width_px, frame_buffer_height_px;
	glfwGetFramebufferSize(window, &frame_buffer_width_px, &frame_buffer_height_px);  // Note, this will be 2x the resolution given to glfwCreateWindow on retina displays
	if (frame_buffer_width_px != window_width_px)
	{
		printf("WARNING: retina display! https://stackoverflow.com/questions/36672935/why-retina-screen-coordinate-value-is-twice-the-value-of-pixel-value\n");
		printf("glfwGetFramebufferSize = %d,%d\n", frame_buffer_width_px, frame_buffer_height_px);
		printf("window width_height = %d,%d\n", window_width_px, window_height_px);
	}

	// Hint: Ask your TA for how to setup pretty OpenGL error callbacks. 
	// This can not be done in macOS, so do not enable
	// it unless you are on Linux or Windows. You will need to change the window creation
	// code to use OpenGL 4.3 (not suported in macOS) and add additional .h and .cpp
	// glDebugMessageCallback((GLDEBUGPROC)errorCallback, nullptr);

	// We are not really using VAO's but without at least one bound we will crash in
	// some systems.
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);
	gl_has_errors();

	initScreenTexture();
    initializeGlTextures();
	initializeGlEffects();
	initializeGlGeometryBuffers();
	mouseGestureInit();

    saveFileExists = doesSaveFileExist();
    initMainMenu(saveFileExists);
	initTutorial();
    initPauseMenu();
    initDeathScreen();
    initWinScreen();
    initGameBackground();
    initFloor();
    initSpaceship();

    if (LIGHT_SYSTEM_TOGGLE) {
        initLight();
    }


	return true;
}

std::string readShaderFile(const std::string& filename)
{
	std::cout << "Loading shader filename: " << filename << std::endl;

	std::ifstream ifs(filename);

	if (!ifs.good())
	{
		std::cerr << "ERROR: invalid filename loading shader from file: " << filename << std::endl;
		return "";
	}

	std::ostringstream oss;
	oss << ifs.rdbuf();
	std::cout << oss.str() << std::endl;
	return oss.str();
}

bool RenderSystem::mouseGestureInit() {
	std::string vertexShaderSource = readShaderFile(PROJECT_SOURCE_DIR + std::string("shaders/gesture.vs.glsl"));
	std::string fragmentShaderSource = readShaderFile(PROJECT_SOURCE_DIR + std::string("shaders/gesture.fs.glsl"));
	const char* vertexShaderSource_c = vertexShaderSource.c_str();
	const char* fragmentShaderSource_c = fragmentShaderSource.c_str();

	glGenVertexArrays(1, &ges_VAO);
	glGenBuffers(1, &ges_VBO);

	unsigned int gest_vertexShader;
	gest_vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(gest_vertexShader, 1, &vertexShaderSource_c, NULL);
	glCompileShader(gest_vertexShader);

	unsigned int ges_fragmentShader;
	ges_fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(ges_fragmentShader, 1, &fragmentShaderSource_c, NULL);
	glCompileShader(ges_fragmentShader);

	ges_shaderProgram = glCreateProgram();
	glAttachShader(ges_shaderProgram, gest_vertexShader);
	glAttachShader(ges_shaderProgram, ges_fragmentShader);
	glLinkProgram(ges_shaderProgram);

	// apply orthographic projection matrix for font, i.e., screen space
	glUseProgram(ges_shaderProgram);
	int w, h;
	glfwGetWindowSize(window, &w, &h);
	glm::mat4 projection = glm::ortho(0.0f, static_cast<float>(w), 0.0f, static_cast<float>(h));
	GLint project_location = glGetUniformLocation(ges_shaderProgram, "projection");
	assert(project_location > -1);
	std::cout << "project_location: " << project_location << std::endl;
	glUniformMatrix4fv(project_location, 1, GL_FALSE, glm::value_ptr(projection));

	glDeleteShader(gest_vertexShader);
	glDeleteShader(ges_fragmentShader);

	glBindVertexArray(ges_VAO);
	glBindBuffer(GL_ARRAY_BUFFER, ges_VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vec2) * 1024, NULL, GL_DYNAMIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(vec2), 0);

	// release buffers
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	return true;
}

bool RenderSystem::fontInit(GLFWwindow* window, const std::string& font_filename, unsigned int font_default_size) {
	// read in our shader files
	std::string vertexShaderSource = readShaderFile(PROJECT_SOURCE_DIR + std::string("shaders/font.vs.glsl"));
	std::string fragmentShaderSource = readShaderFile(PROJECT_SOURCE_DIR + std::string("shaders/font.fs.glsl"));
	const char* vertexShaderSource_c = vertexShaderSource.c_str();
	const char* fragmentShaderSource_c = fragmentShaderSource.c_str();

	// enable blending or you will just get solid boxes instead of text
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// font buffer setup
	glGenVertexArrays(1, &m_font_VAO);
	glGenBuffers(1, &m_font_VBO);

	// font vertex shader
	unsigned int font_vertexShader;
	font_vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(font_vertexShader, 1, &vertexShaderSource_c, NULL);
	glCompileShader(font_vertexShader);

	// font fragement shader
	unsigned int font_fragmentShader;
	font_fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(font_fragmentShader, 1, &fragmentShaderSource_c, NULL);
	glCompileShader(font_fragmentShader);

	// font shader program
	m_font_shaderProgram = glCreateProgram();
	glAttachShader(m_font_shaderProgram, font_vertexShader);
	glAttachShader(m_font_shaderProgram, font_fragmentShader);
	glLinkProgram(m_font_shaderProgram);

	// apply orthographic projection matrix for font, i.e., screen space
	glUseProgram(m_font_shaderProgram);
	int w, h;
	glfwGetWindowSize(window, &w, &h);
	glm::mat4 projection = glm::ortho(0.0f, static_cast<float>(w), 0.0f, static_cast<float>(h));
	GLint project_location = glGetUniformLocation(m_font_shaderProgram, "projection");
	assert(project_location > -1);
	std::cout << "project_location: " << project_location << std::endl;
	glUniformMatrix4fv(project_location, 1, GL_FALSE, glm::value_ptr(projection));

	// clean up shaders
	glDeleteShader(font_vertexShader);
	glDeleteShader(font_fragmentShader);

	// init FreeType fonts
	FT_Library ft;
	if (FT_Init_FreeType(&ft))
	{
		std::cerr << "ERROR::FREETYPE: Could not init FreeType Library" << std::endl;
		return false;
	}

	FT_Face face;
	if (FT_New_Face(ft, font_filename.c_str(), 0, &face))
	{
		std::cerr << "ERROR::FREETYPE: Failed to load font: " << font_filename << std::endl;
		return false;
	}

	// extract a default size
	FT_Set_Pixel_Sizes(face, 0, font_default_size);

	// disable byte-alignment restriction in OpenGL
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	// load each of the chars - note only first 128 ASCII chars
	for (unsigned char c = (unsigned char)0; c < (unsigned char)128; c++)
	{
		// load character glyph 
		if (FT_Load_Char(face, c, FT_LOAD_RENDER))
		{
			std::cerr << "ERROR::FREETYTPE: Failed to load Glyph" << std::endl;
			continue;
		}

		// generate texture
		unsigned int texture;
		glGenTextures(1, &texture);
		glBindTexture(GL_TEXTURE_2D, texture);

		// std::cout << "texture: " << c << " = " << texture << std::endl;

		glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, face->glyph->bitmap.width, face->glyph->bitmap.rows, 0, GL_RED, GL_UNSIGNED_BYTE, face->glyph->bitmap.buffer);

		// set texture options
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// now store character for later use
		Character character = {
			texture,
			glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
			glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
			static_cast<unsigned int>(face->glyph->advance.x),
			(char)c
		};
		m_ftCharacters.insert(std::pair<char, Character>(c, character));
	}
	glBindTexture(GL_TEXTURE_2D, 0);

	// clean up
	FT_Done_Face(face);
	FT_Done_FreeType(ft);

	// bind buffers
	glBindVertexArray(m_font_VAO);
	glBindBuffer(GL_ARRAY_BUFFER, m_font_VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);

	// release buffers
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	return true;
}

void RenderSystem::initializeGlTextures()
{
    glGenTextures((GLsizei)texture_gl_handles.size(), texture_gl_handles.data());

    for(uint i = 0; i < texture_paths.size(); i++)
    {
		const std::string& path = texture_paths[i];
		ivec2& dimensions = texture_dimensions[i];

		stbi_uc* data;
        stbi_set_flip_vertically_on_load(true);
		data = stbi_load(path.c_str(), &dimensions.x, &dimensions.y, NULL, 4);

		if (data == NULL)
		{
			const std::string message = "Could not load the file " + path + ".";
			fprintf(stderr, "%s", message.c_str());
			assert(false);
		}
		glBindTexture(GL_TEXTURE_2D, texture_gl_handles[i]);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, dimensions.x, dimensions.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		gl_has_errors();
		stbi_image_free(data);
    }
	gl_has_errors();
}

void RenderSystem::initializeGlEffects()
{
	for(uint i = 0; i < effect_paths.size(); i++)
	{
		const std::string vertex_shader_name = effect_paths[i] + ".vs.glsl";
		const std::string fragment_shader_name = effect_paths[i] + ".fs.glsl";

		bool is_valid = loadEffectFromFile(vertex_shader_name, fragment_shader_name, effects[i]);
		assert(is_valid && (GLuint)effects[i] != 0);
	}
}

// One could merge the following two functions as a template function...
template <class T>
void RenderSystem::bindVBOandIBO(GEOMETRY_BUFFER_ID gid, std::vector<T> vertices, std::vector<uint16_t> indices)
{
	glBindBuffer(GL_ARRAY_BUFFER, vertex_buffers[(uint)gid]);
	glBufferData(GL_ARRAY_BUFFER,
		sizeof(vertices[0]) * vertices.size(), vertices.data(), GL_STATIC_DRAW);
	gl_has_errors();

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffers[(uint)gid]);
    gl_has_errors();
	glBufferData(GL_ELEMENT_ARRAY_BUFFER,
		sizeof(indices[0]) * indices.size(), indices.data(), GL_STATIC_DRAW);
	gl_has_errors();
}

void RenderSystem::initializeGlMeshes()
{
	for (uint i = 0; i < mesh_paths.size(); i++)
	{
		// Initialize meshes
		GEOMETRY_BUFFER_ID geom_index = mesh_paths[i].first;
		std::string name = mesh_paths[i].second;
		Mesh::loadFromOBJFile(name, 
			meshes[(int)geom_index].vertices,
			meshes[(int)geom_index].vertex_indices,
            meshes[(int)geom_index].uv_indices,
			meshes[(int)geom_index].original_size);

		bindVBOandIBO(geom_index,
			meshes[(int)geom_index].vertices, 
			meshes[(int)geom_index].vertex_indices);
	}
}

void RenderSystem::initializeGlGeometryBuffers()
{
	// Vertex Buffer creation.
	glGenBuffers((GLsizei)vertex_buffers.size(), vertex_buffers.data());
	// Index Buffer creation.
	glGenBuffers((GLsizei)index_buffers.size(), index_buffers.data());

	// Index and Vertex buffer data initialization.
	initializeGlMeshes();

	//////////////////////////
	// Initialize sprite
	// The position corresponds to the center of the texture.
	std::vector<TexturedVertex> textured_vertices(4);
	textured_vertices[0].position = { -1.f/2, +1.f/2, 0.f };
	textured_vertices[1].position = { +1.f/2, +1.f/2, 0.f };
	textured_vertices[2].position = { +1.f/2, -1.f/2, 0.f };
	textured_vertices[3].position = { -1.f/2, -1.f/2, 0.f };
	textured_vertices[0].texcoord = { 0.f, 1.f };
	textured_vertices[1].texcoord = { 1.f, 1.f };
	textured_vertices[2].texcoord = { 1.f, 0.f };
	textured_vertices[3].texcoord = { 0.f, 0.f };

	// Counterclockwise as it's the default opengl front winding direction.
	const std::vector<uint16_t> textured_indices = { 0, 3, 1, 1, 3, 2 };
	bindVBOandIBO(GEOMETRY_BUFFER_ID::SPRITE, textured_vertices, textured_indices);

	//////////////////////////
	// Initialize ui component
	// The position corresponds to the center of the texture.
	std::vector<TexturedVertex> ui_vertices(4);
	ui_vertices[0].position = { -1.f/2, +1.f/2, 0.f };
	ui_vertices[1].position = { +1.f/2, +1.f/2, 0.f };
	ui_vertices[2].position = { +1.f/2, -1.f/2, 0.f };
	ui_vertices[3].position = { -1.f/2, -1.f/2, 0.f };
	ui_vertices[0].texcoord = { 0.f, 1.f };
	ui_vertices[1].texcoord = { 1.f, 1.f };
	ui_vertices[2].texcoord = { 1.f, 0.f };
	ui_vertices[3].texcoord = { 0.f, 0.f };

	// Counterclockwise as it's the default opengl front winding direction.
	const std::vector<uint16_t> ui_indices = { 0, 3, 1, 1, 3, 2 };
	bindVBOandIBO(GEOMETRY_BUFFER_ID::UI_COMPONENT, ui_vertices, ui_indices);


	//////////////////////////
	// Initialize ui component
	// The position corresponds to the center of the texture.
    // Defined in world space
    const int lowerLeftX = 0;
    const int lowerLeftY = 0;
    const int FLOOR_TEXTURE_SIZE = 64;

    float roomWidth = m_roomTileSize.x * m_roomTileDimensions.x;
    float roomHeight = m_roomTileSize.x * m_roomTileDimensions.y;

	std::vector<TexturedVertex> floor_vertices(4);
    // bot left
	floor_vertices[0].position = { lowerLeftX, lowerLeftY, 0.f };
    // bot right
	floor_vertices[1].position = { roomWidth, lowerLeftY, 0.f };
    // top left
	floor_vertices[2].position = { lowerLeftX, roomHeight, 0.f };
    // top right
	floor_vertices[3].position = { roomWidth, roomHeight, 0.f };

	floor_vertices[0].texcoord = { 0.f, 0.f };
	floor_vertices[1].texcoord = { roomWidth/FLOOR_TEXTURE_SIZE, 0.f };
	floor_vertices[2].texcoord = { 0.f, roomHeight/FLOOR_TEXTURE_SIZE };
	floor_vertices[3].texcoord = { roomWidth/FLOOR_TEXTURE_SIZE, roomHeight/FLOOR_TEXTURE_SIZE };


	// Counterclockwise as it's the default opengl front winding direction.
	/* const std::vector<uint16_t> ui_indices = { 0, 3, 1, 1, 3, 2 }; */
	const std::vector<uint16_t> floor_indices = { 0, 1, 2, 1, 3, 2 };
	bindVBOandIBO(GEOMETRY_BUFFER_ID::FLOOR, floor_vertices, floor_indices);
	///////////////////////////////////////////////////////
	// Initialize screen triangle (yes, triangle, not quad; its more efficient).
	std::vector<vec3> screen_vertices(3);
	screen_vertices[0] = { -1, -6, 0.f };
	screen_vertices[1] = { 6, -1, 0.f };
	screen_vertices[2] = { -1, 6, 0.f };

	// Counterclockwise as it's the default opengl front winding direction.
	const std::vector<uint16_t> screen_indices = { 0, 1, 2 };
	bindVBOandIBO(GEOMETRY_BUFFER_ID::SCREEN_TRIANGLE, screen_vertices, screen_indices);

    vec2 innerTopLeftCorner = vec2(window_width_px - 1175, window_height_px - 650);
    vec2 innerTopRightCorner = vec2(window_width_px - 75, window_height_px - 650);
    vec2 innerBottomLeftCorner = vec2(window_width_px - 1175, window_height_px - 100);
    vec2 innerBottomRightCorner = vec2(window_width_px - 75, window_height_px - 100);

	std::vector<vec3> shadow_vertices(4);
    shadow_vertices[0] = { innerTopLeftCorner, 1.0f }; // Top-left
    shadow_vertices[1] = { innerTopRightCorner, 1.0f }; // Top-right
    shadow_vertices[2] = {  innerBottomRightCorner, 1.0f }; // Bottom-right
    shadow_vertices[3] = { innerBottomLeftCorner, 1.0f }; // Bottom-left

	// Counterclockwise as it's the default opengl front winding direction.
	const std::vector<uint16_t> shadow_indices = { 0, 3, 2, 0, 2, 1};
	bindVBOandIBO(GEOMETRY_BUFFER_ID::SHADOW_PLANE,shadow_vertices,shadow_indices);
}

RenderSystem::~RenderSystem()
{
	// Don't need to free gl resources since they last for as long as the program,
	// but it's polite to clean after yourself.
	glDeleteBuffers((GLsizei)vertex_buffers.size(), vertex_buffers.data());
	glDeleteBuffers((GLsizei)index_buffers.size(), index_buffers.data());
	glDeleteTextures((GLsizei)texture_gl_handles.size(), texture_gl_handles.data());
	glDeleteTextures(1, &off_screen_render_buffer_color);
	glDeleteRenderbuffers(1, &off_screen_render_buffer_depth_stencil);
	gl_has_errors();

	for(uint i = 0; i < effect_count; i++) {
		glDeleteProgram(effects[i]);
	}
	// delete allocated resources
	glDeleteFramebuffers(1, &frame_buffer);
	gl_has_errors();

	// remove all entities created by the render system
	while (registry.renderRequests.entities.size() > 0)
	    registry.remove_all_components_of(registry.renderRequests.entities.back());
}

// Initialize the screen texture from a standard sprite
bool RenderSystem::initScreenTexture()
{
	registry.screenStates.emplace(screen_state_entity);

	int framebuffer_width, framebuffer_height;
	glfwGetFramebufferSize(const_cast<GLFWwindow*>(window), &framebuffer_width, &framebuffer_height);  // Note, this will be 2x the resolution given to glfwCreateWindow on retina displays

	glGenTextures(1, &off_screen_render_buffer_color);
	glBindTexture(GL_TEXTURE_2D, off_screen_render_buffer_color);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, framebuffer_width, framebuffer_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	gl_has_errors();

	glGenRenderbuffers(1, &off_screen_render_buffer_depth_stencil);
	glBindRenderbuffer(GL_RENDERBUFFER, off_screen_render_buffer_depth_stencil);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, off_screen_render_buffer_color, 0);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_STENCIL, framebuffer_width, framebuffer_height);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, off_screen_render_buffer_depth_stencil);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, off_screen_render_buffer_depth_stencil);
	gl_has_errors();

	assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);

	return true;
}

bool gl_compile_shader(GLuint shader)
{
	glCompileShader(shader);
	gl_has_errors();
	GLint success = 0;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
	if (success == GL_FALSE)
	{
		GLint log_len;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &log_len);
		std::vector<char> log(log_len);
		glGetShaderInfoLog(shader, log_len, &log_len, log.data());
		glDeleteShader(shader);

		gl_has_errors();

		fprintf(stderr, "GLSL: %s", log.data());
		return false;
}

	return true;
}

bool loadEffectFromFile(
	const std::string& vs_path, const std::string& fs_path, GLuint& out_program)
{
	// Opening files
	std::ifstream vs_is(vs_path);
	std::ifstream fs_is(fs_path);
	if (!vs_is.good() || !fs_is.good())
	{
		fprintf(stderr, "Failed to load shader files %s, %s", vs_path.c_str(), fs_path.c_str());
		assert(false);
		return false;
	}

	// Reading sources
	std::stringstream vs_ss, fs_ss;
	vs_ss << vs_is.rdbuf();
	fs_ss << fs_is.rdbuf();
	std::string vs_str = vs_ss.str();
	std::string fs_str = fs_ss.str();
	const char* vs_src = vs_str.c_str();
	const char* fs_src = fs_str.c_str();
	GLsizei vs_len = (GLsizei)vs_str.size();
	GLsizei fs_len = (GLsizei)fs_str.size();

	GLuint vertex = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertex, 1, &vs_src, &vs_len);
	GLuint fragment = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragment, 1, &fs_src, &fs_len);
	gl_has_errors();

	// Compiling
	if (!gl_compile_shader(vertex))
	{
		fprintf(stderr, "Vertex compilation failed");
		assert(false);
		return false;
	}
	if (!gl_compile_shader(fragment))
	{
		fprintf(stderr, "Vertex compilation failed");
		assert(false);
		return false;
	}

	// Linking
	out_program = glCreateProgram();
	glAttachShader(out_program, vertex);
	glAttachShader(out_program, fragment);
	glLinkProgram(out_program);
	gl_has_errors();

	{
		GLint is_linked = GL_FALSE;
		glGetProgramiv(out_program, GL_LINK_STATUS, &is_linked);
		if (is_linked == GL_FALSE)
		{
			GLint log_len;
			glGetProgramiv(out_program, GL_INFO_LOG_LENGTH, &log_len);
			std::vector<char> log(log_len);
			glGetProgramInfoLog(out_program, log_len, &log_len, log.data());
			gl_has_errors();

			fprintf(stderr, "Link error: %s", log.data());
			assert(false);
			return false;
		}
	}

	// No need to carry this around. Keeping these objects is only useful if we recycle
	// the same shaders over and over, which we don't, so no need and this is simpler.
	glDetachShader(out_program, vertex);
	glDetachShader(out_program, fragment);
	glDeleteShader(vertex);
	glDeleteShader(fragment);
	gl_has_errors();

	return true;
}

bool RenderSystem::initMainMenu(bool saveFileExists) {
	int framebuffer_width, framebuffer_height;
	glfwGetFramebufferSize(const_cast<GLFWwindow*>(window), &framebuffer_width, &framebuffer_height);  // Note, this will be 2x the resolution given to glfwCreateWindow on retina displays

	glGenTextures(1, &mainMenuTexture);

    ivec2 dimensions;

    stbi_uc* data;
    stbi_set_flip_vertically_on_load(true);
    data = stbi_load(mainMenuImgPath.c_str(), &dimensions.x, &dimensions.y, NULL, 4);

    if (data == NULL)
    {
        const std::string message = "Could not load the file " + mainMenuImgPath + ".";
        fprintf(stderr, "%s", message.c_str());
        assert(false);
    }
    glBindTexture(GL_TEXTURE_2D, mainMenuTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, dimensions.x, dimensions.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    gl_has_errors();
    stbi_image_free(data);


    // Make menu buttons
    int mainMenuScreen = (int) SCREEN_ID::MAIN_MENU;
    createButton(vec2(window_width_px/2, 305), mainMenuScreen, (int)SCREEN_ID::GAME_SCREEN, (int)TEXTURE_ASSET_ID::CONTINUE_BUTTON, saveFileExists);
    createButton(vec2(window_width_px/2, 305), mainMenuScreen, (int)SCREEN_ID::GAME_SCREEN, (int)TEXTURE_ASSET_ID::PLAY_BUTTON, !saveFileExists);
    createButton(vec2(window_width_px/2, 460), mainMenuScreen, (int)SCREEN_ID::TUTORIAL_SCREEN, (int)TEXTURE_ASSET_ID::TUTORIAL_BUTTON, true);
    createButton(vec2(window_width_px/2, 610), mainMenuScreen, (int)SCREEN_ID::EXIT_SCREEN, (int) TEXTURE_ASSET_ID::EXIT_BUTTON, true);

    createHoverEffect();
    
	return true;
}

Entity RenderSystem::createButton(vec2 position, int screenTiedTo, int screenGoTo, int textureID, bool isActive) {
    auto entity = Entity();

    Mesh &mesh = getMesh(GEOMETRY_BUFFER_ID::UI_COMPONENT);
    registry.meshPtrs.emplace(entity, &mesh);

    Motion &motion = registry.motions.emplace(entity);
    motion.position = position;
    motion.scale = vec2(MENU_BUTTON_WIDTH, MENU_BUTTON_HEIGHT);
    motion.angle = 0;

    Clickable& c = registry.clickables.emplace(entity);
    c.screenTiedTo = screenTiedTo;
    c.screenGoTo = screenGoTo;
    c.textureID = textureID;
    c.isCurrentlyHoveredOver = false;
    c.isActive = isActive;

    return entity;
}

bool RenderSystem::initTutorial() {
	glGenTextures(1, &tutorialTexture);
	
	stbi_uc* data;
	stbi_set_flip_vertically_on_load(true);
	ivec2 dimensions;
	data = stbi_load(tutorialImgPath.c_str(), &dimensions.x, &dimensions.y, NULL, 4);

	if (data == NULL)
	{
		const std::string message = "Could not load the file " + tutorialImgPath + ".";
		fprintf(stderr, "%s", message.c_str());
		assert(false);
	}
	glBindTexture(GL_TEXTURE_2D, tutorialTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, dimensions.x, dimensions.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	gl_has_errors();
	stbi_image_free(data);

	return true;
}

Entity RenderSystem::createHoverEffect() {
    auto entity = Entity();

    Mesh &mesh = getMesh(GEOMETRY_BUFFER_ID::UI_COMPONENT);
    registry.meshPtrs.emplace(entity, &mesh);

    Motion &motion = registry.motions.emplace(entity);
    motion.position = {0, 0};
    motion.scale = vec2(MENU_BUTTON_WIDTH, MENU_BUTTON_HEIGHT);
    motion.angle = 0;

    hoverEntity = entity;

    return entity;
}

bool RenderSystem::initPauseMenu() {
	glGenTextures(1, &pauseMenuTexture);
	
	stbi_uc* data;
	stbi_set_flip_vertically_on_load(true);
	ivec2 dimensions;
	data = stbi_load(pauseMenuImgPath.c_str(), &dimensions.x, &dimensions.y, NULL, 4);

	if (data == NULL)
	{
		const std::string message = "Could not load the file " + pauseMenuImgPath + ".";
		fprintf(stderr, "%s", message.c_str());
		assert(false);
	}
	glBindTexture(GL_TEXTURE_2D, pauseMenuTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, dimensions.x, dimensions.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	gl_has_errors();
	stbi_image_free(data);
    //
    // Make menu buttons
    int pauseScreen = (int) SCREEN_ID::PAUSE_SCREEN;
    createButton(vec2(window_width_px/2, 305), pauseScreen, (int)SCREEN_ID::GAME_SCREEN, (int)TEXTURE_ASSET_ID::RESUME_BUTTON, false);
    createButton(vec2(window_width_px/2, 460), pauseScreen, (int)SCREEN_ID::MAIN_MENU, (int)TEXTURE_ASSET_ID::TITLE_BUTTON, false);
    createButton(vec2(window_width_px/2, 610), pauseScreen, (int)SCREEN_ID::EXIT_SCREEN, (int) TEXTURE_ASSET_ID::SAVE_QUIT_BUTTON, false);

	return true;

}

bool RenderSystem::initDeathScreen() {
	glGenTextures(1, &deathScreenTexture);
	
	stbi_uc* data;
	stbi_set_flip_vertically_on_load(true);
	ivec2 dimensions;
	data = stbi_load(deathScreenImgPath.c_str(), &dimensions.x, &dimensions.y, NULL, 4);

	if (data == NULL)
	{
		const std::string message = "Could not load the file " + deathScreenImgPath + ".";
		fprintf(stderr, "%s", message.c_str());
		assert(false);
	}
	glBindTexture(GL_TEXTURE_2D, deathScreenTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, dimensions.x, dimensions.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	gl_has_errors();
	stbi_image_free(data);
    //
    // Make menu buttons
    int deathScreen = (int) SCREEN_ID::DEATH_SCREEN;
    createButton(vec2(window_width_px/2, 305), deathScreen, (int)SCREEN_ID::GAME_SCREEN, (int)TEXTURE_ASSET_ID::PLAY_AGAIN_BUTTON, false);
    createButton(vec2(window_width_px/2, 460), deathScreen, (int)SCREEN_ID::MAIN_MENU, (int)TEXTURE_ASSET_ID::TITLE_BUTTON, false);
    createButton(vec2(window_width_px/2, 610), deathScreen, (int)SCREEN_ID::EXIT_SCREEN, (int) TEXTURE_ASSET_ID::EXIT_BUTTON, false);

	return true;

}

bool RenderSystem::initWinScreen() {
	glGenTextures(1, &winScreenTexture);
	
	stbi_uc* data;
	stbi_set_flip_vertically_on_load(true);
	ivec2 dimensions;
	data = stbi_load(winScreenImgPath.c_str(), &dimensions.x, &dimensions.y, NULL, 4);

	if (data == NULL)
	{
		const std::string message = "Could not load the file " + winScreenImgPath + ".";
		fprintf(stderr, "%s", message.c_str());
		assert(false);
	}
	glBindTexture(GL_TEXTURE_2D, winScreenTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, dimensions.x, dimensions.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	gl_has_errors();
	stbi_image_free(data);
    //
    // Make menu buttons
    int winScreen = (int) SCREEN_ID::WIN_SCREEN;
    createButton(vec2(window_width_px/2, 305), winScreen, (int)SCREEN_ID::GAME_SCREEN, (int)TEXTURE_ASSET_ID::PLAY_AGAIN_BUTTON, false);
    createButton(vec2(window_width_px/2, 460), winScreen, (int)SCREEN_ID::MAIN_MENU, (int)TEXTURE_ASSET_ID::TITLE_BUTTON, false);
    createButton(vec2(window_width_px/2, 610), winScreen, (int)SCREEN_ID::EXIT_SCREEN, (int) TEXTURE_ASSET_ID::EXIT_BUTTON, false);
	return true;

}

bool RenderSystem::initGameBackground() {
	glGenTextures(1, &gameBackgroundTexture);
	
	stbi_uc* data;
	stbi_set_flip_vertically_on_load(true);
	ivec2 dimensions;
	data = stbi_load(gameBackgroundImgPath.c_str(), &dimensions.x, &dimensions.y, NULL, 4);

	if (data == NULL)
	{
		const std::string message = "Could not load the file " + gameBackgroundImgPath + ".";
		fprintf(stderr, "%s", message.c_str());
		assert(false);
	}
	glBindTexture(GL_TEXTURE_2D, gameBackgroundTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, dimensions.x, dimensions.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	gl_has_errors();
	stbi_image_free(data);

	return true;
}

bool RenderSystem::initFloor() {
	glGenTextures(1, &floorTexture);
	
	stbi_uc* data;
	stbi_set_flip_vertically_on_load(true);
	ivec2 dimensions;
	data = stbi_load(floorImgPath.c_str(), &dimensions.x, &dimensions.y, NULL, 4);

	if (data == NULL)
	{
		const std::string message = "Could not load the file " + floorImgPath + ".";
		fprintf(stderr, "%s", message.c_str());
		assert(false);
	}
	glBindTexture(GL_TEXTURE_2D, floorTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, dimensions.x, dimensions.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	gl_has_errors();
	stbi_image_free(data);

	return true;
}

bool RenderSystem::initSpaceship() {
	glGenTextures(1, &spaceshipTexture);
	
	stbi_uc* data;
	stbi_set_flip_vertically_on_load(true);
	ivec2 dimensions;
	data = stbi_load(spaceshipImgPath.c_str(), &dimensions.x, &dimensions.y, NULL, 4);

	if (data == NULL)
	{
		const std::string message = "Could not load the file " + spaceshipImgPath + ".";
		fprintf(stderr, "%s", message.c_str());
		assert(false);
	}
	glBindTexture(GL_TEXTURE_2D, spaceshipTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, dimensions.x, dimensions.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	gl_has_errors();
	stbi_image_free(data);

	return true;
}
