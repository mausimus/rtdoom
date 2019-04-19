#pragma once

#include "WADFile.h"

namespace rtdoom
{
class FrameBuffer
{
public:
    FrameBuffer(int width, int height, const Palette& palette);

    const int             m_width;
    const int             m_height;
    const Palette&        m_palette;
    std::function<void()> m_stepCallback;

    virtual void Attach(void* pixels, std::function<void()> stepCallback = nullptr)                                          = 0;
    virtual void Clear()                                                                                                     = 0;
    virtual void SetPixel(int x, int y, int color, float lightness) noexcept                                                 = 0;
    virtual void VerticalLine(int x, int sy, int ey, int colorIndex, float lightness) noexcept                               = 0;
    virtual void VerticalLine(int x, int sy, const std::vector<int>& texels, const std::vector<float>& lightnesses) noexcept = 0;
    virtual void VerticalLine(int x, int sy, const std::vector<int>& texels, float lightness) noexcept                       = 0;
    virtual void HorizontalLine(int sx, int y, const std::vector<int>& texels, float lightness) noexcept                     = 0;
    virtual void HorizontalLine(int sx, int ex, int y, int colorIndex, float lightness) noexcept                             = 0;

    static float Gamma(float lightness);

    virtual ~FrameBuffer();
};
} // namespace rtdoom
