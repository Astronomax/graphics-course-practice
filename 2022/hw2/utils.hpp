#ifndef HW2_UTILS_HPP
#define HW2_UTILS_HPP

#ifdef WIN32
#include <SDL.h>
#undef main
#else
#include <SDL2/SDL.h>
#endif
#include <GL/glew.h>
#include <string>


GLuint create_shader(GLenum type, const char *source);
GLuint create_program(GLuint vertex_shader, GLuint fragment_shader);
SDL_Window *create_window(const std::string &window_title);
SDL_GLContext create_context(SDL_Window *window);

#endif //HW2_UTILS_HPP
