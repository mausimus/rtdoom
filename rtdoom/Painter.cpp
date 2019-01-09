#include "pch.h"
#include "Painter.h"

namespace rtdoom
{
	Painter::Painter(FrameBuffer& frameBuffer) :
		m_frameBuffer(frameBuffer)
	{
	}

	Painter::~Painter()
	{
	}
}