#include <GLFW/glfw3.h>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <sys/stat.h>

GLfloat vert_data[] = {
  -1.0, -1.0,
  1.0, -1.0,
  -1.0, 1.0,

  1.0, 1.0,
  -1.0, 1.0,
  1.0, -1.0
};

#define POSITION_ATTRIB 0
#define COLOR_ATTRIB 1

int nbFrames = 0, lastFrames = 0;
unsigned int res = 2;
double lastTime = 0.0;
double startTime = 0.0;

bool paused = false;

#ifndef TARGET_H
#define TARGET_H
class Target {
    public:
        GLuint framebuffer, renderbuffer, texture;
        Target(GLsizei, GLsizei);
        ~Target();
};

Target::Target(GLsizei width, GLsizei height) {
    // Generate framebuffer, renderbuffer, and texture
    glGenFramebuffers(1, &framebuffer);
    glGenRenderbuffers(1, &renderbuffer);
    glGenTextures(1, &texture);

    // set up framebuffer
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0 );

    // set up renderbuffer

    glBindRenderbuffer( GL_RENDERBUFFER, renderbuffer );

    glRenderbufferStorage( GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, width, height );
    glFramebufferRenderbuffer( GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, renderbuffer );

    // clean up

    glBindTexture( GL_TEXTURE_2D, 0 );
    glBindRenderbuffer( GL_RENDERBUFFER, 0 );
    glBindFramebuffer( GL_FRAMEBUFFER, 0);

}

Target::~Target() {
    glDeleteFramebuffers(1, &framebuffer);
    glDeleteRenderbuffers(1, &renderbuffer);
    glDeleteTextures(1, &texture);
}
#endif // TARGET_H

double currentTime;
// https://stackoverflow.com/questions/18412120/displaying-fps-in-glfw-window-title
void setWindowFPS (GLFWwindow* win) {
    // Measure speed
    if(!paused) {
        currentTime = glfwGetTime() - startTime;
        nbFrames++;

        if ( currentTime - lastTime >= 1.0 ) {
            lastFrames = nbFrames;

            nbFrames = 0;
            lastTime += 1.0;
        }
    }

    char title [256];
    title [255] = '\0';

    snprintf (title, 255, "%s - %ds - [FPS: %d] @ %dx %s", "GLSL Playground", (int) currentTime, lastFrames, res, paused ? "(PAUSED)" : "");

    glfwSetWindowTitle (win, title);
}

static void error_callback(int error, const char* description)
{
    fputs(description, stderr);
}

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        printf("Exiting... (closed by user via Esc key)\n");
        glfwSetWindowShouldClose(window, GL_TRUE);
    }

    else if(key == GLFW_KEY_1 && action == GLFW_PRESS) {
        printf("Switched to 1x\n");
        res = 1;
    }

    else if(key == GLFW_KEY_2 && action == GLFW_PRESS) {
        printf("Switched to 2x\n");
        res = 2;
    }

    else if(key == GLFW_KEY_3 && action == GLFW_PRESS) {
        printf("Switched to 4x\n");
        res = 4;
    }

    else if(key == GLFW_KEY_4 && action == GLFW_PRESS) {
        printf("Switched to 8x\n");
        res = 8;
    } else if(key == GLFW_KEY_R && action == GLFW_PRESS) {
        printf("Resetted time to zero.\n");
        startTime = glfwGetTime();
    } else if(key == GLFW_KEY_SPACE && action == GLFW_PRESS) {
        paused = !paused;

        printf(paused ? "Paused GLSL Playground\n" : "Resuming GLSL Playground\n");

        if(paused) {
            startTime = glfwGetTime() - startTime;
        }
    }
}

double _iMouseX, _iMouseY;

static void mouse_callback(GLFWwindow * window, int button, int action, int mods) {
    if(button == GLFW_MOUSE_BUTTON_1 && action == GLFW_PRESS) {
        glfwGetCursorPos(window, &_iMouseX, &_iMouseY);
    }
}

    GLuint prog;
    GLuint surfaceProg;
    GLuint vert;
    GLuint frag;

off_t fsize(const char *filename) {
    struct stat st;

    if (stat(filename, &st) == 0)
        return st.st_size;

    return -1;
}

void shaderLoadSources(const char ** filePaths, int numOfFiles, GLuint * shaderID, GLint type) {
    int len = 0;
    int prev_len = 0;

    for(int i = 0; i < numOfFiles; i++) {
            len += fsize(filePaths[i]);
    }

    char * result = (char *) malloc((len + 1) * sizeof(char));

    for(int i = 0; i < numOfFiles; i++) {
        FILE * s_file = fopen(filePaths[i], "r");

        fread(&result[prev_len], sizeof(char), len * sizeof(char), s_file);
        fclose(s_file);
        prev_len += fsize(filePaths[i]);
    }

    result[len] = '\0';

    *shaderID = glCreateShader(type);
    glShaderSource(*shaderID, 1, &result, 0);
    glCompileShader(*shaderID);

    GLint compileStatus;
    glGetShaderiv(*shaderID, GL_COMPILE_STATUS, &compileStatus);

    if(compileStatus != GL_TRUE) {
        printf("Failed to compile %s shader:\n", type == GL_VERTEX_SHADER ? "vertex" : type == GL_FRAGMENT_SHADER ? "fragment" : "unknown");
        GLint infoLogLength;
        glGetShaderiv(*shaderID, GL_INFO_LOG_LENGTH, &infoLogLength);
        GLchar * infoLog = new GLchar[infoLogLength + 1];
        glGetShaderInfoLog(*shaderID, infoLogLength + 1, NULL, infoLog);
        printf("%s\n", infoLog);
        delete infoLog;

        exit(EXIT_FAILURE);
    }
}

