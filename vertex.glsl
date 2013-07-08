attribute vec2 a_pos;
attribute vec2 a_texCrds;

varying vec2 v_texCrds;
varying vec3 v_clr;

void main() {	
	v_texCrds = a_texCrds;
	gl_Position = vec4(a_pos, 0.0, 1.0);
}