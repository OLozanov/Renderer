#pragma once

#include <stdint.h>

#include "Render/Pipeline/Image.h"

namespace Render
{

template<class F>
class Surface
{
private:
	uint32_t m_width;
	uint32_t m_height;

	F * m_data;

public:

	Surface()
	: m_width(0)
	, m_height(0)
	, m_data(nullptr)
	{}

	Surface(uint32_t width, uint32_t height, F * data)
	: m_width(width)
	, m_height(height)
	, m_data(data)
	{}

	~Surface()
	{
	}

	void reset(uint32_t width, uint32_t height, F* data)
	{
		m_width = width;
		m_height = height;
		m_data = data;
	}

	uint32_t width() const { return m_width; }
	uint32_t height() const { return m_height; }

	F & pixel(uint32_t x, uint32_t y) { return m_data[m_width*y+x]; }
	const F & pixel(uint32_t x, uint32_t y) const { return m_data[m_width*y+x]; }

    F * data() { return m_data; }
	const F * data() const { return m_data; }
};

using ColorBuffer = Surface<uint32_t>;
using DepthBuffer = Surface<float>;

} // namespace Render
