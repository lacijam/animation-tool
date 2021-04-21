#include "app.h"

#include <assert.h>
#include <stdlib.h>
#include <string>
#include <algorithm>

#include "maths.h"
#include "win32-opengl.h"
#include "opengl-util.h"
#include "shaders.h"
#include "bitmap.h"

static const float SPACING = 1.f;
static const float ROOT_WIDTH = 5;
static const float LEG_WIDTH = 1;
static const float LEG_HEIGHT = 3;
static const float ARM_WIDTH = 3;
static const float ARM_HEIGHT = 1;
static const float SPINE_WIDTH = 1;
static const float SPINE_HEIGHT = 3;
static const float ROOT_LEG_OFFSET = (ROOT_WIDTH - 1.f) / 2.f;
static const float SPINE_ARM_OFFSET = SPINE_WIDTH / 2.f + SPACING;
static const float LOWER_ARM_OFFSET = ARM_WIDTH + SPACING;
static const float LOWER_LEG_OFFSET = LEG_HEIGHT + SPACING;
static const float LOWER_SPINE_OFFSET = 2 * SPACING;
static const float UPPER_SPINE_OFFSET = SPINE_HEIGHT + SPACING;

static const float LIMB_MOVE_RATE = 5.f;
static const float LIMB_ROTATE_RATE = 50.f;

static const int SEGMENTS = 12; // for cylinder

static float lerp(float v0, float v1, float t)
{
	return (1 - t) * v0 + t * v1;
}

#define TOP_MODEL state->model_stack + state->depth

static void push_matrix(app_state *state)
{
	assert(state->depth + 16 < 256);
	CopyMemory(TOP_MODEL + 16, TOP_MODEL, 16 * sizeof(float));
	state->depth += 16;
}

static void pop_matrix(app_state *state)
{
	assert(state->depth > 0);
	state->depth -= 16;
}

static void init_shaders(app_state *state)
{
	state->textured_shader.program = create_shader(Shaders::TEXTURED_VERTEX_SHADER_SOURCE, Shaders::TEXTURED_FRAGMENT_SHADER_SOURCE);
	state->textured_shader.projection = glGetUniformLocation(state->textured_shader.program, "projection");
	state->textured_shader.view = glGetUniformLocation(state->textured_shader.program, "view");
	state->textured_shader.model = glGetUniformLocation(state->textured_shader.program, "model");
	state->textured_shader.light_space_matrix = glGetUniformLocation(state->textured_shader.program, "light_space_matrix");
	state->textured_shader.shadow_map = glGetUniformLocation(state->textured_shader.program, "shadow_map");
	state->textured_shader.gamma_correction = glGetUniformLocation(state->textured_shader.program, "gamma_correction");
	state->textured_shader.view_position = glGetUniformLocation(state->textured_shader.program, "view_position");
	state->textured_shader.texture = glGetUniformLocation(state->textured_shader.program, "tex");

	state->diffuse_shader.program = create_shader(Shaders::TEXTURED_VERTEX_SHADER_SOURCE, Shaders::DIFFUSE_FRAGMENT_SHADER_SOURCE);
	state->diffuse_shader.projection = glGetUniformLocation(state->diffuse_shader.program, "projection");
	state->diffuse_shader.view = glGetUniformLocation(state->diffuse_shader.program, "view");
	state->diffuse_shader.model = glGetUniformLocation(state->diffuse_shader.program, "model");
	state->diffuse_shader.light_space_matrix = glGetUniformLocation(state->diffuse_shader.program, "light_space_matrix");
	state->diffuse_shader.shadow_map = glGetUniformLocation(state->diffuse_shader.program, "shadow_map");
	state->diffuse_shader.gamma_correction = glGetUniformLocation(state->diffuse_shader.program, "gamma_correction");
	state->diffuse_shader.view_position = glGetUniformLocation(state->diffuse_shader.program, "view_position");
	state->diffuse_shader.object_colour = glGetUniformLocation(state->diffuse_shader.program, "object_colour");

	state->depth_shader.program = create_shader(Shaders::DEPTH_VERTEX_SHADER_SOURCE, Shaders::DEPTH_FRAGMENT_SHADER_SOURCE);
	state->depth_shader.projection = glGetUniformLocation(state->depth_shader.program, "projection");
	state->depth_shader.view = glGetUniformLocation(state->depth_shader.program, "view");
	state->depth_shader.model = glGetUniformLocation(state->depth_shader.program, "model");

	state->interface_shader.program = create_shader(Shaders::INTERFACE_VERTEX_SHADER_SOURCE, Shaders::INTERFACE_FRAGMENT_SHADER_SOURCE);
	state->interface_shader.projection = glGetUniformLocation(state->interface_shader.program, "projection");
	state->interface_shader.model = glGetUniformLocation(state->interface_shader.program, "model");
	state->interface_shader.texture = glGetUniformLocation(state->interface_shader.program, "tex");

	state->outline_shader.program = create_shader(Shaders::OUTLINE_VERTEX_SHADER_SOURCE, Shaders::OUTLINE_FRAGMENT_SHADER_SOURCE);
	state->outline_shader.projection = glGetUniformLocation(state->outline_shader.program, "projection");
	state->outline_shader.view = glGetUniformLocation(state->outline_shader.program, "view");
	state->outline_shader.model = glGetUniformLocation(state->outline_shader.program, "model");

	state->skybox_shader.program = create_shader(Shaders::SKYBOX_VERTEX_SHADER_SOURCE, Shaders::SKYBOX_FRAGMENT_SHADER_SOURCE);
	state->skybox_shader.projection = glGetUniformLocation(state->skybox_shader.program, "projection");
	state->skybox_shader.view = glGetUniformLocation(state->skybox_shader.program, "view");
	state->skybox_shader.skybox = glGetUniformLocation(state->skybox_shader.program, "skybox");
}

static void load_bitmaps(app_state *state)
{
	const char BITMAPS_LENGTH = 11;
	const char *bitmap_strings[BITMAPS_LENGTH] = {
		"floor.bmp",
		"position.bmp",
		"rotation.bmp",
		"x.bmp",
		"y.bmp",
		"z.bmp",
		"increase.bmp",
		"decrease.bmp",
		"play.bmp",
		"cam1.bmp",
		"cam2.bmp"
	};

	std::vector<Bitmap *> bitmaps;
	for (unsigned int i = 0; i < BITMAPS_LENGTH; i++) {
		bitmaps.push_back(load_bitmap(bitmap_strings[i]));
	}

	state->floor_tex = create_texture(bitmaps[0]);
	state->pos_tex = create_texture(bitmaps[1]);
	state->rot_tex = create_texture(bitmaps[2]);
	state->x_tex = create_texture(bitmaps[3]);
	state->y_tex = create_texture(bitmaps[4]);
	state->z_tex = create_texture(bitmaps[5]);
	state->inc_tex = create_texture(bitmaps[6]);
	state->dec_tex = create_texture(bitmaps[7]);
	state->play_tex = create_texture(bitmaps[8]);
	state->cam1_tex = create_texture(bitmaps[9]);
	state->cam2_tex = create_texture(bitmaps[10]);

	for (auto &b : bitmaps) {
		delete b;
	}
}

