#include "Resources/Resources.h"

#include <png.h>
#include <iostream>

Render::Image* LoadPng(const char* filename)
{
    FILE* file;

	errno_t error = fopen_s(&file, filename, "rb");

	if (error)
	{
		std::cout << "Can't open file " << filename << std::endl;
		return nullptr;
	}

	png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (!png) return nullptr;

	png_infop info = png_create_info_struct(png);
	if (!info) return nullptr;

	if (setjmp(png_jmpbuf(png))) return nullptr;

	png_init_io(png, file);
	png_read_info(png, info);

	Render::Image* image = nullptr;

	Render::PixelFormat format = Render::PixelFormat::RGBA8;
	uint32_t width = png_get_image_width(png, info);
	uint32_t height = png_get_image_height(png, info);
	png_byte colortype = png_get_color_type(png, info);
	png_byte bitdepth = png_get_bit_depth(png, info);

	bool alpha = true;

	// These color_type don't have an alpha channel then fill it with 0xff.
	if (colortype == PNG_COLOR_TYPE_RGB ||
		colortype == PNG_COLOR_TYPE_PALETTE)
	{
		png_set_filler(png, 0xFF, PNG_FILLER_AFTER);
		alpha = false;
	}

	if (colortype == PNG_COLOR_TYPE_GRAY)
		alpha = false;

	if (colortype == PNG_COLOR_TYPE_GRAY_ALPHA)
		png_set_gray_to_rgb(png);

	if (colortype == PNG_COLOR_TYPE_PALETTE)
	{
		png_set_palette_to_rgb(png);
		format = Render::PixelFormat::RGBA8;
	}
	else if (colortype == PNG_COLOR_TYPE_GRAY)
	{
		if (bitdepth < 8)
		{
			png_set_expand_gray_1_2_4_to_8(png);
			format = Render::PixelFormat::A8;
		}
		else if (bitdepth == 8)
		{
			format = Render::PixelFormat::A8;
		}
		else
		{
			format = Render::PixelFormat::A16;
			png_set_swap(png);
		}
	} 
	else
	{
		format = alpha ? Render::PixelFormat::RGBA8 : Render::PixelFormat::RGBX8;
		png_set_bgr(png);
		png_set_swap_alpha(png);
	}

	if (png_get_valid(png, info, PNG_INFO_tRNS))
		png_set_tRNS_to_alpha(png);

	png_read_update_info(png, info);

	uint32_t mipmaps = log2(std::min(width, height)) + 1;

	image = new Render::Image(format, width, height, mipmaps);

	uint8_t* data = image->data();
	uint8_t* pixels = data;

	// Read image
	png_bytep* rows = new png_bytep[height];

	for (uint32_t y = 0; y < height; y++) 
	{
		rows[height - y - 1] = (png_byte*)data + png_get_rowbytes(png, info) * y;
	}

	png_read_image(png, rows);

	fclose(file);

	png_destroy_read_struct(&png, &info, NULL);
	delete rows;

	image->generateMipmaps();

	return image;
}