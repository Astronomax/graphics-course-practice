#define TINYOBJLOADER_IMPLEMENTATION

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
#include <chrono>
#include <vector>
#include "tiny_obj_loader.h"
#include "obj_parser.hpp"
#include "stb_image.h"

#define GLM_FORCE_SWIZZLE
#define GLM_ENABLE_EXPERIMENTAL

#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/scalar_constants.hpp>
#include <glm/gtx/string_cast.hpp>
#include <set>

#include "shaders.hpp"
#include "utils.hpp"

struct vertex {
    std::array<float, 3> position;
    std::array<float, 3> normal;
    std::array<float, 2> texcoord;
};

int main() try {
    auto *window = create_window("Homework 2");
    auto gl_context = create_context(window);
    int width, height;
    SDL_GetWindowSize(window, &width, &height);
    glClearColor(0.8f, 0.8f, 1.f, 0.f);

    auto shadow_vertex_shader = create_shader(GL_VERTEX_SHADER, shadow_vertex_shader_source);
    auto shadow_fragment_shader = create_shader(GL_FRAGMENT_SHADER, shadow_fragment_shader_source);
    auto shadow_program = create_program(shadow_vertex_shader, shadow_fragment_shader);

    GLint shadow_model_location = glGetUniformLocation(shadow_program, "model");
    GLint shadow_transform_location = glGetUniformLocation(shadow_program, "transform");

    auto vertex_shader = create_shader(GL_VERTEX_SHADER, vertex_shader_source);
    auto fragment_shader = create_shader(GL_FRAGMENT_SHADER, fragment_shader_source);
    auto program = create_program(vertex_shader, fragment_shader);

    GLuint model_location = glGetUniformLocation(program, "model");
    GLuint view_location = glGetUniformLocation(program, "view");
    GLuint projection_location = glGetUniformLocation(program, "projection");
    GLuint camera_position_location = glGetUniformLocation(program, "camera_position");
    GLuint albedo_location = glGetUniformLocation(program, "albedo");
    GLuint sun_direction_location = glGetUniformLocation(program, "sun_direction");
    GLuint sun_color_location = glGetUniformLocation(program, "sun_color");
    GLuint texture_location = glGetUniformLocation(program, "tex");
    GLint shadow_projection_location_ = glGetUniformLocation(program, "shadow_projection");
    GLint shadow_map_location = glGetUniformLocation(program, "shadow_map");


    std::string project_root = PROJECT_ROOT;
    std::string scene_path = project_root + "/sponza/sponza.obj";
    std::string materials_dir = project_root + "/sponza/";

    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    tinyobj::LoadObj(&attrib, &shapes, &materials, nullptr, scene_path.c_str(), materials_dir.c_str());

    std::map<std::string, std::pair<GLuint, GLint>> textures;
    for(auto &material : materials) {
        GLint unit = (GLint)textures.size() + 1;
        if(material.ambient_texname.empty())
            continue;
        if(textures.find(material.ambient_texname) != textures.end())
            continue;
        glGenTextures(1, &textures[material.ambient_texname].first);
        textures[material.ambient_texname].second = unit;
        glActiveTexture(GL_TEXTURE0 + unit);
        glBindTexture(GL_TEXTURE_2D, textures[material.ambient_texname].first);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        int x, y, channels_in_file;
        std::string texture_path = materials_dir + material.ambient_texname;
        std::replace(texture_path.begin(), texture_path.end(), '\\', '/');
        unsigned char *pixels_ = stbi_load(texture_path.c_str(),
                                           &x, &y, &channels_in_file, 4);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, x, y,
                     0, GL_RGBA, GL_UNSIGNED_BYTE, pixels_);
        glGenerateMipmap(GL_TEXTURE_2D);
        stbi_image_free(pixels_);
    }

    std::vector<vertex> vertices;
    for(auto &shape : shapes) {
        for (auto &i: shape.mesh.indices) {
            vertices.push_back({{
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

    GLuint vao, vbo;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(vertex), vertices.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), (void*)(0));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), (void*)(12));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(vertex), (void*) 24);

    auto last_frame_start = std::chrono::high_resolution_clock::now();
    float time = 0.f;
    std::map<SDL_Keycode, bool> button_down;
    float camera_distance = 1000.f;
    float camera_angle = glm::pi<float>() / 2.f;

    bool running = true;
    while (true) {
        for (SDL_Event event; SDL_PollEvent(&event);)
            switch (event.type)
            {
                case SDL_QUIT:
                    running = false;
                    break;
                case SDL_WINDOWEVENT:
                    switch (event.window.event)
                    {
                        case SDL_WINDOWEVENT_RESIZED:
                            width = event.window.data1;
                            height = event.window.data2;
                            break;
                    }
                    break;
                case SDL_KEYDOWN:
                    button_down[event.key.keysym.sym] = true;
                    break;
                case SDL_KEYUP:
                    button_down[event.key.keysym.sym] = false;
                    break;
            }

        if (!running) break;

        auto now = std::chrono::high_resolution_clock::now();
        float dt = std::chrono::duration_cast<std::chrono::duration<float>>(now - last_frame_start).count();
        last_frame_start = now;
        time += dt;

        if (button_down[SDLK_UP])
            camera_distance -= 600.f * dt;
        if (button_down[SDLK_DOWN])
            camera_distance += 600.f * dt;

        if (button_down[SDLK_LEFT])
            camera_angle += 2.f * dt;
        if (button_down[SDLK_RIGHT])
            camera_angle -= 2.f * dt;

        glViewport(0, 0, width, height);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glClearColor(0.8f, 0.8f, 1.f, 0.f);

        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);

        float near = 0.1f;
        float far = 3000.f;

        glm::mat4 model(1.f);

        glm::mat4 view(1.f);
        view = glm::translate(view, {0.f, 0.f, -camera_distance});
        view = glm::rotate(view, glm::pi<float>() / 6.f, {1.f, 0.f, 0.f});
        view = glm::rotate(view, camera_angle, {0.f, 1.f, 0.f});
        view = glm::translate(view, {0.f, -0.5f, 0.f});

        float aspect = (float)height / (float)width;
        glm::mat4 projection = glm::perspective(glm::pi<float>() / 3.f, 1.f / aspect, near, far);
        glm::vec3 camera_position = (glm::inverse(view) * glm::vec4(0.f, 0.f, 0.f, 1.f)).xyz();
        glm::vec3 sun_direction = glm::normalize(glm::vec3(std::sin(time * 0.5f), 2.f, std::cos(time * 0.5f)));

        glUseProgram(program);
        glUniformMatrix4fv(model_location, 1, GL_FALSE, reinterpret_cast<float *>(&model));
        glUniformMatrix4fv(view_location, 1, GL_FALSE, reinterpret_cast<float *>(&view));
        glUniformMatrix4fv(projection_location, 1, GL_FALSE, reinterpret_cast<float *>(&projection));
        glUniform3fv(camera_position_location, 1, (float *)(&camera_position));
        glUniform3f(albedo_location, .8f, .7f, .6f);
        glUniform3f(sun_color_location, 1.f, 1.f, 1.f);
        glUniform3fv(sun_direction_location, 1, reinterpret_cast<float *>(&sun_direction));
        glBindVertexArray(vao);

        GLint i = 0;
        for(auto &shape : shapes) {
            auto material = materials[shape.mesh.material_ids[0]];
            if(!material.ambient_texname.empty())
                assert(textures.find(material.ambient_texname) != textures.end());
            glUniform1i(texture_location, textures[material.ambient_texname].second);
            glDrawArrays(GL_TRIANGLES, i, (GLint)shape.mesh.indices.size());
            i += (GLint)shape.mesh.indices.size();
        }

        SDL_GL_SwapWindow(window);
    }
    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
}
catch (std::exception const &e) {
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
}
