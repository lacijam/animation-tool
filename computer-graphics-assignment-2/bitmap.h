#pragma once



struct Bitmap {
	unsigned char *pixels;
	unsigned int width, height;
};

extern Bitmap *load_bitmap(const char *filename);