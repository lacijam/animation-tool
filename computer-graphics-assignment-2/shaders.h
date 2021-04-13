#ifndef SHADERS_H
#define SHADERS_H

namespace Shaders {
    const char *const SIMPLE_VERTEX_SHADER_SOURCE = R"(
    #version 330

    layout (location = 0) in vec3 a_pos;
    layout (location = 1) in vec3 a_nor;

    out vec3 v_pos;
    out vec3 v_nor;
    out vec4 frag_pos_light_space;

    uniform mat4 projection;
    uniform mat4 view;
    uniform mat4 model;
    uniform mat4 light_space_matrix;
    
    void main()
    {
        vec4 world_position = model * vec4(a_pos, 1.f);
        vec4 world_normal = model * vec4(a_nor, 0.f);

        v_pos = vec3(world_position);
        v_nor = vec3(world_normal);
        frag_pos_light_space = light_space_matrix * world_position;

        gl_Position = projection * view * world_position;
    }
    )";

    const char *const SIMPLE_FRAGMENT_SHADER_SOURCE = R"(
    #version 330

    in vec3 v_pos;
    in vec3 v_nor;
    in vec4 frag_pos_light_space;

    out vec4 frag;

    uniform float ambient_strength;
    uniform float diffuse_strength;
    uniform float gamma_correction;

    uniform vec3 light_pos;
    uniform vec3 light_colour;
    uniform vec3 object_colour;

    uniform sampler2D shadow_map;

    float bias(float x, float b) {
        b = -log2(1.0 - b);
        return 1.0 - pow(1.0 - pow(x, 1./b), b);
    }

    float shadow_calculation(vec4 f_pos_light_space, vec3 light_dir)
    {
        vec3 proj_coords = frag_pos_light_space.xyz / frag_pos_light_space.w;
        proj_coords = proj_coords * 0.5f + 0.5f;

        if (proj_coords.z > 1.f)
            return 0.f;

        float closest_depth = texture(shadow_map, proj_coords.xy).r;
        float current_depth = proj_coords.z;
        float bias = max(0.00001f * (1.f - dot(v_nor, light_dir)), 0.00001f);
        float shadow = 0.0;
        
        vec2 texelSize = 1.0 / textureSize(shadow_map, 0);
        
        for(int x = -1; x <= 1; ++x)
        {
            for(int y = -1; y <= 1; ++y)
            {
                float pcfDepth = texture(shadow_map, proj_coords.xy + vec2(x, y) * texelSize).r; 
                shadow += current_depth - bias > pcfDepth ? 1.0 : 0.0;        
            }    
        }

        if (proj_coords.z > 1.0) {
            shadow = 0.0;
        }
        
        return shadow /= 9.0;
    }

    void main()
    {
        vec3 ambient = ambient_strength * light_colour;

        vec3 light_dir = normalize(light_pos - v_pos);
        float diff = max(dot(v_nor, light_dir), 0.0f);
        vec3 diffuse = diff * light_colour * diffuse_strength;
        
        float shadow = shadow_calculation(frag_pos_light_space, light_dir);
        vec3 lighting = (ambient + (1.f - shadow) * diffuse);        
        vec3 colour = object_colour * lighting;
        colour = pow(colour, vec3(1.f / gamma_correction));

        frag = vec4(colour, 1.f);
    }
    )";

    const char *const DEPTH_VERTEX_SHADER_SOURCE = R"(
    #version 330

    layout (location = 0) in vec3 a_pos;

    uniform mat4 projection;
    uniform mat4 view;
    uniform mat4 model;

    void main()
    {
        gl_Position = projection * view * model * vec4(a_pos, 1.0);
    }
    )";

    const char *const DEPTH_FRAGMENT_SHADER_SOURCE = R"(
    #version 330

    void main()
    {
    }
    )";
};

#endif