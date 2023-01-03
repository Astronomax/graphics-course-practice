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

int main() try {
    auto *window = create_window("Final project");
    auto gl_context = create_context(window);
    int width, height;
    SDL_GetWindowSize(window, &width, &height);

    std::string project_root = PROJECT_ROOT;
    std::string ball_dir = project_root + "/ball/";
    std::string ball_path = ball_dir + "ball.obj";
    std::string pin_dir = project_root + "/pin/";
    std::string pin_path = pin_dir + "pin.obj";

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

    GLint model_location = glGetUniformLocation(bowling_program, "model");
    GLint transform_location = glGetUniformLocation(bowling_program, "transform");
    GLint view_location = glGetUniformLocation(bowling_program, "view");
    GLint projection_location = glGetUniformLocation(bowling_program, "projection");

    std::vector<vertex> ball_vertices;
    {
        tinyobj::attrib_t attrib;
        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> materials;
        tinyobj::LoadObj(&attrib, &shapes, &materials, nullptr, ball_path.c_str(), ball_dir.c_str());
        ball_vertices = get_vertices(attrib, shapes);
    }

    auto ball_bounding_box = get_bounding_box(ball_vertices);
    auto ball_center = std::accumulate(ball_bounding_box.begin(), ball_bounding_box.end(), glm::vec3(0.f)) / 8.f;

    float ball_height = get_bbox_size(ball_bounding_box).y;
    rp3d::RigidBody *ball = world->createRigidBody(rp3d::Transform(rp3d::Vector3(0.f, ball_height / 2.f + 0.2f + 0.01f, -3.f), rp3d::Quaternion::identity()));
    rp3d::SphereShape* ballShape = physicsCommon.createSphereShape(ball_height / 2.f);
    ball->addCollider(ballShape, rp3d::Transform(rp3d::Vector3(0, 0.0f, 0), rp3d::Quaternion::identity()));
    ball->setMass(10);

    std::vector<vertex> pin_vertices;
    {
        tinyobj::attrib_t attrib;
        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> materials;
        tinyobj::LoadObj(&attrib, &shapes, &materials, nullptr, pin_path.c_str(), pin_dir.c_str());
        pin_vertices = get_vertices(attrib, shapes);
    }

    auto pin_bounding_box = get_bounding_box(pin_vertices);
    auto pin_center = std::accumulate(pin_bounding_box.begin(), pin_bounding_box.end(), glm::vec3(0.f)) / 8.f;
    rp3d::Vector3 pin_size = get_bbox_size(ball_bounding_box);
    float pin_height = pin_size.y;


    std::vector<rp3d::RigidBody*> pins;

    float dz = 0.14f;
    for(int i = 1; i <= 4; i++) {
        float x = -(float)(i - 1) * dz;
        float z = (float)(i - 1) * dz;
        float dx = (2.f * z) / (float)(i - 1);
        for(int j = 0; j < i; j++) {
            pins.push_back(world->createRigidBody(rp3d::Transform(rp3d::Vector3(x, pin_height / 2.f + 0.2f + 0.01f, z), rp3d::Quaternion::identity())));
            rp3d::BoxShape *pinShape = physicsCommon.createBoxShape(get_bbox_size(pin_bounding_box) / 2.f);
            pins.back()->addCollider(pinShape, rp3d::Transform(rp3d::Vector3(0, 0.0f, 0), rp3d::Quaternion::identity()));
            pins.back()->setMass(2.5);
            x += dx;
        }
    }

    rp3d::RigidBody *floor = world->createRigidBody(rp3d::Transform(rp3d::Vector3(0, 0, 0), rp3d::Quaternion::identity()));
    rp3d::BoxShape* floorShape = physicsCommon.createBoxShape(rp3d::Vector3(6.f, 0.2f, 6.f));
    floor->addCollider(floorShape, rp3d::Transform(rp3d::Vector3(0, 0.0f, 0), rp3d::Quaternion::identity()));
    floor->updateMassPropertiesFromColliders();
    floor->setType(rp3d::BodyType::STATIC);

    glm::mat4 ball_model = glm::mat4(1.f);
    ball_model = glm::translate(ball_model, -ball_center);
    glm::mat4 pin_model = glm::mat4(1.f);
    pin_model = glm::translate(pin_model, -pin_center);

    glm::mat4 ball_transform = glm::mat4(1.f);
    ball->getTransform().getOpenGLMatrix(reinterpret_cast<float *>(&ball_transform));

    std::vector<glm::mat4> pin_transforms(10, glm::mat4(1.f));
    for(int i = 0; i < 10; i++) {
        pins[i]->getTransform().getOpenGLMatrix(reinterpret_cast<float *>(&pin_transforms[i]));
    }

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


    ball->applyLocalForceAtCenterOfMass(rp3d::Vector3(0.f, 0.f, 40000.f));


    auto debug_vertex_shader = create_shader(GL_VERTEX_SHADER, project_root + "/shaders/debug.vert");
    auto debug_fragment_shader = create_shader(GL_FRAGMENT_SHADER, project_root + "/shaders/debug.frag");
    auto debug_program = create_program(debug_vertex_shader, debug_fragment_shader);

    GLint _model_location = glGetUniformLocation(debug_program, "model");
    GLint _view_location = glGetUniformLocation(debug_program, "view");
    GLint _projection_location = glGetUniformLocation(debug_program, "projection");


    GLuint debug_vao, debug_vbo;
    glGenVertexArrays(1, &debug_vao);
    glBindVertexArray(debug_vao);

    glGenBuffers(1, &debug_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, debug_vbo);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(rp3d::Vector3), (void *) 0);


    auto last_frame_start = std::chrono::high_resolution_clock::now();

    float time = 0.f;

    std::map<SDL_Keycode, bool> button_down;

    float camera_distance = 6.f;
    auto camera_angle = glm::pi<float>() / 2.f, camera_elevation = glm::pi<float>() / 6.f;

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

        world->update(dt);
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


        float near = 0.1f;
        float far = 100.f;

        glm::mat4 model(1.f);

        glm::mat4 view(1.f);
        view = glm::translate(view, {0.f, 0.f, -camera_distance});
        view = glm::rotate(view, camera_elevation, {1.f, 0.f, 0.f});
        view = glm::rotate(view, camera_angle, {0.f, 1.f, 0.f});
        view = glm::translate(view, {0.f, -0.5f, 0.f});

        float aspect = (float)height / (float)width;
        glm::mat4 projection = glm::perspective(glm::pi<float>() / 3.f, 1.f / aspect, near, far);
        glm::vec3 camera_position = (glm::inverse(view) * glm::vec4(0.f, 0.f, 0.f, 1.f)).xyz();
        glm::vec3 sun_direction = glm::normalize(glm::vec3(std::sin(time * 0.5f), 2.f, std::cos(time * 0.5f)));

        auto light_Z = -sun_direction;
        auto light_X = glm::normalize(glm::cross(light_Z, glm::vec3(1, 0, 0)));
        auto light_Y = glm::normalize(glm::cross(light_X, light_Z));
        glm::mat4 shadow_projection = glm::mat4(glm::transpose(
                glm::mat3(light_X, light_Y, light_Z)));

        ball->getTransform().getOpenGLMatrix(reinterpret_cast<float *>(&ball_transform));
        for(int i = 0; i < 10; i++) {
            pins[i]->getTransform().getOpenGLMatrix(reinterpret_cast<float *>(&pin_transforms[i]));
        }
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
        glViewport(0, 0, width, height);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glClearColor(0.8f, 0.8f, 1.f, 0.f);

        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);

        glUseProgram(bowling_program);
        glUniformMatrix4fv(model_location, 1, GL_FALSE, reinterpret_cast<float *>(&ball_model));
        glUniformMatrix4fv(transform_location, 1, GL_FALSE, reinterpret_cast<float *>(&ball_transform));
        glUniformMatrix4fv(view_location, 1, GL_FALSE, reinterpret_cast<float *>(&view));
        glUniformMatrix4fv(projection_location, 1, GL_FALSE, reinterpret_cast<float *>(&projection));

        glBindVertexArray(ball_vao);
        glDrawArrays(GL_TRIANGLES, 0, ball_vertices.size());


        for(int i = 0; i < 10; i++) {
            glUniformMatrix4fv(model_location, 1, GL_FALSE, reinterpret_cast<float *>(&pin_model));
            glUniformMatrix4fv(transform_location, 1, GL_FALSE, reinterpret_cast<float *>(&pin_transforms[i]));
            glBindVertexArray(pin_vao);
            glDrawArrays(GL_TRIANGLES, 0, pin_vertices.size());
        }

