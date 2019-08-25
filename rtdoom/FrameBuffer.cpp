#include "pch.h"
#include "FrameBuffer.h"

namespace rtdoom
{
FrameBuffer::FrameBuffer(int width, int height, const Palette& palette) : m_width {width}, m_height {height}, m_palette {palette} {}

unsigned char FrameBuffer::Gamma(float lightness)
{
    if(lightness >= 1)
    {
        return 255;
    }
    else if(lightness <= 0)
    {
        return 38;
    }
    return static_cast<unsigned char>(255.0f * (0.2f + lightness * 0.8f));
}

FrameBuffer::~FrameBuffer() {}
} // namespace rtdoom
