#ifdef WIN32
#include <SDL.h>
#undef main
#else
#include <SDL2/SDL.h>
#endif

#include <GL/glew.h>

#include <string_view>
#include <stdexcept>
#include <iostream>
#include <fstream>
#include <chrono>
#include <vector>
#include <random>
#include <map>
#include <cmath>

#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/scalar_constants.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/string_cast.hpp>

#include "msdf_loader.hpp"
#include "stb_image.h"

std::string to_string(std::string_view str)
{
    return std::string(str.begin(), str.end());
}

void sdl2_fail(std::string_view message)
{
    throw std::runtime_error(to_string(message) + SDL_GetError());
}

void glew_fail(std::string_view message, GLenum error)
{
    throw std::runtime_error(to_string(message) + reinterpret_cast<const char *>(glewGetErrorString(error)));
}

const char msdf_vertex_shader_source[] =
R"(#version 330 core

uniform mat4 transform;

layout (location = 0) in vec2 in_position;
layout (location = 1) in vec2 in_texcoord;

out vec2 texcoord;

void main() {
    //position = in_position;
    gl_Position = transform * vec4(in_position, 0.0, 1.0);
    texcoord = in_texcoord;
}
)";

const char msdf_fragment_shader_source[] =
R"(#version 330 core
uniform float sdf_scale;
uniform sampler2D sdf_texture;

layout (location = 0) out vec4 out_color;

in vec2 texcoord;

float median(vec3 v) {
    return max(min(v.r, v.g), min(max(v.r, v.g), v.b));
}

void main() {
    float sdf_texture_value = median(texture(sdf_texture, texcoord).rgb);
    float sdf_value = sdf_scale * (sdf_texture_value - 0.5);
    float value = length(vec2(dFdx(sdf_value), dFdy(sdf_value))) / sqrt(2.0);
    float alpha = smoothstep(-value, value, sdf_value);
    float stroke_sdf_value = sdf_value + 1.0;
    float stroke_value = length(vec2(dFdx(stroke_sdf_value), dFdy(stroke_sdf_value))) / sqrt(2.0);
    float stroke_alpha = smoothstep(-stroke_value, stroke_value, stroke_sdf_value);
    if(alpha < 0.1)
    out_color = vec4(1.0, 1.0, 1.0, stroke_alpha);
    else
    out_color = vec4(0.0, 0.0, 0.0, alpha);
}
)";

GLuint create_shader(GLenum type, const char * source)
{
    GLuint result = glCreateShader(type);
    glShaderSource(result, 1, &source, nullptr);
    glCompileShader(result);
    GLint status;
    glGetShaderiv(result, GL_COMPILE_STATUS, &status);
    if (status != GL_TRUE)
    {
        GLint info_log_length;
        glGetShaderiv(result, GL_INFO_LOG_LENGTH, &info_log_length);
        std::string info_log(info_log_length, '\0');
        glGetShaderInfoLog(result, info_log.size(), nullptr, info_log.data());
        throw std::runtime_error("Shader compilation failed: " + info_log);
    }
    return result;
}

template <typename ... Shaders>
GLuint create_program(Shaders ... shaders)
{
    GLuint result = glCreateProgram();
    (glAttachShader(result, shaders), ...);
    glLinkProgram(result);

    GLint status;
    glGetProgramiv(result, GL_LINK_STATUS, &status);
    if (status != GL_TRUE)
    {
        GLint info_log_length;
        glGetProgramiv(result, GL_INFO_LOG_LENGTH, &info_log_length);
        std::string info_log(info_log_length, '\0');
        glGetProgramInfoLog(result, info_log.size(), nullptr, info_log.data());
        throw std::runtime_error("Program linkage failed: " + info_log);
    }

    return result;
}

struct vertex {
    glm::vec2 position;
    glm::vec2 texcoord;
};