static void create_skeleton(app_state *state)
{
	Node *root = create_node();
	root->translation = { 0, 8, 0 };
	root->scale = { ROOT_WIDTH, 1, 1 };

	Node *upper_left_leg = create_node();
	upper_left_leg->translation = { -ROOT_LEG_OFFSET, -SPACING, 0 };
	upper_left_leg->scale = { LEG_WIDTH, LEG_HEIGHT, LEG_WIDTH };
	upper_left_leg->rotation = { 0, 0, 180 };

	Node *upper_right_leg = create_node();
	upper_right_leg->translation = { ROOT_LEG_OFFSET, -SPACING, 0 };
	upper_right_leg->scale = { LEG_WIDTH, LEG_HEIGHT, LEG_WIDTH };
	upper_right_leg->rotation = { 0, 0, 180 };

	Node *lower_left_leg = create_node();
	lower_left_leg->translation = { 0, LOWER_LEG_OFFSET, 0 };
	lower_left_leg->scale = { LEG_WIDTH, LEG_HEIGHT, LEG_WIDTH };
	lower_left_leg->rotation = { 0, 0, 0 };
	lower_left_leg->flip = true;

	Node *lower_right_leg = create_node();
	lower_right_leg->translation = { 0, LOWER_LEG_OFFSET, 0 };
	lower_right_leg->scale = { LEG_WIDTH, LEG_HEIGHT, LEG_WIDTH };
	lower_right_leg->rotation = { 0, 0, 0 };
	lower_right_leg->flip = true;

	Node *lower_spine = create_node();
	lower_spine->translation = { 0, LOWER_SPINE_OFFSET, 0 };
	lower_spine->scale = { SPINE_WIDTH, SPINE_HEIGHT, SPINE_WIDTH };
	lower_spine->rotation = { 0, 0, 0 };

	Node *upper_spine = create_node();
	upper_spine->translation = { 0, UPPER_SPINE_OFFSET, 0 };
	upper_spine->scale = { SPINE_WIDTH, SPINE_HEIGHT, SPINE_WIDTH };
	upper_spine->rotation = { 0, 0, 0 };

	Node *upper_left_arm = create_node();
	upper_left_arm->translation = { -SPINE_ARM_OFFSET, 0, 0 };
	upper_left_arm->scale = { ARM_HEIGHT, ARM_WIDTH, ARM_HEIGHT };
	upper_left_arm->rotation = { 0, 0, 90 };

	Node *upper_right_arm = create_node();
	upper_right_arm->translation = { SPINE_ARM_OFFSET, 0, 0 };
	upper_right_arm->scale = { ARM_HEIGHT, ARM_WIDTH, ARM_HEIGHT };
	upper_right_arm->rotation = { 0, 0, -90 };

	Node *lower_left_arm = create_node();
	lower_left_arm->translation = { 0, LOWER_ARM_OFFSET, 0 };
	lower_left_arm->scale = { ARM_HEIGHT, ARM_WIDTH, ARM_HEIGHT };
	lower_left_arm->rotation = { 0, 0, 10 };

	Node *lower_right_arm = create_node();
	lower_right_arm->translation = { 0, LOWER_ARM_OFFSET, 0 };
	lower_right_arm->scale = { ARM_HEIGHT, ARM_WIDTH, ARM_HEIGHT };
	lower_right_arm->rotation = { 0, 0, -10 };

	root->children.push_back(upper_left_leg);
	root->children.push_back(upper_right_leg);
	root->children.push_back(lower_spine);
	lower_spine->children.push_back(upper_spine);
	upper_spine->children.push_back(upper_left_arm);
	upper_spine->children.push_back(upper_right_arm);
	upper_left_arm->children.push_back(lower_left_arm);
	upper_right_arm->children.push_back(lower_right_arm);
	upper_left_leg->children.push_back(lower_left_leg);
	upper_right_leg->children.push_back(lower_right_leg);

	state->limbs.push_back(root);
	state->limbs.push_back(upper_spine);
	state->limbs.push_back(lower_spine);
	state->limbs.push_back(upper_left_arm);
	state->limbs.push_back(upper_right_arm);
	state->limbs.push_back(lower_left_arm);
	state->limbs.push_back(lower_right_arm);
	state->limbs.push_back(upper_left_leg);
	state->limbs.push_back(upper_right_leg);
	state->limbs.push_back(lower_left_leg);
	state->limbs.push_back(lower_right_leg);
}

static void create_animation(app_state *state)
{
	std::vector<Node> frame1;
	for (unsigned int i = 0; i < 11; i++) {
		frame1.push_back(*state->limbs[i]);
	}

	frame1[3].rotation.x  = -26.f;
	frame1[3].rotation.z  = 161.f;
	frame1[4].rotation.x  = 47.f;
	frame1[4].rotation.z  = -157.f;
	frame1[5].rotation.x  = -61.f;
	frame1[5].rotation.z  = 10.f;
	frame1[6].rotation.x  = -94.f;
	frame1[6].rotation.z  = -10.f;
	frame1[7].rotation.x  = 48.f;
	frame1[8].rotation.x  = -59.f;
	frame1[9].rotation.x  = -14.f;
	frame1[10].rotation.x = 53.f;

	std::vector<Node> frame2;
	for (unsigned int i = 0; i < 11; i++) {
		frame2.push_back(*state->limbs[i]);
	}

	frame2[3].rotation.x = 70.f;
	frame2[3].rotation.z = 152.f;
	frame2[4].rotation.x = -40.f;
	frame2[4].rotation.z = -148.f;
	frame2[5].rotation.x = -91.f;
	frame2[5].rotation.z = 9.f;
	frame2[6].rotation.x = -78.f;
	frame2[6].rotation.z = -2.f;
	frame2[7].rotation.x = -45.f;
	frame2[8].rotation.x = 44.f;
	frame2[9].rotation.x = 48.f;
	frame2[10].rotation.x = 61.f;

	std::vector<Node> frame3;
	for (unsigned int i = 0; i < 11; i++) {
		frame3.push_back(*state->limbs[i]);
	}

	frame3[3].rotation.x = -26.f;
	frame3[3].rotation.z = 161.f;
	frame3[4].rotation.x = 47.f;
	frame3[4].rotation.z = -157.f;
	frame3[5].rotation.x = -61.f;
	frame3[5].rotation.z = 10.f;
	frame3[6].rotation.x = -94.f;
	frame3[6].rotation.z = -10.f;
	frame3[7].rotation.x = 48.f;
	frame3[8].rotation.x = -59.f;
	frame3[9].rotation.x = -14.f;
	frame3[10].rotation.x = 53.f;

	std::vector<Node> frame4;
	for (unsigned int i = 0; i < 11; i++) {
		frame4.push_back(*state->limbs[i]);
	}

	frame4[3].rotation.x = 70.f;
	frame4[3].rotation.z = 152.f;
	frame4[4].rotation.x = -40.f;
	frame4[4].rotation.z = -148.f;
	frame4[5].rotation.x = -91.f;
	frame4[5].rotation.z = 9.f;
	frame4[6].rotation.x = -78.f;
	frame4[6].rotation.z = -2.f;
	frame4[7].rotation.x = -45.f;
	frame4[8].rotation.x = 44.f;
	frame4[9].rotation.x = 48.f;
	frame4[10].rotation.x = 61.f;

	std::vector<Node> frame5;
	for (unsigned int i = 0; i < 11; i++) {
		frame5.push_back(*state->limbs[i]);
	}

	frame5[3].rotation.x = -26.f;
	frame5[3].rotation.z = 161.f;
	frame5[4].rotation.x = 47.f;
	frame5[4].rotation.z = -157.f;
	frame5[5].rotation.x = -61.f;
	frame5[5].rotation.z = 10.f;
	frame5[6].rotation.x = -94.f;
	frame5[6].rotation.z = -10.f;
	frame5[7].rotation.x = 48.f;
	frame5[8].rotation.x = -59.f;
	frame5[9].rotation.x = -14.f;
	frame5[10].rotation.x = 53.f;

	state->key_frames.push_back(frame1);
	state->key_frames.push_back(frame2);
	state->key_frames.push_back(frame3);
	state->key_frames.push_back(frame4);
	state->key_frames.push_back(frame5);
}

