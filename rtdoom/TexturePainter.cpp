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

			const float vStep = textureContext.yScale;
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

	void TexturePainter::PaintSprite(int x, int sy, std::vector<bool> occlusion, const Frame::PainterContext& textureContext) const
	{
		auto it = m_wadFile.m_sprites.find(textureContext.textureName);
		if (it == m_wadFile.m_sprites.end())
		{
			return;
		}
		const auto& sprite = it->second;

		std::vector<int> texels;
		texels.reserve(occlusion.size());
		const float vStep = textureContext.yScale;
		const auto tx = Helpers::Clip(static_cast<int>(textureContext.texelX), sprite->width);
		float vs = 0;
		for (auto o : occlusion)
		{
			if (!o)
			{
				const auto ty = Helpers::Clip(static_cast<int>(vs) + textureContext.yOffset, sprite->height);
				texels.push_back(sprite->pixels[ty * sprite->width + tx]);
			}
			else
			{
				texels.push_back(247);
			}
			vs += vStep;
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
		const auto angle = Projection::NormalizeAngle(m_pov.a);
		const auto cosA = MathCache::instance().Cos(angle);
		const auto sinA = MathCache::instance().Sin(angle);
		const auto aStep = PI4 / (m_frameBuffer.m_width / 2);

		// floors/ceilings are most efficient painted in horizontal strips since distance to the player is constant
		for (auto y = 0; y < plane.spans.size(); y++)
		{
			const auto& spans = plane.spans[y];
			if (spans.empty())
			{
				continue;
			}

			auto mergedSpans = MergeSpans(spans);
			if (!isSky)
			{
				const auto centerDistance = m_projection.PlaneDistance(y, plane.h);
				if (isfinite(centerDistance) && centerDistance > s_minDistance)
				{
					const float lightness = m_projection.Lightness(centerDistance) * plane.lightLevel;
					const auto ccosA = centerDistance * cosA;
					const auto csinA = centerDistance * sinA;

					// texel steps per horizontal pixel
					const auto stepX = -csinA * aStep;
					const auto stepY = ccosA * aStep;

					for (const auto& span : mergedSpans)
					{
						const auto sx = std::max(0, span.s);
						const auto ex = std::min(m_frameBuffer.m_width - 1, span.e);
						const auto nx = ex - sx + 1;
						const auto angleTan = aStep * (sx - m_frameBuffer.m_width / 2);

						// starting texel position
						auto texelX = Helpers::Clip(m_pov.x + ccosA - csinA * angleTan, static_cast<float>(texture->width));
						auto texelY = Helpers::Clip(m_pov.y + csinA + ccosA * angleTan, static_cast<float>(texture->height));

						std::vector<int> texels(nx);
						for (auto x = sx; x <= ex; x++)
						{
							const auto tx = Helpers::Clip(static_cast<int>(texelX), texture->width);
							const auto ty = Helpers::Clip(static_cast<int>(texelY), texture->height);
							texels[x - sx] = texture->pixels[texture->width * ty + tx];
							texelX += stepX;
							texelY += stepY;
						}
						m_frameBuffer.HorizontalLine(sx, y, texels, lightness);
					}
				}
			}
			else
			{
				const auto horizon = m_frameBuffer.m_height / 2.0f;
				const auto ty = Helpers::Clip(static_cast<int>(y / horizon / 2.0f * texture->height), texture->height);
				const auto xScale = 1.0f / PI4 * texture->width;
				for (const auto& span : mergedSpans)
				{
					const auto sx = std::max(0, span.s);
					const auto ex = std::min(m_frameBuffer.m_width - 1, span.e);
					const auto nx = ex - sx + 1;

					std::vector<int> texels(nx);
					for (auto x = sx; x <= ex; x++)
					{
						const auto viewAngle = m_projection.ViewAngle(x);
						const auto tx = Helpers::Clip(static_cast<int>((m_pov.a + viewAngle) * xScale), texture->width);
						texels[x - sx] = texture->pixels[texture->width * ty + tx];
					}
					m_frameBuffer.HorizontalLine(sx, y, texels, 1);
				}
			}
		}
	}

	std::list<Frame::Span> TexturePainter::MergeSpans(const std::vector<Frame::Span>& spans)
	{
		std::list<Frame::Span> spanlist(spans.begin(), spans.end());

		spanlist.sort();
		auto es = spanlist.begin();
		do
		{
			auto ps = es++;
			if (es != spanlist.end() && (ps->e == es->s || ps->e == es->s - 1))
			{
				es->s = ps->s;
				spanlist.erase(ps);
			}
		} while (es != spanlist.end());

		return spanlist;
	}

	TexturePainter::~TexturePainter()
	{
	}
}