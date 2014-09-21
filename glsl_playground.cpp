#include <GLFW/glfw3.h>
#include <cstdio>
#include <cstdlib>
#include <string.h>
#include <sys/stat.h>

#define WINDOW_TITLE "GLSL Playground"
#define AUTHOR "Saurabh Joshi"

#define POSITION_ATTRIB 0
#define COLOR_ATTRIB 1

void init(void);

// Vertex data of screen
// GL_TRIANGLE_STRIP
GLfloat vert_data[] = {
  -1.0, -1.0,
  1.0, -1.0,
  -1.0, 1.0,
  1.0, 1.0
};

int nbFrames = 0, lastFrames = 0;
unsigned int res = 2;
double lastTime = 0.0;
double startTime = 0.0;
double currentTime;

bool paused = false;

GLuint prog;
GLuint surfaceProg;
GLuint vert;
GLuint frag;

GLuint vao;
GLuint buf;

double _iMouseX, _iMouseY;

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

    snprintf (title, 255, "%s - %.2fs - [FPS: %d] @ %dx %s", WINDOW_TITLE, currentTime, lastFrames, res, paused ? "(PAUSED)" : "");

    glfwSetWindowTitle (win, title);
}

static void error_callback(int error, const char* description)
{
    fputs(description, stderr);
}

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if(action == GLFW_PRESS) {
        switch(key) {
            case GLFW_KEY_ESCAPE:
                printf("Exiting... (closed by user via Esc key)\n");
                glfwSetWindowShouldClose(window, GL_TRUE);                
            break;
            
            case GLFW_KEY_1:
                printf("Switched to 1x\n");
                res = 1;
            break;
            
            case GLFW_KEY_2:
                printf("Switched to 2x\n");
                res = 2;
            break;
            
            case GLFW_KEY_3:
                printf("Switched to 4x\n");
                res = 4;
            break;
            
            case GLFW_KEY_4:
                printf("Switched to 8x\n");
                res = 8;
            break;
            
            case GLFW_KEY_R:
                printf("Resetted time to zero.\n");
                startTime = glfwGetTime();
                if(glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS) {
                    init();
                }
            break;
            
            case GLFW_KEY_SPACE:
                paused = !paused;

                printf(paused ? "Paused GLSL Playground\n" : "Resuming GLSL Playground\n");

                startTime = glfwGetTime() - startTime;        
            break;
        }
    }
}

static void mouse_callback(GLFWwindow * window, int button, int action, int mods) {
    if(button == GLFW_MOUSE_BUTTON_1 && action == GLFW_PRESS) {
        glfwGetCursorPos(window, &_iMouseX, &_iMouseY);
    }
}

off_t fsize(const char *filename) {
    struct stat st;

    if (stat(filename, &st) == 0)
        return st.st_size;

    return -1;
}

bool doesFileExist(const char * filename) {
    struct stat st;
    int result = stat(filename, &st);
    return result == 0;
}

void shaderLoadSources(const char ** filePaths, int numOfFiles, GLuint * shaderID, GLint type) {
    int len = 0;
    int prev_len = 0;

    for(int i = 0; i < numOfFiles; i++) {
            len += fsize(filePaths[i]);
    }

    char * result = (char *) malloc((len + 1) * sizeof(char));

    for(int i = 0; i < numOfFiles; i++) {
        if(!doesFileExist(filePaths[i])) {
            printf("Error while opening '%s' file.\n", filePaths[i]);
            exit(EXIT_FAILURE);
        }
        
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
        printf("Failed to compile %s shader:\n\n", type == GL_VERTEX_SHADER ? "vertex" : type == GL_FRAGMENT_SHADER ? "fragment" : "unknown");

        GLint infoLogLength;
        glGetShaderiv(*shaderID, GL_INFO_LOG_LENGTH, &infoLogLength);
        GLchar * infoLog = new GLchar[infoLogLength + 1];
        char * message;
        glGetShaderInfoLog(*shaderID, infoLogLength + 1, NULL, infoLog);
        
        if(type == GL_FRAGMENT_SHADER) {
            int dummy, line_no = -1;

            if(sscanf(infoLog, "%d(%d) : %[^\n]", &dummy, &line_no, message) == 3) {
                int count = 0;
                int i;
                
                GLchar * line_value;
                
                for(i = 0; i < len; i++) {
                    if(result[i] == '\n') {
                        count ++;
                        
                        if(count == line_no - 2) {
                            i ++;
                            
                            int j;
                            
                            for(j = i; j < len; j++) {
                                if(result[j] == '\n') {
                                    break;
                                }
                            }
                            
                            line_value = (GLchar *) malloc((j - i)*sizeof(GLchar));
                            memcpy(line_value, &result[i], j - i);
                            line_value[j - i] = 0;
                                                        
                            break;
                        }
                    }
                }
                
                printf("(Line %i): %s\n\n", line_no - 12, line_value);
            }
        }
        
        printf("%s\n", message);
        
        delete infoLog;

        exit(EXIT_FAILURE);
    }
}