// Interpolates the transformation data of two frames.
// t is the time in the animation.
// e.g t = 1.5 will interpolate frame 1 and frame 2.
//     t = 2.2 will interpolate frame 2 and frame 3.
static void get_frame(app_state *state, const float t)
{
	const unsigned int frames = state->key_frames.size();
	const unsigned int frame = (unsigned int)t;
	const float t_frame = t - frame;
	
	// Prevent any accidental overflows.
	if (t + 0.0000001f >= frames) {
		return;
	}

	const std::vector<Node> this_data = state->key_frames[frame];
	const std::vector<Node> next_data = state->key_frames[frame + 1];

	for (unsigned int i = 0; i < this_data.size(); i++) {
		state->limbs[i]->translation.x = lerp(this_data[i].translation.x, next_data[i].translation.x, t_frame);
		state->limbs[i]->translation.y = lerp(this_data[i].translation.y, next_data[i].translation.y, t_frame);
		state->limbs[i]->translation.z = lerp(this_data[i].translation.z, next_data[i].translation.z, t_frame);

		state->limbs[i]->rotation.x = lerp(this_data[i].rotation.x, next_data[i].rotation.x, t_frame);
		state->limbs[i]->rotation.y = lerp(this_data[i].rotation.y, next_data[i].rotation.y, t_frame);
		state->limbs[i]->rotation.z = lerp(this_data[i].rotation.z, next_data[i].rotation.z, t_frame);

		state->limbs[i]->scale.x = lerp(this_data[i].scale.x, next_data[i].scale.x, t_frame);
		state->limbs[i]->scale.y = lerp(this_data[i].scale.y, next_data[i].scale.y, t_frame);
		state->limbs[i]->scale.z = lerp(this_data[i].scale.z, next_data[i].scale.z, t_frame);
	}
}

static void init_meshes(app_state *state)
{
	glGenVertexArrays(1, &state->triangle_vao);
	glBindVertexArray(state->triangle_vao);

	// --- Quad mesh
	float quad_verts[32] = {
		 0.5f,  0, 0.5f, 0, 1.f, 0, 1.f, 0.f,
		 0.5f, 0, -0.5f, 0, 1.f, 0, 1.f, 1.f,
		-0.5f, 0, -0.5f, 0, 1.f, 0, 0.f, 1.f,
		-0.5f,  0, 0.5f, 0, 1.f, 0, 0.f, 0.f,
	};

	unsigned int quad_indices[6] = {
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
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof Vertex, (void *)(3 * sizeof(float)));
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof Vertex, (void *)(6 * sizeof(float)));

	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glEnableVertexAttribArray(2);

	// ---End of quad mesh

	// --- cylinder mesh
	const float length = 3.f;

	// Calculate angle between each triangle.
	const float theta = (2 * M_PI) / SEGMENTS;
	const float r = 0.5f;

	// Center vertex.
	const unsigned int face_verts_count = SEGMENTS + 1;
	const unsigned int cylinder_verts_count = face_verts_count * 2;

	const unsigned int face_verts_data_offset = face_verts_count * 8;

	// Array for vertex points.
	const unsigned VERT_DATA_COUNT = face_verts_data_offset * 2;
	float *CYLINDER = (float *)calloc(VERT_DATA_COUNT, sizeof(float));

	CYLINDER[4] = 1.f;
	CYLINDER[face_verts_count + 1] = length;
	CYLINDER[face_verts_count + 4] = 1.f;

	// Calculate x,y for each point.
	for (int i = 1, j = i - 1; i < face_verts_count; ++i, ++j) {
		const int k = i * 8;
		CYLINDER[k + 0] = r * sin(j * theta);
		CYLINDER[k + 1] = 0.f;
		CYLINDER[k + 2] = r * cos(j * theta);
		CYLINDER[k + 3] = r * sin(j * theta);
		CYLINDER[k + 4] = -1.f;
		CYLINDER[k + 5] = r * cos(j * theta);
		CYLINDER[k + 6] = 0.f;
		CYLINDER[k + 7] = 0.f;

		// 2nd circle face.
		const int l = (face_verts_count * 8) + i * 8;
		CYLINDER[l + 0] = r * sin(j * theta);
		CYLINDER[l + 1] = length;
		CYLINDER[l + 2] = r * cos(j * theta);
		CYLINDER[l + 3] = r * sin(j * theta);
		CYLINDER[l + 4] = 1.f;
		CYLINDER[l + 5] = r * cos(j * theta);
		CYLINDER[l + 6] = 0.f;
		CYLINDER[l + 7] = 0.f;
	}

	const unsigned int face_indices_offset = SEGMENTS * 3;

	// Polygon data.
	const unsigned POLY_DATA_COUNT = face_indices_offset * 4;
	unsigned int *POLY = (unsigned int *)calloc(POLY_DATA_COUNT, sizeof(unsigned int));

	// Bottom face
	for (int i = 0; i < SEGMENTS; ++i) {
		const int j = i * 3;
		POLY[j + 0] = 0;
		POLY[j + 1] = face_verts_count - i - 2;
		POLY[j + 2] = face_verts_count - i - 1;
	}

	// Top face
	for (int i = 0; i < SEGMENTS; ++i) {
		const int j = (SEGMENTS + i) * 3;
		POLY[j + 0] = face_verts_count;
		POLY[j + 1] = cylinder_verts_count - i - 2;
		POLY[j + 2] = cylinder_verts_count - i - 1;
	}

	// Triangle 1 connecting face
	for (int i = 0; i < SEGMENTS; ++i) {
		const int j = (SEGMENTS * 2 + i) * 3;
		POLY[j + 0] = face_verts_count - i - 2;
		POLY[j + 1] = face_verts_count - i - 1;
		POLY[j + 2] = cylinder_verts_count - i - 1;
	}

	// Triangle 2 connecting face
	for (int i = 0; i < SEGMENTS; ++i) {
		const int j = (SEGMENTS * 3 + i) * 3;
		POLY[j + 0] = face_verts_count - i - 2;
		POLY[j + 1] = cylinder_verts_count - i - 1;
		POLY[j + 2] = cylinder_verts_count - i - 2;
	}

	POLY[POLY_DATA_COUNT - (face_indices_offset * 3) - 2] = SEGMENTS;
	POLY[POLY_DATA_COUNT - (face_indices_offset * 2) - 2] = SEGMENTS * 2 + 1;
	POLY[POLY_DATA_COUNT - (face_indices_offset * 1) - 3] = SEGMENTS;

	POLY[POLY_DATA_COUNT - (face_indices_offset * 0) - 3] = SEGMENTS;
	POLY[POLY_DATA_COUNT - (face_indices_offset * 0) - 1] = SEGMENTS * 2 + 1;

	glGenVertexArrays(1, &state->cylinder_vao);
	glBindVertexArray(state->cylinder_vao);

	glGenBuffers(1, &state->cylinder_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, state->cylinder_vbo);
	glBufferData(GL_ARRAY_BUFFER, VERT_DATA_COUNT * sizeof(float), CYLINDER, GL_STATIC_DRAW);

	glGenBuffers(1, &state->cylinder_ebo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, state->cylinder_ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, POLY_DATA_COUNT * sizeof(unsigned int), POLY, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof Vertex, (void *)0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof Vertex, (void *)(3 * sizeof(float)));
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof Vertex, (void *)(6 * sizeof(float)));

	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glEnableVertexAttribArray(2);

	free(POLY);
	free(CYLINDER);
	// --- end of cylinder mesh
	
	//
	glGenVertexArrays(1, &state->interface_vao);
	glBindVertexArray(state->interface_vao);

	// --- interface mesh
	float interface_verts[16] = {
		 1.f, 1.f, 1.f, 0.f,
		 1.f, 0.f, 1.f, 1.f,
		 0.f, 0.f, 0.f, 1.f,
		 0.f, 1.f, 0.f, 0.f,
	};

	unsigned int interface_indices[6] = {
		0, 1, 3,
		1, 2, 3
	};

	glGenBuffers(1, &state->interface_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, state->interface_vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof interface_verts, interface_verts, GL_STATIC_DRAW);

	glGenBuffers(1, &state->interface_ebo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, state->interface_ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof interface_indices, interface_indices, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)0);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)(2 * sizeof(float)));

	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
}


