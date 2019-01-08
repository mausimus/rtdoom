#include "pch.h"
#include "rtdoom.h"
#include "ViewRenderer.h"
#include "MathCache.h"

using namespace std::literals::string_literals;
using namespace std::chrono_literals;

namespace rtdoom
{
	const auto s_stepDelay = 10ms;

	ViewRenderer::ViewRenderer(const GameState& gameState, const WADFile& wadFile) :
		Renderer{ gameState },
		m_wadFile{ wadFile },
		m_renderingMode{ RenderingMode::Textured }
	{
	}

	// entry method for rendering a frame
	void ViewRenderer::RenderFrame(FrameBuffer& frameBuffer)
	{
		m_frameBuffer = &frameBuffer;
		m_projection.reset(new Projection{ m_gameState.m_player, frameBuffer });
		m_frame.reset(new Frame{ frameBuffer });

		if (m_renderingMode == RenderingMode::Wireframe)
		{
			m_frameBuffer->Clear();
		}

		// iterate through all segments (map lines) in visibility order returned by traversing the map's BSP tree
		for (const auto& segment : m_gameState.m_mapDef->GetSegmentsToDraw(m_gameState.m_player))
		{
			// only draw segments that are facing the player
			if (MapDef::IsInFrontOf(m_gameState.m_player, *segment))
			{
				DrawMapSegment(*segment);

				// stop drawing once the frame has been fully horizontally occluded with solid walls
				if (m_frame->IsHorizontallyOccluded())
				{
					break;
				}
			}
		}

		// fill in floors and ceilings
		DrawPlanes();
	}

	void ViewRenderer::DrawMapSegment(const Segment& segment) const
	{
		VisibleSegment visibleSegment{ segment };

		// calculate relative view angles of the mapSegment and skip rendering if they are not within the field of view
		visibleSegment.startAngle = m_projection->ProjectionAngle(segment.s);
		visibleSegment.endAngle = m_projection->ProjectionAngle(segment.e);
		if (!Projection::NormalizeViewAngleSpan(visibleSegment.startAngle, visibleSegment.endAngle))
		{
			return;
		}

		// project edges of the mapSegment onto the screen from left to right
		visibleSegment.startX = m_projection->ViewX(visibleSegment.startAngle);
		visibleSegment.endX = m_projection->ViewX(visibleSegment.endAngle);
		if (visibleSegment.startX > visibleSegment.endX)
		{
			std::swap(visibleSegment.startX, visibleSegment.endX);
			std::swap(visibleSegment.startAngle, visibleSegment.endAngle);
		}

		// clip the mapSegment against already drawn solid walls (horizontal occlusion)
		const auto visibleSpans = m_frame->ClipHorizontalSegment(visibleSegment.startX, visibleSegment.endX, segment.isSolid);
		if (visibleSpans.empty())
		{
			return;
		}

		// draw all visible spans
		visibleSegment.normalVector = m_projection->NormalVector(segment); // normal vector from the mapSegment towards the player
		visibleSegment.normalOffset = m_projection->NormalOffset(segment); // offset of the normal vector from the start of the line (for texturing)
		for (const auto& span : visibleSpans)
		{
			DrawMapSegmentSpan(span, visibleSegment);
		}

		m_frame->m_numSegments++;
	}

