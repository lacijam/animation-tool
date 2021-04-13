#include "app.h"

#include <assert.h>
#include <stdlib.h>
#include <string>

#include "maths.h"
#include "win32-opengl.h"
#include "opengl-util.h"
#include "shaders.h"

static u32 create_shader(const char *vertex_shader_source, const char *fragment_shader_source)
{
	u32 program_id = glCreateProgram();
	u32 vertex_shader = gl_compile_shader_from_source(vertex_shader_source, program_id, GL_VERTEX_SHADER);
	u32 fragment_shader = gl_compile_shader_from_source(fragment_shader_source, program_id, GL_FRAGMENT_SHADER);
	
	glAttachShader(program_id, vertex_shader);
	glAttachShader(program_id, fragment_shader);
	glLinkProgram(program_id);
	glUseProgram(program_id);

	glDeleteShader(vertex_shader);
	glDeleteShader(fragment_shader);

	return program_id;
}

static u32 create_framebuffer_texture(u32 width, u32 height)
{
	u32 tex = 0;
	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, tex, 0);
	return tex;
}

static void init_depth_map(app_state *state)
{
	glGenFramebuffers(1, &state->depth_map_fbo);

	glGenTextures(1, &state->depth_map);
	glBindTexture(GL_TEXTURE_2D, state->depth_map);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, 4096, 4096, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

	// Fixes artifacts on shadow edges.
	real32 border_color[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, border_color);

	glBindFramebuffer(GL_FRAMEBUFFER, state->depth_map_fbo);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, state->depth_map, 0);
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

static void app_on_destroy(app_state *state)
{
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	glDeleteVertexArrays(1, &state->triangle_vao);

	glDeleteBuffers(1, &state->quad_vbo);
	glDeleteBuffers(1, &state->quad_ebo);
	glDeleteVertexArrays(1, &state->triangle_vao);

	glDeleteProgram(state->simple_shader.program);
}

static void simple_shader_use(app_state *state)
{
	glUseProgram(state->simple_shader.program);

	glUniform1f(state->simple_shader.ambient_strength, state->light.ambient);
	glUniform1f(state->simple_shader.diffuse_strength, state->light.diffuse);
	glUniform1f(state->simple_shader.gamma_correction, 2.2);

	glUniform3fv(state->simple_shader.light_pos, 1, (GLfloat *)(&state->light.pos));
	glUniform3fv(state->simple_shader.light_colour, 1, (GLfloat *)&state->light.colour);

	glUniformMatrix4fv(state->simple_shader.projection, 1, GL_FALSE, state->cur_cam.frustrum);
	glUniformMatrix4fv(state->simple_shader.view, 1, GL_FALSE, state->cur_cam.view);

	glUniform1i(state->simple_shader.shadow_map, 0);
}

static void app_render_scene(app_state *state)
{
	glBindVertexArray(state->triangle_vao);
	glBindBuffer(GL_ARRAY_BUFFER, state->quad_vbo);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof Vertex, (void *)0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof Vertex, (void *)(3 * sizeof(real32)));

	real32 model[16];
	mat4_identity(model);
	mat4_scale(model, 100.f, 1.f, 100.f);
	
	glUniformMatrix4fv(state->simple_shader.model, 1, GL_FALSE, (GLfloat *)model);

	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}

static void app_render(app_state *state)
{
	real32 light_projection[16], light_view[16];
	mat4_identity(light_projection);
	mat4_identity(light_view);
	mat4_ortho(light_projection, -10.f, 10.f, -10.f, 10.f, 1.f, 10000.f);
	mat4_look_at(light_view, state->light.pos, { 0.f, 0.f, 0.f }, { 0.f, 1.f, 0.f });

	real32 light_space_matrix[16];
	mat4_identity(light_space_matrix);
	mat4_multiply(light_space_matrix, light_projection, light_view);

	glClearColor(0.1f, 0.1f, 0.1f, 1.f);

	// Render to frame buffer
	glViewport(0, 0, 2048, 2048);
	glBindFramebuffer(GL_FRAMEBUFFER, state->depth_map_fbo);
	glClear(GL_DEPTH_BUFFER_BIT);

	// Render the shadow map from the lights POV.
	glUseProgram(state->depth_shader.program);
	glUniformMatrix4fv(state->depth_shader.projection, 1, GL_FALSE, light_projection);
	glUniformMatrix4fv(state->depth_shader.view, 1, GL_FALSE, light_view);
	
	glCullFace(GL_FRONT);
	
	//render here
	//app_render_scene(state);

	glCullFace(GL_BACK);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, state->window_info.w, state->window_info.h);

	// Finally render to screen.
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, state->depth_map);
	
	simple_shader_use(state);

	real32 c[3] = { 1.f, 1.f, 1.f };

	glUniformMatrix4fv(state->simple_shader.light_space_matrix, 1, GL_FALSE, light_space_matrix);
	glUniform3fv(state->simple_shader.object_colour, 1, (GLfloat *)c);

	//render here
	app_render_scene(state);
}

