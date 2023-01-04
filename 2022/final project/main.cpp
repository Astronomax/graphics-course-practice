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
#include "reactphysics3d/reactphysics3d.h"

struct mesh
{
    GLuint vao;
    gltf_model::accessor indices;
    gltf_model::material material;
};

rp3d::Vector3 get_bbox_size(bounding_box bbox) {
    float x_bounds[2] = {std::numeric_limits<float>::infinity(),
                         -std::numeric_limits<float>::infinity()};
    float y_bounds[2] = {std::numeric_limits<float>::infinity(),
                         -std::numeric_limits<float>::infinity()};
    float z_bounds[2] = {std::numeric_limits<float>::infinity(),
                         -std::numeric_limits<float>::infinity()};
    for(auto &v : bbox) {
        x_bounds[0] = std::min(x_bounds[0], v.x);
        y_bounds[0] = std::min(y_bounds[0], v.y);
        z_bounds[0] = std::min(z_bounds[0], v.z);
        x_bounds[1] = std::max(x_bounds[1], v.x);
        y_bounds[1] = std::max(y_bounds[1], v.y);
        z_bounds[1] = std::max(z_bounds[1], v.z);
    }
    return { x_bounds[1] - x_bounds[0],
             y_bounds[1] - y_bounds[0],
             z_bounds[1] - z_bounds[0] };
}

std::vector<rp3d::Vector2> generate_pin_positions() {
    std::vector<rp3d::Vector2> res;
    float dz = 0.14f;
    for(int i = 1; i <= 4; i++) {
        float x = -(float)(i - 1) * dz;
        float z = (float)(i - 1) * dz;
        float dx = (2.f * z) / (float)(i - 1);
        for(int j = 0; j < i; j++) {
            res.emplace_back(x, z);
            x += dx;
        }
    }
    return res;
}

