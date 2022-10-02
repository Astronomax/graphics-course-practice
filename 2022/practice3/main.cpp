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
#include <memory>

const GLint sizes[3] = {2, 4, 1};
const GLvoid *pointers[3] = {(GLvoid *) 0, (GLvoid *) 8, (GLvoid *) 12};
const GLenum types[3] = {GL_FLOAT, GL_UNSIGNED_BYTE, GL_FLOAT};
const std::uint8_t colors[2][4] = {{0, 0, 0, 255},
                                   {255, 0, 0, 255}};

std::string to_string(std::string_view str) {
    return std::string(str.begin(), str.end());
}

void sdl2_fail(std::string_view message) {
    throw std::runtime_error(to_string(message) + SDL_GetError());
}

void glew_fail(std::string_view message, GLenum error) {
    throw std::runtime_error(to_string(message) + reinterpret_cast<const char *>(glewGetErrorString(error)));
}

const char vertex_shader_source[] =
        R"(#version 330 core

uniform mat4 view;
uniform float time;

layout (location = 0) in vec2 in_position;
layout (location = 1) in vec4 in_color;
layout (location = 2) in float in_dist;

out vec4 color;
out float dist;

void main() {
    gl_Position = view * vec4(in_position, 0.0, 1.0);
    color = in_color;
    dist = in_dist + time;
}
)";

const char fragment_shader_source[] =
        R"(#version 330 core

uniform int check_dist;

in vec4 color;
in float dist;

layout (location = 0) out vec4 out_color;

void main() {
    out_color = color;
    if(check_dist == 1 && mod(dist, 40.0) < 20.0) {
        discard;
    }
}
)";

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

struct vec2 {
    float x;
    float y;
};

struct vertex {
    vec2 position;
    std::uint8_t color[4];
    float dist;
};

vec2 bezier(std::vector<vertex> const &vertices, float t) {
    std::vector<vec2> points(vertices.size());

    for (std::size_t i = 0; i < vertices.size(); ++i)
        points[i] = vertices[i].position;

    // De Casteljau's algorithm
    for (std::size_t k = 0; k + 1 < vertices.size(); ++k) {
        for (std::size_t i = 0; i + k + 1 < vertices.size(); ++i) {
            points[i].x = points[i].x * (1.f - t) + points[i + 1].x * t;
            points[i].y = points[i].y * (1.f - t) + points[i + 1].y * t;
        }
    }
    return points[0];
}

int m_quality = 4;
std::array<GLuint, 2> m_buffers, m_arrays;
std::vector<vertex> m_polyline, m_bezier;

float get_dist(vertex prev, vec2 cur_position) {
    float prev_dist = prev.dist;
    vec2 prev_position = prev.position;
    return prev_dist + std::hypot(prev_position.x - cur_position.x,
                                  prev_position.y - cur_position.y);
}

void update_vbo(GLuint buffer, const std::vector<vertex> &vertices) {
    std::unique_ptr<vertex[]> _vertices(new vertex[vertices.size()]);
    std::copy(vertices.begin(), vertices.end(), _vertices.get());
    glBindBuffer(GL_ARRAY_BUFFER, buffer);
    glBufferData(GL_ARRAY_BUFFER, (int) vertices.size() * (int) sizeof(vertex), _vertices.get(), GL_STATIC_DRAW);
}

void update_bezier() {
    m_bezier.clear();
    for (int i = 0; i < m_polyline.size() * m_quality; i++) {
        float t = (float) i / ((float) m_polyline.size() * (float) m_quality - 1.f);
        vertex v{};
        v.position = bezier(m_polyline, t);
        v.dist = (m_bezier.empty()) ? 0.f : get_dist(m_bezier.back(), v.position);
        std::copy(colors[1], colors[1] + 4, v.color);
        m_bezier.push_back(v);
    }
}

void update() {
    update_bezier();
    update_vbo(m_buffers[0], m_polyline);
    update_vbo(m_buffers[1], m_bezier);
}

