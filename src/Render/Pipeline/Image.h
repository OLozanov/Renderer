#pragma once

#include <glm/glm.hpp>
#include <stdint.h>

#include <algorithm>

namespace Render
{

using Color = glm::vec4;

struct RGBPixel
{
    uint8_t r;
    uint8_t g;
    uint8_t b;

    RGBPixel() = default;

    constexpr RGBPixel(uint8_t r, uint8_t g, uint8_t b)
    : r(r)
    , g(g)
    , b(b)
    {
    }

    constexpr RGBPixel(const Color& color)
    : r(color.r)
    , g(color.g)
    , b(color.b)
    {
    }

    RGBPixel& operator=(const Color& color)
    {
        r = color.r;
        g = color.g;
        b = color.b;

        return *this;
    }

    Color operator*(float val) const { return Color(r * val, g * val, b * val, 255.0f); }
};

struct RGBAPixel
{
    union
    {
        struct
        {
            uint8_t a;
            uint8_t b;
            uint8_t g;
            uint8_t r;
        };

        uint32_t value;
    };

    RGBAPixel() = default;

    constexpr RGBAPixel(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 0xFF)
    : r(r)
    , g(g)
    , b(b)
    , a(a)
    {
    }

    constexpr RGBAPixel(uint32_t color)
    : value(color)
    {
    }

    constexpr RGBAPixel(const Color& color)
    : r(color.r)
    , g(color.g)
    , b(color.b)
    , a(color.a)
    {
    }

    RGBAPixel& operator=(uint32_t color)
    {
        value = color;
        return *this;
    }

    RGBAPixel& operator=(const Color& color)
    {
        r = color.r;
        g = color.g;
        b = color.b;
        a = color.a;

        return *this;
    }

    operator const uint32_t& () const { return value; }

    Color operator*(float val) const { return Color(r * val, g * val, b * val, a * val); }
};

enum class PixelFormat
{
    RGB8 = 1,
    RGBA8 = 2,
    RGBX8 = 3,
    RGBAF32 = 4,
    A8 = 5,     // grayscale
    A16 = 6,
    F32 = 7     // depth
};

class Image
{
    PixelFormat m_format;

    uint32_t m_width;
    uint32_t m_height;
    uint32_t m_mipmaps;
    uint8_t* m_data;

    size_t m_size;

public:
    Image(PixelFormat format, uint32_t width, uint32_t height, uint32_t mipmaps = 1);
    ~Image();

    PixelFormat format() const { return m_format; }
    uint32_t width() const { return m_width; }
    uint32_t height() const { return m_height; }
    uint32_t mipmaps() const { return m_mipmaps; }

    size_t size() const { return m_size; }
    uint8_t* data() { return m_data; }
    const uint8_t* data() const { return m_data; }

    void resize(uint32_t width, uint32_t height);

    void generateMipmaps();

    template<class T>
    const T* data(uint32_t mip) const
    {
        size_t basesz = m_width * m_height * sizeof(T);
        uint8_t* addr = m_data + (basesz - (basesz >> (mip * 2))) * 4 / 3;

        return reinterpret_cast<T*>(addr);
    }

    inline uint32_t offset(uint32_t mip) const
    {
        mip = std::min(mip, m_mipmaps - 1);

        uint32_t basesz = m_width * m_height;
        return (basesz - (basesz >> (mip * 2))) * 4 / 3;
    }

    template<class T>
    const T& loadLodNearest(float x, float y, uint32_t mip = 0) const
    {
        x = std::clamp(x, 0.0f, 1.0f);
        y = std::clamp(y, 0.0f, 1.0f);

        uint32_t width = m_width >> mip;
        uint32_t height = m_height >> mip;

        size_t basesz = m_width * m_height * sizeof(T);
        uint8_t* base = m_data + (basesz - (basesz >> (mip * 2))) * 4 / 3;

        x *= (width - 1);
        y *= (height - 1);

        size_t ix = floor(x);
        size_t iy = floor(y);

        T* pixel = reinterpret_cast<T*>(base) + (iy * width + ix);

        return *pixel;
    }

    template<class T> 
    T loadLod(float x, float y, uint32_t mip = 0) const
    {
        x = std::clamp(x, 0.0f, 1.0f);
        y = std::clamp(y, 0.0f, 1.0f);

        uint32_t width = m_width >> mip;
        uint32_t height = m_height >> mip;

        size_t basesz = m_width * m_height * sizeof(T);
        uint8_t* base = m_data + (basesz - (basesz >> (mip * 2))) * 4 / 3;

        x *= (width - 1);
        y *= (height - 1);

        size_t ix = floor(x);
        size_t iy = floor(y);

        T* pixel = reinterpret_cast<T*>(base) + (iy * width + ix);

        if (width == 1 && height == 1) return *pixel;

        float fx = x - ix;
        float fy = y - iy;
        float rfx = 1.0f - fx;
        float rfy = 1.0f - fy;

        float weight[4] = { rfx * rfy,
                            fx * rfy,
                            rfx * fy,
                            fx * fy };

        T samples[4] = { *pixel,
                         *(pixel + 1),
                         *(pixel + width),
                         *(pixel + width + 1) };

        return samples[0] * weight[0] +
               samples[1] * weight[1] +
               samples[2] * weight[2] +
               samples[3] * weight[3];
    }

private:
    static size_t RequiredMem(PixelFormat format, uint32_t width, uint32_t height, uint32_t mipmaps);
    static size_t PixelSize(PixelFormat format);

    template<class F>
    F* pixel_addr(uint32_t x, uint32_t y, uint32_t mip = 0)
    {
        uint8_t* base = m_data;

        if (mip > 0)
        {
            // imagesize * (1 - 0.25f ^ mip) / (1 - 0.25f);
            size_t basesz = m_width * m_height * sizeof(F);
            m_data += (basesz - (basesz >> (mip * 2))) * 4 / 3;
        }

        return reinterpret_cast<F*>(m_data) + y * m_width + x;
    }
};

} // namespace Render