int main() try {
    auto *window = create_window("Bowling");
    auto gl_context = create_context(window);
    int width, height;
    SDL_GetWindowSize(window, &width, &height);

    texture_holder textures(3);

    const std::string project_root = PROJECT_ROOT;
    const std::string ball_dir = project_root + "/ball/";
    const std::string ball_path = ball_dir + "ball.obj";
    const std::string pin_dir = project_root + "/pin/";
    const std::string pin_path = pin_dir + "pin.obj";
    const std::string alley_path = project_root + "/bowling_alley_mozilla_hubs_room/scene.gltf";

    auto alley_vertex_shader = create_shader(GL_VERTEX_SHADER, project_root + "/shaders/alley.vert");
    auto alley_fragment_shader = create_shader(GL_FRAGMENT_SHADER, project_root + "/shaders/alley.frag");
    auto alley_program = create_program(alley_vertex_shader, alley_fragment_shader);

    GLuint alley_model_location = glGetUniformLocation(alley_program, "model");
    GLuint alley_view_location = glGetUniformLocation(alley_program, "view");
    GLuint alley_projection_location = glGetUniformLocation(alley_program, "projection");
    GLuint alley_albedo_location = glGetUniformLocation(alley_program, "albedo");
    GLuint alley_normal_location = glGetUniformLocation(alley_program, "normal_texture");
    GLuint alley_color_location = glGetUniformLocation(alley_program, "color");
    GLuint alley_use_texture_location = glGetUniformLocation(alley_program, "use_texture");
    GLuint alley_light_direction_location = glGetUniformLocation(alley_program, "light_direction");

    auto const alley_gltf_model = load_gltf(alley_path);
    GLuint alley_vbo;
    glGenBuffers(1, &alley_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, alley_vbo);
    glBufferData(GL_ARRAY_BUFFER, alley_gltf_model.buffer.size(), alley_gltf_model.buffer.data(), GL_STATIC_DRAW);

    auto setup_attribute = [](int index, gltf_model::accessor const & accessor, bool integer = false) {
        glEnableVertexAttribArray(index);
        if (integer)
            glVertexAttribIPointer(index, accessor.size, accessor.type, accessor.view.stride, reinterpret_cast<void *>(accessor.offset + accessor.view.offset));
        else
            glVertexAttribPointer(index, accessor.size, accessor.type, GL_FALSE, accessor.view.stride, reinterpret_cast<void *>(accessor.offset + accessor.view.offset));
    };

    std::vector<mesh> meshes;
    for (auto const &mesh : alley_gltf_model.meshes) {
        auto& result = meshes.emplace_back();
        glGenVertexArrays(1, &result.vao);
        glBindVertexArray(result.vao);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, alley_vbo);
        result.indices = mesh.indices;
        setup_attribute(0, mesh.position);
        setup_attribute(1, mesh.tangent);
        setup_attribute(2, mesh.normal);
        setup_attribute(3, mesh.texcoord);
        result.material = mesh.material;
    }

    for (auto const &mesh : meshes) {
        if (!mesh.material.ambient_texture) continue;
        auto ambient_path = std::filesystem::path(alley_path).parent_path() / *mesh.material.ambient_texture;
        textures.load_texture(ambient_path);
        auto normal_path = std::filesystem::path(alley_path).parent_path() / *mesh.material.normal_texture;
        textures.load_texture(normal_path);
    }

    glm::mat4 alley_model = glm::mat4(1.f);
    alley_model = glm::translate(alley_model, {0.2f, 1.1f, -11.7f});
    alley_model = glm::rotate(alley_model, glm::pi<float>(), {0.f, 1.f, 0.f});
    alley_model = glm::scale(alley_model, glm::vec3(13.f));

    rp3d::PhysicsCommon physicsCommon;
    rp3d::PhysicsWorld* world = physicsCommon.createPhysicsWorld();
    world->setIsDebugRenderingEnabled(true);
    rp3d::DebugRenderer& debugRenderer = world->getDebugRenderer();
    debugRenderer.setIsDebugItemDisplayed(rp3d::DebugRenderer::DebugItem::COLLISION_SHAPE, true);
    debugRenderer.setIsDebugItemDisplayed(rp3d::DebugRenderer::DebugItem::COLLIDER_AABB, true);
    debugRenderer.setIsDebugItemDisplayed(rp3d::DebugRenderer::DebugItem::COLLIDER_BROADPHASE_AABB, true);

    auto bowling_vertex_shader = create_shader(GL_VERTEX_SHADER, project_root + "/shaders/bowling.vert");
    auto bowling_fragment_shader = create_shader(GL_FRAGMENT_SHADER, project_root + "/shaders/bowling.frag");
    auto bowling_program = create_program(bowling_vertex_shader, bowling_fragment_shader);

    GLint bowling_model_location = glGetUniformLocation(bowling_program, "model");
    GLint bowling_transform_location = glGetUniformLocation(bowling_program, "transform");
    GLint bowling_view_location = glGetUniformLocation(bowling_program, "view");
    GLint bowling_projection_location = glGetUniformLocation(bowling_program, "projection");
    GLint bowling_color_location = glGetUniformLocation(bowling_program, "color");

    float eps = 1e-2;
    float floor_height = 0.2f;
    rp3d::RigidBody *floor = world->createRigidBody(rp3d::Transform(rp3d::Vector3(0, 0, -10.f), rp3d::Quaternion::identity()));
    rp3d::BoxShape* floorShape = physicsCommon.createBoxShape(rp3d::Vector3(20.f, floor_height, 20.f));
    floor->addCollider(floorShape, rp3d::Transform(rp3d::Vector3::zero(), rp3d::Quaternion::identity()));
    floor->setType(rp3d::BodyType::STATIC);

    float wall_thickness = 0.2f;
    rp3d::RigidBody *wall = world->createRigidBody(rp3d::Transform(rp3d::Vector3(3.f, 1.3f, 1.f), rp3d::Quaternion::identity()));
    rp3d::BoxShape* wallShape = physicsCommon.createBoxShape(rp3d::Vector3(7.f, 1.5f, wall_thickness));
    wall->addCollider(wallShape, rp3d::Transform(rp3d::Vector3::zero(), rp3d::Quaternion::identity()));
    wall->setType(rp3d::BodyType::STATIC);

    float border_height = 0.1f;
    float border_spawn_y = border_height / 2.f + floor_height + eps;
    float border_spawn_z = -2.6f;

    rp3d::RigidBody *border1 = world->createRigidBody(rp3d::Transform(rp3d::Vector3(-1.2f, border_spawn_y, border_spawn_z), rp3d::Quaternion::identity()));
    rp3d::BoxShape* borderShape = physicsCommon.createBoxShape(rp3d::Vector3(0.27f, border_height, 3.4f));
    border1->addCollider(borderShape, rp3d::Transform(rp3d::Vector3::zero(), rp3d::Quaternion::identity()));
    border1->setType(rp3d::BodyType::STATIC);

    rp3d::RigidBody *border2 = world->createRigidBody(rp3d::Transform(rp3d::Vector3(1.3f, border_spawn_y, border_spawn_z), rp3d::Quaternion::identity()));
    border2->addCollider(borderShape, rp3d::Transform(rp3d::Vector3::zero(), rp3d::Quaternion::identity()));
    border2->setType(rp3d::BodyType::STATIC);

    tinyobj::attrib_t ball_attrib;
    std::vector<tinyobj::shape_t> ball_shapes;
    std::vector<tinyobj::material_t> ball_materials;
    tinyobj::LoadObj(&ball_attrib, &ball_shapes, &ball_materials, nullptr, ball_path.c_str(), ball_dir.c_str());
    std::vector<vertex> ball_vertices = get_vertices(ball_attrib, ball_shapes);

    for(auto &material : ball_materials) {
        auto ambient_path = std::filesystem::path(ball_path).parent_path() / material.ambient_texname;
        textures.load_texture(ambient_path);
        auto normal_path = std::filesystem::path(ball_path).parent_path() / material.normal_texname;
        textures.load_texture(normal_path);
    }

    auto ball_bounding_box = get_bounding_box(ball_vertices);
    auto ball_center = std::accumulate(ball_bounding_box.begin(), ball_bounding_box.end(), glm::vec3(0.f)) / 8.f;

    float ball_height = get_bbox_size(ball_bounding_box).y;
    float ball_spawn_y = ball_height / 2.f + floor_height + eps;
    rp3d::Vector3 ball_pos(0.f, ball_spawn_y, -6.f);
    rp3d::RigidBody *ball = world->createRigidBody(rp3d::Transform(ball_pos, rp3d::Quaternion::identity()));
    rp3d::SphereShape* ballShape = physicsCommon.createSphereShape(ball_height / 2.f);
    ball->addCollider(ballShape, rp3d::Transform(rp3d::Vector3::zero(), rp3d::Quaternion::identity()));
    ball->updateMassPropertiesFromColliders();

    tinyobj::attrib_t pin_attrib;
    std::vector<tinyobj::shape_t> pin_shapes;
    std::vector<tinyobj::material_t> pin_materials;
    tinyobj::LoadObj(&pin_attrib, &pin_shapes, &pin_materials, nullptr, pin_path.c_str(), pin_dir.c_str());
    std::vector<vertex> pin_vertices = get_vertices(pin_attrib, pin_shapes);

    for(auto &material : pin_materials) {
        auto ambient_path = std::filesystem::path(pin_path).parent_path() / material.ambient_texname;
        textures.load_texture(ambient_path);
        auto normal_path = std::filesystem::path(pin_path).parent_path() / material.normal_texname;
        textures.load_texture(normal_path);
    }

    auto pin_bounding_box = get_bounding_box(pin_vertices);
    auto pin_center = std::accumulate(pin_bounding_box.begin(), pin_bounding_box.end(), glm::vec3(0.f)) / 8.f;
    rp3d::Vector3 pin_size = get_bbox_size(ball_bounding_box);
    float pin_height = pin_size.y;

    std::vector<rp3d::RigidBody*> pins(10);
    auto positions = generate_pin_positions();
    float pin_spawn_y = pin_height / 2.f + floor_height + eps;

    for(int i = 0; i < 10; i++) {
        rp3d::Vector3 pos(positions[i].x, pin_spawn_y, positions[i].y);
        pins[i] = world->createRigidBody(rp3d::Transform(pos, rp3d::Quaternion::identity()));
        rp3d::BoxShape *pinShape = physicsCommon.createBoxShape(get_bbox_size(pin_bounding_box) / 2.f);
        pins[i]->addCollider(pinShape, rp3d::Transform(rp3d::Vector3::zero(), rp3d::Quaternion::identity()));
        pins[i]->updateMassPropertiesFromColliders();
    }

    glm::mat4 ball_model = glm::mat4(1.f);
    ball_model = glm::translate(ball_model, -ball_center);
    glm::mat4 pin_model = glm::mat4(1.f);
    pin_model = glm::translate(pin_model, -pin_center);

    glm::mat4 ball_transform = glm::mat4(1.f);
    std::vector<glm::mat4> pin_transforms(10, glm::mat4(1.f));

    GLuint ball_vao, ball_vbo;
    glGenVertexArrays(1, &ball_vao);
    glBindVertexArray(ball_vao);
    glGenBuffers(1, &ball_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, ball_vbo);
    glBufferData(GL_ARRAY_BUFFER, ball_vertices.size() * sizeof(vertex), ball_vertices.data(), GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), (void *)offsetof(vertex, position));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), (void *)offsetof(vertex, normal));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), (void *)offsetof(vertex, texcoords));

    GLuint pin_vao, pin_vbo;
    glGenVertexArrays(1, &pin_vao);
    glBindVertexArray(pin_vao);
    glGenBuffers(1, &pin_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, pin_vbo);
    glBufferData(GL_ARRAY_BUFFER, pin_vertices.size() * sizeof(vertex), pin_vertices.data(), GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), (void *)offsetof(vertex, position));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), (void *)offsetof(vertex, normal));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), (void *)offsetof(vertex, texcoords));

    auto debug_vertex_shader = create_shader(GL_VERTEX_SHADER, project_root + "/shaders/debug.vert");
    auto debug_fragment_shader = create_shader(GL_FRAGMENT_SHADER, project_root + "/shaders/debug.frag");
    auto debug_program = create_program(debug_vertex_shader, debug_fragment_shader);

    GLint debug_model_location = glGetUniformLocation(debug_program, "model");
    GLint debug_view_location = glGetUniformLocation(debug_program, "view");
    GLint debug_projection_location = glGetUniformLocation(debug_program, "projection");

    GLuint debug_vao, debug_vbo;
    glGenVertexArrays(1, &debug_vao);
    glBindVertexArray(debug_vao);

    glGenBuffers(1, &debug_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, debug_vbo);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(rp3d::Vector3), (void *) 0);

    glm::mat4 debug_model(1.f);

    auto last_frame_start = std::chrono::high_resolution_clock::now();

    std::map<SDL_Keycode, bool> button_down;

    float time = 0.f, accumulated_time = 0.f;
    float time_per_update = 1.f / 60.f;
    float camera_distance = 12.f;
    float camera_angle = glm::pi<float>();
    float camera_elevation = glm::pi<float>() / 36.f;
    glm::vec3 light_direction = glm::normalize(glm::vec3(1.f, 2.f, 3.f));

    ball->applyLocalForceAtCenterOfMass(rp3d::Vector3(0.f, 0.f, 10.f));


    auto draw_obj = [bowling_color_location](
            std::vector<tinyobj::shape_t> &shapes,
            std::vector<tinyobj::material_t> &materials) {
        int current_block = 0;
        for(auto &shape : shapes) {
            auto material = materials[shape.mesh.material_ids[0]];
            glUniform3fv(bowling_color_location, 1, material.ambient);
            glDrawArrays(GL_TRIANGLES, current_block, (GLint)shape.mesh.indices.size());
            current_block += (GLint)shape.mesh.indices.size();
        }
    };

    auto draw_meshes = [&](bool transparent) {
        for (auto const &mesh : meshes) {
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

            if (mesh.material.ambient_texture) {
                auto ambient_path = std::filesystem::path(alley_path).parent_path() / *mesh.material.ambient_texture;
                glUniform1i(alley_albedo_location, textures.get_texture(ambient_path));
                glUniform1i(alley_use_texture_location, 1);
            } else if (mesh.material.color) {
                glUniform1i(alley_use_texture_location, 0);
                glUniform4fv(alley_color_location, 1, reinterpret_cast<const float *>(&(*mesh.material.color)));
            } else continue;

            auto normal_path = std::filesystem::path(alley_path).parent_path() / *mesh.material.normal_texture;
            glUniform1i(alley_normal_location, textures.get_texture(normal_path));

            glBindVertexArray(mesh.vao);

            glDrawElements(GL_TRIANGLES, mesh.indices.count, mesh.indices.type,
                           reinterpret_cast<void *>(mesh.indices.offset + mesh.indices.view.offset));
        }
    };

    while (true)
    {
        bool running = true;
        for (SDL_Event event; SDL_PollEvent(&event);) {
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
        }

        if (!running) break;

        auto now = std::chrono::high_resolution_clock::now();
        float dt = std::chrono::duration_cast<std::chrono::duration<float>>(now - last_frame_start).count();
        last_frame_start = now;
        time += dt;
        accumulated_time += dt;

        while(accumulated_time > time_per_update) {
            world->update(time_per_update);
            accumulated_time -= time_per_update;
        }

        ball->getTransform().getOpenGLMatrix(reinterpret_cast<float *>(&ball_transform));
        for(int i = 0; i < 10; i++)
            pins[i]->getTransform().getOpenGLMatrix(reinterpret_cast<float *>(&pin_transforms[i]));

        auto lines = debugRenderer.getLines();
        auto triangles = debugRenderer.getTriangles();

        if (button_down[SDLK_UP])
            camera_distance -= 4.f * dt;
        if (button_down[SDLK_DOWN])
            camera_distance += 4.f * dt;

        if (button_down[SDLK_LEFT])
            camera_angle += 2.f * dt;
        if (button_down[SDLK_RIGHT])
            camera_angle -= 2.f * dt;

        if (button_down[SDLK_w])
            camera_elevation += 2.f * dt;
        if (button_down[SDLK_s])
            camera_elevation -= 2.f * dt;

        float near = 0.1f, far = 100.f;

        glm::mat4 view(1.f);
        view = glm::translate(view, {0.f, 0.f, -camera_distance});
        view = glm::rotate(view, camera_elevation, {1.f, 0.f, 0.f});
        view = glm::rotate(view, camera_angle, {0.f, 1.f, 0.f});
        view = glm::translate(view, {0.f, -0.5f, 0.f});

        float aspect = (float)height / (float)width;
        glm::mat4 projection = glm::perspective(glm::pi<float>() / 3.f, 1.f / aspect, near, far);
        glm::vec3 camera_position = (glm::inverse(view) * glm::vec4(0.f, 0.f, 0.f, 1.f)).xyz();
        glm::vec3 sun_direction = glm::normalize(glm::vec3(std::sin(time * 0.5f), 2.f, std::cos(time * 0.5f)));

        glViewport(0, 0, width, height);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glClearColor(0.8f, 0.8f, 1.f, 0.f);

        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);

        glUseProgram(alley_program);
        glUniformMatrix4fv(alley_model_location, 1, GL_FALSE, reinterpret_cast<float *>(&alley_model));
        glUniformMatrix4fv(alley_view_location, 1, GL_FALSE, reinterpret_cast<float *>(&view));
        glUniformMatrix4fv(alley_projection_location, 1, GL_FALSE, reinterpret_cast<float *>(&projection));
        glUniform3fv(alley_light_direction_location, 1, reinterpret_cast<float *>(&light_direction));
        glUniform3f(alley_color_location, 0.8f, 0.8f, 0.8f);

        draw_meshes(false);
        glDepthMask(GL_FALSE);
        draw_meshes(true);
        glDepthMask(GL_TRUE);

        glUseProgram(bowling_program);
        glUniformMatrix4fv(bowling_model_location, 1, GL_FALSE, reinterpret_cast<float *>(&ball_model));
        glUniformMatrix4fv(bowling_transform_location, 1, GL_FALSE, reinterpret_cast<float *>(&ball_transform));
        glUniformMatrix4fv(bowling_view_location, 1, GL_FALSE, reinterpret_cast<float *>(&view));
        glUniformMatrix4fv(bowling_projection_location, 1, GL_FALSE, reinterpret_cast<float *>(&projection));
        glBindVertexArray(ball_vao);

        draw_obj(ball_shapes, ball_materials);

        glUniformMatrix4fv(bowling_model_location, 1, GL_FALSE, reinterpret_cast<float *>(&pin_model));
        glBindVertexArray(pin_vao);

        for(int i = 0; i < 10; i++) {
            glUniformMatrix4fv(bowling_transform_location, 1, GL_FALSE, reinterpret_cast<float *>(&pin_transforms[i]));
            draw_obj(pin_shapes, pin_materials);
        }

        std::vector<rp3d::Vector3> vertices(3 * triangles.size());
        for(int i = 0; i < triangles.size(); i++) {
            vertices[3 * i + 0] = triangles[i].point1;
            vertices[3 * i + 1] = triangles[i].point2;
            vertices[3 * i + 2] = triangles[i].point3;
        }
        glUseProgram(debug_program);
        glLineWidth(2.f);
        glUniformMatrix4fv(debug_model_location, 1, GL_FALSE, reinterpret_cast<float *>(&debug_model));
        glUniformMatrix4fv(debug_view_location, 1, GL_FALSE, reinterpret_cast<float *>(&view));
        glUniformMatrix4fv(debug_projection_location, 1, GL_FALSE, reinterpret_cast<float *>(&projection));

        /*
        glBindBuffer(GL_ARRAY_BUFFER, debug_vbo);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(rp3d::Vector3), vertices.data(), GL_STATIC_DRAW);
        glBindVertexArray(debug_vao);
        glDrawArrays(GL_TRIANGLES, 0, vertices.size());
        */

        vertices.resize(2 * lines.size());
        for(int i = 0; i < lines.size(); i++) {
            vertices[2 * i + 0] = lines[i].point1;
            vertices[2 * i + 1] = lines[i].point2;
        }
        glBindBuffer(GL_ARRAY_BUFFER, debug_vbo);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(rp3d::Vector3), vertices.data(), GL_STATIC_DRAW);
        glBindVertexArray(debug_vao);
        glDrawArrays(GL_LINES, 0, vertices.size());

        SDL_GL_SwapWindow(window);
    }

    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
}
catch (std::exception const &e)
{
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
}