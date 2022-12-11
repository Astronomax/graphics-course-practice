#include <iostream>
#include "texture_holder.hpp"

GLint texture_holder::load_texture(const std::string &path) {
    GLint unit = m_first_unit + (GLint)m_textures.size();
    auto it = m_textures.find(path);
    if(it != m_textures.end()) return it->second.second;
    glGenTextures(1, &m_textures[path].first);
    m_textures[path].second = unit;
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D, m_textures[path].first);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    int x, y, channels_in_file;
    auto pixels = stbi_load(path.c_str(), &x, &y, &channels_in_file, 4);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, x, y, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    glGenerateMipmap(GL_TEXTURE_2D);
    stbi_image_free(pixels);
    return unit;
}

GLint texture_holder::get_texture(const std::string &path) {
    auto it = m_textures.find(path);
    if(it == m_textures.end())
        return load_texture(path);
    else return it->second.second;
}

texture_holder::texture_holder(GLint first_unit) : m_first_unit(first_unit) {}
