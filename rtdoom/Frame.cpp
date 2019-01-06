#include "pch.h"
#include "Frame.h"

namespace rtdoom
{
	Frame::Frame(const FrameBuffer& frameBuffer) :
		m_width{ frameBuffer.m_width },
		m_height{ frameBuffer.m_height },
		m_floorClip(frameBuffer.m_width + 1, frameBuffer.m_height),
		m_ceilClip(frameBuffer.m_width + 1, -1),
		m_ceilingPlanes{ frameBuffer.m_width + 1 },
		m_floorPlanes{ frameBuffer.m_width + 1 }
	{
	}

	// for a horizontal segment, determine which columns will be visible based on already occludded map
	// if the segment is isSolid, update occlussion map otherwise only clip
	std::vector<Frame::Span> Frame::ClipHorizontalSegment(int startX, int endX, bool isSolid)
	{
		std::vector<Frame::Span> visibleSpans;

		startX = std::max(startX, 0);
		endX = std::max(endX, 0);
		startX = std::min(startX, m_width);
		endX = std::min(endX, m_width);
		if (startX == endX)
		{
			return visibleSpans;
		}

		if (m_occlusion.empty())
		{
			visibleSpans.push_back(Frame::Span(startX, endX));
			if (isSolid)
			{
				m_occlusion.push_front(Frame::Span(startX, endX));
			}
		}
		else
		{
			auto eo = m_occlusion.begin();
			do
			{
				if (eo->s >= endX)
				{
					// doesn't overlap
					visibleSpans.push_back(Frame::Span(startX, endX));
					if (isSolid)
					{
						m_occlusion.insert(eo, Frame::Span(startX, endX));
					}
					startX = endX;
					break;
				}
				if (eo->e < startX)
				{
					continue;
				}
				if (eo->s <= startX && eo->e >= endX)
				{
					// completely covered
					startX = endX;
					break;
				}
				if (eo->s < startX)
				{
					// occlusion starts before us, roll start point
					startX = std::min(endX, eo->e);
				}
				else if (eo->s > startX)
				{
					// show and occlude left part
					visibleSpans.push_back(Frame::Span(startX, eo->s));
					if (isSolid)
					{
						eo->s = startX;
					}
					startX = std::min(endX, eo->e);
				}
				if (eo->e >= endX)
				{
					// completely covered
					startX = endX;
					break;
				}
				else if (eo->e < endX)
				{
					startX = eo->e;
				}
			} while (++eo != m_occlusion.end() && startX != endX);

			if (endX > startX)
			{
				visibleSpans.push_back(Frame::Span(startX, endX));
				if (isSolid)
				{
					m_occlusion.push_back(Frame::Span(startX, endX));
				}
			}

			// merge neighbouring segments
			if (isSolid && m_occlusion.size())
			{
				eo = m_occlusion.begin();
				do
				{
					auto pe = eo++;
					if (eo != m_occlusion.end() && pe->e == eo->s)
					{
						eo->s = pe->s;
						m_occlusion.erase(pe);
					}
				} while (eo != m_occlusion.end());
			}
		}

		return visibleSpans;
	}

	bool Frame::IsHorizontallyOccluded() const
	{
		return m_occlusion.size() == 1 && m_occlusion.begin()->s == 0 && m_occlusion.begin()->e == m_width;
	}

	bool Frame::IsVerticallyOccluded(int x) const
	{
		return m_floorClip[x] <= m_ceilClip[x];
	}

	// for a vertical segment, determine which section is visible and update occlusion map
	Frame::Span Frame::ClipVerticalSegment(int x, int ceilingProjection, int floorProjection, bool isSolid,
		const float* ceilingHeight, const float* floorHeight, const std::string& ceilingTexture, const std::string& floorTexture,
		float lightLevel)
	{
		Frame::Span span;

		if (IsVerticallyOccluded(x))
		{
			return span;
		}

		if (ceilingProjection > m_ceilClip[x])
		{
			span.s = std::min(ceilingProjection, m_floorClip[x]);
			if (ceilingHeight)
			{
				m_ceilingPlanes[x].push_back(Plane(*ceilingHeight, m_ceilClip[x], span.s, ceilingTexture, lightLevel));
			}
		}
		else
		{
			span.s = m_ceilClip[x];
		}
		if (floorProjection < m_floorClip[x])
		{
			span.e = std::max(floorProjection, m_ceilClip[x]);
			if (floorHeight)
			{
				m_floorPlanes[x].push_back(Plane(*floorHeight, span.e, m_floorClip[x], floorTexture, lightLevel));
			}
		}
		else
		{
			span.e = m_floorClip[x];
		}

		if (isSolid)
		{
			m_floorClip[x] = m_ceilClip[x] = 0;
		}
		else
		{
			if (ceilingProjection > m_ceilClip[x])
			{
				m_ceilClip[x] = ceilingProjection;
			}
			if (floorProjection < m_floorClip[x])
			{
				m_floorClip[x] = floorProjection;
			}
		}

		return span;
	}

	Frame::~Frame()
	{
	}
}