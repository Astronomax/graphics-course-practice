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

std::string to_string(std::string_view str)
{
    return std::string(str.begin(), str.end());
}

void sdl2_fail(std::string_view message)
{
    throw std::runtime_error(to_string(message) + SDL_GetError());
}

void glew_fail(std::string_view message, GLenum error)
{
    throw std::runtime_error(to_string(message) + reinterpret_cast<const char *>(glewGetErrorString(error)));
}

const char vertex_shader_source[] =
R"(#version 330 core

/*Task 1, 2
uniform float scale, angle;
mat2 r = mat2(cos(angle), -sin(angle), sin(angle), cos(angle));
*/

//Task 3
uniform mat4 transform;
//Task 5
uniform mat4 view;

/*const vec2 VERTICES[3] = vec2[3](
    vec2(0.0, 1.0),
    vec2(-sqrt(0.75), -0.5),
    vec2( sqrt(0.75), -0.5)
);

const vec3 COLORS[3] = vec3[3](
    vec3(1.0, 0.0, 0.0),
    vec3(0.0, 1.0, 0.0),
    vec3(0.0, 0.0, 1.0)
);*/

//Task 6
const float pi = 3.14;

const vec2 VERTICES[8] = vec2[8](
    vec2(0.0, 0.0),
    vec2(0.0, 1.0),
    vec2(cos(pi / 6), sin(pi / 6)),
    vec2(cos(pi / 6), -sin(pi / 6)),
    vec2(0.0, -1.0),
    vec2(-cos(pi / 6), -sin(pi / 6)),
    vec2(-cos(pi / 6), sin(pi / 6)),
    vec2(0.0, 1.0)
);

const vec3 COLORS[8] = vec3[8](
    vec3(1.0, 1.0, 1.0),
    vec3(0.0, 0.0, 1.0),
    vec3(0.0, 1.0, 0.0),
    vec3(1.0, 0.0, 0.0),
    vec3(1.0, 1.0, 0.0),
    vec3(1.0, 0.0, 1.0),
    vec3(0.0, 1.0, 1.0),
    vec3(0.0, 0.0, 1.0)
);

out vec3 color;

void main()
{
    vec2 position = VERTICES[gl_VertexID];
    //gl_Position = transform * vec4(r * position * scale, 0.0, 1.0); Task 1
    gl_Position = view * transform * vec4(position, 0.0, 1.0);
    color = COLORS[gl_VertexID];
}
)";

const char fragment_shader_source[] =
R"(#version 330 core

in vec3 color;

layout (location = 0) out vec4 out_color;

void main()
{
    out_color = vec4(color, 1.0);
}
)";

GLuint create_shader(GLenum type, const char * source)
{
    GLuint result = glCreateShader(type);
    glShaderSource(result, 1, &source, nullptr);
    glCompileShader(result);
    GLint status;
    glGetShaderiv(result, GL_COMPILE_STATUS, &status);
    if (status != GL_TRUE)
    {
        GLint info_log_length;
        glGetShaderiv(result, GL_INFO_LOG_LENGTH, &info_log_length);
        std::string info_log(info_log_length, '\0');
        glGetShaderInfoLog(result, info_log.size(), nullptr, info_log.data());
        throw std::runtime_error("Shader compilation failed: " + info_log);
    }
    return result;
}

GLuint create_program(GLuint vertex_shader, GLuint fragment_shader)
{
    GLuint result = glCreateProgram();
    glAttachShader(result, vertex_shader);
    glAttachShader(result, fragment_shader);
    glLinkProgram(result);

    GLint status;
    glGetProgramiv(result, GL_LINK_STATUS, &status);
    if (status != GL_TRUE)
    {
        GLint info_log_length;
        glGetProgramiv(result, GL_INFO_LOG_LENGTH, &info_log_length);
        std::string info_log(info_log_length, '\0');
        glGetProgramInfoLog(result, info_log.size(), nullptr, info_log.data());
        throw std::runtime_error("Program linkage failed: " + info_log);
    }

    return result;
}

