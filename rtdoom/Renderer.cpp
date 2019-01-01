#include "pch.h"
#include "Renderer.h"

namespace rtdoom
{
	Renderer::Renderer(const GameState& gameState)
		: m_gameState(gameState)
	{
	}

	bool Renderer::IsVisible(int x, int y, FrameBuffer& frameBuffer)
	{
		return (x >= 0 && y >= 0 && x < frameBuffer.m_width && y < frameBuffer.m_height);
	}

	void Renderer::DrawLine(int sx, int sy, int dx, int dy, int color, float lightness, FrameBuffer& frameBuffer) const
	{
		const auto rx = dx - sx;
		const auto ry = dy - sy;

		if (!IsVisible(sx, sy, frameBuffer) && !IsVisible(dx, dy, frameBuffer))
		{
			// TODO: support partial lines
			return;
		}

		if (rx == 0 && ry == 0)
		{
			frameBuffer.SetPixel(sx, sy, color, lightness);
			return;
		}

		if (abs(rx) > abs(ry))
		{
			if (sx > dx)
			{
				std::swap(sx, dx);
				std::swap(sy, dy);
			}

			auto y = static_cast<float>(sy);
			const auto slope = static_cast<float>(dy - sy) / static_cast<float>(dx - sx);

			do
			{
				frameBuffer.SetPixel(sx, static_cast<int>(y), color, lightness);
				y += slope;
				sx++;
			} while (sx <= dx);
		}
		else
		{
			if (sy > dy)
			{
				std::swap(sx, dx);
				std::swap(sy, dy);
			}

			auto x = static_cast<float>(sx);
			const auto slope = static_cast<float>(dx - sx) / static_cast<float>(dy - sy);

			do
			{
				frameBuffer.SetPixel(static_cast<int>(x), sy, color, lightness);
				x += slope;
				sy++;
			} while (sy <= dy);
		}
	}

	Renderer::~Renderer()
	{
	}
}