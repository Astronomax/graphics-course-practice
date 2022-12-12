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

#include "gltf_loader.hpp"
#include "texture_holder.hpp"
#include <fstream>
#include <random>
#include "utils.hpp"

int main() try {
    auto *window = create_window("Homework 3");
    auto gl_context = create_context(window);
    int width, height;
    SDL_GetWindowSize(window, &width, &height);

    std::string project_root = PROJECT_ROOT;

    glClearColor(0.8f, 0.8f, 1.f, 0.f);

    auto environment_vertex_shader = create_shader(GL_VERTEX_SHADER, project_root + "/shaders/environment.vert");
    auto environment_fragment_shader = create_shader(GL_FRAGMENT_SHADER, project_root + "/shaders/environment.frag");
    auto environment_program = create_program(environment_vertex_shader, environment_fragment_shader);

    GLuint view_projection_inverse_location = glGetUniformLocation(environment_program, "view_projection_inverse");
    GLuint _camera_position_location = glGetUniformLocation(environment_program, "camera_position");
    GLuint _environment_map_texture_location = glGetUniformLocation(environment_program, "environment_map_texture");
    GLuint _ambient_location = glGetUniformLocation(environment_program, "ambient");

    auto shadow_vertex_shader = create_shader(GL_VERTEX_SHADER, project_root + "/shaders/shadow.vert");
    auto shadow_fragment_shader = create_shader(GL_FRAGMENT_SHADER, project_root + "/shaders/shadow.frag");
    auto shadow_program = create_program(shadow_vertex_shader, shadow_fragment_shader);

    GLint shadow_model_location = glGetUniformLocation(shadow_program, "model");
    GLint shadow_transform_location = glGetUniformLocation(shadow_program, "transform");
    GLint alpha_texture_location = glGetUniformLocation(shadow_program, "alpha_texture");
    GLint have_alpha_location = glGetUniformLocation(shadow_program, "have_alpha");
    GLint use_bones_location = glGetUniformLocation(shadow_program, "use_bones");
    GLuint _bones_location = glGetUniformLocation(shadow_program, "bones");

    auto christmas_tree_vertex_shader = create_shader(GL_VERTEX_SHADER, project_root + "/shaders/christmas_tree.vert");
    auto christmas_tree_fragment_shader = create_shader(GL_FRAGMENT_SHADER, project_root + "/shaders/christmas_tree.frag");
    auto christmas_tree_program = create_program(christmas_tree_vertex_shader, christmas_tree_fragment_shader);

    GLint _model_location = glGetUniformLocation(christmas_tree_program, "model");
    GLint _view_location = glGetUniformLocation(christmas_tree_program, "view");
    GLint _projection_location = glGetUniformLocation(christmas_tree_program, "projection");
    GLint ambient_location = glGetUniformLocation(christmas_tree_program, "ambient");
    GLint _albedo_location = glGetUniformLocation(christmas_tree_program, "albedo_color");
    GLint texture_location = glGetUniformLocation(christmas_tree_program, "ambient_texture");
    GLint shadow_map_location = glGetUniformLocation(christmas_tree_program, "shadow_map");
    GLint transform_location = glGetUniformLocation(christmas_tree_program, "transform");
    GLint _light_direction_location = glGetUniformLocation(christmas_tree_program, "light_direction");
    GLint light_color_location = glGetUniformLocation(christmas_tree_program, "light_color");
    GLint glossiness_location = glGetUniformLocation(christmas_tree_program, "glossiness");
    GLint power_location = glGetUniformLocation(christmas_tree_program, "power");
    GLint __camera_position_location = glGetUniformLocation(christmas_tree_program, "camera_position");
    GLint _alpha_texture_location = glGetUniformLocation(christmas_tree_program, "alpha_texture");
    GLint _have_alpha_location = glGetUniformLocation(christmas_tree_program, "have_alpha");
    GLint have_ambient_texture_location = glGetUniformLocation(christmas_tree_program, "have_ambient_texture");

    std::string christmas_tree_dir = project_root + "/christmas_tree/";
    std::string christmas_tree_path = christmas_tree_dir + "12150_Christmas_Tree_V2_L2.obj";
    std::string environment_path = project_root + "/textures/winter_in_forest.jpg";
    std::string wolf_dir = project_root + "/wolf/";
    std::string particle_texture_path = project_root + "/particles/particle.png";
    std::string cloud_texture_path = project_root + "/cloud.data";

    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    tinyobj::LoadObj(&attrib, &shapes, &materials, nullptr, christmas_tree_path.c_str(), christmas_tree_dir.c_str());

    texture_holder textures(3);
    textures.load_texture(environment_path);
    textures.load_texture(particle_texture_path);
    textures.load_texture(cloud_texture_path);

    for(auto &material : materials) {
        std::string texture_path = christmas_tree_dir + material.ambient_texname;
        std::replace(texture_path.begin(), texture_path.end(), '\\', '/');
        textures.load_texture(texture_path);
    }

    glm::mat4 christmas_tree_model = glm::mat4(1.f);
    christmas_tree_model = glm::rotate(christmas_tree_model, -0.5f * glm::pi<float>(), {1.f, 0.f, 0.f});
    christmas_tree_model = glm::scale(christmas_tree_model, glm::vec3(0.007f));
    christmas_tree_model = glm::translate(christmas_tree_model, glm::vec3(0.f, 0.f, -44.f));

    auto christmas_vertices = get_vertices(attrib, shapes);
    bounding_box bounding_box;
    glm::vec3 c;

    GLuint vao, vbo;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, christmas_vertices.size() * sizeof(vertex), christmas_vertices.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), (void *)offsetof(vertex, position));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), (void *)offsetof(vertex, normal));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), (void *)offsetof(vertex, texcoords));

    GLsizei shadow_map_resolution = 1024;
    GLuint shadow_map, render_buffer, shadow_fbo;
    glGenTextures(1, &shadow_map);
    glGenRenderbuffers(1, &render_buffer);
    glGenFramebuffers(1, &shadow_fbo);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, shadow_map);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RG32F, shadow_map_resolution, shadow_map_resolution, 0, GL_RGBA, GL_FLOAT, nullptr);
    glBindRenderbuffer(GL_RENDERBUFFER, render_buffer);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, shadow_map_resolution, shadow_map_resolution);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, shadow_fbo);
    glFramebufferTexture(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, shadow_map, 0);
    glFramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, render_buffer);
    if (glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        throw std::runtime_error("Incomplete framebuffer!");

    auto sphere_vertex_shader = create_shader(GL_VERTEX_SHADER, project_root + "/shaders/sphere.vert");
    auto sphere_fragment_shader = create_shader(GL_FRAGMENT_SHADER, project_root + "/shaders/sphere.frag");
    auto sphere_program = create_program(sphere_vertex_shader, sphere_fragment_shader);

    GLuint environment_vao;
    glGenVertexArrays(1, &environment_vao);

    GLuint model_location = glGetUniformLocation(sphere_program, "model");
    GLuint view_location = glGetUniformLocation(sphere_program, "view");
    GLuint projection_location = glGetUniformLocation(sphere_program, "projection");
    GLuint light_direction_location = glGetUniformLocation(sphere_program, "light_direction");
    GLuint ___light_color_location = glGetUniformLocation(sphere_program, "light_color");
    GLuint camera_position_location = glGetUniformLocation(sphere_program, "camera_position");
    GLuint environment_map_texture_location = glGetUniformLocation(sphere_program, "environment_map_texture");
    GLuint ____ambient_location = glGetUniformLocation(sphere_program, "ambient");

    GLuint sphere_vao, sphere_vbo, sphere_ebo;
    glGenVertexArrays(1, &sphere_vao);
    glBindVertexArray(sphere_vao);
    glGenBuffers(1, &sphere_vbo);
    glGenBuffers(1, &sphere_ebo);
    GLuint sphere_index_count;
    {
        auto [vertices, indices] = generate_sphere(1.f, 16);
        bounding_box = get_bounding_box(vertices);
        c = std::accumulate(bounding_box.begin(), bounding_box.end(), glm::vec3(0.f)) / 8.f;
        glBindBuffer(GL_ARRAY_BUFFER, sphere_vbo);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(vertices[0]), vertices.data(), GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sphere_ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(indices[0]), indices.data(), GL_STATIC_DRAW);
        sphere_index_count = indices.size();
    }
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), (void *)offsetof(vertex, position));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), (void *)offsetof(vertex, normal));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), (void *)offsetof(vertex, texcoords));

    float floor_angle = 0.4f * glm::pi<float>();

    auto floor_vertex_shader = create_shader(GL_VERTEX_SHADER, project_root + "/shaders/floor.vert");
    auto floor_fragment_shader = create_shader(GL_FRAGMENT_SHADER, project_root + "/shaders/floor.frag");

    auto floor_program = create_program(floor_vertex_shader, floor_fragment_shader);

    GLuint __model_location = glGetUniformLocation(floor_program, "model");
    GLuint __view_location = glGetUniformLocation(floor_program, "view");
    GLuint __projection_location = glGetUniformLocation(floor_program, "projection");
    GLuint __light_direction_location = glGetUniformLocation(floor_program, "light_direction");
    GLuint ___camera_position_location = glGetUniformLocation(floor_program, "camera_position");
    GLuint _transform_location = glGetUniformLocation(floor_program, "transform");
    GLuint _shadow_map_location = glGetUniformLocation(floor_program, "shadow_map");
    GLuint ___ambient_location = glGetUniformLocation(floor_program, "ambient");
    GLuint __light_color_location = glGetUniformLocation(floor_program, "light_color");

    GLuint floor_vao, floor_vbo, floor_ebo;
    glGenVertexArrays(1, &floor_vao);
    glBindVertexArray(floor_vao);
    glGenBuffers(1, &floor_vbo);
    glGenBuffers(1, &floor_ebo);
    GLuint floor_index_count;
    {
        auto [vertices, indices] = generate_floor(0.97f, 16, floor_angle);
        glBindBuffer(GL_ARRAY_BUFFER, floor_vbo);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(vertices[0]), vertices.data(), GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, floor_ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(indices[0]), indices.data(), GL_STATIC_DRAW);
        floor_index_count = indices.size();
    }
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), (void *)offsetof(vertex, position));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), (void *)offsetof(vertex, normal));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), (void *)offsetof(vertex, texcoords));

    auto debug_vertex_shader = create_shader(GL_VERTEX_SHADER, project_root + "/shaders/debug.vert");
    auto debug_fragment_shader = create_shader(GL_FRAGMENT_SHADER, project_root + "/shaders/debug.frag");
    auto debug_program = create_program(debug_vertex_shader, debug_fragment_shader);
    GLuint __shadow_map_location = glGetUniformLocation(debug_program, "shadow_map");
    GLuint debug_vao;
    glGenVertexArrays(1, &debug_vao);

    auto wolf_vertex_shader = create_shader(GL_VERTEX_SHADER, project_root + "/shaders/wolf.vert");
    auto wolf_fragment_shader = create_shader(GL_FRAGMENT_SHADER, project_root + "/shaders/wolf.frag");
    auto wolf_program = create_program(wolf_vertex_shader, wolf_fragment_shader);

    GLuint ___model_location = glGetUniformLocation(wolf_program, "model");
    GLuint ___view_location = glGetUniformLocation(wolf_program, "view");
    GLuint ___projection_location = glGetUniformLocation(wolf_program, "projection");
    GLuint color_location = glGetUniformLocation(wolf_program, "color");
    GLuint use_texture_location = glGetUniformLocation(wolf_program, "use_texture");
    GLuint ___light_direction_location = glGetUniformLocation(wolf_program, "light_direction");
    GLuint bones_location = glGetUniformLocation(wolf_program, "bones");
    GLuint albedo_location = glGetUniformLocation(wolf_program, "albedo");
    GLuint _light_color_location = glGetUniformLocation(wolf_program, "light_color");
    GLuint ___shadow_map_location = glGetUniformLocation(wolf_program, "shadow_map");
    GLint __transform_location = glGetUniformLocation(wolf_program, "transform");
    GLuint __ambient_location = glGetUniformLocation(wolf_program, "ambient");

    std::string model_path = project_root + "/wolf/Wolf-Blender-2.82a.gltf";

    auto const input_model = load_gltf(model_path);
    GLuint wolf_vbo;
    glGenBuffers(1, &wolf_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, wolf_vbo);
    glBufferData(GL_ARRAY_BUFFER, input_model.buffer.size(), input_model.buffer.data(), GL_STATIC_DRAW);

    const gltf_model::animation& animation1 = input_model.animations.at("01_Run");
    const gltf_model::animation& animation2 = input_model.animations.at("02_walk");

    struct mesh {
        GLuint vao;
        gltf_model::accessor indices;
        gltf_model::material material;
    };

    auto setup_attribute = [](int index, gltf_model::accessor const & accessor, bool integer = false) {
        glEnableVertexAttribArray(index);
        if (integer)
            glVertexAttribIPointer(index, accessor.size, accessor.type, 0, reinterpret_cast<void *>(accessor.view.offset));
        else
            glVertexAttribPointer(index, accessor.size, accessor.type, GL_FALSE, 0, reinterpret_cast<void *>(accessor.view.offset));
    };

    std::vector<mesh> meshes;
    for (auto const &mesh : input_model.meshes) {
        auto& result = meshes.emplace_back();
        glGenVertexArrays(1, &result.vao);
        glBindVertexArray(result.vao);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, wolf_vbo);
        result.indices = mesh.indices;

        setup_attribute(0, mesh.position);
        setup_attribute(1, mesh.normal);
        setup_attribute(2, mesh.texcoord);
        setup_attribute(3, mesh.joints, true);
        setup_attribute(4, mesh.weights);

        result.material = mesh.material;
    }

    for (auto const &mesh : meshes) {
        if (!mesh.material.texture_path) continue;
        textures.load_texture(wolf_dir + *mesh.material.texture_path);
    }

    std::default_random_engine rng;
    std::vector<particle> particles;

    auto snow_vertex_shader = create_shader(GL_VERTEX_SHADER, project_root + "/shaders/snow.vert");
    auto snow_geometry_shader = create_shader(GL_GEOMETRY_SHADER, project_root + "/shaders/snow.geom");
    auto snow_fragment_shader = create_shader(GL_FRAGMENT_SHADER, project_root + "/shaders/snow.frag");
    auto snow_program = create_program(snow_vertex_shader, snow_geometry_shader, snow_fragment_shader);

    GLuint ____model_location = glGetUniformLocation(snow_program, "model");
    GLuint ____view_location = glGetUniformLocation(snow_program, "view");
    GLuint ____projection_location = glGetUniformLocation(snow_program, "projection");
    GLuint ____camera_position_location = glGetUniformLocation(snow_program, "camera_position");
    GLuint _texture_location = glGetUniformLocation(snow_program, "_texture");

    GLuint snow_vao, snow_vbo;
    glGenVertexArrays(1, &snow_vao);
    glBindVertexArray(snow_vao);
    glGenBuffers(1, &snow_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, snow_vbo);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(particle), (void*)(0));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, sizeof(particle), (void*)(12));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, sizeof(particle), (void*)(16));




    auto smog_vertex_shader = create_shader(GL_VERTEX_SHADER, project_root + "/shaders/smog.vert");
    auto smog_fragment_shader = create_shader(GL_FRAGMENT_SHADER, project_root + "/shaders/smog.frag");
    auto smog_program = create_program(smog_vertex_shader, smog_fragment_shader);

    GLuint _____view_location = glGetUniformLocation(smog_program, "view");
    GLuint _____projection_location = glGetUniformLocation(smog_program, "projection");
    GLuint center_location = glGetUniformLocation(smog_program, "center");
    GLuint r_location = glGetUniformLocation(smog_program, "r");
    GLuint _____camera_position_location = glGetUniformLocation(smog_program, "camera_position");
    GLuint ____light_direction_location = glGetUniformLocation(smog_program, "light_direction");

    /*
    GLuint smog_vao, smog_vbo, smog_ebo;
    glGenVertexArrays(1, &smog_vao);
    glBindVertexArray(smog_vao);

    glGenBuffers(1, &smog_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, smog_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cube_vertices), cube_vertices, GL_STATIC_DRAW);

    glGenBuffers(1, &smog_ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, smog_ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(cube_indices), cube_indices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
    */



    float ambient = 0.2f;

    auto last_frame_start = std::chrono::high_resolution_clock::now();

    float time = 0.f;

    std::map<SDL_Keycode, bool> button_down;

    float view_elevation = glm::radians(20.f);
    float view_azimuth = 0.f;
    float camera_distance = 2.f;

    float f = 1.f, wolf_speed = 0.5f;

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
            break;
        case SDL_KEYUP:
            button_down[event.key.keysym.sym] = false;
            break;
        }

        if (!running)
            break;

        auto now = std::chrono::high_resolution_clock::now();
        float dt = std::chrono::duration_cast<std::chrono::duration<float>>(now - last_frame_start).count();
        last_frame_start = now;
        time += dt;

        if (button_down[SDLK_UP])
            camera_distance -= 4.f * dt;
        if (button_down[SDLK_DOWN])
            camera_distance += 4.f * dt;

        if (button_down[SDLK_LEFT])
            view_azimuth -= 2.f * dt;
        if (button_down[SDLK_RIGHT])
            view_azimuth += 2.f * dt;

        if (button_down[SDLK_w])
            view_elevation += 2.f * dt;
        if (button_down[SDLK_s])
            view_elevation -= 2.f * dt;
        if (button_down[SDLK_q])
            ambient = std::min(1.f, ambient + 2.f * dt);
        if (button_down[SDLK_a])
            ambient = std::max(0.f, ambient - 2.f * dt);

        glm::vec3 ambient_color(ambient);

        float near = 0.1f;
        float far = 100.f;

        glm::mat4 view(1.f);
        view = glm::translate(view, {0.f, 0.f, -camera_distance});
        view = glm::rotate(view, view_elevation, {1.f, 0.f, 0.f});
        view = glm::rotate(view, view_azimuth, {0.f, 1.f, 0.f});

        glm::mat4 projection = glm::mat4(1.f);
        projection = glm::perspective(glm::pi<float>() / 2.f, (1.f * width) / height, near, far);

        glm::vec3 light_direction = glm::normalize(glm::vec3(2.f * cos(time), 2.f, 2.f * sin(time)));

        glm::vec3 camera_position = (glm::inverse(view) * glm::vec4(0.f, 0.f, 0.f, 1.f)).xyz();
        glm::mat4 view_projection_inverse = inverse(projection * view);

        glm::vec3 light_z = -light_direction;
        glm::vec3 light_x = glm::normalize(glm::cross(light_z, {0.f, 1.f, 0.f}));
        glm::vec3 light_y = glm::normalize(glm::cross(light_x, light_z));
        float dx = -std::numeric_limits<float>::infinity();
        float dy = -std::numeric_limits<float>::infinity();
        float dz = -std::numeric_limits<float>::infinity();
        for(auto _v : bounding_box) {
            glm::vec3 v = _v - c;
            dx = std::max(dx, abs(glm::dot(v, light_x)));
            dy = std::max(dy, abs(glm::dot(v, light_y)));
            dz = std::max(dz, abs(glm::dot(v, light_z)));
        }
        glm::mat4 transform = glm::inverse(glm::mat4({
            {dx * light_x.x, dx * light_x.y, dx * light_x.z, 0.f},
            {dy * light_y.x, dy * light_y.y, dy * light_y.z, 0.f},
            {dz * light_z.x, dz * light_z.y, dz * light_z.z, 0.f},
            {c.x, c.y, c.z, 1.f}
        }));

        float A = -1.5, B = 0.1, C = 0.1;

        for(auto &p : particles) {
            if(p.position.y < std::sin(floor_angle - glm::pi<float>() / 2.f)) {
                float lat = std::uniform_real_distribution<float>{0.5f * glm::pi<float>() - floor_angle, 0.5f * glm::pi<float>()}(rng);
                float lon = std::uniform_real_distribution<float>{0.f, 2.f * glm::pi<float>()}(rng);
                p.position = glm::vec3(std::cos(lat) * std::cos(lon), std::sin(lat), std::cos(lat) * std::sin(lon)) * 0.97f;
                p.size = std::uniform_real_distribution<float>{0.01f, 0.03f}(rng);
                p.velocity.y = -std::uniform_real_distribution<float>{0.0f, 0.9f}(rng);
                p.angular_velocity = std::uniform_real_distribution<float>{0.0f, 0.5f}(rng);
            }
            else {
                p.velocity.y += dt * A;
                p.velocity *= exp(-C * dt);
                p.size *= exp(-B * dt);
                p.position += p.velocity * dt;
                p.rotation += p.angular_velocity * dt;
            }
        }
        if(particles.size() < 512) {
            particles.emplace_back();
            float lat = std::uniform_real_distribution<float>{0.5f * glm::pi<float>() - floor_angle, 0.5f * glm::pi<float>()}(rng);
            float lon = std::uniform_real_distribution<float>{0.f, 2.f * glm::pi<float>()}(rng);
            particles.back().position = glm::vec3(std::cos(lat) * std::cos(lon), std::sin(lat), std::cos(lat) * std::sin(lon)) * 0.97f;
            particles.back().size = std::uniform_real_distribution<float>{0.01f, 0.03f}(rng);
            particles.back().velocity.y = -std::uniform_real_distribution<float>{0.0f, 0.1f}(rng);
            particles.back().angular_velocity = std::uniform_real_distribution<float>{0.0f, 0.5f}(rng);
        }

        glm::mat4 wolf_model(1.f);
        wolf_model = glm::scale(wolf_model, glm::vec3(0.7f));
        wolf_model = glm::rotate(wolf_model, -wolf_speed * time, glm::vec3(0.f, 1.f, 0.f));
        wolf_model = glm::translate(wolf_model, glm::vec3(0.9f, -0.45f, 0.f));
        wolf_model = glm::rotate(wolf_model, 0.18f, glm::vec3(1.f, 0.f, 0.f));

        std::vector<glm::mat4x3> bones(input_model.bones.size(), glm::mat4x3(0.0));
        float t1 = std::fmod(time, animation1.max_time);
        float t2 = std::fmod(time, animation2.max_time);
        for(int i = 0; i < bones.size(); ++i) {
            glm::mat4 translation = glm::translate(glm::mat4(1.f),
                                                   glm::lerp(animation1.bones[i].translation(t1),
                                                             animation2.bones[i].translation(t2), f));
            glm::mat4 scale = glm::scale(glm::mat4(1.f),
                                         glm::lerp(animation1.bones[i].scale(t1),
                                                   animation2.bones[i].scale(t2), f));
            glm::mat4 rotation = glm::toMat4(glm::slerp(animation1.bones[i].rotation(t1),
                                                        animation2.bones[i].rotation(t2), f));

            glm::mat4 _transform = translation * rotation * scale;
            if(input_model.bones[i].parent != -1)
                _transform = bones[input_model.bones[i].parent] * _transform;
            bones[i] = _transform;
        }

        for(int i = 0; i < bones.size(); ++i)
            bones[i] = bones[i] * input_model.bones[i].inverse_bind_matrix;

        auto draw_meshes_to_shadow = [&](bool transparent) {
            for (auto const & mesh : meshes) {
                if (mesh.material.transparent != transparent)
                    continue;
                if (mesh.material.two_sided)
                    glDisable(GL_CULL_FACE);
                else
                    glEnable(GL_CULL_FACE);
                if (transparent)
                    glEnable(GL_BLEND);
                else
                    glDisable(GL_BLEND);
                glBindVertexArray(mesh.vao);
                glDrawElements(GL_TRIANGLES, mesh.indices.count, mesh.indices.type, reinterpret_cast<void *>(mesh.indices.view.offset));
            }
        };

        auto draw_meshes = [&](bool transparent) {
            for (auto const & mesh : meshes) {
                if (mesh.material.transparent != transparent)
                    continue;
                if (mesh.material.two_sided)
                    glDisable(GL_CULL_FACE);
                else
                    glEnable(GL_CULL_FACE);
                if (transparent)
                    glEnable(GL_BLEND);
                else
                    glDisable(GL_BLEND);
                if (mesh.material.texture_path) {
                    glUniform1i(albedo_location, textures.get_texture(wolf_dir + *mesh.material.texture_path));
                    glUniform1i(use_texture_location, 1);
                }
                else if (mesh.material.color) {
                    glUniform1i(use_texture_location, 0);
                    glUniform4fv(color_location, 1, reinterpret_cast<const float *>(&(*mesh.material.color)));
                }
                else continue;
                glBindVertexArray(mesh.vao);
                glDrawElements(GL_TRIANGLES, mesh.indices.count, mesh.indices.type, reinterpret_cast<void *>(mesh.indices.view.offset));
            }
        };

        glDisable(GL_BLEND);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, shadow_fbo);
        glClearColor(1.f, 1.f, 0.f, 0.f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glViewport(0, 0, shadow_map_resolution, shadow_map_resolution);
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LEQUAL);
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
        glUseProgram(shadow_program);
        glUniformMatrix4fv(shadow_model_location, 1, GL_FALSE, reinterpret_cast<float *>(&christmas_tree_model));
        glUniformMatrix4fv(shadow_transform_location, 1, GL_FALSE, reinterpret_cast<float *>(&transform));
        glUniform1i(use_bones_location, 0);
        glBindVertexArray(vao);
        GLint current_block = 0;
        for(auto &shape : shapes) {
            auto material = materials[shape.mesh.material_ids[0]];
            std::string texture_path = christmas_tree_dir + material.alpha_texname;
            std::replace(texture_path.begin(), texture_path.end(), '\\', '/');
            glUniform1i(have_alpha_location, !material.alpha_texname.empty());
            glUniform1i(alpha_texture_location, textures.get_texture(texture_path));
            glDrawArrays(GL_TRIANGLES, current_block, (GLint)shape.mesh.indices.size());
            current_block += (GLint)shape.mesh.indices.size();
        }

        glUniform1i(use_bones_location, 1);
        glUniformMatrix4x3fv(_bones_location, input_model.bones.size(), GL_FALSE, (float*)bones.data());
        glUniformMatrix4fv(shadow_model_location, 1, GL_FALSE, reinterpret_cast<float *>(&wolf_model));
        draw_meshes_to_shadow(false);
        glDepthMask(GL_FALSE);
        draw_meshes_to_shadow(true);
        glDepthMask(GL_TRUE);

        glBindTexture(GL_TEXTURE_2D, shadow_map);
        glGenerateMipmap(GL_TEXTURE_2D);

        glEnable(GL_BLEND);
        glBlendEquation(GL_FUNC_ADD);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
        glClearColor(0.8f, 0.8f, 1.f, 0.f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glViewport(0, 0, width, height);

        glUseProgram(environment_program);
        glDisable(GL_DEPTH_TEST);
        glUniformMatrix4fv(view_projection_inverse_location, 1, GL_FALSE, reinterpret_cast<float *>(&view_projection_inverse));
        glUniform3fv(_camera_position_location, 1, reinterpret_cast<float *>(&camera_position));
        glUniform1i(_environment_map_texture_location, textures.get_texture(environment_path));
        glUniform3fv(_ambient_location, 1, reinterpret_cast<float *>(&ambient_color));
        glBindVertexArray(environment_vao);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LEQUAL);
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);

        glUseProgram(christmas_tree_program);
        glUniformMatrix4fv(_model_location, 1, GL_FALSE, reinterpret_cast<float *>(&christmas_tree_model));
        glUniformMatrix4fv(_view_location, 1, GL_FALSE, reinterpret_cast<float *>(&view));
        glUniformMatrix4fv(_projection_location, 1, GL_FALSE, reinterpret_cast<float *>(&projection));
        glUniformMatrix4fv(transform_location, 1, GL_FALSE, reinterpret_cast<float *>(&transform));
        glUniform3fv(ambient_location, 1, reinterpret_cast<float *>(&ambient_color));
        glUniform3fv(_light_direction_location, 1, reinterpret_cast<float *>(&light_direction));
        glUniform3f(light_color_location, 0.8f, 0.8f, 0.8f);
        glUniform3fv(__camera_position_location, 1, reinterpret_cast<float *>(&camera_position));
        glUniform1i(shadow_map_location, 1);
        glBindVertexArray(vao);
        current_block = 0;
        for(auto &shape : shapes) {
            auto material = materials[shape.mesh.material_ids[0]];
            std::string texture_path = christmas_tree_dir + material.ambient_texname;
            std::replace(texture_path.begin(), texture_path.end(), '\\', '/');
            glUniform3fv(_albedo_location, 1, material.ambient);
            glUniform1f(power_location, material.shininess);
            glUniform1f(glossiness_location, material.specular[0]);
            glUniform1i(texture_location, textures.get_texture(texture_path));
            material = materials[shape.mesh.material_ids[0]];
            texture_path = christmas_tree_dir + material.alpha_texname;
            std::replace(texture_path.begin(), texture_path.end(), '\\', '/');
            glUniform1i(have_ambient_texture_location, !material.ambient_texname.empty());
            glUniform1i(_have_alpha_location, !material.alpha_texname.empty());
            glUniform1i(_alpha_texture_location, textures.get_texture(texture_path));
            glDrawArrays(GL_TRIANGLES, current_block, (GLint)shape.mesh.indices.size());
            current_block += (GLint)shape.mesh.indices.size();
        }

        glm::mat4 sphere_model(1.f);

        glUseProgram(floor_program);
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
        glUniformMatrix4fv(__model_location, 1, GL_FALSE, reinterpret_cast<float *>(&sphere_model));
        glUniformMatrix4fv(__view_location, 1, GL_FALSE, reinterpret_cast<float *>(&view));
        glUniformMatrix4fv(__projection_location, 1, GL_FALSE, reinterpret_cast<float *>(&projection));
        glUniform3fv(__light_direction_location, 1, reinterpret_cast<float *>(&light_direction));
        glUniform3fv(___camera_position_location, 1, reinterpret_cast<float *>(&camera_position));
        glUniform1i(_shadow_map_location, 1);
        glUniformMatrix4fv(_transform_location, 1, GL_FALSE, reinterpret_cast<float *>(&transform));
        glUniform3fv(___ambient_location, 1, reinterpret_cast<float *>(&ambient_color));
        glUniform3f(__light_color_location, 0.8f, 0.8f, 0.8f);

        glBindVertexArray(floor_vao);
        glDrawElements(GL_TRIANGLES, floor_index_count, GL_UNSIGNED_INT, nullptr);

        glUseProgram(wolf_program);
        glUniformMatrix4fv(___model_location, 1, GL_FALSE, reinterpret_cast<float *>(&wolf_model));
        glUniformMatrix4fv(___view_location, 1, GL_FALSE, reinterpret_cast<float *>(&view));
        glUniformMatrix4fv(___projection_location, 1, GL_FALSE, reinterpret_cast<float *>(&projection));
        glUniform3fv(___light_direction_location, 1, reinterpret_cast<float *>(&light_direction));
        glUniform3f(_light_color_location, 0.8f, 0.8f, 0.8f);
        glUniform1i(___shadow_map_location, 1);
        glUniform3fv(__ambient_location, 1, reinterpret_cast<float *>(&ambient_color));
        glUniformMatrix4fv(__transform_location, 1, GL_FALSE, reinterpret_cast<float *>(&transform));
        glUniformMatrix4x3fv(bones_location, input_model.bones.size(), GL_FALSE, (float*)bones.data());

        draw_meshes(false);
        glDepthMask(GL_FALSE);
        draw_meshes(true);
        glDepthMask(GL_TRUE);

        glm::mat4 snow_model(1.f);
        glUseProgram(snow_program);
        glBindBuffer(GL_ARRAY_BUFFER, snow_vbo);
        glBufferData(GL_ARRAY_BUFFER, particles.size() * sizeof(particle), particles.data(), GL_STATIC_DRAW);
        glUniformMatrix4fv(____model_location, 1, GL_FALSE, reinterpret_cast<float *>(&snow_model));
        glUniformMatrix4fv(____view_location, 1, GL_FALSE, reinterpret_cast<float *>(&view));
        glUniformMatrix4fv(____projection_location, 1, GL_FALSE, reinterpret_cast<float *>(&projection));
        glUniform3fv(____camera_position_location, 1, reinterpret_cast<float *>(&camera_position));
        glUniform1i(_texture_location, textures.get_texture(particle_texture_path));
        glBindVertexArray(snow_vao);
        glDrawArrays(GL_POINTS, 0, particles.size());





        glCullFace(GL_FRONT);
        glUseProgram(smog_program);
        glEnable(GL_BLEND);
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
        glUniformMatrix4fv(_____view_location, 1, GL_FALSE, reinterpret_cast<float *>(&view));
        glUniformMatrix4fv(_____projection_location, 1, GL_FALSE, reinterpret_cast<float *>(&projection));
        glUniform3f(center_location, 0.f, 0.f, 0.f);
        glUniform1f(r_location, 0.97f);
        glUniform3fv(_____camera_position_location, 1, reinterpret_cast<float *>(&camera_position));
        glUniform3fv(____light_direction_location, 1, reinterpret_cast<float *>(&light_direction));

        glBindVertexArray(sphere_vao);
        glDrawElements(GL_TRIANGLES, sphere_index_count, GL_UNSIGNED_INT, nullptr);





        glCullFace(GL_BACK);
        glUseProgram(sphere_program);
        glEnable(GL_BLEND);
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
        glUniformMatrix4fv(model_location, 1, GL_FALSE, reinterpret_cast<float *>(&sphere_model));
        glUniformMatrix4fv(view_location, 1, GL_FALSE, reinterpret_cast<float *>(&view));
        glUniformMatrix4fv(projection_location, 1, GL_FALSE, reinterpret_cast<float *>(&projection));
        glUniform3fv(light_direction_location, 1, reinterpret_cast<float *>(&light_direction));
        glUniform3fv(camera_position_location, 1, reinterpret_cast<float *>(&camera_position));
        glUniform1i(environment_map_texture_location, textures.get_texture(environment_path));
        glUniform3f(___light_color_location, 0.8f, 0.8f, 0.8f);
        glUniform3fv(____ambient_location, 1, reinterpret_cast<float *>(&ambient_color));
        glBindVertexArray(sphere_vao);
        glDrawElements(GL_TRIANGLES, sphere_index_count, GL_UNSIGNED_INT, nullptr);

        glUseProgram(debug_program);
        glUniform1i(__shadow_map_location, 1);
        glBindVertexArray(debug_vao);
        glDrawArrays(GL_TRIANGLES, 0, 6);

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