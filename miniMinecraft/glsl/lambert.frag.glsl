#version 150
// ^ Change this to version 130 if you have compatibility issues

// This is a fragment shader. If you've opened this file first, please
// open and read lambert.vert.glsl before reading on.
// Unlike the vertex shader, the fragment shader actually does compute
// the shading of geometry. For every pixel in your program's output
// screen, the fragment shader is run for every bit of geometry that
// particular pixel overlaps. By implicitly interpolating the position
// data passed into the fragment shader by the vertex shader, the fragment shader
// can compute what color to apply to its pixel based on things like vertex
// position, light position, and vertex color.

uniform sampler2D u_Texture; // The texture to be read from by this shader

uniform sampler2D u_NormalMap; // The normal map to be read from by this shader

uniform int u_Time;

// These are the interpolated values out of the rasterizer, so you can't know
// their specific values without knowing the vertices that contributed to them
in vec3 fs_Nor;
in vec3 fs_LightVec;
in vec2 fs_UV;

out vec4 out_Col; // This is the final output color that you will see on your
                  // screen for the pixel that is currently being processed.

const float TIME_SCALE = 0.01;

// Sun palette
const vec3 sun[3] = vec3[](vec3(255, 255, 190) / 255.0,
                           vec3(255, 140, 100) / 255.0,
                           vec3(200, 140, 100) / 255.0);

void main()
{
    vec2 uv = fs_UV;
    // animation for WATER and LAVA
    float wave = sin(float(u_Time) / 1000.0) * 0.01;
    // WATER
    if (fs_UV.x >= 14.0/16.0 && fs_UV.x < 15.0/16.0 && fs_UV.y >= 3.0/16.0 && fs_UV.y < 4.0/16.0) {
        uv.x = fs_UV.x + wave;
    }
    // LAVA
    if (fs_UV.x >= 14.0/16.0 && fs_UV.x < 15.0/16.0 && fs_UV.y >= 1.0/16.0 && fs_UV.y < 2.0/16.0) {
        uv.x = fs_UV.x + wave;
    }

    float time = sin(u_Time * TIME_SCALE * 0.01 + 0.001);
    vec3 sunDir = normalize(vec3(cos(u_Time * TIME_SCALE * 0.01 + 0.001), sin(u_Time * TIME_SCALE * 0.01 + 0.001), 0.f));
    vec3 sunColor;
    if (time > 0.33) {
        sunColor = sun[0];
    }
    else if (time < -0.33) {
        sunColor = vec3(0.5, 0.5, 0.5);
    }
    else {
        if (time > 0) {
            sunColor = mix(sun[1], sun[0], smoothstep(0.f, 1.f, abs(time) / 0.33));
        } else {
            sunColor = mix(sun[1], sun[2], smoothstep(0.f, 1.f, abs(time) / 0.33));
        }
    }

    // Material base color (before shading)
    vec4 diffuseColor = texture(u_Texture, uv);
    if (diffuseColor.a < 0.1) {
        discard;
    }

    // Normal
    vec4 normalMapValue = texture(u_NormalMap, uv);
    vec3 nor = normalMapValue.rgb;
    if (normalMapValue.a < 0.1) {
        nor = vec3(0, 0, 1);
    } else {
        nor = normalize(nor * 2.0 - 1.0);
    }
    nor = normalize(fs_Nor + nor - vec3(0, 0, 1));

    // Calculate the diffuse term for Lambert shading
    float diffuseTerm = dot(nor, normalize(fs_LightVec * 0.2 + sunDir));
    // Avoid negative lighting values
    diffuseTerm = clamp(diffuseTerm, 0, 1);

    float ambientTerm = 0.2;

    float lightIntensity = diffuseTerm + ambientTerm;   //Add a small float value to the color multiplier
                                                        //to simulate ambient lighting. This ensures that faces that are not
                                                        //lit by our point light are not completely black.

    // Compute final shaded color
    out_Col = vec4(diffuseColor.rgb * lightIntensity * sunColor, diffuseColor.a);
}
