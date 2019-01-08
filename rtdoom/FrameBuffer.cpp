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

	float FrameBuffer::Gamma(float lightness)
	{
		if (lightness >= 1)
		{
			return 1;
		}
		else if (lightness <= 0)
		{
			return 0.15f;
		}
		return 0.2f + lightness * 0.8f;
	}

	FrameBuffer::~FrameBuffer()
	{
	}
}