	// draw a visible span of a mapSegment on the frame buffer
	void ViewRenderer::DrawMapSegmentSpan(const Frame::Span& span, const VisibleSegment& visibleSegment) const
	{
		const Segment& mapSegment = visibleSegment.mapSegment;

		// iterate through all vertical columns from left to right
		for (auto x = span.s; x <= span.e; x++)
		{
			// calculate the relative angle and distance to the mapSegment from our viewpoint
			auto viewAngle = GetViewAngle(x, visibleSegment);
			auto distance = m_projection->Distance(visibleSegment.normalVector, viewAngle);
			if (distance < s_minDistance)
			{
				continue;
			}

			// calculate on-screen vertical start and end positions for the column, based on the front (outer) side
			const auto outerTopY = m_projection->ViewY(distance, mapSegment.frontSide.sector.ceilingHeight - m_gameState.m_player.z);
			const auto outerBottomY = m_projection->ViewY(distance, mapSegment.frontSide.sector.floorHeight - m_gameState.m_player.z);

			TextureContext outerTexture;
			outerTexture.yScale = m_projection->TextureScale(distance, 1.0f); // yScale of 1 height
			outerTexture.yPegging = mapSegment.lowerUnpegged ? outerBottomY : outerTopY;
			outerTexture.textureName = mapSegment.frontSide.middleTexture;
			outerTexture.yOffset = mapSegment.frontSide.yOffset;
			// texel x position is the offset from player to normal vector plus offset from normal vector to view, plus static mapSegment and linedef offsets
			outerTexture.xPos = visibleSegment.normalOffset + m_projection->Offset(visibleSegment.normalVector, viewAngle)
				+ mapSegment.xOffset + mapSegment.frontSide.xOffset;

			const auto lightness = GetLightness(distance, &mapSegment) * mapSegment.frontSide.sector.lightLevel;
			const auto ceilingHeight = mapSegment.frontSide.sector.isSky ? s_skyHeight : (mapSegment.frontSide.sector.ceilingHeight - m_gameState.m_player.z);
			const auto floorHeight = mapSegment.frontSide.sector.floorHeight - m_gameState.m_player.z;

			// clip the column based on what we've already have drawn (vertical occlusion)
			const auto& outerSpan = m_frame->ClipVerticalSegment(x, outerTopY, outerBottomY, mapSegment.isSolid, &ceilingHeight, &floorHeight,
				mapSegment.frontSide.sector.ceilingTexture, mapSegment.frontSide.sector.floorTexture, mapSegment.frontSide.sector.lightLevel);
			if (outerSpan.isVisible())
			{
				RenderColumnSpan(x, outerSpan, outerTexture, lightness, visibleSegment);
			}

			// if the mapSegment is not a solid wall but a pass-through portal clip its back (inner) side
			if (!mapSegment.isSolid)
			{
				// recalculate column size for the back side
				const auto innerTopY = m_projection->ViewY(distance, mapSegment.backSide.sector.ceilingHeight - m_gameState.m_player.z);
				const auto innerBottomY = m_projection->ViewY(distance, mapSegment.backSide.sector.floorHeight - m_gameState.m_player.z);

				// segments that connect open sky sectors should not have their top sections drawn
				const auto isSky = mapSegment.frontSide.sector.isSky && mapSegment.backSide.sector.isSky;

				// clip the inner span against what's already been drawn
				const auto& innerSpan = m_frame->ClipVerticalSegment(x, innerTopY, innerBottomY, mapSegment.isSolid, isSky ? &s_skyHeight : nullptr, nullptr,
					mapSegment.frontSide.sector.ceilingTexture, mapSegment.frontSide.sector.floorTexture, mapSegment.frontSide.sector.lightLevel);
				if (innerSpan.isVisible())
				{
					// render the inner section (floor/ceiling height change)
					if (!isSky)
					{
						Frame::Span upperSpan(outerSpan.s, innerSpan.s);
						TextureContext upperTexture;
						upperTexture.textureName = mapSegment.frontSide.upperTexture;
						upperTexture.yScale = outerTexture.yScale;
						upperTexture.xPos = outerTexture.xPos;
						upperTexture.yOffset = outerTexture.yOffset;
						upperTexture.yPegging = mapSegment.upperUnpegged ? outerTopY : innerTopY;
						RenderColumnSpan(x, upperSpan, upperTexture, lightness, visibleSegment);
					}

					Frame::Span lowerSpan(innerSpan.e, outerSpan.e);
					TextureContext lowerTexture;
					lowerTexture.textureName = mapSegment.frontSide.lowerTexture;
					lowerTexture.yScale = outerTexture.yScale;
					lowerTexture.xPos = outerTexture.xPos;
					lowerTexture.yOffset = outerTexture.yOffset;
					lowerTexture.yPegging = mapSegment.lowerUnpegged ? outerTopY : innerBottomY;
					RenderColumnSpan(x, lowerSpan, lowerTexture, lightness, visibleSegment);
				}
			}
		}
	}

