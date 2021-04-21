#include "bitmap.h"

#include <windows.h>

Bitmap *load_bitmap(const char *filename)
{
	Bitmap *bitmap = (Bitmap *)malloc(sizeof Bitmap);

	if (bitmap) {
		BITMAP bmp_info;
		HBITMAP hbmp = (HBITMAP)::LoadImageA(NULL, filename, IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION | LR_LOADFROMFILE);
		GetObject(hbmp, sizeof BITMAP, &bmp_info);

		bitmap->width = bmp_info.bmWidth;
		bitmap->height = bmp_info.bmHeight;
		unsigned char *data = (unsigned char *)malloc(4 * bitmap->width * bitmap->height);

		if (data) {
			unsigned int *dest = (unsigned int *)data;
			unsigned char *source = (unsigned char *)bmp_info.bmBits;

			for (int j = 0; j < bmp_info.bmHeight; j++) {
				for (int i = 0; i < bmp_info.bmWidth; i++) {
					int r = source[i * 3];
					int g = source[i * 3 + 1];
					int b = source[i * 3 + 2];

					*dest = 0xFF000000 | (r << 16) | (g << 8) | b;
					++dest;
				}

				source += bmp_info.bmWidthBytes;
			}

			bitmap->pixels = data;
		}
	}

	return bitmap;
}