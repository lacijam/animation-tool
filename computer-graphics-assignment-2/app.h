#ifndef APP_H
#define APP_H

#include <thread>
#include <random>
#include <vector>
#include <array>

#include "types.h"
#include "maths.h"
#include "camera.h"

struct app_button_state {
    bool32 started_down;
    bool32 ended_down;
    bool32 toggled;
};

struct app_keyboard_input {
    union {
        app_button_state buttons[8];
        struct {
            app_button_state forward;
            app_button_state backward;
            app_button_state left;
            app_button_state right;
            app_button_state cam_up;
            app_button_state cam_down;
            app_button_state cam_left;
            app_button_state cam_right;
            app_button_state wireframe;
            app_button_state fly;
        };
    };
};

struct app_input {
    app_keyboard_input keyboard;
};

struct app_memory {
    bool32 is_initialized;
    u64 free_offset;
    u64 permenant_storage_size;
    void *permenant_storage;
};

struct app_window_info {
    u32 w, h;
    bool32 resize, running;
};

struct SimpleShader {
    u32 program;
    u32 projection;
    u32 view;
    u32 model;
    u32 light_space_matrix;
    u32 shadow_map;
    u32 ambient_strength;
    u32 diffuse_strength;
    u32 gamma_correction;
    u32 light_pos;
    u32 light_colour;
    u32 object_colour;
};

struct DepthShader {
    u32 program;
    u32 projection;
    u32 view;
    u32 model;
};

struct Vertex {
    V3 pos;
    V3 nor;
};

struct QuadIndices {
	u32 i[6];
};

struct Light {
    V3 pos, colour;
    real32 ambient, diffuse, specular;
};

struct app_state {
    app_window_info window_info;

    SimpleShader simple_shader;
    DepthShader depth_shader;

    Camera cur_cam;
    Light light;

    std::vector<std::thread> generation_threads;
    std::mt19937 rng;

    u32 triangle_vao, quad_vbo, quad_ebo;
    u32 depth_map_fbo, depth_map;
};

extern void app_update_and_render(real32 dt, app_state *state, app_input *input, app_window_info *window_info);
extern app_state *app_init(u32 w, u32 h);

#endif