#ifndef SKYBOX_H
#define SKYBOX_H



struct Skybox {
	unsigned vbos;
	unsigned texture;
};

Skybox *skybox_init();
void skybox_destroy(Skybox *skybox);

#endif