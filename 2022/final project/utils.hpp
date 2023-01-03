#ifndef HW2_UTILS_HPP
#define HW2_UTILS_HPP

//#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

#ifdef WIN32
#include <SDL.h>
#undef main
#else
#include <SDL2/SDL.h>
#endif
#include <GL/glew.h>
#include <string>
#include <array>
#include <vector>

#define GLM_FORCE_SWIZZLE
#define GLM_ENABLE_EXPERIMENTAL

#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/scalar_constants.hpp>
#include <glm/gtx/string_cast.hpp>
#include <stdexcept>
#include "obj_parser.hpp"

struct vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texcoords;
};

struct particle
{
    glm::vec3 position;
    float size;
    float rotation;
    glm::vec3 velocity;
    float angular_velocity;
};

typedef std::array<glm::vec3, 8> bounding_box;

std::pair<std::vector<vertex>, std::vector<std::uint32_t>> generate_sphere(float radius, int quality);
std::pair<std::vector<vertex>, std::vector<std::uint32_t>> generate_floor(float radius, int quality, float angle);
GLuint create_shader(GLenum type, const std::string &file_path);

template <typename ... Shaders>
GLuint create_program(Shaders ... shaders)
{
    GLuint result = glCreateProgram();
    (glAttachShader(result, shaders), ...);
    glLinkProgram(result);
    GLint status;
    glGetProgramiv(result, GL_LINK_STATUS, &status);
    if (status != GL_TRUE) {
        GLint info_log_length;
        glGetProgramiv(result, GL_INFO_LOG_LENGTH, &info_log_length);
        std::string info_log(info_log_length, '\0');
        glGetProgramInfoLog(result, info_log.size(), nullptr, info_log.data());
        throw std::runtime_error("Program linkage failed: " + info_log);
    }
    return result;
}

SDL_Window *create_window(const std::string &window_title);
SDL_GLContext create_context(SDL_Window *window);
std::vector<vertex> get_vertices(
        const tinyobj::attrib_t &attrib,
        const std::vector<tinyobj::shape_t> &shapes);

bounding_box get_bounding_box(const std::vector<obj_data::vertex> &scene);
bounding_box get_bounding_box(const std::vector<vertex> &scene);

#endif //HW2_UTILS_HPP
