#include <string>
#include <fstream>
#include <assert.h>

#include "win32-opengl.h"
#include "object.h"
#include "app.h"

void add_vertex(Object *obj, std::string line) 
{
    assert(line.length() && (line[0] & 0xFFDF) == 'V');
    ObjVertex *vertex = (ObjVertex*)malloc(sizeof(ObjVertex));

    if (vertex) {
        memset(&vertex->nor, 0, 3 * sizeof(float));
        memset(&vertex->tex, 0, 2 * sizeof(float));
        vertex->smoothing_group = -1;
        vertex->next = -1;

        const char *text = line.c_str();

        int index = 0;
        vertex->pos.E[index++] = atof(text + 1);
        int start = 1;

        do {
            while (text[start] == ' ' || text[start] == '\t' || text[start] == '-') {
                assert(start < strlen(text));
                ++start;
            }

            while (text[start] != ' ' && text[start] != '\t') {
                assert(start < strlen(text));
                ++start;
            }

            vertex->pos.E[index++] = atof(text + start);
        } while (index < 3);

        vertex->tex.x = vertex->pos.x + 0.5f;
        vertex->tex.y = vertex->pos.z + 0.5f;

        obj->vertices.push_back(vertex);
    }
}

void add_polygon(Object *obj, std::string line, int smoothing) 
{
    assert(line.length() && (line[0] & 0xFFDF) == 'F');
    Poly *polygon = (Poly*)malloc(sizeof(Poly));

    if (polygon) {
        const char *text = line.c_str();
    
        int index = 0;
        polygon->indices[index++] = atoi(text + 1) - 1;
        int start = 1;

        do {
            while (text[start] == ' ' || text[start] == '\t' || text[start] == '-') {
                assert(start < strlen(text));
                ++start;
            }

            while (text[start] != ' ' && text[start] != '\t') {
                assert(start < strlen(text));
                ++start;
            }

            polygon->indices[index++] = atoi(text + start) - 1;
        } while (index < 3);

        polygon->smoothing_group = smoothing;
        obj->polygons.push_back(polygon);
    }
}

Object *load_object(const char *filename)
{
    int smoothing = 0;

    Object *obj = new Object();
    
    std::ifstream file(filename);

    if (file) {
        std::string line;
        std::getline(file, line);

        while (!file.eof()) {
            if (line.length()) {
                switch (line[0]) {
                    case 'v': case 'V': add_vertex(obj, line); break;
                    case 'f': case 'F': add_polygon(obj, line, smoothing); break;
                    case 's': case 'S': smoothing = atoi(line.c_str() + 1); break;
                }
            }

            std::getline(file, line);
        }

        file.close();

        //process smoothing groups
        for (unsigned int i = 0; i < obj->polygons.size(); i++) {
            Poly *poly = obj->polygons[i];
            int group = poly->smoothing_group;
            for (int j = 0; j < 3; j++) {
                int index = poly->indices[j];
                ObjVertex *vertex = obj->vertices[index];

                if (vertex->smoothing_group == -1) {
                    vertex->smoothing_group = group;
                } else {
                    while (vertex->smoothing_group != group && vertex->next != -1) {
                        index = vertex->next;
                        vertex = obj->vertices[index];
                    }

                    if (vertex->smoothing_group == group) {
                        poly->indices[j] = index;
                    } else {
                        ObjVertex *duplicate = (ObjVertex*)malloc(sizeof(ObjVertex));
                        if (duplicate) {
                            poly->indices[j] = vertex->next = obj->vertices.size();

                            memcpy(duplicate, vertex, sizeof(ObjVertex));
                            duplicate->smoothing_group = group;
                            duplicate->next = -1;
                            obj->vertices.push_back(duplicate);
                        }
                    }
                }
            }
        }

        //Calculate normals for each vertex for each sum normals of surrounding.
        for (unsigned int i = 0; i < obj->polygons.size(); i++) {
                ObjVertex *a = obj->vertices[obj->polygons[i]->indices[0]];
                ObjVertex *b = obj->vertices[obj->polygons[i]->indices[1]];
                ObjVertex *c = obj->vertices[obj->polygons[i]->indices[2]];

                V3 cp = v3_cross(b->pos - a->pos, c->pos - a->pos);
                a->nor += cp;
                b->nor += cp;
                c->nor += cp;
        }

        // Average sum of normals for each vertex.
        for (unsigned int i = 0; i < obj->vertices.size(); i++) {
            obj->vertices[i]->nor = v3_normalise(obj->vertices[i]->nor);
        }
    }

    return obj;
}

extern void destroy_object(Object *obj)
{
    glDeleteVertexArrays(1, &obj->vao);
    glDeleteBuffers(2, obj->vbos);
    
    for (auto &v : obj->vertices) {
        delete v;
    }

    for (auto &p : obj->polygons) {
        delete p;
    }
}

void create_vbos(Object *obj)
{
    glGenVertexArrays(1, &obj->vao);
    glBindVertexArray(obj->vao);

    int vertex_size = sizeof(float) * obj->vertices.size() * 8;
    float *vert_data = (float*)malloc(vertex_size);

    int polygon_size = sizeof(unsigned int) * obj->polygons.size() * 3;
    unsigned int *poly_data = (unsigned int*)malloc(polygon_size);

    if (vert_data && poly_data) {
        glGenBuffers(2, obj->vbos);

        int offset = 0;
        for (int i = 0; i < obj->vertices.size(); i++, offset += 8) {
            memcpy(vert_data + offset, obj->vertices[i], 8 * sizeof(float));
        }

        glBindBuffer(GL_ARRAY_BUFFER, obj->vbos[0]);
        glBufferData(GL_ARRAY_BUFFER, vertex_size, vert_data, GL_STATIC_DRAW);

        offset = 0;
        for (int i = 0; i < obj->polygons.size(); i++, offset += 3) {
            memcpy(poly_data + offset, obj->polygons[i]->indices, 3 * sizeof(unsigned int));
        }

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, obj->vbos[1]);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, polygon_size, poly_data, GL_STATIC_DRAW);
    }

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)(3 * sizeof(float)));
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)(6 * sizeof(float)));

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);

    free(vert_data);
    free(poly_data);
}