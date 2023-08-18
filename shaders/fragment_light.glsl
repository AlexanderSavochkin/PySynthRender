#version 330 core

out vec3 fragColor;

in VS_OUT {
    vec3 FragPos;
    vec2 TexCoords;
    vec3 TangentSearchLightPos;
    vec3 TangentSearchLightDir;
    vec3 TangentExternalLightDir;
    vec3 TangentViewPos;
    vec3 TangentFragPos;
} fs_in;

//Light properties
uniform vec3 external_light_color;
uniform vec3 searchlight_color;
uniform float searchlight_cone_angle_sin;
uniform vec3 ambient_light_color;

//Mesh material properties
uniform vec3 Kd;
uniform vec3 Ks;
uniform vec3 Ka;
uniform vec3 Ke;
uniform float Ns;


//Textures
uniform sampler2D texture_normal1;
uniform sampler2D texture_diffuse1;
uniform sampler2D texture_specular1;
uniform sampler2D texture_height1;

void main() {
    float shininess = Ns;

    vec3 normal = vec3(0.0f, 0.0f, 1.0f);  //texture(texture_normal1, fs_in.TexCoords).rgb;
    //normal = normalize(normal * 2.0 - 1.0); // this normal is in tangent space

    //Get the diffuse surface color
    vec3 diffuse_color = Kd * texture(texture_diffuse1, fs_in.TexCoords).rgb;
    // Ambient
    vec3 ambient = ambient_light_color * Ka;


    vec3 search_light_vec = fs_in.TangentSearchLightPos - fs_in.TangentFragPos;
    float search_light_dist = length(search_light_vec);
    vec3 search_light_dir = normalize(search_light_vec);
    vec3 search_light_target_dir = normalize(fs_in.TangentSearchLightDir);
    vec3 view_dir = normalize(fs_in.TangentViewPos - fs_in.TangentFragPos);
    vec3 external_light_dir = normalize(fs_in.TangentExternalLightDir);

    // Diffuse
    float search_light_axis_to_frag_sin = length(cross(-search_light_dir, search_light_target_dir));
    float diff_search_light = (1.0 - step(searchlight_cone_angle_sin, search_light_axis_to_frag_sin))
                                 * max(dot(normal, search_light_dir), 0.0) / (search_light_dist * search_light_dist);
    float diff_external_light = max(dot(normal, -external_light_dir), 0.0);

    vec3 diffuse = (diff_search_light * searchlight_color + diff_external_light * external_light_color) * diffuse_color;

    // Specular
    vec3 search_light_halfway_dir = normalize(search_light_dir + view_dir);
    vec3 external_light_halfway_dir = normalize(-external_light_dir + view_dir);

    float search_spec = pow(max(dot(normal, search_light_halfway_dir), 0.0), shininess);
    float external_spec = pow(max(dot(normal, external_light_halfway_dir), 0.0), shininess);

    vec3 specular = (
            (1.0 - step(searchlight_cone_angle_sin, search_light_axis_to_frag_sin)) * search_spec * searchlight_color / (search_light_dist * search_light_dist) + 
            external_spec * external_light_color
        ) * Ks * texture(texture_specular1, fs_in.TexCoords).rgb;

    fragColor = ambient + diffuse + specular;
}
