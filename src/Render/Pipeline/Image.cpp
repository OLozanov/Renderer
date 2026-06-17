#include "Image.h"

#include <stdexcept>
#include <math.h>

namespace Render
{

size_t Image::RequiredMem(PixelFormat format, uint32_t width, uint32_t height, uint32_t mipmaps)
{
    size_t basesz = width * height * PixelSize(format);
    return (basesz - (basesz >> (mipmaps * 2))) * 4 / 3;
}

size_t Image::PixelSize(PixelFormat format)
{
    switch (format)
    {
    case PixelFormat::RGB8: return sizeof(RGBPixel); break;
    case PixelFormat::RGBX8:
    case PixelFormat::RGBA8: return sizeof(RGBAPixel); break;
    case PixelFormat::RGBAF32: return sizeof(Color); break;
    case PixelFormat::A8: return sizeof(uint8_t); break;
    case PixelFormat::A16: return sizeof(uint16_t); break;
    case PixelFormat::F32: return sizeof(float); break;
    default:
        return 1;
    }

    return 1;
}

Image::Image(PixelFormat format, uint32_t width, uint32_t height, uint32_t mipmaps)
: m_format(format)
, m_width(width)
, m_height(height)
, m_mipmaps(mipmaps)
, m_size(RequiredMem(format, width, height, mipmaps))
{
    m_data = new uint8_t[m_size];
}

Image::~Image()
{
    if (m_data) delete[] m_data;
}

void Image::resize(uint32_t width, uint32_t height)
{
    if (m_data) delete[] m_data;

    m_width = width;
    m_height = height;
    m_size = RequiredMem(m_format, m_width, m_height, m_mipmaps);

    m_data = new uint8_t[m_size];
}

template <class T>
void downsample(uint8_t* in, uint8_t* out, int width, int height)
{
    auto in_pixel = [in, width, height](int x, int y) -> T*
    {
        return reinterpret_cast<T*>(in) + y * width + x;
    };

    auto out_pixel = [out, width = width / 2, height = height / 2](int x, int y) -> T&
    {
        return *(reinterpret_cast<T*>(out) + y * width + x);
    };

    for (int y = 0; y < height / 2; y++)
    {
        for (int x = 0; x < width / 2; x++)
        {
            T* ptr = in_pixel(x * 2, y * 2);

            T& a = *ptr;
            T& b = *(ptr + 1);
            T& c = *(ptr + width);
            T& d = *(ptr + width + 1);

            out_pixel(x, y) = a * 0.25f + b * 0.25f + c * 0.25f + d * 0.25f;
        }
    }
}

template void downsample<uint8_t>(uint8_t* in, uint8_t* out, int width, int height);
template void downsample<uint16_t>(uint16_t* in, uint16_t* out, int width, int height);
template void downsample<float>(uint16_t* in, uint16_t* out, int width, int height);
template void downsample<RGBPixel>(uint16_t* in, uint16_t* out, int width, int height);
template void downsample<RGBAPixel>(uint16_t* in, uint16_t* out, int width, int height);

void Image::generateMipmaps()
{
    size_t pixelsz = PixelSize(m_format);

    void (*downsample_func)(uint8_t * in, uint8_t * out, int width, int height);

    switch (m_format)
    {
    case PixelFormat::RGB8: downsample_func = downsample<RGBPixel>; break;
    case PixelFormat::RGBX8:
    case PixelFormat::RGBA8: downsample_func = downsample<RGBAPixel>; break;
    case PixelFormat::A8: downsample_func = downsample<uint8_t>; break;
    case PixelFormat::A16: downsample_func = downsample<uint16_t>; break;
    case PixelFormat::F32: downsample_func = downsample<float>; break;
    default:
        throw std::runtime_error("MipMaps generation: unsupported format!");
    }

    uint32_t width = m_width;
    uint32_t height = m_height;

    uint8_t* in = m_data;
    uint8_t* out;

    for (size_t i = 0; i < m_mipmaps - 1; i++)
    {
        size_t offset = width * height * pixelsz;
        out = in + offset;

        downsample_func(in, out, width, height);

        in = out;

        width /= 2;
        height /= 2;
    }
}

template RGBAPixel Image::loadLod<RGBAPixel>(float x, float y, uint32_t mip) const;

} // namespace Render