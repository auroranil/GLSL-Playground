#include <GLFW/glfw3.h>
#include <cstdio>
#include <cstdlib>
#include <string.h>
#include <sys/stat.h>

#define WINDOW_TITLE "GLSL Playground"
#define AUTHOR "Saurabh Joshi"

#define POSITION_ATTRIB 0
#define COLOR_ATTRIB 1

GLuint init(void);

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
GLuint vao;
GLuint buf;

double _iMouseX, _iMouseY;

// https://stackoverflow.com/questions/18412120/displaying-fps-in-glfw-window-title
void updateTitle(GLFWwindow* win) {
    char title [256];
    title [255] = '\0';

    snprintf (title, 255, "%s - %ds - [FPS: %d] @ %dx %s", WINDOW_TITLE, (int) currentTime, lastFrames, res, paused ? "(PAUSED)" : "");

    glfwSetWindowTitle (win, title);
}

void setWindowFPS (GLFWwindow* win, bool forceUpdate = false) {
    // Measure speed
    if(!paused) {
        currentTime = glfwGetTime() - startTime;
        nbFrames++;

        if ( currentTime - lastTime >= 1.0 ) {
            lastFrames = nbFrames;

            nbFrames = 0;
            lastTime += 1.0;
            
            updateTitle(win);
        } else if(forceUpdate) {
            updateTitle(win);
        }
    } else if(forceUpdate) {
        updateTitle(win);
    }
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
                printf("%s - Exiting... (closed by user via Esc key)\n", __func__);
                glfwSetWindowShouldClose(window, GL_TRUE);                
            break;
            
            case GLFW_KEY_1:
                printf("%s - Switched to 1x\n", __func__);
                res = 1;
            break;
            
            case GLFW_KEY_2:
                printf("%s - Switched to 2x\n", __func__);
                res = 2;
            break;
            
            case GLFW_KEY_3:
                printf("%s - Switched to 4x\n", __func__);
                res = 4;
            break;
            
            case GLFW_KEY_4:
                printf("%s - Switched to 8x\n", __func__);
                res = 8;
            break;
            
            case GLFW_KEY_R:
                startTime = glfwGetTime();
                if(glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS) {
                    glDeleteProgram(prog);
                    prog = init();
                    printf("%s - Reloaded program.\n", __func__);
                } else {
                    printf("%s - Resetted time to zero.\n", __func__);
                }
            break;
            
            case GLFW_KEY_SPACE:
                paused = !paused;

                printf("%s - ", __func__);
                printf(paused ? "Paused GLSL Playground\n" : "Resuming GLSL Playground\n");

                startTime = glfwGetTime() - startTime;        
            break;
        }
        
        setWindowFPS(window, true);
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

GLuint shaderLoadSources(const char ** filePaths, int numOfFiles, GLint type) {
    int len = 0;
    int prev_len = 0;

    for(int i = 0; i < numOfFiles; i++) {
            len += fsize(filePaths[i]);
    }

    char * result = (char *) malloc((len + 1) * sizeof(char));

    for(int i = 0; i < numOfFiles; i++) {
        if(!doesFileExist(filePaths[i])) {
            printf("%s - Error while opening '%s' file.\n", __func__, filePaths[i]);
            exit(EXIT_FAILURE);
        }
        
        FILE * s_file = fopen(filePaths[i], "r");

        fread(&result[prev_len], sizeof(char), len * sizeof(char), s_file);
        fclose(s_file);
        prev_len += fsize(filePaths[i]);
    }

    result[len] = '\0';

    GLuint shaderID = glCreateShader(type);
    glShaderSource(shaderID, 1, &result, 0);
    glCompileShader(shaderID);

    GLint compileStatus;
    glGetShaderiv(shaderID, GL_COMPILE_STATUS, &compileStatus);

    if(compileStatus != GL_TRUE) {
        printf("%s - Failed to compile %s shader:\n\n", __func__, type == GL_VERTEX_SHADER ? "vertex" : type == GL_FRAGMENT_SHADER ? "fragment" : "unknown");

        GLint infoLogLength;
        glGetShaderiv(shaderID, GL_INFO_LOG_LENGTH, &infoLogLength);
        GLchar * infoLog = new GLchar[infoLogLength + 1];
        char * message = (char *) calloc(infoLogLength + 1, sizeof(char));
        glGetShaderInfoLog(shaderID, infoLogLength + 1, NULL, infoLog);
        
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
                
                printf("%s - (Line %i): %s\n\n", __func__, line_no - 12, line_value);
            }
        }
        
        printf("%s - %s\n", __func__, message);
        
        delete infoLog;

        exit(EXIT_FAILURE);
    }
    
    return shaderID;
}

GLuint loadProgram(GLuint vert_shader, GLuint frag_shader) {
    GLuint prog = glCreateProgram();
    
    glAttachShader(prog, vert_shader);
    glAttachShader(prog, frag_shader);
    
    glBindAttribLocation(prog, POSITION_ATTRIB, "position");

    glLinkProgram(prog);
    
    glDeleteShader(vert_shader);
    glDeleteShader(frag_shader);
    
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
        fprintf(stderr, "%s - Program linking failed:\n%s\n", __func__, log);
        free(log);

        /* delete the program */
        glDeleteProgram(prog);

        exit(EXIT_FAILURE);
    }
    
    return prog;
}

GLuint init(void) {
    const char * vertexFilePath = "vertex.vsh";
    GLuint vert = shaderLoadSources(&vertexFilePath, 1, GL_VERTEX_SHADER);

    const char * fragmentFilePath[2] = {"shader_toy_inputs.fsh", "playground.fsh"};
    GLuint frag = shaderLoadSources(fragmentFilePath, 2, GL_FRAGMENT_SHADER);
    
    return loadProgram(vert, frag);
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
        printf("%s - Usage: ./glsl_playground [width height]\n", __func__);
        exit(EXIT_SUCCESS);
    }
    
    GLFWwindow* window;

    glfwSetErrorCallback(error_callback);

    if(!glfwInit()) {
        printf("%s - Error while initialising glfw. Exiting...\n", __func__);
        exit(EXIT_FAILURE);
    }

    window = glfwCreateWindow(width, height, WINDOW_TITLE, NULL, NULL);

    if(!window) {
        printf("%s - Error while initialising window via glfw. Terminating...\n", __func__);
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

    prog = init();

    double mouseX, mouseY;
    float mouse[2];
    float iMouse[4];

    // GLSL Heroku
    GLint resolutionLocation = glGetUniformLocation(prog, "resolution");
    GLint timeLocation = glGetUniformLocation(prog, "time");
    GLint mouseLocation = glGetUniformLocation(prog, "mouse");

    // Shadertoy
    GLint iResolutionLocation = glGetUniformLocation(prog, "iResolution");
    GLint iTimeLocation = glGetUniformLocation(prog, "iTime");
    GLint iMouseLocation = glGetUniformLocation(prog, "iMouse");

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
            glUniform1f(iTimeLocation, (float) (glfwGetTime() - startTime));
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

    glDeleteProgram(prog);
    glfwTerminate();

    return 0;
}