template<std::size_t N>
std::vector<GLdouble> mult(const std::vector<GLdouble>& a,
                           const std::vector<GLdouble> &b) {
    std::vector<GLdouble> res(N * N);
    for(int i = 0; i < N; i++)
        for(int j = 0; j < N; j++)
            for(int k = 0; k < N; k++)
                res[i * N + j] += a[i * N + k] * b[k * N + j];
    return res;
}

int main() try
{
    if (SDL_Init(SDL_INIT_VIDEO) != 0)
        sdl2_fail("SDL_Init: ");

    SDL_Window * window = SDL_CreateWindow("Graphics course practice 2",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        800, 600,
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_MAXIMIZED);

    if (!window)
        sdl2_fail("SDL_CreateWindow: ");

    int width, height;
    SDL_GetWindowSize(window, &width, &height);

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    SDL_GL_SetSwapInterval(0);

    if (!gl_context)
        sdl2_fail("SDL_GL_CreateContext: ");

    if (auto result = glewInit(); result != GLEW_NO_ERROR)
        glew_fail("glewInit: ", result);

    if (!GLEW_VERSION_3_3)
        throw std::runtime_error("OpenGL 3.3 is not supported");

    glClearColor(0.8f, 0.8f, 1.f, 0.f);

    GLuint vertex_shader = create_shader(GL_VERTEX_SHADER, vertex_shader_source);
    GLuint fragment_shader = create_shader(GL_FRAGMENT_SHADER, fragment_shader_source);

    GLuint program = create_program(vertex_shader, fragment_shader);
    glUseProgram(program);
    glUniform1f(glGetUniformLocation(program, "scale"), 0.5);
    GLuint vao;
    glGenVertexArrays(1, &vao);

    auto last_frame_start = std::chrono::high_resolution_clock::now();

    GLdouble time = 0.f;

    //GLint angle_id = glGetUniformLocation(program, "angle"); Task 1
    GLint transform_id = glGetUniformLocation(program, "transform"); //Task 2
    GLint view_id = glGetUniformLocation(program, "view"); //Task 58

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
        }

        if (!running)
            break;

        auto now = std::chrono::high_resolution_clock::now();
        //GLdouble dt = std::chrono::duration_cast<std::chrono::duration<float>>(now - last_frame_start).count();
        GLdouble dt = 0.016f; //Task 5

        last_frame_start = now;

        glClear(GL_COLOR_BUFFER_BIT);

        //glUniform1f(angle_id, time); Task 1
        /*Task 3
        float transform[16] = {
                0.5f * cos(time), -0.5f * sin(time), 0, 0,
                0.5f * sin(time), 0.5f * cos(time), 0, 0
                0, 0, 1, 0,
                0, 0, 0, 1
        };*/
        //Task 4
        GLdouble tx = time * 0.01f, ty = 0;
        std::vector<GLdouble> scale = {
                0.5f, 0, 0, 0,
                0, 0.5f, 0, 0,
                0, 0, 1, 0,
                0, 0, 0, 1
        };
        std::vector<GLdouble> shift = {
                1, 0, 0, tx,
                0, 1, 0, ty,
                0, 0, 1, 0,
                0, 0, 0, 1
        };
        std::vector<GLdouble> rotation = {
                cos(time), -sin(time), 0, 0,
                sin(time), cos(time), 0, 0,
                0, 0, 1, 0,
                0, 0, 0, 1
        };
        std::vector<GLdouble> transform_ = mult<4>(scale, mult<4>(rotation, shift));
        std::unique_ptr<GLfloat []> transform(new GLfloat[16]);
        std::copy(transform_.begin(), transform_.end(), transform.get());
        glUniformMatrix4fv(transform_id, 1, GL_TRUE, transform.get());

        //Task 5
        GLfloat view[16] = {
                (GLfloat)height / (GLfloat)width, 0, 0, 0,
                0, 1, 0, 0,
                0, 0, 1, 0,
                0, 0, 0, 1
        };
        glUniformMatrix4fv(view_id, 1, GL_TRUE, view);

        glBindVertexArray(vao);
        //glDrawArrays(GL_TRIANGLES, 0, 3);
        glDrawArrays(GL_TRIANGLE_FAN, 0, 8);
        SDL_GL_SwapWindow(window);
        time += dt;
        //std::cout << dt << "\n"; Task 5
    }
    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
}
catch (std::exception const & e)
{
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
}
