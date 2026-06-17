#pragma once

#include <stdint.h>

enum class TgaDataType : uint8_t
{
    Rgb = 2,
    RgbCompressed = 10
};

#pragma pack(push, 1)
struct TgaHeader
{
    uint8_t      idLength;
    uint8_t      colourMapType;
    TgaDataType  dataType;
    uint16_t     colourMapOrigin;
    uint16_t     colourMapLength;
    uint8_t      colourMapDepth;
    uint16_t     xOrigin;
    uint16_t     yOrigin;
    uint16_t     width;
    uint16_t     height;
    uint8_t      bitsPerPixel;
    uint8_t      imageDescriptor;
};
#pragma pack(pop)