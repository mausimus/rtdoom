#include "pch.h"
#include "FrameBuffer.h"

namespace rtdoom
{
	FrameBuffer::FrameBuffer(int width, int height, const Palette& palette) :
		m_width{ width },
		m_height{ height },
		m_palette{ palette }
	{
	}

	float FrameBuffer::Gamma(float lightness) const
	{
		if (lightness >= 1)
		{
			return 1;
		}
		else if (lightness <= 0)
		{
			return 0;
		}
		return 0.3f + lightness * 0.7f;
	}

	FrameBuffer::~FrameBuffer()
	{
	}
}