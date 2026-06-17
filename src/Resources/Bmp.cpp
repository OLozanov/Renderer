#include <stdio.h>

#include "Resources/Resources.h"

#include <stdexcept>
#include <iostream>
#include <algorithm>

enum class BmpCompression : uint32_t
{
	RGB = 0,
	RLE8 = 1,
	RLE4 = 2,
	BITFIELDS = 3,
	JPEG = 4,
	PNG = 5
};

#pragma pack(push, 1)
struct BmpFileHeader
{
	uint16_t type;
	uint32_t size;
	uint16_t reserved1;
	uint16_t reserved2;
	uint32_t offBits;
};

struct BmpInfoHeader 
{
	uint32_t size;
	int32_t width;
	int32_t height;
	uint16_t planes;
	uint16_t bitCount;
	BmpCompression compression;
	uint32_t sizeImage;
	int32_t xPelsPerMeter;
	int32_t yPelsPerMeter;
	uint32_t clrUsed;
	uint32_t clrImportant;
};
#pragma pack(pop)

Render::Image* LoadBmp(const char* filename)
{
	FILE* file;

	BmpFileHeader fileHeader;
	BmpInfoHeader infoHeader;

	errno_t error = fopen_s(&file, filename, "rb");

	if (error)
	{
		std::cout << "Can't open file " << filename << std::endl;
		return nullptr;
	}

	fread(&fileHeader, sizeof(BmpFileHeader), 1, file);
	if (fileHeader.type != 0x4d42) throw std::runtime_error("LoadBMP: incorrect header type!");

	size_t infoSz = fileHeader.offBits - sizeof(BmpFileHeader);

	fread(&infoHeader, std::min(infoSz, sizeof(BmpInfoHeader)), 1, file);

	if (infoSz > sizeof(BmpInfoHeader)) fseek(file, fileHeader.offBits, SEEK_SET);

	if (infoHeader.bitCount < 24) throw std::runtime_error("LoadBMP: incorrect bit depth!");
	if (infoHeader.compression != BmpCompression::RGB) throw std::runtime_error("LoadBMP: doesn't support compression!");

	size_t imageSize = infoHeader.width * infoHeader.height * sizeof(uint32_t);

	uint32_t width = infoHeader.width;
	uint32_t height = infoHeader.height;
	uint32_t mipmaps = log2(std::min(infoHeader.width, infoHeader.height)) + 1;

	Render::PixelFormat format = infoHeader.bitCount == 32 ? Render::PixelFormat::RGBA8 : 
															 Render::PixelFormat::RGBX8;

	Render::Image* image = new Render::Image(format, width, height, mipmaps);

	uint8_t* data = image->data();
	uint8_t* pixels = data;

	if (infoHeader.bitCount == 32)
	{
		for (int i = 0; i < infoHeader.width * infoHeader.height; i++, pixels += 4)
		{
			fread(pixels + 1, 1, 1, file);
			fread(pixels + 2, 1, 1, file);
			fread(pixels + 3, 1, 1, file);
			fread(pixels, 1, 1, file);

			pixels[0] = 0xFF;
		}
	}
	else
	{
		for (int i = 0; i < infoHeader.width * infoHeader.height; i++, pixels += 4)
		{
			fread(pixels + 1, 1, 1, file);
			fread(pixels + 2, 1, 1, file);
			fread(pixels + 3, 1, 1, file);

			pixels[0] = 0xFF;
		}
	}

	fclose(file);

	image->generateMipmaps();

	return image;
}