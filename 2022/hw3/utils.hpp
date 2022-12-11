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

struct vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texcoords;
};

typedef std::array<glm::vec3, 8> bounding_box;

std::pair<std::vector<vertex>, std::vector<std::uint32_t>> generate_sphere(float radius, int quality);
std::pair<std::vector<vertex>, std::vector<std::uint32_t>> generate_floor(float radius, int quality, float angle);
GLuint create_shader(GLenum type, const std::string &file_path);
GLuint create_program(GLuint vertex_shader, GLuint fragment_shader);
SDL_Window *create_window(const std::string &window_title);
SDL_GLContext create_context(SDL_Window *window);
std::vector<vertex> get_vertices(
        const tinyobj::attrib_t &attrib,
        const std::vector<tinyobj::shape_t> &shapes);
bounding_box get_bounding_box(const std::vector<vertex> &scene);

#endif //HW2_UTILS_HPP