static bool is_between(float value, float min, float max)
{
	return value >= min && value <= max;
}

static bool overlaps(float min1, float max1, float min2, float max2)
{
	return is_between(min2, min1, max1) || is_between(min1, min2, max2);
}

static bool check_limb_collisions(std::vector<Node*> &limbs)
{
	const unsigned int NOOFPTS = 8;
	float points[4 * NOOFPTS] = {
		-0.55f, -0.05f, -0.55f, 1.f,
		-0.55f, -0.05f,  0.55f, 1.f,
		 0.55f, -0.05f, -0.55f, 1.f,
		 0.55f, -0.05f,  0.55f, 1.f,
		-0.55f,  1.05f, -0.55f, 1.f,
		-0.55f,  1.05f,  0.55f, 1.f,
		 0.55f,  1.05f, -0.55f, 1.f,
		 0.55f,  1.05f,  0.55f, 1.f
	};

	for (unsigned int l1 = 0; l1 < limbs.size(); l1++) {
		for (unsigned int l2 = 0; l2 < limbs.size(); l2++) {
			if (l1 == l2) {
				continue;
			}

			float arm_points[4 * NOOFPTS];
			float root_points[4 * NOOFPTS];

			for (unsigned int p = 0; p < NOOFPTS; p++) {
				float *current_point = points + 4 * p;
				float *arm_current_point = arm_points + 4 * p;
				float *root_current_point = root_points + 4 * p;

				for (unsigned int i = 0; i < 4; i++) {
					arm_current_point[i] = 0;
					root_current_point[i] = 0;
					for (unsigned int j = 0; j < 4; j++) {
						arm_current_point[i] += limbs[l1]->model[i + 4 * j] * current_point[j];
						root_current_point[i] += limbs[l2]->model[i + 4 * j] * current_point[j];
					}
				}
			}

			bool intersect = true;

			for (unsigned int o = 0; o < 2 && intersect; o++) {
				Node *source = (o == 0) ? limbs[l1] : limbs[l2];

				for (unsigned int a = 0; a < 3 && intersect; a++) {
					V3 nor;
					nor.E[0] = source->model[a * 4 + 0];
					nor.E[1] = source->model[a * 4 + 1];
					nor.E[2] = source->model[a * 4 + 2];
					nor = v3_normalise(nor);

					float min_along_arm, max_along_arm;
					min_along_arm = max_along_arm = v3_dot(nor, v4_to_v3(*(V4 *)arm_points));
					for (unsigned int p = 1; p < NOOFPTS; p++) {
						V3 temp = { arm_points[(4 * p) + 0], arm_points[(4 * p) + 1], arm_points[(4 * p) + 2] };
						float distance = v3_dot(nor, temp);
						if (distance < min_along_arm) {
							min_along_arm = distance;
						}
						else if (distance > max_along_arm) {
							max_along_arm = distance;
						}
					}

					float min_along_root, max_along_root;
					min_along_root = max_along_root = v3_dot(nor, v4_to_v3(*(V4 *)root_points));
					for (unsigned int p = 1; p < NOOFPTS; p++) {
						V3 temp = { root_points[(4 * p) + 0], root_points[(4 * p) + 1], root_points[(4 * p) + 2] };
						float distance = v3_dot(nor, temp);
						if (distance < min_along_root) {
							min_along_root = distance;
						}
						else if (distance > max_along_root) {
							max_along_root = distance;
						}
					}

					if (!overlaps(min_along_arm, max_along_arm, min_along_root, max_along_root)) {
						intersect = false;
					}
				}
			}

			if (intersect) {
				return true;
			}
		}
	}

	return false;
}

static void update_node_tree(app_state *state, Node *node)
{
	push_matrix(state);

	mat4_translate(TOP_MODEL, node->translation.x, node->translation.y, node->translation.z);

	mat4_rotate_z(TOP_MODEL, node->rotation.z);
	mat4_rotate_y(TOP_MODEL, node->rotation.y);
	mat4_rotate_x(TOP_MODEL, node->rotation.x);

	push_matrix(state);

	mat4_scale(TOP_MODEL, node->scale.x, node->scale.y, node->scale.z);

	memcpy(node->model, TOP_MODEL, 16 * sizeof(float));

	pop_matrix(state);

	for (unsigned int i = 0; i < node->children.size(); i++) {
		update_node_tree(state, node->children[i]);
	}

	pop_matrix(state);
}

