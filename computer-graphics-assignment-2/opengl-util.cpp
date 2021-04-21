#include "opengl-util.h"

#include <stdio.h>
#include <stdlib.h>

#include "win32-opengl.h"
#include "bitmap.h"

bool gl_check_shader_compile_log(unsigned int shader)
{
	int success;
	char info_log[512];
	glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(shader, 512, 0, info_log);
		printf(">>>Shader compilation error: %s", info_log);
	};

	return success;
}

bool gl_check_program_link_log(unsigned int program)
{
	int success;
	char info_log[512];
	glGetProgramiv(program, GL_LINK_STATUS, &success);
	if (!success)
	{
		glGetProgramInfoLog(program, 512, NULL, info_log);
		printf(">>>Shader program linking error: %s", info_log);
	}

	return success;
}

unsigned int gl_compile_shader_from_source(const char *source, unsigned int program, int type)
{
    unsigned int shader = glCreateShader(type);
	if (!shader) {
        printf("Failed to create shader!\n");
		return 0;
	}
	
    glShaderSource(shader, 1, &source, 0);
    glCompileShader(shader);
    gl_check_shader_compile_log(shader);

    return shader;
}

unsigned int create_shader(const char *vertex_shader_source, const char *fragment_shader_source)
{
	unsigned int program_id = glCreateProgram();
	unsigned int vertex_shader = gl_compile_shader_from_source(vertex_shader_source, program_id, GL_VERTEX_SHADER);
	unsigned int fragment_shader = gl_compile_shader_from_source(fragment_shader_source, program_id, GL_FRAGMENT_SHADER);

	glAttachShader(program_id, vertex_shader);
	glAttachShader(program_id, fragment_shader);
	glLinkProgram(program_id);
	glUseProgram(program_id);

	glDeleteShader(vertex_shader);
	glDeleteShader(fragment_shader);

	return program_id;
}

unsigned int create_texture(Bitmap *bitmap)
{
	if (!bitmap) {
		return -1;
	}

	unsigned int tex = 0;
	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, bitmap->width, bitmap->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, bitmap->pixels);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	return tex;
}

void create_depth_map(unsigned int &fbo, unsigned int &texture)
{
	glGenFramebuffers(1, &fbo);

	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, 2048, 2048, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

	// Fixes artifacts on shadow edges.
	float border_color[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, border_color);

	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, texture, 0);
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}