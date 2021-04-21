#ifndef OPENGL_UTIL_H
#define OPENGL_UTIL_H



struct Bitmap;

extern bool gl_check_shader_compile_log(unsigned int shader);
extern bool gl_check_program_link_log(unsigned int program);
extern unsigned int gl_compile_shader_from_source(const char *source, unsigned int program, int type);
extern unsigned int create_shader(const char *vertex_shader_source, const char *fragment_shader_source);
extern unsigned int create_texture(Bitmap *bitmap);
extern void create_depth_map(unsigned int &fbo, unsigned int &texture);

#endif