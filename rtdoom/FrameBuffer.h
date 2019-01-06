#pragma once

#include "WADFile.h"

namespace rtdoom
{
	class FrameBuffer
	{
	public:
		FrameBuffer(int width, int height, const Palette& palette);

		const int m_width;
		const int m_height;
		const Palette& m_palette;

		virtual void Attach(void* pixels) = 0;
		virtual void Clear() = 0;
		virtual void SetPixel(int x, int y, int color, float lightness) noexcept = 0;
		virtual void VerticalLine(int x, int sy, int ey, int colorIndex, float lightness) noexcept = 0;
		virtual float Gamma(float lightness) const;

		virtual ~FrameBuffer();
	};
}