static void create_ui(app_state *state)
{
	Button b;
	b.texture = state->pos_tex;
	b.pos = { 20, 20 };
	b.size = { 70, 50 };
	b.on_click = [&](app_state *state)
	{
		state->edit_mode = 0;
	};
	state->buttons.push_back(b);

	b.texture = state->rot_tex;
	b.pos = { 100, 20 };
	b.size = { 70, 50 };
	b.on_click = [&](app_state *state)
	{
		state->edit_mode = 1;
	};
	state->buttons.push_back(b);

	b.texture = state->x_tex;
	b.pos = { 20, 80 };
	b.size = { 50, 50 };
	b.on_click = [&](app_state *state)
	{
		state->axis = 0;
	};
	state->buttons.push_back(b);

	b.texture = state->y_tex;
	b.pos = { 80, 80 };
	b.size = { 50, 50 };
	b.on_click = [&](app_state *state)
	{
		state->axis = 1;
	};
	state->buttons.push_back(b);

	b.texture = state->z_tex;
	b.pos = { 140, 80 };
	b.size = { 50, 50 };
	b.on_click = [&](app_state *state)
	{
		state->axis = 2;
	};
	state->buttons.push_back(b);

	b.texture = state->dec_tex;
	b.pos = { 20, 140 };
	b.size = { 50, 50 };
	b.on_click = [&](app_state *state)
	{
		if (state->selected) {
			if (state->edit_mode == 0) {
				state->selected->translation.E[state->axis] -= LIMB_MOVE_RATE * state->dt;
			} else {
				state->selected->rotation.E[state->axis] -= LIMB_ROTATE_RATE * state->dt;
			}

			update_node_tree(state, state->limbs[0]);

			if (check_limb_collisions(state->limbs)) {
				if (state->edit_mode == 0) {
					state->selected->translation.E[state->axis] += LIMB_MOVE_RATE * state->dt;
				} else {
					state->selected->rotation.E[state->axis] += LIMB_ROTATE_RATE * state->dt;
				}
			}
		}
	};
	state->buttons.push_back(b);

	b.texture = state->inc_tex;
	b.pos = { 80, 140 };
	b.size = { 50, 50 };
	b.on_click = [&](app_state *state)
	{
		if (state->selected) {
			if (state->edit_mode == 0) {
				state->selected->translation.E[state->axis] += LIMB_MOVE_RATE * state->dt;
			} else {
				state->selected->rotation.E[state->axis] += LIMB_ROTATE_RATE * state->dt;
			}

			update_node_tree(state, state->limbs[0]);

			if (check_limb_collisions(state->limbs)) {
				if (state->edit_mode == 0) {
					state->selected->translation.E[state->axis] -= LIMB_MOVE_RATE * state->dt;
				} else {
					state->selected->rotation.E[state->axis] -= LIMB_ROTATE_RATE * state->dt;
				}
			}
		}
	};
	state->buttons.push_back(b);

	b.texture = state->play_tex;
	b.pos = { 20, 200 };
	b.size = { 50, 50 };
	b.on_click = [&](app_state *state)
	{
		state->playing = true;
		for (unsigned int i = 0; i < state->limbs.size(); i++) {
			state->backup[i] = *state->limbs[i];
		}
	};
	state->buttons.push_back(b);

	b.texture = state->cam1_tex;
	b.pos = { 20, 260 };
	b.size = { 50, 50 };
	b.on_click = [&](app_state *state)
	{
		state->cur_cam = &state->main_cam;
	};
	state->buttons.push_back(b);

	b.texture = state->cam2_tex;
	b.pos = { 80, 260 };
	b.size = { 50, 50 };
	b.on_click = [&](app_state *state)
	{
		state->skeleton_cam.pos = { state->limbs[1]->model[12], state->limbs[1]->model[13] + state->limbs[1]->scale.y + 1.f, state->limbs[1]->model[14] };
		state->skeleton_cam.front = { 0.f, 0.f, -1.f };

		// Slight hack. Sum the rotations of the root, lower spine and upper spine as they are the only limbs
		// that can affect the camera orientation.
		const float total_x_rotation = state->limbs[0]->rotation.x + state->limbs[1]->rotation.x + state->limbs[2]->rotation.x;
		const float total_y_rotation = state->limbs[0]->rotation.y + state->limbs[1]->rotation.y + state->limbs[2]->rotation.y;
		const float total_z_rotation = state->limbs[0]->rotation.z + state->limbs[1]->rotation.z + state->limbs[2]->rotation.z;

		state->skeleton_cam.yaw = 90.f - total_y_rotation;
		state->skeleton_cam.pitch = -total_x_rotation * cosf(total_y_rotation) - total_z_rotation * cosf(total_y_rotation);
		camera_update(&state->skeleton_cam);
		camera_look_at(&state->skeleton_cam);
		state->cur_cam = &state->skeleton_cam;
	};
	state->buttons.push_back(b);
}

static void textured_shader_use(app_state *state)
{
	glUseProgram(state->textured_shader.program);

	glUniform1f(state->textured_shader.gamma_correction, 2.2);

	glUniformMatrix4fv(state->textured_shader.projection, 1, GL_FALSE, state->cur_cam->frustrum);
	glUniformMatrix4fv(state->textured_shader.view, 1, GL_FALSE, state->cur_cam->view);

	glUniform3fv(glGetUniformLocation(state->textured_shader.program, "lights[0].pos"), 1, (GLfloat *)&state->light_0.pos);
	glUniform3fv(glGetUniformLocation(state->textured_shader.program, "lights[0].colour"), 1, (GLfloat *)&state->light_0.colour);
	glUniform1f(glGetUniformLocation(state->textured_shader.program, "lights[0].ambient"), state->light_0.ambient);
	glUniform1f(glGetUniformLocation(state->textured_shader.program, "lights[0].diffuse"), state->light_0.diffuse);

	glUniform3fv(glGetUniformLocation(state->textured_shader.program, "lights[1].pos"), 1, (GLfloat *)&state->light_1.pos);
	glUniform3fv(glGetUniformLocation(state->textured_shader.program, "lights[1].colour"), 1, (GLfloat *)&state->light_1.colour);
	glUniform1f(glGetUniformLocation(state->textured_shader.program, "lights[1].ambient"), state->light_1.ambient);
	glUniform1f(glGetUniformLocation(state->textured_shader.program, "lights[1].diffuse"), state->light_1.diffuse);

	glUniform3fv(state->textured_shader.view_position, 1, (GLfloat*)&state->cur_cam->pos);

	glUniform1i(state->textured_shader.shadow_map, 0);
	glUniform1i(state->textured_shader.texture, 1);
}

static void diffuse_shader_use(app_state *state)
{
	glUseProgram(state->diffuse_shader.program);

	glUniform1f(state->diffuse_shader.gamma_correction, 2.2);

	glUniformMatrix4fv(state->diffuse_shader.projection, 1, GL_FALSE, state->cur_cam->frustrum);
	glUniformMatrix4fv(state->diffuse_shader.view, 1, GL_FALSE, state->cur_cam->view);

	glUniform3fv(glGetUniformLocation(state->diffuse_shader.program, "lights[0].pos"), 1, (GLfloat *)&state->light_0.pos);
	glUniform3fv(glGetUniformLocation(state->diffuse_shader.program, "lights[0].colour"), 1, (GLfloat *)&state->light_0.colour);
	glUniform1f(glGetUniformLocation(state->diffuse_shader.program, "lights[0].ambient"), state->light_0.ambient);
	glUniform1f(glGetUniformLocation(state->diffuse_shader.program, "lights[0].diffuse"), state->light_0.diffuse);

	glUniform3fv(glGetUniformLocation(state->diffuse_shader.program, "lights[1].pos"), 1, (GLfloat *)&state->light_1.pos);
	glUniform3fv(glGetUniformLocation(state->diffuse_shader.program, "lights[1].colour"), 1, (GLfloat *)&state->light_1.colour);
	glUniform1f(glGetUniformLocation(state->diffuse_shader.program, "lights[1].ambient"), state->light_1.ambient);
	glUniform1f(glGetUniformLocation(state->diffuse_shader.program, "lights[1].diffuse"), state->light_1.diffuse);

	glUniform3fv(state->diffuse_shader.view_position, 1, (GLfloat *)&state->cur_cam->pos);

	glUniform1i(state->diffuse_shader.shadow_map, 0);
}

