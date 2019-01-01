#include "pch.h"
#include "FrameBuffer.h"

namespace rtdoom
{
	FrameBuffer::FrameBuffer(int width, int height) :
		m_width{ width },
		m_height{ height }
	{
	}

	FrameBuffer::~FrameBuffer()
	{
	}
}