	// render the outer (front) section of a column of a visible span on the framebuffer, either a isSolid wall or only edges for pass-through portals
	void ViewRenderer::RenderColumnSpan(int x, const Frame::Span& span, const TextureContext& textureContext, float lightness, const VisibleSegment& visibleSegment) const
	{
		switch (m_renderingMode)
		{
		case RenderingMode::Wireframe:
			m_frameBuffer->VerticalLine(x, span.s, span.s, s_wallColor, lightness);
			m_frameBuffer->VerticalLine(x, span.e, span.e, s_wallColor, lightness);
			if (textureContext.textureName != "-" && (x == visibleSegment.startX || x == visibleSegment.endX))
			{
				m_frameBuffer->VerticalLine(x, span.s + 1, span.e - 1, s_wallColor, lightness / 2.0f);
			}
			break;
		case RenderingMode::Solid:
			if (textureContext.textureName != "-")
			{
				m_frameBuffer->VerticalLine(x, span.s, span.e, s_wallColor, lightness);
			}
			break;
		case RenderingMode::Textured:
			TextureWall(x, span, textureContext, lightness);
			break;
		default:
			break;
		}
	}

	// texture a solid wall portion
	void ViewRenderer::TextureWall(int x, const Frame::Span& span, const TextureContext& textureContext, float lightness) const
	{
		if (textureContext.textureName.length() && textureContext.textureName[0] != '-')
		{
			const auto it = m_wadFile.m_textures.find(textureContext.textureName);
			if (it == m_wadFile.m_textures.end())
			{
				return;
			}
			const auto& texture = it->second;
			const auto tx = Utils::Clip(static_cast<int>(textureContext.xPos), texture->width);

			const auto sy = std::max(0, span.s);
			const auto ey = std::min(m_frameBuffer->m_height - 1, span.e);
			const auto ny = ey - sy + 1;
			std::vector<int> texels(ny);

			const float vStep = 1.0f / textureContext.yScale;
			float vs = (sy - textureContext.yPegging) * vStep;
			for (auto dy = sy; dy <= ey; dy++)
			{
				const auto ty = Utils::Clip(static_cast<int>(vs) + textureContext.yOffset, texture->height);
				texels[dy - sy] = texture->pixels[ty * texture->width + tx];
				vs += vStep;
			}
			m_frameBuffer->VerticalLine(x, sy, texels, lightness);
			if (m_stepFrame)
			{
				std::this_thread::sleep_for(s_stepDelay);
			}
		}
	}

	// render floors and ceilings based on data collected during drawing walls
	void ViewRenderer::DrawPlanes() const
	{
		if (m_renderingMode == RenderingMode::Solid || m_renderingMode == RenderingMode::Textured)
		{
			for (const auto& floorPlane : m_frame->m_floorPlanes)
			{
				TexturePlane(floorPlane);
				m_frame->m_numFloorPlanes++;
			}
			for (const auto& ceilingPlane : m_frame->m_ceilingPlanes)
			{
				TexturePlane(ceilingPlane);
				m_frame->m_numCeilingPlanes++;
			}
		}
	}

