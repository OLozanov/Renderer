#pragma once

#include "Render/Pipeline/Surface.h"
#include "Render/Pipeline/Image.h"

namespace Render
{

class Bitmap
{
private:

	union
	{
		ColorBuffer m_colorBuffer;
		DepthBuffer m_depthBuffer;
	};

	Image m_image;

public:
	Bitmap(PixelFormat format, uint32_t width, uint32_t height, uint32_t mipmaps = 1)
	: m_image(format, width, height, mipmaps)
	{
		m_colorBuffer.reset(width, height, reinterpret_cast<uint32_t*>(m_image.data()));
	}

	~Bitmap() {}

	void resize(uint32_t width, uint32_t height)
	{
		m_image.resize(width, height);
		m_colorBuffer.reset(width, height, reinterpret_cast<uint32_t*>(m_image.data()));
	}

	template<class T>
	operator Surface<T>* () { return reinterpret_cast<Surface<T>*>(&m_colorBuffer); }
	template<class T>
	operator const Surface<T>* () const { return reinterpret_cast<Surface<T>*>(&m_colorBuffer); }

	operator ColorBuffer* () { return &m_colorBuffer; }
	operator const ColorBuffer* () const { return &m_colorBuffer; }

	operator ColorBuffer& () { return m_colorBuffer; }
	operator const ColorBuffer& () const { return m_colorBuffer; }

	operator DepthBuffer* () { return &m_depthBuffer; }
	operator const DepthBuffer* () const { return &m_depthBuffer; }

	operator DepthBuffer& () { return m_depthBuffer; }
	operator const DepthBuffer& () const { return m_depthBuffer; }

	operator Image* () { return &m_image; }
	operator const Image* () const { return &m_image; }

	operator Image& () { return m_image; }
	operator const Image& () const { return m_image; }
};

} // namespace Render