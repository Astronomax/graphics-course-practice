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
#include <memory>

std::string to_string(std::string_view str) {
    return std::string(str.begin(), str.end());
}

void sdl2_fail(std::string_view message) {
    throw std::runtime_error(to_string(message) + SDL_GetError());
}

void glew_fail(std::string_view message, GLenum error) {
    throw std::runtime_error(to_string(message) + reinterpret_cast<const char *>(glewGetErrorString(error)));
}

GLuint create_shader(GLenum shader_type, const GLchar *shader_source) {
    GLuint shader = glCreateShader(shader_type);
    if (!shader) throw std::runtime_error("fail!");
    glShaderSource(shader, 1, &shader_source, nullptr);
    glCompileShader(shader);
    GLint status;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if (!status) {
        GLint length;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
        std::unique_ptr<char[]> log(new char[length]);
        glGetShaderInfoLog(shader, length, nullptr, log.get());
        throw std::runtime_error(log.get());
    }
    return shader;
}

GLuint create_program(GLuint vertex_shader, GLuint fragment_shader) {
    GLuint program = glCreateProgram();
    if (!program) throw std::runtime_error("fail!");
    glAttachShader(program, vertex_shader);
    glAttachShader(program, fragment_shader);
    glLinkProgram(program);
    GLint status;
    glGetProgramiv(program, GL_LINK_STATUS, &status);
    if (!status) {
        GLint length;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);
        std::unique_ptr<char[]> log(new char[length]);
        glGetProgramInfoLog(program, length, nullptr, log.get());
        throw std::runtime_error(log.get());
    }
    return program;
}

int main() try {
    if (SDL_Init(SDL_INIT_VIDEO) != 0)
        sdl2_fail("SDL_Init: ");

    SDL_Window *window = SDL_CreateWindow("Graphics course practice 1",
                                          SDL_WINDOWPOS_CENTERED,
                                          SDL_WINDOWPOS_CENTERED,
                                          800, 600,
                                          SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_MAXIMIZED);

    if (!window)
        sdl2_fail("SDL_CreateWindow: ");

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    if (!gl_context)
        sdl2_fail("SDL_GL_CreateContext: ");

    if (auto result = glewInit(); result != GLEW_NO_ERROR)
        glew_fail("glewInit: ", result);

    if (!GLEW_VERSION_3_3)
        throw std::runtime_error("OpenGL 3.3 is not supported");

    const GLchar *fragment_shader_source = R"(#version 330 core
        layout (location = 0) out vec4 out_color;
        //flat in vec3 color;
        in vec2 position;

        const vec3 COLORS[2] = vec3[2](
            vec3(0.0, 0.0, 0.0),
            vec3(1.0, 1.0, 1.0)
        );

        void main(){
        // vec4(R, G, B, A)
        out_color = vec4(COLORS[int(floor(position.x * 16) + floor(position.y * 16)) % 2], 1.0);
        })";

    GLuint fragment_shader = create_shader(GL_FRAGMENT_SHADER, fragment_shader_source);

    const GLchar *vertex_shader_source = R"(#version 330 core
        const vec2 VERTICES[3] = vec2[3](
        vec2(0.0, 0.0),
        vec2(1.0, 0.0),
        vec2(0.0, 1.0)
        );

        const vec3 COLORS[3] = vec3[3](
        vec3(0.0, 0.0, 0.0),
        vec3(1.0, 0.0, 0.0),
        vec3(0.0, 1.0, 0.0)
        );

        //flat out vec3 color;
        out vec2 position;

        void main() {
            gl_Position = vec4(VERTICES[gl_VertexID], 0.0, 1.0);
            position = VERTICES[gl_VertexID];
            //color = COLORS[gl_VertexID];
        })";

    GLuint vertex_shader = create_shader(GL_VERTEX_SHADER, vertex_shader_source);

    GLuint program = create_program(vertex_shader, fragment_shader);

    GLuint arr;
    glGenVertexArrays(1, &arr);
    glClearColor(0.8f, 0.8f, 1.f, 0.f);

    bool running = true;
    while (running) {
        for (SDL_Event event; SDL_PollEvent(&event);)
            switch (event.type) {
                case SDL_QUIT:
                    running = false;
                    break;
            }

        if (!running)
            break;

        glClear(GL_COLOR_BUFFER_BIT);
        glUseProgram(program);
        glBindVertexArray(arr);
        glDrawArrays(GL_TRIANGLES, 0, 3);
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
