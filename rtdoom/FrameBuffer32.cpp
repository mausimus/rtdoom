#include "pch.h"
#include "FrameBuffer32.h"

namespace rtdoom
{
FrameBuffer32::FrameBuffer32(int width, int height, const Palette& palette) : FrameBuffer {width, height, palette}
{
    for(auto l = 0; l < 256; l++)
    {
        c_lightMap[l] = new unsigned char[256];
        for(auto v = 0; v < 256; v++)
        {
            c_lightMap[l][v] = static_cast<unsigned char>(static_cast<float>(v) * static_cast<float>(l) / 256.0f);
        }
    }
}

void FrameBuffer32::Attach(void* pixels, std::function<void()> stepCallback)
{
    m_pixels       = reinterpret_cast<Pixel32*>(pixels);
    m_stepCallback = stepCallback;
}

void FrameBuffer32::Clear()
{
    memset(reinterpret_cast<void*>(m_pixels), 0, m_width * m_height * 4);
    m_stepCallback();
}

void FrameBuffer32::VerticalLine(int x, int sy, int ey, int colorIndex, float lightness) noexcept
{
    if(m_pixels != nullptr && x >= 0 && x < m_width)
    {
        sy = std::max(0, std::min(sy, m_height - 1));
        ey = std::max(0, std::min(ey, m_height - 1));
        if(sy > ey)
        {
            std::swap(sy, ey);
        }

        const auto color    = s_colors.at(colorIndex % s_colors.size());
        const auto lightMap = c_lightMap[Gamma(lightness)];
        Pixel32    pixel;

        pixel.argb32  = color;
        pixel.argb8.r = lightMap[pixel.argb8.r];
        pixel.argb8.g = lightMap[pixel.argb8.g];
        pixel.argb8.b = lightMap[pixel.argb8.b];

        auto offset = m_width * sy + (m_width - x - 1);
        for(auto y = sy; y <= ey; y++)
        {
            m_pixels[offset].argb32 = pixel.argb32;
            offset += m_width;
        }
        m_stepCallback();
    }
}

void FrameBuffer32::VerticalLine(int x, int sy, const std::vector<int>& texels, float lightness) noexcept
{
    if(m_pixels != nullptr && x >= 0 && x < m_width && texels.size() && sy < m_height)
    {
        const auto lightMap = c_lightMap[Gamma(lightness)];
        auto       offset   = m_width * sy + (m_width - x - 1);
        for(const auto t : texels)
        {
            if(t != 247 && sy >= 0)
            {
                const auto& color = m_palette.colors[t];
                Pixel32&    pixel = m_pixels[offset];

                pixel.argb8.r = lightMap[color.r];
                pixel.argb8.g = lightMap[color.g];
                pixel.argb8.b = lightMap[color.b];
            }
            offset += m_width;
            sy++;
            if(sy == m_height)
            {
                break;
            }
        }
        m_stepCallback();
    }
}

void FrameBuffer32::HorizontalLine(int sx, int y, const std::vector<int>& texels, float lightness) noexcept
{
    if(m_pixels != nullptr && y >= 0 && y < m_height && texels.size())
    {
        const auto lightMap = c_lightMap[Gamma(lightness)];
        auto       offset   = m_width * y + (m_width - sx - 1);
        for(const auto t : texels)
        {
            const auto& color = m_palette.colors[t];
            Pixel32&    pixel = m_pixels[offset];

            pixel.argb8.r = lightMap[color.r];
            pixel.argb8.g = lightMap[color.g];
            pixel.argb8.b = lightMap[color.b];
            offset--;
        }
        m_stepCallback();
    }
}

void FrameBuffer32::HorizontalLine(int sx, int ex, int y, int colorIndex, float lightness) noexcept
{
    if(m_pixels != nullptr && y >= 0 && y < m_height && sx >= 0 && ex < m_width)
    {
        const auto lightMap = c_lightMap[Gamma(lightness)];
        auto       offset   = m_width * y + (m_width - sx - 1);
        for(auto x = sx; x <= ex; x++)
        {
            const auto& color = s_colors[colorIndex];
            Pixel32&    pixel = m_pixels[offset];

            pixel.argb32  = color;
            pixel.argb8.r = lightMap[pixel.argb8.r];
            pixel.argb8.g = lightMap[pixel.argb8.g];
            pixel.argb8.b = lightMap[pixel.argb8.b];
            offset--;
        }
        m_stepCallback();
    }
}

void FrameBuffer32::VerticalLine(int x, int sy, const std::vector<int>& texels, const std::vector<float>& lightnesses) noexcept
{
    if(m_pixels != nullptr && x >= 0 && x < m_width && texels.size())
    {
        auto offset        = m_width * sy + (m_width - x - 1);
        auto lightnessIter = lightnesses.begin();
        for(const auto t : texels)
        {
            const auto& color    = m_palette.colors[t];
            Pixel32&    pixel    = m_pixels[offset];
            const auto  lightMap = c_lightMap[Gamma(*lightnessIter++)];

            pixel.argb8.r = lightMap[color.r];
            pixel.argb8.g = lightMap[color.g];
            pixel.argb8.b = lightMap[color.b];
            offset += m_width;
        }
        m_stepCallback();
    }
}

void FrameBuffer32::SetPixel(int x, int y, int colorIndex, float lightness) noexcept
{
    if(m_pixels != nullptr && x >= 0 && y >= 0 && x < m_width && y < m_height)
    {
        const auto& color    = m_palette.colors[colorIndex];
        const auto  offset   = m_width * y + (m_width - x - 1);
        const auto  lightMap = c_lightMap[Gamma(lightness)];
        Pixel32&    pixel    = m_pixels[offset];

        pixel.argb8.r = lightMap[color.r];
        pixel.argb8.g = lightMap[color.g];
        pixel.argb8.b = lightMap[color.b];
    }
}

FrameBuffer32::~FrameBuffer32()
{
    for(auto l = 0; l < 256; l++)
    {
        delete[] c_lightMap[l];
    }
}
} // namespace rtdoom
