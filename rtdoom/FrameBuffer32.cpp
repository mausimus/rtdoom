#include "pch.h"
#include "FrameBuffer32.h"

namespace rtdoom
{
	FrameBuffer32::FrameBuffer32(int width, int height, const Palette& palette) : FrameBuffer{ width, height, palette }
	{
	}

	void FrameBuffer32::Attach(void* pixels)
	{
		m_pixels = reinterpret_cast<Pixel32*>(pixels);
	}

	void FrameBuffer32::Clear()
	{
		memset(reinterpret_cast<void*>(m_pixels), 0, m_width * m_height * 4);
	}

	void FrameBuffer32::VerticalLine(int x, int sy, int ey, int colorIndex, float lightness) noexcept
	{
		if (m_pixels != nullptr && x >= 0 && x < m_width)
		{
			sy = std::max(0, std::min(sy, m_height - 1));
			ey = std::max(0, std::min(ey, m_height - 1));
			if (sy > ey)
			{
				std::swap(sy, ey);
			}

			const auto color = s_colors.at(colorIndex % s_colors.size());
			Pixel32 pixel;
			pixel.argb32 = color;

			if (lightness < 1)
			{
				if (lightness <= 0)
				{
					pixel.argb32 = 0;
				}
				else
				{
					lightness = Gamma(lightness);
					pixel.argb8.r = static_cast<unsigned char>(pixel.argb8.r * lightness);
					pixel.argb8.g = static_cast<unsigned char>(pixel.argb8.g * lightness);
					pixel.argb8.b = static_cast<unsigned char>(pixel.argb8.b * lightness);
				}
			}

			auto offset = m_width * sy + (m_width - x - 1);
			for (auto y = sy; y <= ey; y++)
			{
				m_pixels[offset].argb32 = pixel.argb32;
				offset += m_width;
			}
		}
	}

	void FrameBuffer32::SetPixel(int x, int y, int colorIndex, float lightness) noexcept
	{
		if (m_pixels != nullptr && x >= 0 && y >= 0 && x < m_width && y < m_height)
		{
			auto color = m_palette.colors[colorIndex];
			const auto offset = m_width * y + (m_width - x - 1);

			Pixel32& pixel = m_pixels[offset];
			pixel.argb8.a = 0;
			pixel.argb8.r = color.r;
			pixel.argb8.g = color.g;
			pixel.argb8.b = color.b;

			if (lightness < 1)
			{
				if (lightness <= 0)
				{
					pixel.argb32 = 0;
				}
				else
				{
					lightness = Gamma(lightness);
					pixel.argb8.r = static_cast<unsigned char>(pixel.argb8.r * lightness);
					pixel.argb8.g = static_cast<unsigned char>(pixel.argb8.g * lightness);
					pixel.argb8.b = static_cast<unsigned char>(pixel.argb8.b * lightness);
				}
			}
		}
	}

	FrameBuffer32::~FrameBuffer32()
	{
	}
}