	// texture a floor/ceiling plane
	void ViewRenderer::TexturePlane(const Frame::Plane& plane) const
	{
		const bool isSky = plane.isSky();
		auto it = m_wadFile.m_textures.find(isSky ? "SKY1" : plane.textureName);
		if (m_renderingMode == RenderingMode::Textured && it == m_wadFile.m_textures.end())
		{
			return;
		}

		const auto& texture = it->second;
		auto angle = Projection::NormalizeAngle(m_gameState.m_player.a);
		for (auto y = 0; y < plane.spans.size(); y++)
		{
			const auto& spans = plane.spans[y];
			auto centerDistance = m_projection->PlaneDistance(y, plane.h);
			const float lightness = isSky ? 1 : GetLightness(centerDistance) * plane.lightLevel;

			for (auto span : spans)
			{
				const auto sx = std::max(0, span.s);
				const auto ex = std::min(m_frameBuffer->m_width - 1, span.e);
				const auto nx = ex - sx + 1;

				std::vector<int> texels(nx);
				for (auto x = sx; x <= ex; x++)
				{
					auto viewAngle = m_projection->ViewAngle(x);
					const auto distance = centerDistance / MathCache::instance().Cos(viewAngle);
					if (m_renderingMode == RenderingMode::Solid)
					{
						if (isSky)
						{
							m_frameBuffer->VerticalLine(x, y, y, s_planeColor, 1);
						}
						else
						{
							m_frameBuffer->VerticalLine(x, y, y, s_planeColor, GetLightness(distance) * plane.lightLevel);
						}
					}
					else if (isSky)
					{
						const auto horizon = m_frameBuffer->m_height / 2.0f;
						const auto viewAngle = m_projection->ViewAngle(x);
						const auto tx = Utils::Clip(static_cast<int>((m_gameState.m_player.a + viewAngle) / (PI / 4) * texture->width), texture->width);

						const auto ty = Utils::Clip(static_cast<int>(y / (horizon * 2.0f) * texture->height), texture->height);
						texels[x - sx] = texture->pixels[texture->width * ty + tx];
					}
					else if (isfinite(distance) && distance > s_minDistance)
					{
						// texel is simply ray hit location (player location + distance/angle) since floors are regularly tiled
						auto tx = Utils::Clip(static_cast<int>(m_gameState.m_player.x + distance * MathCache::instance().Cos(angle + viewAngle)), texture->width);
						auto ty = Utils::Clip(static_cast<int>(m_gameState.m_player.y + distance * MathCache::instance().Sin(angle + viewAngle)), texture->height);
						texels[x - sx] = texture->pixels[texture->width * ty + tx];
					}
				}
				if (m_renderingMode == RenderingMode::Textured)
				{
					m_frameBuffer->HorizontalLine(sx, y, texels, lightness);
				}
			}
		}
		if (m_stepFrame)
		{
			std::this_thread::sleep_for(s_stepDelay);
		}
	}

	// return the view angle for vertical screen column
	Angle ViewRenderer::GetViewAngle(int x, const VisibleSegment & visibleSegment) const
	{
		auto viewAngle = m_projection->ViewAngle(x);

		// due to limited resolution we have to reduce bleeding from converting angles to columns and back
		if (x == visibleSegment.startX)
		{
			viewAngle = visibleSegment.startAngle;
		}
		else if (x == visibleSegment.endX)
		{
			viewAngle = visibleSegment.endAngle;
		}
		return viewAngle;
	}

	// calculate the lightness of a mapSegment based on its distance
	float ViewRenderer::GetLightness(float distance, const Segment* segment) const
	{
		auto lightness = 0.9f - (distance / s_lightnessFactor);

		if (segment)
		{
			// auto-shade 90-degree edges
			if (segment->isVertical)
			{
				lightness *= 1.1f;
			}
			else if (segment->isHorizontal)
			{
				lightness *= 0.9f;
			}
		}

		return lightness;
	}

	void ViewRenderer::SetMode(RenderingMode renderingMode)
	{
		m_renderingMode = renderingMode;
	}

	Frame* ViewRenderer::GetLastFrame() const
	{
		return m_frame.get();
	}

	void ViewRenderer::StepFrame()
	{
		m_stepFrame = !m_stepFrame;
	}

	ViewRenderer::~ViewRenderer()
	{
	}
}