#define TINYOBJLOADER_IMPLEMENTATION
//#include "tiny_obj_loader.h"

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
#include <set>

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
#include <numeric>

#include "shaders.hpp"
#include "utils.hpp"
#include "texture_holder.hpp"

int main() try {
    auto *window = create_window("Homework 2");
    auto gl_context = create_context(window);
    int width, height;
    SDL_GetWindowSize(window, &width, &height);

    auto shadow_vertex_shader = create_shader(GL_VERTEX_SHADER, shadow_vertex_shader_source);
    auto shadow_fragment_shader = create_shader(GL_FRAGMENT_SHADER, shadow_fragment_shader_source);
    auto shadow_program = create_program(shadow_vertex_shader, shadow_fragment_shader);

    GLint shadow_model_location = glGetUniformLocation(shadow_program, "model");
    GLint shadow_transform_location = glGetUniformLocation(shadow_program, "transform");

    auto vertex_shader = create_shader(GL_VERTEX_SHADER, vertex_shader_source);
    auto fragment_shader = create_shader(GL_FRAGMENT_SHADER, fragment_shader_source);
    auto program = create_program(vertex_shader, fragment_shader);

    GLint model_location = glGetUniformLocation(program, "model");
    GLint view_location = glGetUniformLocation(program, "view");
    GLint projection_location = glGetUniformLocation(program, "projection");
    GLint texture_location = glGetUniformLocation(program, "tex");
    GLint shadow_map_location = glGetUniformLocation(program, "shadow_map");
    GLint transform_location = glGetUniformLocation(program, "transform");
    GLint ambient_location = glGetUniformLocation(program, "ambient");
    GLint light_direction_location = glGetUniformLocation(program, "light_direction");
    GLint light_color_location = glGetUniformLocation(program, "light_color");
    GLint glossiness_location = glGetUniformLocation(program, "glossiness");
    GLint power_location = glGetUniformLocation(program, "power");
    GLint camera_position_location = glGetUniformLocation(program, "camera_position");

    std::string project_root = PROJECT_ROOT;
    std::string scene_path = project_root + "/sponza/sponza.obj";
    std::string materials_dir = project_root + "/sponza/";

    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    tinyobj::LoadObj(&attrib, &shapes, &materials, nullptr, scene_path.c_str(), materials_dir.c_str());

    texture_holder textures(2);
    for(auto &material : materials) {
        std::string texture_path = materials_dir + material.ambient_texname;
        std::replace(texture_path.begin(), texture_path.end(), '\\', '/');
        textures.load_texture(texture_path);
    }
    auto vertices = get_vertices(attrib, shapes);
    auto bounding_box = get_bounding_box(vertices);
    glm::vec3 c = std::accumulate(bounding_box.begin(), bounding_box.end(), glm::vec3(0.f)) / 8.f;

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

    GLsizei shadow_map_resolution = 1024;
    GLuint shadow_map, render_buffer, shadow_fbo;
    glGenTextures(1, &shadow_map);
    glGenRenderbuffers(1, &render_buffer);
    glGenFramebuffers(1, &shadow_fbo);
    glActiveTexture(GL_TEXTURE0 + 1);
    glBindTexture(GL_TEXTURE_2D, shadow_map);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RG32F, shadow_map_resolution,
                 shadow_map_resolution, 0, GL_RGBA, GL_FLOAT, nullptr);
    glBindRenderbuffer(GL_RENDERBUFFER, render_buffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24,
                          shadow_map_resolution, shadow_map_resolution);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, shadow_fbo);
    glFramebufferTexture(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, shadow_map, 0);
    glFramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, render_buffer);
    if (glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        throw std::runtime_error("Incomplete framebuffer!");

    auto last_frame_start = std::chrono::high_resolution_clock::now();
    float time = 0.f;
    std::map<SDL_Keycode, bool> button_down;
    float camera_distance = 1000.f;
    float camera_angle = glm::pi<float>() / 2.f;
    float view_elevation = glm::pi<float>() / 4.f;

    glm::vec3 light_direction = glm::normalize(glm::vec3(0.1f, 1.f, 0.1f));
    glm::vec3 light_z = -light_direction;
    glm::vec3 light_x = glm::normalize(glm::cross(light_z, {0.f, 1.f, 0.f}));
    glm::vec3 light_y = glm::cross(light_x, light_z);
    float dx = -std::numeric_limits<float>::infinity();
    float dy = -std::numeric_limits<float>::infinity();
    float dz = -std::numeric_limits<float>::infinity();
    for(auto _v : bounding_box) {
        glm::vec3 v = _v - c;
        dx = std::max(dx, glm::dot(v, light_x));
        dy = std::max(dy, glm::dot(v, light_y));
        dz = std::max(dz, glm::dot(v, light_z));
    }
    glm::mat4 transform = glm::inverse(glm::mat4({
        {dx * light_x.x, dx * light_x.y, dx * light_x.z, 0.f},
        {dy * light_y.x, dy * light_y.y, dy * light_y.z, 0.f},
        {dz * light_z.x, dz * light_z.y, dz * light_z.z, 0.f},
        {c.x, c.y, c.z, 1.f}
    }));

    bool running = true;
    while (true) {
        for (SDL_Event event; SDL_PollEvent(&event);)
            switch (event.type) {
                case SDL_QUIT:
                    running = false;
                    break;
                case SDL_WINDOWEVENT:
                    switch (event.window.event) {
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
        if (button_down[SDLK_a])
            camera_angle += 2.f * dt;
        if (button_down[SDLK_d])
            camera_angle -= 2.f * dt;
        if (button_down[SDLK_w])
            view_elevation += 2.f * dt;
        if (button_down[SDLK_s])
            view_elevation -= 2.f * dt;

        glm::mat4 model(1.f);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, shadow_fbo);
        glClearColor(1.f, 1.f, 0.f, 0.f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glViewport(0, 0, shadow_map_resolution, shadow_map_resolution);
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LEQUAL);
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
        glUseProgram(shadow_program);
        glUniformMatrix4fv(shadow_model_location, 1, GL_FALSE, reinterpret_cast<float *>(&model));
        glUniformMatrix4fv(shadow_transform_location, 1, GL_FALSE, reinterpret_cast<float *>(&transform));
        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLES, 0, (GLint)vertices.size());

        glBindTexture(GL_TEXTURE_2D, shadow_map);
        glGenerateMipmap(GL_TEXTURE_2D);

        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
        glViewport(0, 0, width, height);
        glClearColor(0.8f, 0.8f, 0.9f, 0.f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LEQUAL);
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);

        float near = 0.1f, far = 3000.f;

        glm::mat4 view(1.f);
        view = glm::translate(view, {0.f, 0.f, -camera_distance});
        view = glm::rotate(view, view_elevation, {1.f, 0.f, 0.f});
        view = glm::rotate(view, camera_angle, {0.f, 1.f, 0.f});
        view = glm::translate(view, {0.f, -0.5f, 0.f});

        glm::vec3 camera_position = (glm::inverse(view) * glm::vec4(0.f, 0.f, 0.f, 1.f)).xyz();

        glm::mat4 projection = glm::mat4(1.f);
        projection = glm::perspective(glm::pi<float>() / 2.f, (float)width / (float)height, near, far);

        glUseProgram(program);
        glUniformMatrix4fv(model_location, 1, GL_FALSE, reinterpret_cast<float *>(&model));
        glUniformMatrix4fv(view_location, 1, GL_FALSE, reinterpret_cast<float *>(&view));
        glUniformMatrix4fv(projection_location, 1, GL_FALSE, reinterpret_cast<float *>(&projection));
        glUniformMatrix4fv(transform_location, 1, GL_FALSE, reinterpret_cast<float *>(&transform));
        glUniform3f(ambient_location, 0.2f, 0.2f, 0.2f);
        glUniform3fv(light_direction_location, 1, reinterpret_cast<float *>(&light_direction));
        glUniform3f(light_color_location, 0.8f, 0.8f, 0.8f);
        glUniform3fv(camera_position_location, 1, reinterpret_cast<float *>(&camera_position));
        glUniform1i(shadow_map_location, 1);

        GLint current_block = 0;
        for(auto &shape : shapes) {
            auto material = materials[shape.mesh.material_ids[0]];
            std::string texture_path = materials_dir + material.ambient_texname;
            std::replace(texture_path.begin(), texture_path.end(), '\\', '/');
            textures.load_texture(texture_path);
            glUniform1f(power_location, material.shininess);
            glUniform1f(glossiness_location, material.specular[0]);
            glUniform1i(texture_location, textures.get_texture(texture_path));
            glDrawArrays(GL_TRIANGLES, current_block, (GLint)shape.mesh.indices.size());
            current_block += (GLint)shape.mesh.indices.size();
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
