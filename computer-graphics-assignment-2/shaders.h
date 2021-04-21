#ifndef SHADERS_H
#define SHADERS_H

namespace Shaders {
    const char *const TEXTURED_VERTEX_SHADER_SOURCE = R"(
    #version 330

    layout (location = 0) in vec3 a_pos;
    layout (location = 1) in vec3 a_nor;
    layout (location = 2) in vec2 a_tex;

    out vec3 v_pos;
    out vec3 v_nor;
    out vec2 v_tex;
    out vec4 frag_pos_light_space;

    uniform mat4 projection;
    uniform mat4 view;
    uniform mat4 model;
    uniform mat4 light_space_matrix;
    
    void main()
    {
        vec4 world_position = model * vec4(a_pos, 1.f);
        vec4 world_normal = transpose(inverse(model)) * vec4(a_nor, 0.f);

        v_pos = vec3(world_position);
        v_nor = vec3(world_normal);
        v_tex = a_tex;

        frag_pos_light_space = light_space_matrix * world_position;

        gl_Position = projection * view * world_position;
    }
    )";

    const char *const TEXTURED_FRAGMENT_SHADER_SOURCE = R"(
    #version 330
    
    #define NUM_LIGHTS 2

    struct DirectionalLight {
        vec3 pos;
        vec3 colour;
        float ambient;
        float diffuse;
    };

    in vec3 v_pos;
    in vec3 v_nor;
    in vec2 v_tex;

    in vec4 frag_pos_light_space;

    out vec4 frag;
    
    uniform DirectionalLight lights[NUM_LIGHTS];

    uniform float gamma_correction;

    uniform vec3 view_position;

    uniform sampler2D tex;
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

    vec3 calculate_lighting(DirectionalLight light)
    {
        vec3 ambient = light.ambient * light.colour;

        vec3 light_dir = normalize(light.pos - v_pos);
        float diff = max(dot(v_nor, light_dir), 0.0f);
        vec3 diffuse = diff * light.colour * light.diffuse;

        vec3 view_dir = normalize(view_position + v_pos);
        vec3 halfway_dir = normalize(light_dir + view_dir);
        float spec = pow(max(dot(v_nor, halfway_dir), 0.f), 3);
        vec3 specular = 0.5f * spec * light.colour;
        
        float shadow = shadow_calculation(frag_pos_light_space, light_dir);

        return ambient + (1.f - shadow) * diffuse + specular;
    }

    void main()
    {
        vec3 result = vec3(0.f, 0.f, 0.f);
        for (int i = 0; i < NUM_LIGHTS; i++) {
            result += calculate_lighting(lights[i]);
        }
        
        vec3 colour = vec3(texture(tex, v_tex)) * result;
        colour = pow(colour, vec3(1.f / gamma_correction));

        frag = vec4(colour, 1.f);
    }
    )";

    const char *const DIFFUSE_FRAGMENT_SHADER_SOURCE = R"(
    #version 330
    
    #define NUM_LIGHTS 2

    struct DirectionalLight {
        vec3 pos;
        vec3 colour;
        float ambient;
        float diffuse;
    };

    in vec3 v_pos;
    in vec3 v_nor;
    in vec2 v_tex;

    in vec4 frag_pos_light_space;

    out vec4 frag;
    
    uniform vec3 view_position;

    uniform DirectionalLight lights[NUM_LIGHTS];
    uniform float gamma_correction;
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

    vec3 calculate_lighting(DirectionalLight light)
    {
        vec3 ambient = light.ambient * light.colour;

        vec3 light_dir = normalize(light.pos - v_pos);
        float diff = max(dot(v_nor, light_dir), 0.0f);
        vec3 diffuse = diff * light.colour * light.diffuse;

        vec3 view_dir = normalize(view_position + v_pos);
        vec3 halfway_dir = normalize(light_dir + view_dir);
        float spec = pow(max(dot(v_nor, halfway_dir), 0.f), 30);
        vec3 specular = 1.f * spec * light.colour;
        
        float shadow = shadow_calculation(frag_pos_light_space, light_dir);

        return ambient + (1.f - shadow) * diffuse + specular;
    }

    void main()
    {
        vec3 result = vec3(0.f, 0.f, 0.f);
        for (int i = 0; i < NUM_LIGHTS; i++) {
            result += calculate_lighting(lights[i]);
        }
        
        vec3 colour = object_colour * result;
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

    const char *const INTERFACE_VERTEX_SHADER_SOURCE = R"(
    #version 330

    layout (location = 0) in vec2 a_pos;
    layout (location = 1) in vec2 a_tex;

    out vec2 v_tex;

    uniform mat4 projection;
    uniform mat4 model;

    void main()
    {
        v_tex = a_tex;

        gl_Position = projection * model * vec4(a_pos, 0.f, 1.f);
    }
    )";

    const char *const INTERFACE_FRAGMENT_SHADER_SOURCE = R"(
    #version 330

    in vec2 v_tex;

    out vec4 frag;

    uniform sampler2D tex;

    void main()
    {
        frag = texture(tex, v_tex);
    }
    )";

    const char *const OUTLINE_VERTEX_SHADER_SOURCE = R"(
    #version 330

    layout (location = 0) in vec3 a_pos;
    layout (location = 1) in vec3 a_nor;
    layout (location = 2) in vec2 a_tex;

    uniform mat4 projection;
    uniform mat4 view;
    uniform mat4 model;
    
    void main()
    {
        vec4 world_position = model * vec4(a_pos, 1.f);

        gl_Position = projection * view * world_position;
    }
    )";

    const char *const OUTLINE_FRAGMENT_SHADER_SOURCE = R"(
    #version 330

    out vec4 frag;

    void main()
    {
        frag = vec4(1.f, 0.f, 0.f, 1.f);
    }
    )";

        const char* const SKYBOX_VERTEX_SHADER_SOURCE = R"(
    #version 330

    layout (location = 0) in vec3 a_pos;

    uniform mat4 projection;
    uniform mat4 view;

    out vec3 tex_coords;

    void main()
    {
        tex_coords = normalize(a_pos);
        vec4 pos = projection * view * vec4(a_pos, 1.0);
        gl_Position = pos.xyww;
    }
    )";

    const char* const SKYBOX_FRAGMENT_SHADER_SOURCE = R"(
    #version 330

    in vec3 tex_coords;

    uniform samplerCube skybox;

    out vec4 frag;

    void main()
    {
        frag = texture(skybox, tex_coords);
    }
    )";
};

#endif