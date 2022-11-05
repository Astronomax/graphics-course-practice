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

struct vertex {
    std::array<float, 3> position;
    std::array<float, 3> normal;
    std::array<float, 2> texcoord;
};

typedef std::array<std::array<float, 3>, 8> bounding_box;

GLuint create_shader(GLenum type, const char *source);
GLuint create_program(GLuint vertex_shader, GLuint fragment_shader);
SDL_Window *create_window(const std::string &window_title);
SDL_GLContext create_context(SDL_Window *window);
std::vector<vertex> get_vertices(
        const tinyobj::attrib_t &attrib,
        const std::vector<tinyobj::shape_t> &shapes);
bounding_box get_bounding_box(const std::vector<vertex> &scene);

#endif //HW2_UTILS_HPP
