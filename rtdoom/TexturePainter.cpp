#include "pch.h"
#include "TexturePainter.h"
#include "Projection.h"
#include "MathCache.h"

namespace rtdoom
{
	TexturePainter::TexturePainter(FrameBuffer& frameBuffer, const Thing& pov, const Projection& projection, const WADFile& wadFile)
		: Painter{ frameBuffer }, m_pov{ pov }, m_projection{ projection }, m_wadFile{ wadFile }
	{
	}

	void TexturePainter::PaintWall(int x, const Frame::Span& span, const Frame::PainterContext& textureContext) const
	{
		if (textureContext.textureName.length() && textureContext.textureName[0] != '-')
		{
			const auto it = m_wadFile.m_textures.find(textureContext.textureName);
			if (it == m_wadFile.m_textures.end())
			{
				return;
			}
			const auto& texture = it->second;
			const auto tx = Helpers::Clip(static_cast<int>(textureContext.texelX), texture->width);

			const auto sy = std::max(0, span.s);
			const auto ey = std::min(m_frameBuffer.m_height - 1, span.e);
			const auto ny = ey - sy + 1;
			std::vector<int> texels(ny);

			const float vStep = 1.0f / textureContext.yScale;
			float vs = (sy - textureContext.yPegging) * vStep;
			for (auto dy = sy; dy <= ey; dy++)
			{
				const auto ty = Helpers::Clip(static_cast<int>(vs) + textureContext.yOffset, texture->height);
				texels[dy - sy] = texture->pixels[ty * texture->width + tx];
				vs += vStep;
			}
			m_frameBuffer.VerticalLine(x, sy, texels, textureContext.lightness);
		}
	}

	void TexturePainter::PaintSprite(int x, int sy, std::vector<bool> clipping, const Frame::PainterContext & textureContext) const
	{
		auto it = m_wadFile.m_sprites.find(textureContext.textureName);
		if (it == m_wadFile.m_sprites.end())
		{
			return;
		}
		const auto& sprite = it->second;

		std::vector<int> texels;
		texels.reserve(clipping.size());
		const float vStep = 1.0f / textureContext.yScale;
		const auto tx = Helpers::Clip(static_cast<int>(textureContext.texelX), sprite->width);
		float vs = 0;
		for (auto b : clipping)
		{
			if (b)
			{
				const auto ty = Helpers::Clip(static_cast<int>(vs) + textureContext.yOffset, sprite->height);
				texels.push_back(sprite->pixels[ty * sprite->width + tx]);
				vs += vStep;
			}
			else
			{
				texels.push_back(255);
			}
		}
		m_frameBuffer.VerticalLine(x, sy, texels, textureContext.lightness);
	}

	void TexturePainter::PaintPlane(const Frame::Plane& plane) const
	{
		const bool isSky = plane.isSky();
		auto it = m_wadFile.m_textures.find(isSky ? "SKY1" : plane.textureName);
		if (it == m_wadFile.m_textures.end())
		{
			return;
		}

		const auto& texture = it->second;
		auto angle = Projection::NormalizeAngle(m_pov.a);
		for (size_t y = 0; y < plane.spans.size(); y++)
		{
			const auto& spans = plane.spans[y];
			auto centerDistance = m_projection.PlaneDistance(y, plane.h);
			const float lightness = isSky ? 1 : m_projection.Lightness(centerDistance) * plane.lightLevel;

			for (const auto& span : spans)
			{
				const auto sx = std::max(0, span.s);
				const auto ex = std::min(m_frameBuffer.m_width - 1, span.e);
				const auto nx = ex - sx + 1;

				std::vector<int> texels(nx);
				for (auto x = sx; x <= ex; x++)
				{
					auto viewAngle = m_projection.ViewAngle(x);
					const auto distance = centerDistance / MathCache::instance().Cos(viewAngle);
					if (isSky)
					{
						const auto horizon = m_frameBuffer.m_height / 2.0f;
						const auto tx = Helpers::Clip(static_cast<int>((m_pov.a + viewAngle) / (PI / 4) * texture->width), texture->width);

						const auto ty = Helpers::Clip(static_cast<int>(y / (horizon * 2.0f) * texture->height), texture->height);
						texels[x - sx] = texture->pixels[texture->width * ty + tx];
					}
					else if (isfinite(distance) && distance > s_minDistance)
					{
						// texel is simply ray hit location (player location + distance/angle) since floors are regularly tiled
						auto tx = Helpers::Clip(static_cast<int>(m_pov.x + distance * MathCache::instance().Cos(angle + viewAngle)), texture->width);
						auto ty = Helpers::Clip(static_cast<int>(m_pov.y + distance * MathCache::instance().Sin(angle + viewAngle)), texture->height);
						texels[x - sx] = texture->pixels[texture->width * ty + tx];
					}
				}
				m_frameBuffer.HorizontalLine(sx, y, texels, lightness);
			}
		}
	}

	TexturePainter::~TexturePainter()
	{
	}
}