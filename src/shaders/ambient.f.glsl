uniform vec3 ambient_color;

void main(void) {
    gl_FragColor = vec4(ambient_color, 1);
}