#include "pch.h"
#include "rtdoom.h"
#include "ViewRenderer.h"
#include "MathCache.h"
#include "WireframePainter.h"
#include "SolidPainter.h"
#include "TexturePainter.h"

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
		Initialize(frameBuffer);

		// iterate through all segments (map lines) in visibility order returned by traversing the map's BSP tree
		for (const auto& segment : m_gameState.m_mapDef->GetSegmentsToDraw(m_gameState.m_player))
		{
			// only draw segments that are facing the player
			if (MapDef::IsInFrontOf(m_gameState.m_player, *segment))
			{
				RenderMapSegment(*segment);

				// stop drawing once the frame has been fully horizontally occluded with solid walls
				if (m_frame->IsHorizontallyOccluded())
				{
					break;
				}
			}
		}

		// fill in floors and ceilings
		RenderPlanes();
	}

	void ViewRenderer::Initialize(FrameBuffer& frameBuffer)
	{
		m_frameBuffer = &frameBuffer;
		m_projection = std::make_unique<Projection>(m_gameState.m_player, frameBuffer);
		m_frame = std::make_unique<Frame>(frameBuffer);
		switch (m_renderingMode)
		{
		case RenderingMode::Wireframe:
			m_painter = std::make_unique<WireframePainter>(frameBuffer);
			break;
		case RenderingMode::Solid:
			m_painter = std::make_unique<SolidPainter>(frameBuffer, *m_projection);
			break;
		case RenderingMode::Textured:
			m_painter = std::make_unique<TexturePainter>(frameBuffer, m_gameState.m_player, *m_projection, m_wadFile);
			break;
		default:
			throw std::runtime_error("Unsupported rendering mode");
		}
	}

	void ViewRenderer::RenderMapSegment(const Segment& segment) const
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
			RenderMapSegmentSpan(span, visibleSegment);
		}

		m_frame->m_numSegments++;
	}

	// draw a visible span of a mapSegment on the frame buffer
	void ViewRenderer::RenderMapSegmentSpan(const Frame::Span& span, const VisibleSegment& visibleSegment) const
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
			outerTexture.isEdge = x == visibleSegment.startX || x == visibleSegment.endX;
			outerTexture.lightness = m_projection->Lightness(distance, &mapSegment) * mapSegment.frontSide.sector.lightLevel;
			const auto ceilingHeight = mapSegment.frontSide.sector.isSky ? s_skyHeight : (mapSegment.frontSide.sector.ceilingHeight - m_gameState.m_player.z);
			const auto floorHeight = mapSegment.frontSide.sector.floorHeight - m_gameState.m_player.z;

			// clip the column based on what we've already have drawn (vertical occlusion)
			const auto& outerSpan = m_frame->ClipVerticalSegment(x, outerTopY, outerBottomY, mapSegment.isSolid, &ceilingHeight, &floorHeight,
				mapSegment.frontSide.sector.ceilingTexture, mapSegment.frontSide.sector.floorTexture, mapSegment.frontSide.sector.lightLevel);
			if (outerSpan.isVisible())
			{
				m_painter->PaintWall(x, outerSpan, outerTexture);
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
						TextureContext upperTexture(outerTexture);
						upperTexture.textureName = mapSegment.frontSide.upperTexture;
						upperTexture.yPegging = mapSegment.upperUnpegged ? outerTopY : innerTopY;
						m_painter->PaintWall(x, upperSpan, upperTexture);
					}

					Frame::Span lowerSpan(innerSpan.e, outerSpan.e);
					TextureContext lowerTexture(outerTexture);
					lowerTexture.textureName = mapSegment.frontSide.lowerTexture;
					lowerTexture.yPegging = mapSegment.lowerUnpegged ? outerTopY : innerBottomY;
					m_painter->PaintWall(x, lowerSpan, lowerTexture);
				}
			}
		}
	}

	// render floors and ceilings based on data collected during drawing walls
	void ViewRenderer::RenderPlanes() const
	{
		if (m_renderingMode == RenderingMode::Solid || m_renderingMode == RenderingMode::Textured)
		{
			for (const auto& floorPlane : m_frame->m_floorPlanes)
			{
				m_painter->PaintPlane(floorPlane);
				m_frame->m_numFloorPlanes++;
			}
			for (const auto& ceilingPlane : m_frame->m_ceilingPlanes)
			{
				m_painter->PaintPlane(ceilingPlane);
				m_frame->m_numCeilingPlanes++;
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