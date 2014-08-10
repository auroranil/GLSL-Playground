#version 120

attribute vec3 position;

void main(void) { 
    //vcol = color;
    gl_Position = vec4(position, 1.0);
}
