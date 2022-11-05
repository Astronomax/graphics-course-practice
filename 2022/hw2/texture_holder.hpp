#ifndef HW2_TEXTURE_HOLDER_HPP
#define HW2_TEXTURE_HOLDER_HPP
#include <unordered_map>
#include <string>
#ifdef WIN32
#include <SDL.h>
#undef main
#else
#include <SDL2/SDL.h>
#endif

#include <GL/glew.h>
#include "stb_image.h"

class texture_holder {
public:
    explicit texture_holder(GLint first_unit);
    GLint load_texture(const std::string &path);
    GLint get_texture(const std::string &path);

private:
    GLint m_first_unit;
    std::unordered_map<std::string, std::pair<GLuint, GLint>> m_textures;
};


#endif //HW2_TEXTURE_HOLDER_HPP
