#include "pch.h"
#include "FrameBuffer32.h"

namespace rtdoom
{
	FrameBuffer32::FrameBuffer32(int width, int height, const Palette& palette) : FrameBuffer{ width, height, palette }
	{
	}

	void FrameBuffer32::Attach(void* pixels, std::function<void()> stepCallback)
	{
		m_pixels = reinterpret_cast<Pixel32*>(pixels);
		m_stepCallback = stepCallback;
	}

	void FrameBuffer32::Clear()
	{
		memset(reinterpret_cast<void*>(m_pixels), 0, m_width * m_height * 4);
		m_stepCallback();
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

			lightness = Gamma(lightness);
			pixel.argb8.r = static_cast<unsigned char>(pixel.argb8.r * lightness);
			pixel.argb8.g = static_cast<unsigned char>(pixel.argb8.g * lightness);
			pixel.argb8.b = static_cast<unsigned char>(pixel.argb8.b * lightness);

			auto offset = m_width * sy + (m_width - x - 1);
			for (auto y = sy; y <= ey; y++)
			{
				m_pixels[offset].argb32 = pixel.argb32;
				offset += m_width;
			}
			m_stepCallback();
		}
	}

	void FrameBuffer32::VerticalLine(int x, int sy, const std::vector<int>& texels, float lightness) noexcept
	{
		if (m_pixels != nullptr && x >= 0 && x < m_width && texels.size())
		{
			lightness = Gamma(lightness);
			auto offset = m_width * sy + (m_width - x - 1);
			for (const auto t : texels)
			{
				const auto& color = m_palette.colors[t];
				Pixel32& pixel = m_pixels[offset];

				pixel.argb8.a = 0;
				pixel.argb8.r = static_cast<unsigned char>(color.r * lightness);
				pixel.argb8.g = static_cast<unsigned char>(color.g * lightness);
				pixel.argb8.b = static_cast<unsigned char>(color.b * lightness);

				m_pixels[offset].argb32 = pixel.argb32;
				offset += m_width;
			}
			m_stepCallback();
		}
	}

	void FrameBuffer32::HorizontalLine(int sx, int y, const std::vector<int>& texels, float lightness) noexcept
	{
		if (m_pixels != nullptr && y >= 0 && y < m_height && texels.size())
		{
			lightness = Gamma(lightness);
			auto offset = m_width * y + (m_width - sx - 1);
			for (const auto t: texels)
			{
				const auto& color = m_palette.colors[t];
				Pixel32& pixel = m_pixels[offset];

				pixel.argb8.a = 0;
				pixel.argb8.r = static_cast<unsigned char>(color.r * lightness);
				pixel.argb8.g = static_cast<unsigned char>(color.g * lightness);
				pixel.argb8.b = static_cast<unsigned char>(color.b * lightness);

				m_pixels[offset].argb32 = pixel.argb32;
				offset--;
			}
			m_stepCallback();
		}
	}

	void FrameBuffer32::VerticalLine(int x, int sy, const std::vector<int>& texels, const std::vector<float>& lightnesses) noexcept
	{
		if (m_pixels != nullptr && x >= 0 && x < m_width && texels.size())
		{
			auto offset = m_width * sy + (m_width - x - 1);
			auto lightnessIter = lightnesses.begin();
			for (const auto t : texels)
			{
				const auto& color = m_palette.colors[t];
				Pixel32& pixel = m_pixels[offset];

				const auto lightness = Gamma(*lightnessIter++);
				pixel.argb8.a = 0;
				pixel.argb8.r = static_cast<unsigned char>(color.r * lightness);
				pixel.argb8.g = static_cast<unsigned char>(color.g * lightness);
				pixel.argb8.b = static_cast<unsigned char>(color.b * lightness);

				m_pixels[offset].argb32 = pixel.argb32;
				offset += m_width;
			}
			m_stepCallback();
		}
	}

	void FrameBuffer32::SetPixel(int x, int y, int colorIndex, float lightness) noexcept
	{
		if (m_pixels != nullptr && x >= 0 && y >= 0 && x < m_width && y < m_height)
		{
			const auto& color = m_palette.colors[colorIndex];
			const auto offset = m_width * y + (m_width - x - 1);

			lightness = Gamma(lightness);
			Pixel32& pixel = m_pixels[offset];
			pixel.argb8.a = 0;
			pixel.argb8.r = static_cast<unsigned char>(color.r * lightness);
			pixel.argb8.g = static_cast<unsigned char>(color.g * lightness);
			pixel.argb8.b = static_cast<unsigned char>(color.b * lightness);
		}
	}

	FrameBuffer32::~FrameBuffer32()
	{
	}
}