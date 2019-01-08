#pragma once

#include "FrameBuffer.h"

namespace rtdoom
{
#pragma pack(1)
	union Pixel32
	{
		struct ARGB
		{
			uint8_t b;
			uint8_t g;
			uint8_t r;
			uint8_t a;
		};
		ARGB argb8;
		uint32_t argb32;
	};
#pragma pack()

	class FrameBuffer32 : public FrameBuffer
	{
	protected:

		Pixel32* m_pixels{ nullptr };

		const std::array<uint32_t, 6> s_colors{ 0x00ff000,0x0000ff00,0x000000ff,0x00ff00ff,0x0000ffff,0x003f7f0f };

	public:
		virtual void Attach(void* pixels) override;
		virtual void Clear() override;
		virtual void SetPixel(int x, int y, int color, float lightness) noexcept override;
		virtual void VerticalLine(int x, int sy, int ey, int colorIndex, float lightness) noexcept override;
		virtual void VerticalLine(int x, int sy, const std::vector<int>& colorIndexes, const std::vector<float>& lightnesses) noexcept;
		virtual void VerticalLine(int x, int sy, const std::vector<int>& colorIndexes, float lightness) noexcept;
		virtual void HorizontalLine(int sx, int y, const std::vector<int>& colorIndexes, float lightness) noexcept;

		FrameBuffer32(int width, int height, const Palette& palette);
		~FrameBuffer32();
	};
}