/*
        std::vector<rp3d::Vector3> vertices(3 * triangles.size());
        for(int i = 0; i < triangles.size(); i++) {
            vertices[3 * i + 0] = triangles[i].point1;
            vertices[3 * i + 1] = triangles[i].point2;
            vertices[3 * i + 2] = triangles[i].point3;
        }

        glUseProgram(debug_program);
        glLineWidth(2.f);
        glUniformMatrix4fv(_model_location, 1, GL_FALSE, reinterpret_cast<float *>(&model));
        glUniformMatrix4fv(_view_location, 1, GL_FALSE, reinterpret_cast<float *>(&view));
        glUniformMatrix4fv(_projection_location, 1, GL_FALSE, reinterpret_cast<float *>(&projection));


        glBindBuffer(GL_ARRAY_BUFFER, debug_vbo);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(rp3d::Vector3), vertices.data(), GL_STATIC_DRAW);
        glBindVertexArray(debug_vao);
        glDrawArrays(GL_TRIANGLES, 0, vertices.size());
*/
/*
        vertices.resize(2 * lines.size());
        for(int i = 0; i < lines.size(); i++) {
            vertices[2 * i + 0] = lines[i].point1;
            vertices[2 * i + 1] = lines[i].point2;
        }

        glBindBuffer(GL_ARRAY_BUFFER, debug_vbo);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(rp3d::Vector3), vertices.data(), GL_STATIC_DRAW);
        glBindVertexArray(debug_vao);
        glDrawArrays(GL_LINES, 0, vertices.size());
*/

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