int main() try
{
    if (SDL_Init(SDL_INIT_VIDEO) != 0)
        sdl2_fail("SDL_Init: ");

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

    SDL_Window * window = SDL_CreateWindow("Graphics course practice 15",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        800, 600,
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_MAXIMIZED);

    if (!window)
        sdl2_fail("SDL_CreateWindow: ");

    int width, height;
    SDL_GetWindowSize(window, &width, &height);

    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    if (!gl_context)
        sdl2_fail("SDL_GL_CreateContext: ");

    if (auto result = glewInit(); result != GLEW_NO_ERROR)
        glew_fail("glewInit: ", result);

    if (!GLEW_VERSION_3_3)
        throw std::runtime_error("OpenGL 3.3 is not supported");

    auto msdf_vertex_shader = create_shader(GL_VERTEX_SHADER, msdf_vertex_shader_source);
    auto msdf_fragment_shader = create_shader(GL_FRAGMENT_SHADER, msdf_fragment_shader_source);
    auto msdf_program = create_program(msdf_vertex_shader, msdf_fragment_shader);

    GLuint transform_location = glGetUniformLocation(msdf_program, "transform");
    GLuint texture_location = glGetUniformLocation(msdf_program, "sdf_texture");
    GLuint scale_location = glGetUniformLocation(msdf_program, "sdf_scale");
    const std::string project_root = PROJECT_ROOT;
    const std::string font_path = project_root + "/font/font-msdf.json";

    auto const font = load_msdf_font(font_path);

    GLuint texture;
    int texture_width, texture_height;
    {
        int channels;
        auto data = stbi_load(font.texture_path.c_str(), &texture_width, &texture_height, &channels, 4);
        assert(data);

        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, texture_width, texture_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        stbi_image_free(data);
    }
    std::string text = "Hello, world!";

    GLuint vao, vbo;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glGenBuffers(1, &vbo);
    {
        vertex vertices[3];
        vertices[0] = {{0, 0}, {0, 0}};
        vertices[1] = {{100, 0}, {1, 0}};
        vertices[2] = {{0, 100}, {0, 1}};
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, 3 * sizeof(vertex), &vertices[0], GL_STATIC_DRAW);
    }
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(vertex), (void *)offsetof(vertex, position));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(vertex), (void *)offsetof(vertex, texcoord));

    auto last_frame_start = std::chrono::high_resolution_clock::now();

    float time = 0.f;

    SDL_StartTextInput();

    std::map<SDL_Keycode, bool> button_down;
    glm::mat4 transform;

    bool text_changed = true;

    bool running = true;
    while (running)
    {
        for (SDL_Event event; SDL_PollEvent(&event);) switch (event.type)
        {
        case SDL_QUIT:
            running = false;
            break;
        case SDL_WINDOWEVENT: switch (event.window.event)
            {
            case SDL_WINDOWEVENT_RESIZED:
                width = event.window.data1;
                height = event.window.data2;
                glViewport(0, 0, width, height);
                break;
            }
            break;
        case SDL_KEYDOWN:
            button_down[event.key.keysym.sym] = true;
            if (event.key.keysym.sym == SDLK_BACKSPACE && !text.empty())
            {
                text.pop_back();
                text_changed = true;
            }
            break;
        case SDL_TEXTINPUT:
            text.append(event.text.text);
            text_changed = true;
        case SDL_KEYUP:
            button_down[event.key.keysym.sym] = false;
            break;
        }

        if (!running)
            break;

        auto now = std::chrono::high_resolution_clock::now();
        float dt = std::chrono::duration_cast<std::chrono::duration<float>>(now - last_frame_start).count();
        last_frame_start = now;

        if(text_changed) {
            glm::vec2 pen(0.f);
            std::vector<vertex> vertices(6 * text.size());
            for(int i = 0; i < text.size(); i++) {
                auto g = font.glyphs.at(text[i]);
                glm::vec2 offset(g.xoffset, g.yoffset);
                vertices[6 * i + 0].position = pen + offset + glm::vec2(0, 0);
                vertices[6 * i + 1].position = pen + offset + glm::vec2(g.width, 0);
                vertices[6 * i + 2].position = pen + offset + glm::vec2(0, g.height);
                vertices[6 * i + 3].position = pen + offset + glm::vec2(g.width, 0);
                vertices[6 * i + 4].position = pen + offset + glm::vec2(0, g.height);
                vertices[6 * i + 5].position = pen + offset + glm::vec2(g.width, g.height);
                glm::vec2 texcorner(g.x, g.y);
                vertices[6 * i + 0].texcoord = (texcorner + glm::vec2(0, 0)) / glm::vec2(texture_width, texture_height);
                vertices[6 * i + 1].texcoord = (texcorner + glm::vec2(g.width, 0)) / glm::vec2(texture_width, texture_height);
                vertices[6 * i + 2].texcoord = (texcorner + glm::vec2(0, g.height)) / glm::vec2(texture_width, texture_height);
                vertices[6 * i + 3].texcoord = (texcorner + glm::vec2(g.width, 0)) / glm::vec2(texture_width, texture_height);
                vertices[6 * i + 4].texcoord = (texcorner + glm::vec2(0, g.height)) / glm::vec2(texture_width, texture_height);
                vertices[6 * i + 5].texcoord = (texcorner + glm::vec2(g.width, g.height)) / glm::vec2(texture_width, texture_height);
                pen.x += g.advance;
            }
            glBindBuffer(GL_ARRAY_BUFFER, vbo);
            glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(vertex), vertices.data(), GL_STATIC_DRAW);

            float xmin = std::numeric_limits<float>::max();
            float ymin = std::numeric_limits<float>::max();
            float xmax = std::numeric_limits<float>::min();
            float ymax = std::numeric_limits<float>::min();

            for(auto &v : vertices) {
                xmin = std::min(xmin, v.position.x);
                xmax = std::max(xmax, v.position.x);
                ymin = std::min(ymin, v.position.y);
                ymax = std::max(ymax, v.position.y);
            }

            float text_width = xmax - xmin;
            float text_height = ymax - ymin;

            transform = glm::mat4(1.f);
            transform = glm::scale(transform, glm::vec3(5.f, 5.f, 0.f));
            transform = glm::rotate(transform, glm::pi<float>(), glm::vec3(1.f, 0.f, 0.f));
            transform = glm::translate(transform, glm::vec3(-1.f, -1.f, 0.f));
            transform = glm::scale(transform, glm::vec3(2.f / (float)width, 2.f / (float)height, 0.f));
            transform = glm::translate(transform, glm::vec3((float)width / 2.f - text_width / 2.f, (float)height / 2.f - text_height / 2.f, 0.f));
            text_changed = false;
        }

        glClearColor(0.8f, 0.8f, 1.f, 0.f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        glUseProgram(msdf_program);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture);
        glUniform1i(texture_location, 0);
        glUniform1f(scale_location, font.sdf_scale);
        glUniformMatrix4fv(transform_location, 1, GL_FALSE, reinterpret_cast<float *>(&transform));
        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLES, 0, 6 * text.size());
        SDL_GL_SwapWindow(window);
    }

    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
}
catch (std::exception const & e)
{
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
}
