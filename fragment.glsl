varying vec2 v_texCrds;
uniform sampler2D u_tex;

void main() {

float offset[8];
offset[0] = 0.0 / 256.0;
offset[1] = 1.0 / 256.0;
offset[2] = 2.0 / 256.0;
offset[3] = 3.0 / 256.0;
offset[4] = 4.0 / 256.0;
offset[5] = 5.0 / 256.0;
offset[6] = 6.0 / 256.0;
offset[7] = 7.0 / 256.0;

float weight[8];
weight[7] = 1.0 / 23150.0;
weight[6] = 14.0 / 23150.0;
weight[5] = 91.0 / 23150.0;
weight[4] = 364.0 / 23150.0;
weight[3] = 1001.0 / 23150.0;
weight[2] = 2002.0 / 23150.0;
weight[1] = 3003.0 / 23150.0;
weight[0] = 3432.0 / 23150.0;


/*float weight[8];
weight[7] = 0.125 / 2.0;
weight[6] = 0.125 / 2.0;
weight[5] = 0.125 / 2.0;
weight[4] = 0.125 / 2.0;
weight[3] = 0.125 / 2.0;
weight[2] = 0.125 / 2.0;
weight[1] = 0.125 / 2.0;
weight[0] = 0.125 / 2.0;*/


	vec4 FragmentColor;

    FragmentColor = texture2D( u_tex, v_texCrds) * weight[0];
    // FragmentColor = texture2D( u_tex, v_texCrds);

    for (int i=0; i<8; i++) {
        FragmentColor += texture2D( u_tex, ( v_texCrds+vec2(0.0, offset[i]) ) ) * weight[i] / 2.0;
        FragmentColor += texture2D( u_tex, ( v_texCrds-vec2(0.0, offset[i]) ) ) * weight[i] / 2.0;
    }

    for (int i=0; i<8; i++) {
        FragmentColor += texture2D( u_tex, ( v_texCrds + vec2(offset[i], 0.0) ) ) * weight[i] / 2.0;
        FragmentColor += texture2D( u_tex, ( v_texCrds - vec2(offset[i], 0.0) ) ) * weight[i] / 2.0;
    }

    gl_FragColor = FragmentColor;
}