void draw_sphere(app_state *state)
{
	glBindVertexArray(state->sphere->vao);
	glDrawElements(GL_TRIANGLES, 3 * state->sphere->polygons.size(), GL_UNSIGNED_INT, 0);
}

void draw_skybox(app_state *state)
{
	glBindVertexArray(state->triangle_vao);

	float view[16];
	mat4_copy(view, state->cur_cam->view);
	mat4_remove_translation(view);

	glUseProgram(state->skybox_shader.program);

	glBindTexture(GL_TEXTURE_CUBE_MAP, state->skybox->texture);

	glUniform1i(state->skybox_shader.skybox, 0);
	glUniformMatrix4fv(state->skybox_shader.projection, 1, GL_FALSE, state->cur_cam->frustrum);
	glUniformMatrix4fv(state->skybox_shader.view, 1, GL_FALSE, view);

	glBindBuffer(GL_ARRAY_BUFFER, state->skybox->vbos);
	glVertexAttribPointer(0, 3, GL_FLOAT, false, 8 * sizeof(float), (void *)0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)(3 * sizeof(float)));
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)(6 * sizeof(float)));

	glDrawArrays(GL_TRIANGLES, 0, 36);
	glBindBuffer(GL_ARRAY_BUFFER, state->quad_vbo);
	glVertexAttribPointer(0, 3, GL_FLOAT, false, 8 * sizeof(float), (void *)0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)(3 * sizeof(float)));
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)(6 * sizeof(float)));
}

void draw_node_tree(app_state *state, Node *node, unsigned int model_handle, bool only_selected, bool reflect)
{
	push_matrix(state);

	mat4_translate(TOP_MODEL, node->translation.x, node->translation.y, node->translation.z);

	mat4_rotate_z(TOP_MODEL, node->rotation.z);
	mat4_rotate_y(TOP_MODEL, node->rotation.y);
	mat4_rotate_x(TOP_MODEL, node->rotation.x);

	push_matrix(state);

	if (reflect) {
		mat4_scale(TOP_MODEL, node->scale.x, node->scale.y * -1.f, node->scale.z);
	} else {
		mat4_scale(TOP_MODEL, node->scale.x, node->scale.y, node->scale.z);
	}

	memcpy(node->model, TOP_MODEL, 16 * sizeof(float));

	if (only_selected) {
		if (node == state->selected) {
			glUniformMatrix4fv(model_handle, 1, GL_FALSE, TOP_MODEL);
			glBindVertexArray(state->box->vao);
			glDrawElements(GL_TRIANGLES, 3 * state->box->polygons.size(), GL_UNSIGNED_INT, 0);
		}
	} else {
		glUniformMatrix4fv(model_handle, 1, GL_FALSE, TOP_MODEL);
		glBindVertexArray(state->box->vao);
		glDrawElements(GL_TRIANGLES, 3 * state->box->polygons.size(), GL_UNSIGNED_INT, 0);
	}

	pop_matrix(state);

	for (unsigned int i = 0; i < node->children.size(); i++) {
		draw_node_tree(state, node->children[i], model_handle, only_selected, reflect);
	}

	pop_matrix(state);
}

static void render_interface(app_state *state)
{
	glBindVertexArray(state->interface_vao);

	glUniformMatrix4fv(state->interface_shader.projection, 1, GL_FALSE, (GLfloat *)state->cur_cam->ortho);
	glUniform1i(state->interface_shader.texture, 0);

	float model[16];

	for (auto &b : state->buttons) {
		mat4_identity(model);
		mat4_translate(model, b.pos.x, b.pos.y, 0.f);
		mat4_scale(model, b.size.x, b.size.y, 0.f);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, b.texture);

		glUniformMatrix4fv(state->interface_shader.model, 1, GL_FALSE, model);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
	}
}

static void render_skeleton(app_state *state, unsigned int model_handle, bool reflect)
{
	mat4_identity(TOP_MODEL);
	draw_node_tree(state, state->limbs.at(0), model_handle, false, reflect);
}

static void render_floor(app_state *state, unsigned int model_handle)
{
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, state->floor_tex);

	float model[16];

	mat4_identity(model);
	mat4_scale(model, 300.f, 1.f, 300.f);

	glUniformMatrix4fv(model_handle, 1, GL_FALSE, (GLfloat *)model);

	glBindVertexArray(state->triangle_vao);

	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}

static void render_selected_limb(app_state *state)
{
	glBindVertexArray(state->triangle_vao);

	glStencilFunc(GL_NOTEQUAL, 1, 0xFF);
	glStencilMask(0x00);
	glDisable(GL_DEPTH_TEST);

	glUseProgram(state->outline_shader.program);

	glUniformMatrix4fv(state->outline_shader.projection, 1, GL_FALSE, state->cur_cam->frustrum);
	glUniformMatrix4fv(state->outline_shader.view, 1, GL_FALSE, state->cur_cam->view);

	mat4_identity(TOP_MODEL);

	draw_node_tree(state, state->limbs.at(0), state->outline_shader.model, true, false);

	glStencilMask(0xFF);
	glStencilFunc(GL_ALWAYS, 1, 0xFF);
	glEnable(GL_DEPTH_TEST);
}

static void render_cylinders(app_state *state, unsigned int model_handle, bool reflect)
{
	glBindVertexArray(state->cylinder_vao);

	float model[16];
	for (unsigned int i = 0; i < 20; i++) {
		mat4_identity(model);
		mat4_translate(model, state->cylinders[i].x, state->cylinders[i].y, state->cylinders[i].z);

		if (reflect) {
			mat4_scale(model, 3.f, -2.f, 3.f);
		} else {
			mat4_scale(model, 3.f, 2.f, 3.f);
		}

		glUniformMatrix4fv(model_handle, 1, GL_FALSE, (GLfloat *)model);
		glDrawElements(GL_TRIANGLES, SEGMENTS * 3 * 4, GL_UNSIGNED_INT, 0);
	}
}

