#include "pch.h"
#include "rtdoom.h"
#include "ViewRenderer.h"
#include "MathCache.h"

using namespace std::literals::string_literals;

namespace rtdoom
{
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
		for (const auto& l : m_gameState.m_mapDef->GetSegmentsToDraw(m_gameState.m_player))
		{
			DrawMapSegment(l);

			// stop drawing once the frame has been fully horizontally occluded with solid walls
			if (m_frame->IsHorizontallyOccluded())
			{
				break;
			}
		}

		// fill in floors and ceilings
		DrawPlanes();
	}

	void ViewRenderer::DrawMapSegment(const Segment& segment) const
	{
		// do not draw any segments that are not facing the player
		if (!MapDef::IsInFrontOf(m_gameState.m_player, segment))
		{
			return;
		}

		VisibleSegment visibleSegment{ segment };

		// calculate relative view angles of the segment and skip rendering if they are not within the field of view
		visibleSegment.startAngle = m_projection->ProjectionAngle(segment.s);
		visibleSegment.endAngle = m_projection->ProjectionAngle(segment.e);
		if (!Projection::NormalizeViewAngleSpan(visibleSegment.startAngle, visibleSegment.endAngle))
		{
			return;
		}

		// project edges of the segment onto the screen from left to right
		visibleSegment.startX = m_projection->ViewX(visibleSegment.startAngle);
		visibleSegment.endX = m_projection->ViewX(visibleSegment.endAngle);
		if (visibleSegment.startX > visibleSegment.endX)
		{
			std::swap(visibleSegment.startX, visibleSegment.endX);
			std::swap(visibleSegment.startAngle, visibleSegment.endAngle);
		}

		// clip the segment against already drawn solid walls (horizontal occlusion)
		const auto visibleSpans = m_frame->ClipHorizontalSegment(visibleSegment.startX, visibleSegment.endX, segment.isSolid);
		if (visibleSpans.empty())
		{
			return;
		}

		// draw all visible spans
		visibleSegment.normalVector = m_projection->NormalVector(segment); // normal vector from the segment towards the player
		visibleSegment.normalOffset = m_projection->NormalOffset(segment); // offset of the normal vector from the start of the line (for texturing)
		for (const auto& s : visibleSpans)
		{
			DrawMapSegmentSpan(s, visibleSegment);
		}

		m_frame->m_drawnSectors.push_back(segment.frontSide.sector);
	}

	// draw a visible span of a segment on the frame buffer
	void ViewRenderer::DrawMapSegmentSpan(const Frame::Span& span, const VisibleSegment& visibleSegment) const
	{
		const Segment& segment = visibleSegment.segment;

		// iterate through all vertical columns from left to right
		for (auto x = span.s; x <= span.e; x++)
		{
			// calculate the relative angle and distance to the segment from our viewpoint
			auto viewAngle = GetViewAngle(x, visibleSegment);
			auto distance = m_projection->Distance(visibleSegment.normalVector, viewAngle);
			if (distance < s_minDistance)
			{
				continue;
			}

			// calculate on-screen vertical start and end positions for the column, based on the front (outer) side
			const auto outerTopY = m_projection->ViewY(distance, segment.frontSide.sector.ceilingHeight - m_gameState.m_player.z);
			const auto outerBottomY = m_projection->ViewY(distance, segment.frontSide.sector.floorHeight - m_gameState.m_player.z);

			TextureContext outerTexture;
			outerTexture.yScale = m_projection->TextureScale(distance, 1.0f); // yScale of 1 height
			outerTexture.yPegging = visibleSegment.segment.lowerUnpegged ? outerBottomY : outerTopY;
			outerTexture.textureName = visibleSegment.segment.frontSide.middleTexture;
			outerTexture.yOffset = visibleSegment.segment.frontSide.yOffset;
			outerTexture.xPos = visibleSegment.normalOffset + m_projection->Offset(visibleSegment.normalVector, viewAngle)
				+ visibleSegment.segment.xOffset + visibleSegment.segment.frontSide.xOffset;

			const auto lightness = GetLightness(distance, &segment) * segment.frontSide.sector.lightLevel;
			const auto ceilingHeight = segment.frontSide.sector.isSky ? s_skyHeight : (segment.frontSide.sector.ceilingHeight - m_gameState.m_player.z);
			const auto floorHeight = segment.frontSide.sector.floorHeight - m_gameState.m_player.z;

			// clip the column based on what we've already have drawn (vertical occlusion)
			const auto& outerSpan = m_frame->ClipVerticalSegment(x, outerTopY, outerBottomY, segment.isSolid, &ceilingHeight, &floorHeight,
				segment.frontSide.sector.ceilingTexture, segment.frontSide.sector.floorTexture, segment.frontSide.sector.lightLevel);
			if (outerSpan.isVisible())
			{
				RenderColumnSpan(x, outerSpan, outerTexture, lightness, visibleSegment);
			}

			// if the segment is not a solid wall but a pass-through portal clip its back (inner) side
			if (!segment.isSolid)
			{
				// recalculate column size for the back side
				const auto innerTopY = m_projection->ViewY(distance, segment.backSide.sector.ceilingHeight - m_gameState.m_player.z);
				const auto innerBottomY = m_projection->ViewY(distance, segment.backSide.sector.floorHeight - m_gameState.m_player.z);

				// segments that connect open sky sectors should not have their top sections drawn
				const auto isSky = segment.frontSide.sector.isSky && segment.backSide.sector.isSky;

				// clip the inner span against what's already been drawn
				const auto& innerSpan = m_frame->ClipVerticalSegment(x, innerTopY, innerBottomY, segment.isSolid, isSky ? &s_skyHeight : nullptr, nullptr,
					segment.frontSide.sector.ceilingTexture, segment.frontSide.sector.floorTexture, segment.frontSide.sector.lightLevel);
				if (innerSpan.isVisible())
				{
					// render the inner section (floor/ceiling height change)
					if (!isSky)
					{
						Frame::Span upperSpan(outerSpan.s, innerSpan.s);
						TextureContext upperTexture;
						upperTexture.textureName = visibleSegment.segment.frontSide.upperTexture;
						upperTexture.yScale = outerTexture.yScale;
						upperTexture.xPos = outerTexture.xPos;
						upperTexture.yOffset = outerTexture.yOffset;
						upperTexture.yPegging = visibleSegment.segment.upperUnpegged ? outerTopY : innerTopY;
						RenderColumnSpan(x, upperSpan, upperTexture, lightness, visibleSegment);
					}

					Frame::Span lowerSpan(innerSpan.e, outerSpan.e);
					TextureContext lowerTexture;
					lowerTexture.textureName = visibleSegment.segment.frontSide.lowerTexture;
					lowerTexture.yScale = outerTexture.yScale;
					lowerTexture.xPos = outerTexture.xPos;
					lowerTexture.yOffset = outerTexture.yOffset;
					lowerTexture.yPegging = visibleSegment.segment.lowerUnpegged ? outerTopY : innerBottomY;
					RenderColumnSpan(x, lowerSpan, lowerTexture, lightness, visibleSegment);
				}
			}
		}
	}

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

			auto dy = span.s;
			while (dy <= span.e)
			{
				const auto vs = (dy - textureContext.yPegging) / textureContext.yScale;
				const auto ty = Utils::Clip(static_cast<int>(vs) + textureContext.yOffset, texture->height);

				m_frameBuffer->SetPixel(x, dy, texture->pixels[ty * texture->width + tx], lightness);
				dy++;
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

	void ViewRenderer::TexturePlane(int x, const Frame::Plane& plane) const
	{
		auto it = m_wadFile.m_textures.find(plane.textureName);

		if (plane.isSky())
		{
			if (m_renderingMode == RenderingMode::Solid)
			{
				m_frameBuffer->VerticalLine(x, plane.s, plane.e, s_planeColor, 0.2f);
			}
			else
			{
				it = m_wadFile.m_textures.find("SKY1");
				const auto& texture = it->second;
				const auto horizon = m_frameBuffer->m_height / 2.0f;
				const auto viewAngle = m_projection->ViewAngle(x);
				const auto tx = Utils::Clip(static_cast<int>((m_gameState.m_player.a + viewAngle) / (PI / 4) * texture->width), texture->width);
				for (auto y = plane.s; y <= plane.e; y++)
				{
					const auto ty = Utils::Clip(static_cast<int>(y / (horizon * 2.0f) * texture->height), texture->height);
					m_frameBuffer->SetPixel(x, y, texture->pixels[texture->width * ty + tx], 1);
				}
			}
			return;
		}

		for (auto y = plane.s; y <= plane.e; y++)
		{
			if (m_renderingMode == RenderingMode::Textured)
			{
				if (it == m_wadFile.m_textures.end())
				{
					return;
				}
				const auto& texture = it->second;
				auto angle = Projection::NormalizeAngle(m_gameState.m_player.a);
				auto viewAngle = m_projection->ViewAngle(x);
				auto distance = m_projection->PlaneDistance(y, plane.h) / MathCache::instance().Cos(viewAngle);
				if (isfinite(distance) && distance > s_minDistance)
				{
					auto tx = Utils::Clip(static_cast<int>(m_gameState.m_player.x + distance * MathCache::instance().Cos(angle + viewAngle)), texture->width);
					auto ty = Utils::Clip(static_cast<int>(m_gameState.m_player.y + distance * MathCache::instance().Sin(angle + viewAngle)), texture->height);
					m_frameBuffer->SetPixel(x, y, texture->pixels[texture->width * ty + tx], GetLightness(distance) * plane.lightLevel);
				}
			}
			else
			{
				m_frameBuffer->VerticalLine(x, y, y, s_planeColor, GetLightness(m_projection->PlaneDistance(y, plane.h) * plane.lightLevel));
			}
		}
	}

	// render floors and ceilings based on data collected during drawing walls
	void ViewRenderer::DrawPlanes() const
	{
		if (m_renderingMode == RenderingMode::Solid || m_renderingMode == RenderingMode::Textured)
		{
			for (auto x = 0; x < m_frameBuffer->m_width; x++)
			{
				for (const auto& floorPlane : m_frame->m_floorPlanes[x])
				{
					TexturePlane(x, floorPlane);
				}
				for (const auto& ceilingPlane : m_frame->m_ceilingPlanes[x])
				{
					TexturePlane(x, ceilingPlane);
				}
			}
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

	// calculate the lightness of a segment based on its distance
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

	ViewRenderer::~ViewRenderer()
	{
	}
}