app_state *app_init(u32 w, u32 h)
{
	app_state *state = new app_state;

	if (!state) {
		return nullptr;
	}

	state->window_info.w = w;
	state->window_info.h = h;
	state->window_info.resize = false;
	state->window_info.running = true;

	glGenVertexArrays(1, &state->triangle_vao);
	glBindVertexArray(state->triangle_vao);

	// --- Quad mesh
	real32 quad_verts[24] = {
		 0.5f,  0, 0.5f, 0, 1.f, 0,
		 0.5f, 0, -0.5f, 0, 1.f, 0,
		-0.5f, 0, -0.5f, 0, 1.f, 0,
		-0.5f,  0, 0.5f, 0, 1.f, 0,
	};

	u32 quad_indices[6] = {
		0, 1, 3,
		1, 2, 3
	};

	glGenBuffers(1, &state->quad_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, state->quad_vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof quad_verts, quad_verts, GL_STATIC_DRAW);

	glGenBuffers(1, &state->quad_ebo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, state->quad_ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof quad_indices, quad_indices, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof Vertex, (void *)0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof Vertex, (void *)(3 * sizeof(real32)));

	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	// ---End of quad mesh

	// ---Shaders
	state->simple_shader.program = create_shader(Shaders::SIMPLE_VERTEX_SHADER_SOURCE, Shaders::SIMPLE_FRAGMENT_SHADER_SOURCE);
	state->simple_shader.projection = glGetUniformLocation(state->simple_shader.program, "projection");
	state->simple_shader.view = glGetUniformLocation(state->simple_shader.program, "view");
	state->simple_shader.model = glGetUniformLocation(state->simple_shader.program, "model");
	state->simple_shader.light_space_matrix = glGetUniformLocation(state->simple_shader.program, "light_space_matrix");
	state->simple_shader.shadow_map = glGetUniformLocation(state->simple_shader.program, "shadow_map");
	state->simple_shader.ambient_strength = glGetUniformLocation(state->simple_shader.program, "ambient_strength");
	state->simple_shader.diffuse_strength = glGetUniformLocation(state->simple_shader.program, "diffuse_strength");
	state->simple_shader.gamma_correction = glGetUniformLocation(state->simple_shader.program, "gamma_correction");
	state->simple_shader.light_pos = glGetUniformLocation(state->simple_shader.program, "light_pos");
	state->simple_shader.light_colour = glGetUniformLocation(state->simple_shader.program, "light_colour");
	state->simple_shader.object_colour = glGetUniformLocation(state->simple_shader.program, "object_colour");
	
	state->depth_shader.program = create_shader(Shaders::DEPTH_VERTEX_SHADER_SOURCE, Shaders::DEPTH_FRAGMENT_SHADER_SOURCE);
	state->depth_shader.projection = glGetUniformLocation(state->depth_shader.program, "projection");
	state->depth_shader.view = glGetUniformLocation(state->depth_shader.program, "view");
	state->depth_shader.model = glGetUniformLocation(state->depth_shader.program, "model");
	// ---End of shaders

	state->rng = std::mt19937(0);

	camera_init(&state->cur_cam);
	state->cur_cam.pos = { 0.f, 10.f, 0.f };
	state->cur_cam.front = { 0.f, 0.f, 0.f };

	glViewport(0, 0, state->window_info.w, state->window_info.h);
	camera_frustrum(&state->cur_cam, state->window_info.w, state->window_info.h);
	camera_ortho(&state->cur_cam, state->window_info.w, state->window_info.h);

	state->light.pos = { 0.f, 10.f, 0.f };
	state->light.colour = { 1.f, 1.f, 1.f };
	state->light.ambient = 0.5f;
	state->light.diffuse = 0.5f;
	state->light.specular = 0.3f;

	init_depth_map(state);

	return state;
}

void app_handle_input(real32 dt, app_state *state, app_keyboard_input *keyboard)
{
	if (keyboard->forward.ended_down) {
		camera_move_forward(&state->cur_cam, dt);
	}
	else if (keyboard->backward.ended_down) {
		camera_move_backward(&state->cur_cam, dt);
	}

	if (keyboard->left.ended_down) {
		camera_move_left(&state->cur_cam, dt);
	}
	else if (keyboard->right.ended_down) {
		camera_move_right(&state->cur_cam, dt);
	}

	if (keyboard->cam_up.ended_down) {
		state->cur_cam.pitch += state->cur_cam.look_speed * dt;
	}
	else if (keyboard->cam_down.ended_down) {
		state->cur_cam.pitch -= state->cur_cam.look_speed * dt;
	}

	if (keyboard->cam_left.ended_down) {
		state->cur_cam.yaw -= state->cur_cam.look_speed * dt;
	}
	else if (keyboard->cam_right.ended_down) {
		state->cur_cam.yaw += state->cur_cam.look_speed * dt;
	}

	if (keyboard->fly.toggled) {
		state->cur_cam.flying = !state->cur_cam.flying;
	}
}

void app_update(app_state *state)
{
	camera_update(&state->cur_cam);
	camera_look_at(&state->cur_cam);
}

void app_update_and_render(real32 dt, app_state *state, app_input *input, app_window_info *window_info)
{
	state->window_info = *window_info;

	if (!window_info->running) {
		app_on_destroy(state);
		return;
	}

	if (window_info->resize) {
		glViewport(0, 0, window_info->w, window_info->h);
		camera_frustrum(&state->cur_cam, window_info->w, window_info->h);
	}

	app_handle_input(dt, state, &input->keyboard);
	app_update(state);
	app_render(state);
}