void init(void) {
    GLuint vao;
    GLuint bufs[2];
    glGenVertexArrays(1, &vao);
    glGenBuffers(2, bufs);

    glBindVertexArray(vao);
    glEnableVertexAttribArray(POSITION_ATTRIB);

    glBindBuffer(GL_ARRAY_BUFFER, bufs[0]);
    glBufferData(GL_ARRAY_BUFFER, 6*2*sizeof(GLfloat), vert_data, GL_STATIC_DRAW);
    glVertexAttribPointer(POSITION_ATTRIB, 2, GL_FLOAT, GL_FALSE, 0, 0);

    const char * vertexFilePath = "vertex.vsh";
    shaderLoadSources(&vertexFilePath, 1, &vert, GL_VERTEX_SHADER);

    const char * fragmentFilePath[2] = {"shader_toy_inputs.fsh", "playground.fsh"};
    shaderLoadSources(fragmentFilePath, 2, &frag, GL_FRAGMENT_SHADER);

    prog = glCreateProgram();
    surfaceProg = glCreateProgram();

    glAttachShader(prog, vert);
    glAttachShader(prog, frag);

    glBindAttribLocation(prog, POSITION_ATTRIB, "position");

    glLinkProgram(prog);

    GLint result;

    glGetProgramiv(prog, GL_LINK_STATUS, &result);

    if(result == GL_FALSE) {
        GLint length;
        char *log;

        /* get the program info log */
        glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &length);
        log = (char *) malloc(length);
        glGetProgramInfoLog(prog, length, &result, log);

        /* print an error message and the info log */
        fprintf(stderr, "init(): Program linking failed:\n%s\n", log);
        free(log);

        /* delete the program */
        glDeleteProgram(prog);
        prog = 0;

        exit(EXIT_FAILURE);
    }
}

void clean_up() {
    glDeleteShader(vert);
    glDeleteShader(frag);
    glDeleteProgram(prog);
    glfwTerminate();
}

int main(int argc, char ** argv) {
    int width, height;

    if(argc == 3) {
        sscanf(argv[1], "%d", &width);
        sscanf(argv[2], "%d", &height);
    } else {
        width = 480;
        height = 360;
    }
    GLFWwindow* window;

    glfwSetErrorCallback(error_callback);

    if(!glfwInit()) {
        printf("Error while initialising glfw. Exiting...");
        exit(EXIT_FAILURE);
    }

    window = glfwCreateWindow(width, height, "GLSL Playground", NULL, NULL);

    if(!window) {
        printf("Error while initialising window via glfw. Terminating...");
        glfwTerminate();
        exit(EXIT_FAILURE);
    }

    glfwMakeContextCurrent(window);

    glfwSetKeyCallback(window, key_callback);
    glfwSetMouseButtonCallback(window, mouse_callback);

    int maxtexsize;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE,&maxtexsize);
    //printf("GL_MAX_TEXTURE_SIZE, %d\n",maxtexsize);

    init();

    double mouseX, mouseY;
    float mouse[2];
    float iMouse[4];

    float ratio;

    // GLSL Heroku
    GLint resolutionLocation = glGetUniformLocation(prog, "resolution");
    GLint timeLocation = glGetUniformLocation(prog, "time");
    GLint mouseLocation = glGetUniformLocation(prog, "mouse");

    // Shadertoy
    GLint iResolutionLocation = glGetUniformLocation(prog, "iResolution");
    GLint iGlobalTimeLocation = glGetUniformLocation(prog, "iGlobalTime");
    GLint iMouseLocation = glGetUniformLocation(prog, "iMouse");

    Target * backTarget = new Target(width/res, height/res);
    Target * frontTarget = new Target(width, height);

    float resolution[3];
    resolution[2] = 1.0;

    while(!glfwWindowShouldClose(window)) {

        setWindowFPS(window);

        if(!paused) {

            glfwGetCursorPos(window, &mouseX, &mouseY);
            mouse[0] = mouseX / (float) width;
            mouse[1] = 1.0 - mouseY / (float) height;

            iMouse[2] = (float) _iMouseX;
            iMouse[3] = height - (float) _iMouseY;

            if(glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_1) == GLFW_PRESS) {
                iMouse[0] = mouseX;
                iMouse[1] = height - mouseY;
            } else {
                iMouse[2] *= -1;
                iMouse[3] *= -1;
            }

            glfwGetFramebufferSize(window, &width, &height);
            ratio = width / (float) height;

            glUseProgram(prog);

            resolution[0] = (float) width;
            resolution[1] = (float) height;

            glUniform2fv(resolutionLocation, 1, resolution);
            glUniform1f(timeLocation, (float) (glfwGetTime() - startTime));
            glUniform2fv(mouseLocation, 1, mouse);

            glUniform3fv(iResolutionLocation, 1, resolution);
            glUniform1f(iGlobalTimeLocation, (float) (glfwGetTime() - startTime));
            glUniform4fv(iMouseLocation, 1, iMouse);

            glViewport(0, 0, width, height);
            glClear(GL_COLOR_BUFFER_BIT);

            glDrawArrays(GL_TRIANGLES, 0, 6);

        }

        /* Swap front and back buffers */
        glfwSwapBuffers(window);

        /* Poll for and process events */
        glfwPollEvents();

        // disable program
        glUseProgram(0);
    }

    clean_up();

    return 0;
}