static void render_selected_button(app_state *state)
{
	glUseProgram(state->outline_shader.program);

	float m[16];
	mat4_identity(m);

	glUniformMatrix4fv(state->outline_shader.projection, 1, GL_FALSE, state->cur_cam->ortho);
	glUniformMatrix4fv(state->outline_shader.view, 1, GL_FALSE, m);

	glStencilFunc(GL_ALWAYS, 1, 0xFF);
	glStencilMask(0xFF);

	Button edit_button = state->buttons.at(state->edit_mode);

	float model[16];
	mat4_identity(model);
	mat4_translate(model, edit_button.pos.x, edit_button.pos.y, 0.f);
	mat4_scale(model, edit_button.size.x * 1.1f, edit_button.size.y * 1.1f, 0.f);

	glStencilFunc(GL_NOTEQUAL, 1, 0xFF);
	glStencilMask(0x00);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, state->buttons.at(state->edit_mode).texture);

	glUniformMatrix4fv(state->outline_shader.model, 1, GL_FALSE, model);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

	unsigned int axis_button_index = 2 + state->axis;
	Button axis_button = state->buttons.at(axis_button_index);

	mat4_identity(model);
	mat4_translate(model, axis_button.pos.x, axis_button.pos.y, 0.f);
	mat4_scale(model, axis_button.size.x * 1.1f, axis_button.size.y * 1.1f, 0.f);

	glStencilFunc(GL_NOTEQUAL, 1, 0xFF);
	glStencilMask(0x00);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, axis_button.texture);

	glUniformMatrix4fv(state->outline_shader.model, 1, GL_FALSE, model);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

	glStencilMask(0xFF);
	glStencilFunc(GL_ALWAYS, 1, 0xFF);
}

static void render(app_state *state)
{
	glStencilMask(0x00);

	float light_projection[16], light_view[16];
	mat4_identity(light_projection);
	mat4_identity(light_view);
	mat4_ortho(light_projection, -200.f, 200.f, -200.f, 200.f, 1.f, 1000.f);
	mat4_look_at(light_view, state->light_0.pos, { 0.f, 0.f, 0.f }, { 0.f, 1.f, 0.f });

	float light_space_matrix[16];
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
	
	render_floor(state, state->depth_shader.model);
	render_skeleton(state, state->depth_shader.model, false);
	render_cylinders(state, state->depth_shader.model, false);

	glCullFace(GL_BACK);
	
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, state->window_info.w, state->window_info.h);
	
	// Finally render to screen.
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, state->depth_map);

	textured_shader_use(state);
	glUniformMatrix4fv(state->textured_shader.light_space_matrix, 1, GL_FALSE, light_space_matrix);
	render_floor(state, state->textured_shader.model);

	diffuse_shader_use(state);
	glUniformMatrix4fv(state->diffuse_shader.light_space_matrix, 1, GL_FALSE, light_space_matrix);
	V3 c = { 1.f, 1.f, 1.f };
	glUniform3fv(state->diffuse_shader.object_colour, 1, (GLfloat *)&c);
	render_skeleton(state, state->diffuse_shader.model, false);

	c = { 0.f, 0.5f, 0.f };
	glUniform3fv(state->diffuse_shader.object_colour, 1, (GLfloat *)&c);
	render_cylinders(state, state->diffuse_shader.model, false);

	glDepthFunc(GL_LEQUAL);
	draw_skybox(state);
	glDepthFunc(GL_LESS);

	glClear(GL_STENCIL_BUFFER_BIT);

	glEnable(GL_DEPTH_TEST);
	glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

	diffuse_shader_use(state);
	glUniformMatrix4fv(state->diffuse_shader.light_space_matrix, 1, GL_FALSE, light_space_matrix);
	render_selected_limb(state);
	
	glUseProgram(state->interface_shader.program);
	render_interface(state);
	render_selected_button(state);
}

app_state *app_init(unsigned int w, unsigned int h)
{
	app_state *state = new app_state;

	if (!state) {
		return nullptr;
	}

	state->window_info.w = w;
	state->window_info.h = h;
	state->window_info.resize = false;
	state->window_info.running = true;

	init_meshes(state);

	init_shaders(state);

	create_depth_map(state->depth_map_fbo, state->depth_map);

	load_bitmaps(state);

	state->sphere = load_object("sphere.obj");
	state->box = load_object("box.obj");

	create_vbos(state->sphere);
	create_vbos(state->box);

	state->skybox = skybox_init();

	create_skeleton(state);
	create_animation(state);

	state->backup.resize(state->limbs.size());

	create_ui(state);

	glViewport(0, 0, state->window_info.w, state->window_info.h);

	camera_init(&state->main_cam);
	state->main_cam.pos = { 0.f, 10.f, -20.f };
	state->main_cam.front = { 0.f, 0.f, -1.f };
	state->main_cam.yaw = 90;

	camera_init(&state->skeleton_cam);

	camera_frustrum(&state->skeleton_cam, state->window_info.w, state->window_info.h);
	camera_ortho(&state->skeleton_cam, state->window_info.w, state->window_info.h);

	camera_frustrum(&state->main_cam, state->window_info.w, state->window_info.h);
	camera_ortho(&state->main_cam, state->window_info.w, state->window_info.h);

	state->cur_cam = &state->main_cam;

	state->depth = 0;

	state->selected = 0;

	state->light_0.pos = { -100.f, 400.f, -500.f };
	state->light_0.colour = { 1.f, 1.f, 1.f };
	state->light_0.ambient = 0.2f;
	state->light_0.diffuse = 0.4f;
	state->light_0.specular = 0.3f;

	state->light_1.pos = { 100.f, 400.f, 500.f };
	state->light_1.colour = { 0.f, 0.f, 1.f };
	state->light_1.ambient = 0.2f;
	state->light_1.diffuse = 1.f;
	state->light_1.specular = 1.f;

	state->edit_mode = 0; // 0 - position, 1 - rotation.
	state->axis = 0; // 0 - x, 1 - y, 2 - z.

	state->ray_pos = { 0, 0, 0 };
	state->ray_dir = { 1, 0, 0 };

	state->playing = false;

	state->rng = std::mt19937(0);

	for (unsigned int i = 0; i < 20; i++) {
		std::uniform_int_distribution<> pos(-100, 100);

		state->cylinders[i].x = pos(state->rng);
		state->cylinders[i].y = 0;
		state->cylinders[i].z = pos(state->rng);
	}

	return state;
}

bool mouse_in_button(unsigned int x, unsigned int y, unsigned int w, unsigned int h, unsigned int mx, unsigned int my)
{
	return (mx >= x && mx < x + w) && (my >= y && my < y + h);
}

float testRayOOBIntersect(V3 ray_pos, V3 ray_dir, float *min, float *max, float *model)
{
	float d_min = 0.f;
	float d_max = 1000000;

	V3 temp;
	temp.E[0] = model[12] - ray_pos.E[0];
	temp.E[1] = model[13] - ray_pos.E[1];
	temp.E[2] = model[14] - ray_pos.E[2];

	for (unsigned int i = 0; i < 3; i++) {
		int axis = i * 4;

		V3 naxis;
		for (unsigned int i = 0; i < 3; i++) {
			naxis.E[i] = model[axis + i];
		}
		naxis = v3_normalise(naxis);

		float e = v3_dot(temp, naxis);
		float f = v3_dot(ray_dir, naxis);

		if (abs(f) > 0.0001f) {
			float d1 = (e + min[i]) / f;
			float d2 = (e + max[i]) / f;

			if (d1 > d2) {
				float s = d1;
				d1 = d2;
				d2 = s;
			}

			if (d2 < d_max) {
				d_max = d2;
			}

			if (d1 > d_min) {
				d_min = d1;
			}

			if (d_max < d_min) {
				return -1.f;
			}
		}
	}

	return d_min;
}

float distance(const V3 &a, const V3 &b)
{
	return std::hypot(std::hypot(a.x - b.x, a.y - b.y), a.z - b.z);
}

