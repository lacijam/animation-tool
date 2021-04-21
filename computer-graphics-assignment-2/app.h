#ifndef APP_H
#define APP_H

#include <thread>
#include <random>
#include <vector>
#include <array>
#include <functional>

#include "maths.h"
#include "camera.h"
#include "object.h"
#include "node.h"
#include "skybox.h"
#include "bitmap.h"

struct app_button_state {
    bool started_down;
    bool ended_down;
    bool toggled;
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
        };
    };
};

struct app_mouse_state {
    unsigned int x, y;
};

struct app_mouse_input {
    union {
        app_button_state buttons[2];
        struct {
            app_button_state left;
            app_button_state right;
        };
    };
    app_mouse_state pos;
};

struct app_input {
    app_keyboard_input keyboard;
    app_mouse_input mouse;
};

struct app_window_info {
    unsigned int w, h;
    bool resize, running;
};

struct TexturedShader {
    unsigned int program;
    unsigned int projection;
    unsigned int view;
    unsigned int model;
    unsigned int light_space_matrix;
    unsigned int shadow_map;
    unsigned int gamma_correction;
    unsigned int view_position;
    unsigned int texture;
};

struct DiffuseShader {
    unsigned int program;
    unsigned int projection;
    unsigned int view;
    unsigned int model;
    unsigned int light_space_matrix;
    unsigned int shadow_map;
    unsigned int gamma_correction;
    unsigned int view_position;
    unsigned int object_colour;
};

struct InterfaceShader {
    unsigned int program;
    unsigned int projection;
    unsigned int model;
    unsigned int texture;
};

struct DepthShader {
    unsigned int program;
    unsigned int projection;
    unsigned int view;
    unsigned int model;
};

struct OutlineShader {
    unsigned int program;
    unsigned int projection;
    unsigned int view;
    unsigned int model;
};

struct SkyboxShader {
    unsigned int program;
    unsigned int projection;
    unsigned int view;
    unsigned int skybox;
};

struct Vertex {
    V3 pos;
    V3 nor;
    V2 tex;
};

struct QuadIndices {
	unsigned int i[6];
};

struct Light {
    V3 pos, colour;
    float ambient, diffuse, specular;
};

struct Button {
    unsigned int texture;
    V2 pos, size;
    std::function<void(app_state*)> on_click;
};

struct app_state {
    app_window_info window_info;

    TexturedShader textured_shader;
    DiffuseShader diffuse_shader;
    InterfaceShader interface_shader;
    DepthShader depth_shader;
    OutlineShader outline_shader;
    SkyboxShader skybox_shader;

    Camera *cur_cam;
    Camera main_cam;
    Camera skeleton_cam;

    Light light_0, light_1;
    Skybox *skybox;

    std::vector<Button> buttons;
    std::vector<Node *> limbs;
    std::vector<Node> backup; // Stores the limbs when animation is played.
    std::vector<std::vector<Node>> key_frames;
    Node *selected;

    Object *box, *sphere;

    float model_stack[256];
    unsigned int depth;

    std::mt19937 rng;

    V3 ray_pos, ray_dir;
    V3 cylinders[20];

    unsigned int triangle_vao, quad_vbo, quad_ebo;
    unsigned int cylinder_vao, cylinder_vbo, cylinder_ebo;
    unsigned int interface_vao, interface_vbo, interface_ebo;
    unsigned int depth_map_fbo, depth_map;

    unsigned int floor_tex, pos_tex, rot_tex, x_tex, y_tex, z_tex, inc_tex, dec_tex, play_tex, cam1_tex, cam2_tex;
    
    unsigned int edit_mode;
    unsigned int axis;

    float dt;

    bool playing;
};

extern void app_update_and_render(float dt, app_state *state, app_input *input, app_window_info *window_info);
extern app_state *app_init(unsigned int w, unsigned int h);

#endif