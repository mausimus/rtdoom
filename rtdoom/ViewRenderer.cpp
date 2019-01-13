#include "pch.h"
#include "rtdoom.h"
#include "ViewRenderer.h"
#include "MathCache.h"
#include "WireframePainter.h"
#include "SolidPainter.h"
#include "TexturePainter.h"

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

		// render things/sprites
		RenderSprites();

		// HUD etc.
		RenderOverlay();
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
		VisibleSegment vs{ segment };

		// calculate relative view angles of the mapSegment and skip rendering if they are not within the field of view
		vs.startAngle = m_projection->ProjectionAngle(segment.s);
		vs.endAngle = m_projection->ProjectionAngle(segment.e);
		if (!Projection::NormalizeViewAngleSpan(vs.startAngle, vs.endAngle))
		{
			return;
		}

		// project edges of the mapSegment onto the screen from left to right
		vs.startX = m_projection->ViewX(vs.startAngle);
		vs.endX = m_projection->ViewX(vs.endAngle);
		if (vs.startX > vs.endX)
		{
			std::swap(vs.startX, vs.endX);
			std::swap(vs.startAngle, vs.endAngle);
		}

		// clip the mapSegment against already drawn solid walls (horizontal occlusion)
		const auto visibleSpans = m_frame->ClipHorizontalSegment(vs.startX, vs.endX, segment.isSolid);
		if (visibleSpans.empty())
		{
			return;
		}

		// draw all visible spans
		vs.normalVector = m_projection->NormalVector(segment); // normal vector from the mapSegment towards the player
		vs.normalOffset = m_projection->NormalOffset(segment); // offset of the normal vector from the start of the line (for texturing)
		for (const auto& span : visibleSpans)
		{
			RenderMapSegmentSpan(span, vs);
		}

		m_frame->m_sectors.emplace(segment.frontSide.sector.sectorId);
		m_frame->m_sectors.emplace(segment.backSide.sector.sectorId);
		m_frame->m_numSegments++;
	}

	// draw a visible span of a mapSegment on the frame buffer
	void ViewRenderer::RenderMapSegmentSpan(const Frame::Span& span, const VisibleSegment& visibleSegment) const
	{
		const auto& mapSegment = visibleSegment.mapSegment;
		const auto& frontSector = mapSegment.frontSide.sector;
		Frame::Clip lowerClip{ span }, middleClip{ span }, upperClip{ span };

		// iterate through all vertical columns from left to right
		for (auto x = span.s; x <= span.e; x++)
		{
			// calculate the relative angle and distance to the mapSegment from our viewpoint
			auto viewAngle = GetViewAngle(x, visibleSegment);
			auto distance = m_projection->Distance(visibleSegment.normalVector, viewAngle);
			if (distance < s_minDistance)
			{
				lowerClip.Add(x, Frame::PainterContext(), 0, 0);
				middleClip.Add(x, Frame::PainterContext(), 0, 0);
				upperClip.Add(x, Frame::PainterContext(), 0, 0);
				continue;
			}

			// calculate on-screen vertical start and end positions for the column, based on the front (outer) side
			const auto outerTopY = m_projection->ViewY(distance, frontSector.ceilingHeight - m_gameState.m_player.z);
			const auto outerBottomY = m_projection->ViewY(distance, frontSector.floorHeight - m_gameState.m_player.z);

			Frame::PainterContext outerTexture;
			outerTexture.yScale = m_projection->TextureScale(distance);
			outerTexture.yPegging = mapSegment.lowerUnpegged ? outerBottomY : outerTopY;
			outerTexture.textureName = mapSegment.frontSide.middleTexture;
			outerTexture.yOffset = mapSegment.frontSide.yOffset;
			// texel x position is the offset from player to normal vector plus offset from normal vector to view, plus static mapSegment and linedef offsets
			outerTexture.texelX = visibleSegment.normalOffset + m_projection->Offset(visibleSegment.normalVector, viewAngle)
				+ mapSegment.xOffset + mapSegment.frontSide.xOffset;
			outerTexture.isEdge = x == visibleSegment.startX || x == visibleSegment.endX;
			outerTexture.lightness = m_projection->Lightness(distance, &mapSegment) * frontSector.lightLevel;

			// clip the column based on what we've already have drawn (vertical occlusion)
			const auto ceilingHeight = frontSector.isSky ? s_skyHeight : (frontSector.ceilingHeight - m_gameState.m_player.z);
			const auto floorHeight = frontSector.floorHeight - m_gameState.m_player.z;
			const auto& outerSpan = m_frame->ClipVerticalSegment(x, outerTopY, outerBottomY, mapSegment.isSolid, &ceilingHeight, &floorHeight,
				frontSector.ceilingTexture, frontSector.floorTexture, frontSector.lightLevel);

			if (outerSpan.isVisible())
			{
				m_painter->PaintWall(x, outerSpan, outerTexture);
			}

			if (mapSegment.isSolid)
			{
				middleClip.Add(x, outerTexture, 0, m_frameBuffer->m_height - 1);
			}
			else
			{
				// if the mapSegment is not a solid wall but a pass-through portal clip its back (inner) side
				const auto& backSector = mapSegment.backSide.sector;

				// recalculate column size for the back side
				const auto innerTopY = m_projection->ViewY(distance, backSector.ceilingHeight - m_gameState.m_player.z);
				const auto innerBottomY = m_projection->ViewY(distance, backSector.floorHeight - m_gameState.m_player.z);

				// segments that connect open sky sectors should not have their top sections drawn
				const auto isSky = frontSector.isSky && backSector.isSky;

				// clip the inner span against what's already been drawn
				const auto& innerSpan = m_frame->ClipVerticalSegment(x, innerTopY, innerBottomY, mapSegment.isSolid, isSky ? &s_skyHeight : nullptr, nullptr,
					frontSector.ceilingTexture, frontSector.floorTexture, frontSector.lightLevel);

				upperClip.Add(x, outerTexture, 0, std::max(innerTopY, outerTopY));
				lowerClip.Add(x, outerTexture, std::min(innerBottomY, outerBottomY), m_frameBuffer->m_height - 1);

				if (innerSpan.isVisible())
				{
					// render the inner section (floor/ceiling height change)
					if (!isSky)
					{
						Frame::Span upperSpan{ outerSpan.s, innerSpan.s };
						Frame::PainterContext upperTexture{ outerTexture };
						upperTexture.textureName = mapSegment.frontSide.upperTexture;
						upperTexture.yPegging = mapSegment.upperUnpegged ? outerTopY : innerTopY;
						m_painter->PaintWall(x, upperSpan, upperTexture);
					}

					Frame::Span lowerSpan{ innerSpan.e, outerSpan.e };
					Frame::PainterContext lowerTexture{ outerTexture };
					lowerTexture.textureName = mapSegment.frontSide.lowerTexture;
					lowerTexture.yPegging = mapSegment.lowerUnpegged ? outerTopY : innerBottomY;
					m_painter->PaintWall(x, lowerSpan, lowerTexture);
				}
			}
		}

		if (mapSegment.isSolid)
		{
			m_frame->m_clips.push_back(middleClip);
		}
		else
		{
			m_frame->m_clips.push_back(upperClip);
			m_frame->m_clips.push_back(lowerClip);
		}
	}

	// render floors and ceilings based on data collected during drawing walls
	void ViewRenderer::RenderPlanes() const
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

	// render sprites (things)
	void ViewRenderer::RenderSprites() const
	{
		// collect things to draw by going through things located in sectors visited during frame drawing
		std::vector<Thing> things;
		for (const auto s : m_frame->m_sectors)
		{
			if (s >= 0)
			{
				things.insert(things.end(), m_gameState.m_mapDef->m_things[s].begin(), m_gameState.m_mapDef->m_things[s].end());
			}
		}

		// sort from farthest to nearest
		for (auto& thing : things)
		{
			thing.distance = Projection::Distance(thing, m_gameState.m_player);
		}
		std::sort(things.begin(), things.end());

		for (const auto& thing : things)
		{
			const auto sprite = m_wadFile.m_sprites.find(thing.textureName);
			const auto angle = m_projection->ProjectionAngle(thing);
			if (thing.distance < s_minDistance || thing.textureName.empty() || sprite == m_wadFile.m_sprites.end() || angle < -PI4 || angle > PI4)
			{
				continue;
			}

			const auto centerDistance = thing.distance * MathCache::instance().Cos(angle);
			const auto scale = m_projection->TextureScale(centerDistance);
			if (scale < s_minScale)
			{
				continue;
			}

			const auto midDistance = MathCache::instance().Tan(angle) / PI4;
			const auto spriteX = static_cast<int>((m_frameBuffer->m_width / 2) * (1 + midDistance));
			const auto spriteY = m_projection->ViewY(centerDistance, thing.z - m_gameState.m_player.z);
			const auto& tx = sprite->second;
			const auto px = static_cast<int>(tx->width / scale);
			const auto py = static_cast<int>(tx->height / scale);
			const auto sy = static_cast<int>(spriteY - tx->top / scale);
			auto sx = static_cast<int>(spriteX - tx->left / scale);

			// build sprite occlusion matrix
			// our sprite spans from [sx, sx + px] and [sy, sy + py]
			std::vector<std::vector<bool>> occlusion(px);
			for (auto& o : occlusion)
			{
				o.resize(py);
			}
			for (const auto& clip : m_frame->m_clips)
			{
				// for every clip that's in front of the sprite and overlaps it, add it to occlusion matrix
				if ((clip.yScaleStart < scale || clip.yScaleEnd < scale) && clip.xSpan.s < sx + px && clip.xSpan.e > sx)
				{
					// extra check if is sprite is alongside the clip via scale interpolation
					if (clip.yScaleStart > scale || clip.yScaleEnd > scale)
					{
						const auto yScaleOnSprite = clip.yScaleStart + (spriteX - clip.xSpan.s) * (clip.yScaleEnd - clip.yScaleStart) / (clip.xSpan.e - clip.xSpan.s);
						if (yScaleOnSprite > scale)
						{
							continue;
						}
					}
					for (auto x = sx; x < sx + px; x++)
					{
						if (x < clip.xSpan.s || x > clip.xSpan.e)
						{
							continue;
						}
						const auto clipX = x - clip.xSpan.s;
						for (auto y = sy; y < sy + py; y++)
						{
							if (y >= clip.topClips[clipX] && y <= clip.bottomClips[clipX])
							{
								occlusion[x - sx][y - sy] = true;
							}
						}
					}
				}
			}

			// draw individual sprite columns
			Frame::PainterContext sc;
			sc.textureName = thing.textureName;
			sc.yScale = scale;
			sc.lightness = m_gameState.m_mapDef->m_sectors[thing.sectorId].lightLevel * m_projection->Lightness(centerDistance);
			for (int x = 0; x < px; x++)
			{
				const auto dx = sx + x;
				if (dx >= 0 && dx < m_frameBuffer->m_width)
				{
					sc.texelX = static_cast<float>(x) * tx->width / px;
					m_painter->PaintSprite(dx, sy, occlusion[x], sc);
				}
			}
		}
	}

	void ViewRenderer::RenderOverlay() const
	{
		// TODO: render HUD etc.
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