void add_vertex(vec2 position) {
    vertex v{};
    v.position = position;
    v.dist = (m_polyline.empty()) ? 0.f : get_dist(m_polyline.back(), v.position);
    std::copy(colors[0], colors[0] + 4, v.color);
    m_polyline.push_back(v);
    update();
}

void pop_vertex() {
    if (m_polyline.empty()) return;
    m_polyline.pop_back();
    update();
}

//Task 7
void incr_quality() {
    if (m_quality > 1) {
        --m_quality;
        update();
    }
}

void decr_quality() {
    ++m_quality;
    update();
}

int main() try {
    if (SDL_Init(SDL_INIT_VIDEO) != 0)
        sdl2_fail("SDL_Init: ");

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4);

    SDL_Window *window = SDL_CreateWindow("Graphics course practice 3",
                                          SDL_WINDOWPOS_CENTERED,
                                          SDL_WINDOWPOS_CENTERED,
                                          800, 600,
                                          SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_MAXIMIZED);

    if (!window)
        sdl2_fail("SDL_CreateWindow: ");

    int width, height;
    SDL_GetWindowSize(window, &width, &height);

    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    if (!gl_context)
        sdl2_fail("SDL_GL_CreateContext: ");

    SDL_GL_SetSwapInterval(0);

    if (auto result = glewInit(); result != GLEW_NO_ERROR)
        glew_fail("glewInit: ", result);

    if (!GLEW_VERSION_3_3)
        throw std::runtime_error("OpenGL 3.3 is not supported");

    glClearColor(0.8f, 0.8f, 1.f, 0.f);

    auto vertex_shader = create_shader(GL_VERTEX_SHADER, vertex_shader_source);
    auto fragment_shader = create_shader(GL_FRAGMENT_SHADER, fragment_shader_source);
    auto program = create_program(vertex_shader, fragment_shader);

    GLint view_location = glGetUniformLocation(program, "view");
    auto last_frame_start = std::chrono::high_resolution_clock::now();

    float time = 0.f;

    //Task 1
    /*
    vertex vertices[3] = {
            {{0, 0}, {1, 0, 0, 1}},
            {{(float)width, 0}, {0, 1, 0, 1}},
            {{0, (float)height}, {0, 0, 1, 1}}
    };
    GLuint buffer;
    glGenBuffers(1, &buffer);
    glBindBuffer(GL_ARRAY_BUFFER, buffer);
    glBufferData(GL_ARRAY_BUFFER, 3 * sizeof(vertex), vertices, GL_DYNAMIC_DRAW);
    vertex first_vertex{};
    for(int i = 0; i < 3; i++) {
        glGetBufferSubData(GL_ARRAY_BUFFER, i * sizeof(vertex), sizeof(vertex), &first_vertex);
        std::cout << first_vertex.position.x << " "
                  << first_vertex.position.y << " "
                  << (int32_t) first_vertex.color[0] << " "
                  << (int32_t) first_vertex.color[1] << " "
                  << (int32_t) first_vertex.color[2] << " "
                  << (int32_t) first_vertex.color[3] << "\n";
    }
    */
    //Task 1

    //Task 2
    /*
    GLuint array;
    glGenVertexArrays(1, &array);
    glBindVertexArray(array);
    GLint sizes[2] = {2, 4};
    GLvoid* pointers[2] = {(GLvoid*)0, (GLvoid*)8};
    GLenum types[2] = { GL_FLOAT, GL_UNSIGNED_BYTE };
    for (GLuint i = 0; i < 2; i++) {
        glEnableVertexAttribArray(i);
        glVertexAttribPointer(i, sizes[i], types[i], false, sizeof(vertex), pointers[i]);
    }
    */
    //Task 2

    //Task 4
    /*
    GLuint buffer, array;
    glGenBuffers(1, &buffer);
    glBindBuffer(GL_ARRAY_BUFFER, buffer);
    glGenVertexArrays(1, &array);
    glBindVertexArray(array);
    const GLint sizes[2] = {2, 4};
    const GLvoid* pointers[2] = {(GLvoid*)0, (GLvoid*)8};
    const GLenum types[2] = { GL_FLOAT, GL_UNSIGNED_BYTE };
    for (GLuint i = 0; i < 2; i++) {
        glEnableVertexAttribArray(i);
        glVertexAttribPointer(i, sizes[i], types[i], false, sizeof(vertex), pointers[i]);
    }
    std::vector<vertex> vertices;
    */
    glLineWidth(3.f);
    //Task 4
    glPointSize(10.f); //Task 5


    //Task 6
    GLuint _buffers[2], _arrays[2];
    glGenBuffers(2, _buffers);
    glGenVertexArrays(2, _arrays);
    m_buffers = {_buffers[0], _buffers[1]};
    m_arrays = {_arrays[0], _arrays[1]};

    glBindBuffer(GL_ARRAY_BUFFER, m_buffers[0]);
    glBindVertexArray(m_arrays[0]);
    for (GLuint i = 0; i < 3; i++) {
        glEnableVertexAttribArray(i);
        glVertexAttribPointer(i, sizes[i], types[i], false, sizeof(vertex), pointers[i]);
    }
    glBindBuffer(GL_ARRAY_BUFFER, m_buffers[1]);
    glBindVertexArray(m_arrays[1]);
    for (GLuint i = 0; i < 3; i++) {
        glEnableVertexAttribArray(i);
        glVertexAttribPointer(i, sizes[i], types[i], false, sizeof(vertex), pointers[i]);
    }
    //Task 6

    //Task 7
    GLint time_location = glGetUniformLocation(program, "time");
    GLint check_dist_location = glGetUniformLocation(program, "check_dist");

    bool running = true;
    while (running) {
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
                            glViewport(0, 0, width, height);
                            break;
                    }
                    break;
                case SDL_MOUSEBUTTONDOWN:
                    if (event.button.button == SDL_BUTTON_LEFT) {
                        add_vertex({(float) event.button.x,
                                    (float) event.button.y});
                        std::cout << event.button.x << " " << event.button.y << "\n";
                    }
                    else if (event.button.button == SDL_BUTTON_RIGHT)
                        pop_vertex();
                    break;
                case SDL_KEYDOWN:
                    if (event.key.keysym.sym == SDLK_LEFT)
                        incr_quality();
                    else if (event.key.keysym.sym == SDLK_RIGHT)
                        decr_quality();
                    break;
            }

        if (!running)
            break;

        auto now = std::chrono::high_resolution_clock::now();
        float dt = std::chrono::duration_cast<std::chrono::duration<float>>(now - last_frame_start).count();
        last_frame_start = now;
        time += dt;

        glClear(GL_COLOR_BUFFER_BIT);

        //Task 3
        float view[16] = {
                2.f / (float) width, 0.f, 0.f, -1.f,
                0.f, -2.f / (float) height, 0.f, 1.f,
                0.f, 0.f, 1.f, 0.f,
                0.f, 0.f, 0.f, 1.f,
        };
        //Task 3

        glUseProgram(program);
        glUniformMatrix4fv(view_location, 1, GL_TRUE, view);
        glUniform1f(time_location, time * 100.f);
        //glDrawArrays(GL_TRIANGLES, 0, 3);
        glBindBuffer(GL_ARRAY_BUFFER, m_buffers[0]);
        glBufferData(GL_ARRAY_BUFFER, (int) m_polyline.size() * (int) sizeof(vertex), m_polyline.data(), GL_STATIC_DRAW);

        glBindVertexArray(m_arrays[0]);
        glUniform1i(check_dist_location, 0);
        glDrawArrays(GL_LINE_STRIP, 0, m_polyline.size());  //Task 4
        glDrawArrays(GL_POINTS, 0, m_polyline.size());  //Task 5
        glBindVertexArray(m_arrays[1]);
        glUniform1i(check_dist_location, 1);
        glDrawArrays(GL_LINE_STRIP, 0, m_bezier.size());  //Task 6

        SDL_GL_SwapWindow(window);
    }
    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
}
catch (std::exception const &e) {
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
}
