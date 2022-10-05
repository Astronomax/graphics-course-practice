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
#include <random>

const float view[16] = {
        2.f, 0.f, 0.f, -1.f,
        0.f, 2.f, 0.f, -1.f,
        0.f, 0.f, 1.f, 0.f,
        0.f, 0.f, 0.f, 1.f,
};

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
layout (location = 0) in vec2 in_position;
layout (location = 1) in vec4 in_color;
out vec4 color;
void main() {
    gl_Position = view * vec4(in_position, 0.0, 1.0);
    color = in_color;
}
)";

const char fragment_shader_source[] =
        R"(#version 330 core
in vec4 color;
layout (location = 0) out vec4 out_color;
void main() {
    out_color = color;
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
        glGetShaderInfoLog(result, (GLsizei) info_log.size(), nullptr, info_log.data());
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
        glGetProgramInfoLog(result, (GLsizei) info_log.size(), nullptr, info_log.data());
        throw std::runtime_error("Program linkage failed: " + info_log);
    }
    return result;
}

struct vec2 {
    float x, y;
};

struct vertex {
    vec2 position;
    std::uint8_t color[4];
};

int quality = 8, N = 3;
float step = 0.2f, eps = 1e-9;

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
    glLineWidth(2.5f);

    auto vertex_shader = create_shader(GL_VERTEX_SHADER, vertex_shader_source);
    auto fragment_shader = create_shader(GL_FRAGMENT_SHADER, fragment_shader_source);
    auto program = create_program(vertex_shader, fragment_shader);

    std::random_device random_device;
    std::default_random_engine random_engine(random_device());
    std::uniform_real_distribution<float> uniform(0, 1);

    std::vector<std::array<std::uint8_t, 4>> c(N);
    c[0] = {255, 0, 0, 255};
    c[1] = {255, 255, 0, 255};
    c[2] = {0, 255, 255, 255};

    std::vector<std::pair<float, float>> start_positions(N);
    std::vector<std::pair<float, float>> move_vectors(N);
    std::vector<std::pair<float, float>> current_positions(N);

    for (int i = 0; i < N; i++) {
        start_positions[i].first = uniform(random_engine);
        start_positions[i].second = uniform(random_engine);
        move_vectors[i].first = uniform(random_engine);
        move_vectors[i].second = uniform(random_engine);
    }

    std::vector<vertex> vertices((quality + 1) * (quality + 1));
    std::vector<int> indices(6 * (quality + 1) * (quality + 1));

    auto update_vertices = [&]() {
        vertices.assign((quality + 1) * (quality + 1), {});
        indices.assign(6 * (quality + 1) * (quality + 1), 0);

        for (int i = 0; i <= quality; i++) {
            for (int j = 0; j <= quality; j++) {
                int ind = i * (quality + 1) + j;
                vertices[ind].position.x = (float) j / (float) quality;
                vertices[ind].position.y = (float) i / (float) quality;
            }
        }
        for (int i = 0; i < quality; i++) {
            for (int j = 0; j < quality; j++) {
                int ind = i * quality + j;
                indices[ind * 6] = ind + i;
                indices[ind * 6 + 1] = ind + i + 1;
                indices[ind * 6 + 2] = ind + i + quality + 1;
                indices[ind * 6 + 3] = ind + i + quality + 1;
                indices[ind * 6 + 4] = ind + i + 1;
                indices[ind * 6 + 5] = ind + i + quality + 2;
            }
        }
    };
    update_vertices();
    GLuint view_location = glGetUniformLocation(program, "view");

    GLuint vbo[2], ebo[2], vao[2];
    glGenBuffers(2, vbo);
    glGenBuffers(2, ebo);
    glGenVertexArrays(2, vao);
    for (int i = 0; i < 2; i++) {
        glBindBuffer(GL_ARRAY_BUFFER, vbo[i]);
        glBindVertexArray(vao[i]);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, false, sizeof(vertex), (GLvoid *) 0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, true, sizeof(vertex), (GLvoid *) 8);
    }

    auto start_time = std::chrono::high_resolution_clock::now();
    while (true) {
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
                            glViewport(0, 0, width, height);
                            break;
                    }
                    break;
                case SDL_MOUSEBUTTONDOWN:
                    break;
                case SDL_KEYDOWN:
                    if (event.key.keysym.sym == SDLK_UP) {
                        if (step > 0.1f + eps)
                            step -= 0.1f;
                    } else if (event.key.keysym.sym == SDLK_DOWN) {
                        if (step < 10.f - eps)
                            step += 0.1f;
                    } else if (event.key.keysym.sym == SDLK_LEFT) {
                        if (quality > 1) {
                            --quality;
                            update_vertices();
                        }
                        std::cout << quality << std::endl;
                    } else if (event.key.keysym.sym == SDLK_RIGHT) {
                        if (quality < 200) {
                            ++quality;
                            update_vertices();
                        }
                        std::cout << quality << std::endl;
                    }
                    break;
                default:
                    break;
            }
        }
        if (!running) break;

        auto now = std::chrono::high_resolution_clock::now();
        float time = std::chrono::duration_cast<std::chrono::duration<float>>(now - start_time).count();;

        for (int i = 0; i < N; i++) {
            current_positions[i].first = start_positions[i].first + 0.001f * time * move_vectors[i].first;
            current_positions[i].second = start_positions[i].second + 0.001f * time * move_vectors[i].second;
            current_positions[i].first -= 2.f * floor(current_positions[i].first / 2.f);
            current_positions[i].second -= 2.f * floor(current_positions[i].second / 2.f);
            if (current_positions[i].first > 1.f)
                current_positions[i].first = 2.f - current_positions[i].first;
            if (current_positions[i].second > 1.f)
                current_positions[i].second = 2.f - current_positions[i].second;
        }
        std::vector<float> f((quality + 1) * (quality + 1));
        for (int i = 0; i <= quality; i++) {
            for (int j = 0; j <= quality; j++) {
                int ind = i * (quality + 1) + j;
                for (int k = 0; k < N; k++) {
                    float dx = vertices[ind].position.x - current_positions[k].first;
                    float dy = vertices[ind].position.y - current_positions[k].second;
                    f[ind] += exp(dx * dx + dy * dy);
                }
                vertices[ind].color[0] = (std::uint8_t) (255.f * (cos(f[ind]) + 1.f) / 2.f);
                vertices[ind].color[1] = (std::uint8_t) (255.f * (sin(f[ind]) + 1.f) / 2.f);
                vertices[ind].color[2] = (std::uint8_t) (255.f * (cos(f[ind] + M_PI / 2.f) + 1.f) / 2.f);
                vertices[ind].color[3] = 255;
            }
        }
        std::vector<vertex> line_points;
        for (int i = 0; i < quality; i++) {
            for (int j = 0; j < quality; j++) {
                int corners[2][3] = {{i * (quality + 1) + j + 1,
                                             i * (quality + 1) + j,
                                             (i + 1) * (quality + 1) + j},
                                     {i * (quality + 1) + j + 1,
                                             (i + 1) * (quality + 1) + j,
                                             (i + 1) * (quality + 1) + j + 1}};
                for (auto &corner: corners) {
                    float max_f = 0.f;
                    for (int k: corner)
                        max_f = std::max(max_f, f[k]);
                    float value = step * floor(max_f / step);
                    std::cout << value << std::endl;
                    bool near_value = true;
                    for (int k: corner)
                        if (abs(f[k] - value) > step)
                            near_value = false;

                    if (!near_value) continue;
                    for (int k = 0; k < 3; k++) {
                        if ((f[corner[(k + 1) % 3]] - value > eps &&
                             f[corner[(k + 2) % 3]] - value > eps &&
                             value - f[corner[k]] > eps) ||
                            (value - f[corner[(k + 1) % 3]] > eps &&
                             value - f[corner[(k + 2) % 3]] > eps &&
                             f[corner[k]] - value > eps)) {
                            float d[2] = {
                                    (f[corner[k]] - value) / (f[corner[k]] - f[corner[(k + 1) % 3]]),
                                    (f[corner[k]] - value) / (f[corner[k]] - f[corner[(k + 2) % 3]]),
                            };
                            float x[2][2] = {
                                    {vertices[corner[k]].position.x,
                                     vertices[corner[(k + 1) % 3]].position.x},
                                    {vertices[corner[k]].position.x,
                                     vertices[corner[(k + 2) % 3]].position.x}
                            };
                            float y[2][2] = {
                                    {vertices[corner[k]].position.y,
                                     vertices[corner[(k + 1) % 3]].position.y},
                                    {vertices[corner[k]].position.y,
                                     vertices[corner[(k + 2) % 3]].position.y}
                            };
                            for (int q = 0; q < 2; q++)
                                line_points.push_back({{(x[q][0] * d[q] + x[q][1] * (1.f - d[q])),
                                                               (y[1][0] * d[q] + y[q][1] * (1.f - d[q]))},
                                                        {0,0,0,255}});
                        }
                    }
                }
            }
        }
        std::vector<int> line_indices(line_points.size());
        for (int i = 0; i < line_points.size(); i++)
            line_indices[i] = i;
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(program);
        glUniformMatrix4fv(view_location, 1, GL_TRUE, view);

        glBindVertexArray(vao[0]);
        glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(vertex), vertices.data(), GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo[0]);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(std::uint32_t), indices.data(), GL_STATIC_DRAW);
        glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);

        glBindVertexArray(vao[1]);
        glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
        glBufferData(GL_ARRAY_BUFFER, line_points.size() * sizeof(vertex), line_points.data(), GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo[1]);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, line_indices.size() * sizeof(std::uint32_t), line_indices.data(),
                     GL_STATIC_DRAW);
        glDrawElements(GL_LINES, line_indices.size(), GL_UNSIGNED_INT, 0);

        SDL_GL_SwapWindow(window);
    }
    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
}
catch (std::exception const &e) {
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
}