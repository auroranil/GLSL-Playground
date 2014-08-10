// Insert your shader here!
// Not sure what to do? Goto glsl.heroku.com or shadertoy.com to
// find some cool shaders 

uniform float time;
uniform vec2 mouse;
uniform vec2 resolution;

void main( void ) {
    float a = resolution.x/resolution.y;
    vec2 uv = gl_FragCoord.xy/resolution.xy;
    uv.x *= a;
    
    float col = 1.0 - 0.5*length (uv - vec2(mouse.x*a, mouse.y));
    
    gl_FragColor = vec4(col*0.1, col*0.7, col, 1.);
}
