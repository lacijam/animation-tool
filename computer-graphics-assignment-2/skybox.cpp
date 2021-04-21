#include "Skybox.h"

#include <stdlib.h>

#include "win32-opengl.h"

#include "bitmap.h"

Skybox *skybox_init()
{
    Skybox *skybox = (Skybox *)malloc(sizeof(Skybox));
    glGenBuffers(1, &skybox->vbos);

    const float VERTEX_DATA[] = {
    -1.0f,  1.0f, -1.0f, 0.f, 0.f, 0.f, 0.f, 0.f,
    -1.0f, -1.0f, -1.0f, 0.f, 0.f, 0.f, 0.f, 0.f,
     1.0f, -1.0f, -1.0f, 0.f, 0.f, 0.f, 0.f, 0.f,
     1.0f, -1.0f, -1.0f, 0.f, 0.f, 0.f, 0.f, 0.f,
     1.0f,  1.0f, -1.0f, 0.f, 0.f, 0.f, 0.f, 0.f,
    -1.0f,  1.0f, -1.0f, 0.f, 0.f, 0.f, 0.f, 0.f,
                                                
    -1.0f, -1.0f,  1.0f, 0.f, 0.f, 0.f, 0.f, 0.f,
    -1.0f, -1.0f, -1.0f, 0.f, 0.f, 0.f, 0.f, 0.f,
    -1.0f,  1.0f, -1.0f, 0.f, 0.f, 0.f, 0.f, 0.f,
    -1.0f,  1.0f, -1.0f, 0.f, 0.f, 0.f, 0.f, 0.f,
    -1.0f,  1.0f,  1.0f, 0.f, 0.f, 0.f, 0.f, 0.f,
    -1.0f, -1.0f,  1.0f, 0.f, 0.f, 0.f, 0.f, 0.f,
                                                
     1.0f, -1.0f, -1.0f, 0.f, 0.f, 0.f, 0.f, 0.f,
     1.0f, -1.0f,  1.0f, 0.f, 0.f, 0.f, 0.f, 0.f,
     1.0f,  1.0f,  1.0f, 0.f, 0.f, 0.f, 0.f, 0.f,
     1.0f,  1.0f,  1.0f, 0.f, 0.f, 0.f, 0.f, 0.f,
     1.0f,  1.0f, -1.0f, 0.f, 0.f, 0.f, 0.f, 0.f,
     1.0f, -1.0f, -1.0f, 0.f, 0.f, 0.f, 0.f, 0.f,
                                                
    -1.0f, -1.0f,  1.0f, 0.f, 0.f, 0.f, 0.f, 0.f,
    -1.0f,  1.0f,  1.0f, 0.f, 0.f, 0.f, 0.f, 0.f,
     1.0f,  1.0f,  1.0f, 0.f, 0.f, 0.f, 0.f, 0.f,
     1.0f,  1.0f,  1.0f, 0.f, 0.f, 0.f, 0.f, 0.f,
     1.0f, -1.0f,  1.0f, 0.f, 0.f, 0.f, 0.f, 0.f,
    -1.0f, -1.0f,  1.0f, 0.f, 0.f, 0.f, 0.f, 0.f,
                                                
    -1.0f,  1.0f, -1.0f, 0.f, 0.f, 0.f, 0.f, 0.f,
     1.0f,  1.0f, -1.0f, 0.f, 0.f, 0.f, 0.f, 0.f,
     1.0f,  1.0f,  1.0f, 0.f, 0.f, 0.f, 0.f, 0.f,
     1.0f,  1.0f,  1.0f, 0.f, 0.f, 0.f, 0.f, 0.f,
    -1.0f,  1.0f,  1.0f, 0.f, 0.f, 0.f, 0.f, 0.f,
    -1.0f,  1.0f, -1.0f, 0.f, 0.f, 0.f, 0.f, 0.f,
                                                
    -1.0f, -1.0f, -1.0f, 0.f, 0.f, 0.f, 0.f, 0.f,
    -1.0f, -1.0f,  1.0f, 0.f, 0.f, 0.f, 0.f, 0.f,
     1.0f, -1.0f, -1.0f, 0.f, 0.f, 0.f, 0.f, 0.f,
     1.0f, -1.0f, -1.0f, 0.f, 0.f, 0.f, 0.f, 0.f,
    -1.0f, -1.0f,  1.0f, 0.f, 0.f, 0.f, 0.f, 0.f,
     1.0f, -1.0f,  1.0f,  0.f, 0.f, 0.f, 0.f, 0.f
    };

    const unsigned int FACE_SIZE = 8 * 6;
    glBindBuffer(GL_ARRAY_BUFFER, skybox->vbos);
    glBufferData(GL_ARRAY_BUFFER, 6 * FACE_SIZE * sizeof(float), VERTEX_DATA, GL_STATIC_DRAW);

    const char *faces[6] = {
    "right.bmp",
    "left.bmp",
    "top.bmp",
    "bottom.bmp",
    "front.bmp",
    "back.bmp"
    };

    unsigned int texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_CUBE_MAP, texture);

    for (unsigned int i = 0; i < 6; i++) {
        Bitmap *b = load_bitmap(faces[i]);
        if (b) {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                0, GL_RGBA, b->width, b->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, b->pixels
            );
            delete b;
        }
        else {
            delete b;
        }
    }

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    skybox->texture = texture;

    return skybox;
}

void skybox_destroy(Skybox *skybox)
{
    if (skybox) {
        glDeleteBuffers(1, &skybox->vbos);
        free(skybox);
    }
}