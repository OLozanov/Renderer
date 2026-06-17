#include "Tga.h"
#include "Resources/Resources.h"

#include "stdio.h"
#include "string.h"

#include <iostream>

Render::Image* LoadTga(const char* filename)
{
    FILE* file;

    //Open file
    errno_t error = fopen_s(&file, filename, "rb");

    if (error)
    {
        std::cout << "Can't open file " << filename << std::endl;
        return nullptr;
    }

    Render::Image* image = nullptr;

    TgaHeader header;

    fread(&header, sizeof(TgaHeader), 1, file);

    if (header.dataType == TgaDataType::Rgb)
    {
        if (header.bitsPerPixel != 32 && header.bitsPerPixel != 24)
        {
            fclose(file);
            return nullptr;
        }

        uint32_t width = header.width;
        uint32_t height = header.height;
        uint32_t mipmaps = log2(std::min(header.width, header.height)) + 1;

        Render::PixelFormat format = header.bitsPerPixel == 32 ? Render::PixelFormat::RGBA8 : 
                                                                 Render::PixelFormat::RGBX8;

        image = new Render::Image(format, width, height, mipmaps);

        uint8_t* data = image->data();
        uint8_t* pixels = data;

        if (header.idLength) fseek(file, header.idLength, SEEK_CUR);

        if (header.bitsPerPixel == 32)
        {
            for (int i = 0; i < header.width * header.height; i++, pixels += 4)
            {
                fread(pixels + 1, 1, 1, file);
                fread(pixels + 2, 1, 1, file);
                fread(pixels + 3, 1, 1, file);
                fread(pixels, 1, 1, file);
            }
        }
        else
            for (size_t i = 0; i < header.width * header.height; i++, pixels += 4)
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