void loadProgram(GLuint * prog, GLuint * vert_shader, GLuint * frag_shader) {
    if(*prog) {
        glDeleteProgram(*prog);
    }

    *prog = glCreateProgram();
    
    glAttachShader(*prog, *vert_shader);
    glAttachShader(*prog, *frag_shader);
    
    glBindAttribLocation(*prog, POSITION_ATTRIB, "position");

    glLinkProgram(*prog);
    
    glDeleteShader(*vert_shader);
    glDeleteShader(*frag_shader);
    
    GLint result;

    glGetProgramiv(*prog, GL_LINK_STATUS, &result);

    if(result == GL_FALSE) {
        GLint length;
        char *log;

        /* get the program info log */
        glGetProgramiv(*prog, GL_INFO_LOG_LENGTH, &length);
        log = (char *) malloc(length);
        glGetProgramInfoLog(*prog, length, &result, log);

        /* print an error message and the info log */
        fprintf(stderr, "loadProgram(): Program linking failed:\n%s\n", log);
        free(log);

        /* delete the program */
        glDeleteProgram(*prog);

        exit(EXIT_FAILURE);
    }
}

void init(void) {
    const char * vertexFilePath = "vertex.vsh";
    shaderLoadSources(&vertexFilePath, 1, &vert, GL_VERTEX_SHADER);

    const char * fragmentFilePath[2] = {"shader_toy_inputs.fsh", "playground.fsh"};
    shaderLoadSources(fragmentFilePath, 2, &frag, GL_FRAGMENT_SHADER);
    
    loadProgram(&prog, &vert, &frag);

    //surfaceProg = glCreateProgram();
}

void clean_up() {
    glDeleteProgram(prog);
    glfwTerminate();
}

int main(int argc, char ** argv) {
    int width, height;

    // if width and height is specified, open window with those dimensions
    if(argc == 3) {
        sscanf(argv[1], "%d", &width);
        sscanf(argv[2], "%d", &height);
    }
    // If the program is opened with no parameters, open with default dimensions 480x360
    else if(argc == 1) {
        width = 480;
        height = 360;
    }
    
    // Print the help dialog
    else {
        printf("%s, by %s.\n", WINDOW_TITLE, AUTHOR);
        printf("\n");
        printf("Usage: ./glsl_playground [width height]\n");
        exit(EXIT_SUCCESS);
    }
    
    GLFWwindow* window;

    glfwSetErrorCallback(error_callback);

    if(!glfwInit()) {
        printf("Error while initialising glfw. Exiting...\n");
        exit(EXIT_FAILURE);
    }

    window = glfwCreateWindow(width, height, WINDOW_TITLE, NULL, NULL);

    if(!window) {
        printf("Error while initialising window via glfw. Terminating...\n");
        glfwTerminate();
        exit(EXIT_FAILURE);
    }

    glfwMakeContextCurrent(window);

    glfwSetKeyCallback(window, key_callback);
    glfwSetMouseButtonCallback(window, mouse_callback);
    
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &buf);

    glBindVertexArray(vao);
    glEnableVertexAttribArray(POSITION_ATTRIB);

    glBindBuffer(GL_ARRAY_BUFFER, buf);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vert_data), vert_data, GL_STATIC_DRAW);
    glVertexAttribPointer(POSITION_ATTRIB, 2, GL_FLOAT, GL_FALSE, 0, 0);

    init();

    double mouseX, mouseY;
    float mouse[2];
    float iMouse[4];

    // GLSL Heroku
    GLint resolutionLocation = glGetUniformLocation(prog, "resolution");
    GLint timeLocation = glGetUniformLocation(prog, "time");
    GLint mouseLocation = glGetUniformLocation(prog, "mouse");

    // Shadertoy
    GLint iResolutionLocation = glGetUniformLocation(prog, "iResolution");
    GLint iGlobalTimeLocation = glGetUniformLocation(prog, "iGlobalTime");
    GLint iMouseLocation = glGetUniformLocation(prog, "iMouse");

    //Target * backTarget = new Target(width/res, height/res);
    //Target * frontTarget = new Target(width, height);

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
            
            resolution[0] = (float) width;
            resolution[1] = (float) height;

            glfwGetFramebufferSize(window, &width, &height);

            glUseProgram(prog);

            glUniform2fv(resolutionLocation, 1, resolution);
            glUniform1f(timeLocation, (float) (glfwGetTime() - startTime));
            glUniform2fv(mouseLocation, 1, mouse);

            glUniform3fv(iResolutionLocation, 1, resolution);
            glUniform1f(iGlobalTimeLocation, (float) (glfwGetTime() - startTime));
            glUniform4fv(iMouseLocation, 1, iMouse);

            glViewport(0, 0, width, height);
            glClear(GL_COLOR_BUFFER_BIT);

            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            
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
