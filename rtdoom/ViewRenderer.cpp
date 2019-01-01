#include "pch.h"
#include "rtdoom.h"
#include "ViewRenderer.h"

namespace rtdoom
{
	ViewRenderer::ViewRenderer(const GameState& gameState) :
		Renderer{ gameState },
		m_renderingMode{ RenderingMode::Solid }
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

		// clip the segment against already drawn isSolid walls (horizontal occlusion)
		const auto visibleSpans = m_frame->ClipHorizontalSegment(visibleSegment.startX, visibleSegment.endX, segment.isSolid);
		if (visibleSpans.empty())
		{
			return;
		}

		// draw all visible spans
		visibleSegment.normalVector = m_projection->NormalVector(segment);
		for (const auto& s : visibleSpans)
		{
			DrawMapSegmentSpan(s, visibleSegment);
		}

		m_frame->m_drawnSectors.push_back(segment.frontSector);
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
			const auto outerTopY = m_projection->ViewY(distance, segment.frontSector.ceilingHeight - m_gameState.m_player.z);
			const auto outerBottomY = m_projection->ViewY(distance, segment.frontSector.floorHeight - m_gameState.m_player.z);

			const auto lightness = GetLightness(distance, &segment);
			const auto ceilingHeight = segment.frontSector.isSky ? s_skyHeight : (segment.frontSector.ceilingHeight - m_gameState.m_player.z);
			const auto floorHeight = segment.frontSector.floorHeight - m_gameState.m_player.z;

			// clip the column based on what we've already have drawn (vertical occlusion)
			const auto& outerSpan = m_frame->ClipVerticalSegment(x, outerTopY, outerBottomY, segment.isSolid, &ceilingHeight, &floorHeight);
			if (outerSpan.isVisible())
			{
				RenderColumnOuterSpan(x, outerSpan, lightness, visibleSegment);
			}

			// if the segment is not a solid wall but a pass-through portal clip its back (inner) side
			if (!segment.isSolid)
			{
				// recalculate column size for the back side
				const auto innerTopY = m_projection->ViewY(distance, segment.backSector.ceilingHeight - m_gameState.m_player.z);
				const auto innerBottomY = m_projection->ViewY(distance, segment.backSector.floorHeight - m_gameState.m_player.z);

				// segments that connect open sky sectors should not have their top sections drawn
				const auto isSky = segment.frontSector.isSky && segment.backSector.isSky;

				// clip the inner span against what's already been drawn
				const auto& innerSpan = m_frame->ClipVerticalSegment(x, innerTopY, innerBottomY, segment.isSolid, isSky ? &s_skyHeight : nullptr, nullptr);
				if (innerSpan.isVisible())
				{
					// render the inner section (floor/ceiling height change)
					RenderColumnInnerSpan(x, innerSpan, isSky, lightness, outerSpan, visibleSegment);
				}
			}
		}
	}

	// render the outer (front) section of a column of a visible span on the framebuffer, either a isSolid wall or only edges for pass-through portals
	void ViewRenderer::RenderColumnOuterSpan(int x, const Frame::Span& outerSpan, float lightness, const VisibleSegment& visibleSegment) const
	{
		switch (m_renderingMode)
		{
		case RenderingMode::Wireframe:
			m_frameBuffer->SetPixel(x, outerSpan.s, s_wallColor, lightness);
			m_frameBuffer->SetPixel(x, outerSpan.e, s_wallColor, lightness);
			if (visibleSegment.segment.isSolid && (x == visibleSegment.startX || x == visibleSegment.endX))
			{
				m_frameBuffer->VerticalLine(x, outerSpan.s + 1, outerSpan.e - 1, s_wallColor, lightness / 2.0f);
			}
			break;
		case RenderingMode::Solid:
			if (visibleSegment.segment.isSolid)
			{
				m_frameBuffer->VerticalLine(x, outerSpan.s, outerSpan.e, s_wallColor, lightness);
			}
			break;
		default:
			break;
		}
	}

	// render the inner (back) section of a column of a visible span on the framebuffer, representing change in floor/ceiling height
	void ViewRenderer::RenderColumnInnerSpan(int x, const Frame::Span& innerSpan, bool isSky, float lightness, const Frame::Span& outerSpan, const VisibleSegment& visibleSegment) const
	{
		switch (m_renderingMode)
		{
		case RenderingMode::Wireframe:
			m_frameBuffer->SetPixel(x, innerSpan.s, s_wallColor, lightness);
			m_frameBuffer->SetPixel(x, innerSpan.e, s_wallColor, lightness);
			if (x == visibleSegment.startX || x == visibleSegment.endX)
			{
				m_frameBuffer->VerticalLine(x, outerSpan.s + 1, innerSpan.s - 1, s_wallColor, lightness / 3.0f);
				m_frameBuffer->VerticalLine(x, outerSpan.e - 1, innerSpan.e + 1, s_wallColor, lightness / 3.0f);
			}
			break;
		case RenderingMode::Solid:
			if (!isSky)
			{
				m_frameBuffer->VerticalLine(x, outerSpan.s, innerSpan.s, s_wallColor, lightness);
			}
			m_frameBuffer->VerticalLine(x, innerSpan.e, outerSpan.e, s_wallColor, lightness);
			break;
		default:
			break;
		}
	}

	// render floors and ceilings based on data collected during drawing walls
	void ViewRenderer::DrawPlanes() const
	{
		if (m_renderingMode == RenderingMode::Solid)
		{
			for (auto x = 0; x < m_frameBuffer->m_width; x++)
			{
				for (const auto& floorPlane : m_frame->m_floorPlanes[x])
				{
					for (auto y = floorPlane.s; y <= floorPlane.e; y++)
					{
						m_frameBuffer->SetPixel(x, y, s_planeColor, GetLightness(m_projection->PlaneDistance(y, floorPlane.h)));
					}
				}
				for (const auto& ceilingPlane : m_frame->m_ceilingPlanes[x])
				{
					if (ceilingPlane.isSky())
					{
						m_frameBuffer->VerticalLine(x, ceilingPlane.s, ceilingPlane.e, s_planeColor, 0.2f);
					}
					else
					{
						for (auto y = ceilingPlane.s; y <= ceilingPlane.e; y++)
						{
							m_frameBuffer->SetPixel(x, y, s_planeColor, GetLightness(m_projection->PlaneDistance(y, ceilingPlane.h)));
						}
					}
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