void handle_input(float dt, app_state *state, app_keyboard_input *keyboard, app_mouse_input *mouse)
{
	if (state->cur_cam == &state->main_cam) {
		if (keyboard->forward.ended_down) {
			camera_move_forward(state->cur_cam, dt);
		}
		else if (keyboard->backward.ended_down) {
			camera_move_backward(state->cur_cam, dt);
		}

		if (keyboard->left.ended_down) {
			camera_move_left(state->cur_cam, dt);
		}
		else if (keyboard->right.ended_down) {
			camera_move_right(state->cur_cam, dt);
		}

		if (keyboard->cam_up.ended_down) {
			state->cur_cam->pitch += state->cur_cam->look_speed * dt;
		}
		else if (keyboard->cam_down.ended_down) {
			state->cur_cam->pitch -= state->cur_cam->look_speed * dt;
		}

		if (keyboard->cam_left.ended_down) {
			state->cur_cam->yaw -= state->cur_cam->look_speed * dt;
		}
		else if (keyboard->cam_right.ended_down) {
			state->cur_cam->yaw += state->cur_cam->look_speed * dt;
		}
	}

	// Only allow mouse clicks when animation is not playing.
	if (mouse->buttons[0].ended_down && !state->playing) {
		bool clicked_button = false;
		for (auto &b : state->buttons) {
			if (mouse_in_button(b.pos.x, b.pos.y, b.size.x, b.size.y, mouse->pos.x, mouse->pos.y)) {
				b.on_click(state);
				clicked_button = true;
				break;
			}
		}

		// If no button is clicked then check if a limb has been clicked.
		if (!clicked_button) {
			float near_clip = .05f;
			float fov_y = (45.f * (float)M_PI / 180.f);
			float aspect = (float)state->window_info.w / state->window_info.h;

			V3 view, h, v;
			view = state->cur_cam->front;
			view = v3_normalise(view);

			h = v3_cross(view, state->cur_cam->up);
			h = v3_normalise(h);

			v = v3_cross(h, view);
			v = v3_normalise(v);

			float v_length = tan(fov_y / 2.f) * near_clip;

			float h_length = aspect * v_length;

			v = v * v_length;
			h = h * h_length;

			unsigned int mx, my;
			mx = mouse->pos.x;
			my = mouse->pos.y;

			float nx, ny;
			nx = (((float)mx / state->window_info.w) * 2.f) - 1.f;
			ny = 1.f - (((float)my / state->window_info.h) * 2.f);

			for (unsigned int i = 0; i < 3; i++) {
				state->ray_dir.E[i] = (h.E[i] * nx) + (v.E[i] * ny) + (view.E[i] * near_clip);
			}

			state->ray_dir = v3_normalise(state->ray_dir);
			for (unsigned int i = 0; i < 3; i++) {
				state->ray_pos.E[i] = state->cur_cam->pos.E[i] + state->ray_dir.E[i];
			}

			std::vector<Node *> ordered = state->limbs;

			// Sort the limbs by distance to the camera.
			// The limbs translation is relative to its parents so
			// instead we can use the translation componenent of its model
			// matrix which is calculated when rendering.
			std::sort(ordered.begin() + 1, ordered.end(),
				[&](const Node *a, const Node *b) -> bool
				{
					const V3 a_t = { a->model[12], a->model[13], a->model[14] };
					const V3 b_t = { b->model[12], b->model[13], b->model[14] };

					return distance(a_t, state->cur_cam->pos) < distance(b_t, state->cur_cam->pos);
				});

			bool clicked_limb = false;
			for (auto &l : ordered) {
				float min[] = { -0.5f, 0.f, -0.5f, 1.f };
				float max[] = { 0.5f, 1.f, 0.5f, 1.f };

				for (unsigned int i = 0; i < 3; i++) {
					min[i] *= l->scale.E[i];
					max[i] *= l->scale.E[i];
				}

				float dist = testRayOOBIntersect(state->cur_cam->pos, state->ray_dir, min, max, l->model);

				if (dist >= 0) {
					state->selected = l;
					clicked_limb = true;
					break;
				}
			}

			if (!clicked_limb) {
				state->selected = 0;
			}
		}
	}
}

void update(app_state *state, float dt)
{
	state->dt = dt;

	// Spin the light0 around the center of the scene.
	static float j = 0;
	state->light_0.pos.x = (cosf(j)) * 400 - 200.f;
	state->light_0.pos.z = (sinf(j)) * 400 - 200.f;
	j += 0.01f;

	camera_update(state->cur_cam);
	camera_look_at(state->cur_cam);


	static float i = 0;
	if (state->playing) {
		i += 1.f * dt; // 1 frame / second

		// Stop the animation when it reaches the last frame.
		if (i >= state->key_frames.size() - 1) {
			state->playing = false;
			for (unsigned int i = 0; i < state->limbs.size(); i++) {
				*state->limbs[i] = state->backup[i];
			}
			i = 0;
		} else {
			get_frame(state, i);
		}
	}
}


static void app_on_destroy(app_state *state)
{
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	glDeleteBuffers(1, &state->quad_vbo);
	glDeleteBuffers(1, &state->quad_ebo);
	glDeleteVertexArrays(1, &state->triangle_vao);

	glDeleteBuffers(1, &state->cylinder_vbo);
	glDeleteBuffers(1, &state->cylinder_ebo);
	glDeleteVertexArrays(1, &state->cylinder_vao);

	glDeleteBuffers(1, &state->interface_vbo);
	glDeleteBuffers(1, &state->interface_ebo);
	glDeleteVertexArrays(1, &state->interface_vao);

	glDeleteBuffers(1, &state->depth_map_fbo);

	glDeleteTextures(1, &state->depth_map);
	glDeleteTextures(1, &state->floor_tex);
	glDeleteTextures(1, &state->pos_tex);
	glDeleteTextures(1, &state->rot_tex);
	glDeleteTextures(1, &state->x_tex);
	glDeleteTextures(1, &state->y_tex);
	glDeleteTextures(1, &state->z_tex);
	glDeleteTextures(1, &state->inc_tex);
	glDeleteTextures(1, &state->dec_tex);
	glDeleteTextures(1, &state->play_tex);

	glDeleteProgram(state->depth_shader.program);
	glDeleteProgram(state->interface_shader.program);
	glDeleteProgram(state->diffuse_shader.program);
	glDeleteProgram(state->textured_shader.program);
	glDeleteProgram(state->outline_shader.program);
	glDeleteProgram(state->skybox_shader.program);

	skybox_destroy(state->skybox);

	for (auto &l : state->limbs) {
		delete l;
	}

	state->limbs.clear();

	destroy_object(state->box);
	destroy_object(state->sphere);
}

void app_update_and_render(float dt, app_state *state, app_input *input, app_window_info *window_info)
{
	state->window_info = *window_info;

	if (!window_info->running) {
		app_on_destroy(state);
		return;
	}

	if (window_info->resize) {
		glViewport(0, 0, window_info->w, window_info->h);
		camera_frustrum(state->cur_cam, window_info->w, window_info->h);
		camera_ortho(state->cur_cam, state->window_info.w, state->window_info.h);
	}

	update(state, dt);
	handle_input(dt, state, &input->keyboard, &input->mouse);
	render(state);
}