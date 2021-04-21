#ifndef OBJECT_H
#define OBJECT_H

#include <vector>

#include "maths.h"

struct app_state;

struct ObjVertex {
    V3 pos;
    V3 nor;
    V2 tex;
    int next, smoothing_group;
};

struct Poly {
    unsigned int indices[3];
    int smoothing_group;
};

struct Object {
    unsigned int vao; // lets be inefficient
    unsigned int vbos[2];
    std::vector<ObjVertex*> vertices;
    std::vector<Poly*> polygons;
};

extern Object *load_object(const char *filename);
extern void create_vbos(Object *obj);
extern void destroy_object(Object *obj);

inline float atof_ex(const char *text) { return (float)atof(text); }
inline unsigned int atoi_ex(const char *text) { return (unsigned int)(atoi(text) - 1); }

#endif