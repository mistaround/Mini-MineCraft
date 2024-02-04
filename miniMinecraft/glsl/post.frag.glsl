#version 150
// ^ Change this to version 130 if you have compatibility issues

// Refer to the lambert shader files for useful comments

in vec2 fs_UV;

out vec4 out_Col;

uniform int u_Time;
uniform int u_PostType;
uniform sampler2D u_PostQuad;

void main()
{
    vec3 color = texture(u_PostQuad, fs_UV).rgb;

    if (u_PostType == 1) {
        color += vec3(0, 0, 0.5);
    } else if (u_PostType == 2) {
        color += vec3(0.5, 0, 0);
    }

    out_Col = vec4(color, 1);
}
