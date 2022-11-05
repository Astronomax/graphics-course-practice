#include "utils.hpp"
#include <stdexcept>

std::string to_string(std::string_view str) {
    return {str.begin(), str.end()};
}

void sdl2_fail(std::string_view message) {
    throw std::runtime_error(to_string(message) + SDL_GetError());
}

void glew_fail(std::string_view message, GLenum error) {
    throw std::runtime_error(to_string(message) + reinterpret_cast<const char *>(glewGetErrorString(error)));
}

GLuint create_shader(GLenum type, const char *source) {
    GLuint result = glCreateShader(type);
    glShaderSource(result, 1, &source, nullptr);
    glCompileShader(result);
    GLint status;
    glGetShaderiv(result, GL_COMPILE_STATUS, &status);
    if (status != GL_TRUE) {
        GLint info_log_length;
        glGetShaderiv(result, GL_INFO_LOG_LENGTH, &info_log_length);
        std::string info_log(info_log_length, '\0');
        glGetShaderInfoLog(result, info_log.size(), nullptr, info_log.data());
        throw std::runtime_error("Shader compilation failed: " + info_log);
    }
    return result;
}

GLuint create_program(GLuint vertex_shader, GLuint fragment_shader) {
    GLuint result = glCreateProgram();
    glAttachShader(result, vertex_shader);
    glAttachShader(result, fragment_shader);
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

SDL_Window *create_window(const std::string &window_title) {
    if (SDL_Init(SDL_INIT_VIDEO) != 0)
        sdl2_fail("SDL_Init: ");
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,
                        SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4);
    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_Window *window = SDL_CreateWindow(window_title.c_str(),
                                          SDL_WINDOWPOS_CENTERED,
                                          SDL_WINDOWPOS_CENTERED,
                                          800, 600,
                                          SDL_WINDOW_OPENGL |
                                          SDL_WINDOW_RESIZABLE |
                                          SDL_WINDOW_MAXIMIZED);
    if (!window) sdl2_fail("SDL_CreateWindow: ");
    return window;
}

SDL_GLContext create_context(SDL_Window *window) {
    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    if (!gl_context)
        sdl2_fail("SDL_GL_CreateContext: ");
    if (auto result = glewInit(); result != GLEW_NO_ERROR)
        glew_fail("glewInit: ", result);
    if (!GLEW_VERSION_3_3)
        throw std::runtime_error("OpenGL 3.3 is not supported");
    return gl_context;
}

std::vector<vertex> get_vertices(
        const tinyobj::attrib_t &attrib,
        const std::vector<tinyobj::shape_t> &shapes) {

    std::vector<vertex> res;
    for(auto &shape : shapes) {
        for (auto &i: shape.mesh.indices) {
            res.push_back({{
                                   attrib.vertices[3 * i.vertex_index],
                                   attrib.vertices[3 * i.vertex_index + 1],
                                   attrib.vertices[3 * i.vertex_index + 2]
                           }, {
                                   attrib.normals[3 * i.normal_index],
                                   attrib.normals[3 * i.normal_index + 1],
                                   attrib.normals[3 * i.normal_index + 2]
                           }, {
                                   attrib.texcoords[2 * i.texcoord_index],
                                   attrib.texcoords[2 * i.texcoord_index + 1]
                           }
                          });
        }
    }
    return res;
}

bounding_box get_bounding_box(const std::vector<vertex> &scene) {
    float x_bounds[2] = {std::numeric_limits<float>::infinity(),
                         -std::numeric_limits<float>::infinity()};
    float y_bounds[2] = {std::numeric_limits<float>::infinity(),
                         -std::numeric_limits<float>::infinity()};
    float z_bounds[2] = {std::numeric_limits<float>::infinity(),
                         -std::numeric_limits<float>::infinity()};
    for(auto &v : scene) {
        x_bounds[0] = std::min(x_bounds[0], v.position[0]);
        y_bounds[0] = std::min(y_bounds[0], v.position[1]);
        z_bounds[0] = std::min(z_bounds[0], v.position[2]);
        x_bounds[1] = std::max(x_bounds[1], v.position[0]);
        y_bounds[1] = std::max(y_bounds[1], v.position[1]);
        z_bounds[1] = std::max(z_bounds[1], v.position[2]);
    }
    bounding_box res;
    for(int i = 0; i < 2; i++)
        for(int j = 0; j < 2; j++)
            for(int k = 0; k < 2; k++)
                res[1 * i + 2 * j + 4 * k] =
                        { x_bounds[i], y_bounds[j], z_bounds[k] };
    return res;
}