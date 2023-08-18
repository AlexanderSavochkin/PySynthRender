#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;
layout (location = 3) in vec3 aTangent;
layout (location = 4) in vec3 aBitangent;

out VS_OUT {
    vec3 FragPos;
    vec2 TexCoords;
    vec3 TangentSearchLightPos;
    vec3 TangentSearchLightDir;
    vec3 TangentExternalLightDir;
    vec3 TangentViewPos;
    vec3 TangentFragPos;
} vs_out;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

uniform vec3 searchLightPos;
uniform vec3 viewPos;
uniform vec3 externalLightDir;
uniform vec3 searchLightDir;


void main()
{
    vs_out.FragPos = vec3(model * vec4(aPos, 1.0));
    vs_out.TexCoords = aTexCoords;

    
    vec3 T = normalize(mat3(model) * aTangent);
    vec3 N = normalize(mat3(model) * aNormal);
    T = normalize(T - dot(T, N) * N);
    vec3 B = cross(N, T);

    mat3 invTBN = transpose(mat3(T, B, N)); //Since TBN is orthogonal, its inverse is its transpose

    vs_out.TangentSearchLightPos = invTBN * searchLightPos;
    vs_out.TangentSearchLightDir = invTBN * searchLightDir;
    vs_out.TangentExternalLightDir = invTBN * externalLightDir;
    vs_out.TangentViewPos = invTBN * viewPos;
    vs_out.TangentFragPos = invTBN * vs_out.FragPos;

    gl_Position = projection * view * model * vec4(aPos, 1.0f);
}
