#vertex shader
#version 330 core

layout (location = 0) in vec3 position;
layout (location = 1) in vec2 tex_coordinate;
layout (location = 2) in vec3 normal;

out vec3 f_n;
out vec2 f_tex;
out vec3 f_pos;

uniform mat4 model;
uniform mat4 proj;
uniform mat4 view;

void main() {
    gl_Position  = proj * view * model * vec4(position, 1.0f);
    f_tex = tex_coordinate;
    f_n = normal;
    f_pos = model * position;
}

#fragment shader
#version 330 core

in vec3 f_n;
in vec2 f_tex;
in vec3 f_pos;

struct pointLight {
    vec3 position;
    vec3 color;
    float intensity;   // 光的强度
    float max_range; // r_max;
    float min_range; // r0
};

layout (std140) uniform PointLightBlock {
    pointLight lights[10];
    int count;
} light_array;

layout (std140) uniform Material {
    float ambient;
    float diffuse_factor;
    float specular_factor;
    float shinness;
}material;

uniform sampler2D diffuse_map;
uniform sampler2D specular_map;
uniform vec3 eye;

out vec4 fragColor;

/*
** Point light using lambertain model
**  attenuation function  (1 - (r / rmax) ^ 4) ^ +2
*/

float calculate_diffuse_light(vec3 refl, vec3 view, float dist, int index);

float calculate_specular_light(vec3 refl, vec3 view,, float dist, int index);

void main() {
    vec3 light = pointLight.position - f_pos;
    vec3 view = eye - f_pos;
    float dist = distance(f_pos, pointLight.position);
    light = normalize(light);
    view = normalize(view);
    vec3 refl = reflect(-light, f_n);

    vec3 out_color = vec3(0.0f, 0.0f, 0.0f);

    for (int i = 0; i < count; i++) {
        float d_i = calculate_diffuse_light(refl, view, dist, i);
        float s_i = calculate_specular_light(refl, view, dist, i);
        vec3 diffuse = texture(diffuse_map, f_tex) * (material.ambient + d_i * material.diffuse_factor);
        vec3 specular = texture(specular_map, f_tex) * material.specular_factor * s_i;
        out_color += diffuse + specular;
    }

    fragColor = vec4(out_color, 1.0f);
}


/*
**  diffuse = cos(view, reflect)(0 - 1) * light_intensity * 
**  (r0 / r)^ 2+ * attenuation function (1 - (r / r_max)^4)^2+ 
*/
float calculate_diffuse_light(vec3 refl, vec3 view, float dist, int index) {
    float intensity = light_array[index].intensity * clamp(dot(view, refl), 0.0, 1.0);

    // 计算距离相关的衰减
    float attenuation = (light_array[index].min_range / dist) * (light_array[index].min_range / dist);
    attenuation *= pow(1.0 - pow(dist / light_array[index].max_range, 4.0), 2.0);

    // 应用衰减到强度
    intensity *= attenuation;

    return intensity; // 确保返回计算的强度
};

float calculate_specular_light(vec3 refl, vec3 view,, float dist, int index) {
    // 计算镜面光强度
    float spec = pow(clamp(dot(view, refl), 0.0, 1.0), material.shininess);

    // 计算衰减因子
    float attenuation = (light_array[index].min_range / dist) * (light_array[index].min_range / dist);
    attenuation *= pow(1.0 - pow(dist / light_array[index].max_range, 4.0), 2.0);

    // 应用衰减到镜面光强度
    float intensity = light_array[index].intensity * spec * attenuation;

    return intensity; // 确保返回计算的强度
};