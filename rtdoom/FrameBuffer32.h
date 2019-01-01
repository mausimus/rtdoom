#pragma once

#include "FrameBuffer.h"

namespace rtdoom
{
	class FrameBuffer32 : public FrameBuffer
	{
	protected:

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

		Pixel32* m_pixels{ nullptr };

		const std::array<uint32_t, 6> s_colors{ 0x00ff000,0x0000ff00,0x000000ff,0x00ff00ff,0x0000ffff,0x00ffff00 };

	public:
		virtual void Attach(void* pixels) override;
		virtual void Clear() override;
		virtual void SetPixel(int x, int y, int color, float lightness) noexcept override;
		virtual void VerticalLine(int x, int sy, int ey, int colorIndex, float lightness) noexcept override;

		FrameBuffer32(int width, int height);
		~FrameBuffer32();
	};
}
