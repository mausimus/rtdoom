#include "pch.h"
#include "rtdoom.h"
#include "Projection.h"
#include "MathCache.h"

namespace rtdoom
{
	Projection::Projection(const Thing& player, const FrameBuffer& frameBuffer) :
		m_player{ player },
		m_viewWidth{ frameBuffer.m_width },
		m_midPointX{ frameBuffer.m_width / 2 },
		m_viewHeight{ frameBuffer.m_height },
		m_midPointY{ frameBuffer.m_height / 2 }
	{
	}

	// normal vector from a map line towards the player
	Vector Projection::NormalVector(const Line& l) const
	{
		auto lineAngle = MathCache::instance().ArcTan(l.e.y - l.s.y, l.e.x - l.s.x); // 0 is towards positive X axis, clockwise, (0,0) is top-left
		auto normalAngle = lineAngle + PI2;
		auto sx = l.s.x - m_player.x;
		auto sy = l.s.y - m_player.y;
		auto startDistance = sqrt(sx * sx + sy * sy);
		auto startAngle = MathCache::instance().ArcTan(sy, sx);
		auto normalDistance = startDistance * MathCache::instance().Cos(normalAngle - startAngle);
		return Vector(normalAngle, abs(normalDistance));
	}

	// distance the starting edge of the line to its normal vector start
	float Projection::NormalOffset(const Line& l) const
	{
		auto lineAngle = MathCache::instance().ArcTan(l.e.y - l.s.y, l.e.x - l.s.x); // 0 is towards positive X axis, clockwise, (0,0) is top-left
		auto normalAngle = lineAngle + PI2;
		auto sx = l.s.x - m_player.x;
		auto sy = l.s.y - m_player.y;
		auto startDistance = sqrt(sx * sx + sy * sy);
		auto startAngle = MathCache::instance().ArcTan(sy, sx);
		auto normalOffset = fabs(startDistance * MathCache::instance().Sin(normalAngle - startAngle));
		if (NormalizeAngle(normalAngle - startAngle) > 0)
		{
			normalOffset *= -1;
		}
		return normalOffset;
	}

	// relative projected view a for a map point
	Angle Projection::ProjectionAngle(const Point& p) const
	{
		auto absoluteAngle = MathCache::instance().ArcTan(p.y - m_player.y, p.x - m_player.x);
		return NormalizeAngle(absoluteAngle - m_player.a);
	}

	// projection d from player to the line specified by the normal vector at specific viewAngle
	float Projection::Distance(const Vector& normalVector, Angle viewAngle) const
	{
		auto inverseNormalAngle = normalVector.a - PI;
		auto viewRelativeAngle = NormalizeAngle(inverseNormalAngle - (m_player.a + viewAngle));
		auto interceptDistance = normalVector.d / MathCache::instance().Cos(viewRelativeAngle);
		return abs(MathCache::instance().Cos(viewAngle) * interceptDistance);
	}

	// texture offset vs normal intercept
	float Projection::Offset(const Vector& normalVector, Angle viewAngle) const
	{
		auto inverseNormalAngle = normalVector.a - PI;
		auto viewRelativeAngle = NormalizeAngle(inverseNormalAngle - (m_player.a + viewAngle));
		auto interceptDistance = normalVector.d / MathCache::instance().Cos(viewRelativeAngle);
		auto offset = fabsf(interceptDistance * MathCache::instance().Sin(viewRelativeAngle));
		if (viewRelativeAngle > 0)
		{
			offset *= -1;
		}
		return offset;
	}

	float Projection::PlaneDistance(int y, float height) const noexcept
	{
		auto dy = abs(y - m_midPointY);
		return (m_viewHeight * 30.0f) * fabsf(height / 23.0f) / static_cast<float>(dy);
	}

	Angle Projection::AngleDist(Angle a1, Angle a2) noexcept
	{
		auto diff = abs(NormalizeAngle(a2) - NormalizeAngle(a1));
		return diff;
	}

	// normalize a to -PI..PI
	Angle Projection::NormalizeAngle(Angle angle) noexcept
	{
		while (angle < -PI)
		{
			angle += 2 * PI;
		}
		while (angle > PI)
		{
			angle -= 2 * PI;
		}
		return angle;
	}

	// convert eye a (-PI4..PI4) view to screen X coordinate
	int Projection::ViewX(Angle viewAngle) const noexcept
	{
		viewAngle = NormalizeAngle(viewAngle);
		if (viewAngle <= -PI4)
		{
			return -1;
		}
		if (viewAngle >= PI4)
		{
			return m_viewWidth;
		}
		auto midDistance = MathCache::instance().Tan(viewAngle) / PI4;
		if (midDistance <= -1)
		{
			return -1;
		}
		if (midDistance >= 1)
		{
			return m_viewWidth;
		}
		return static_cast<int>(m_midPointX * (1 + midDistance));
	}

	// convert screen X coordinate to eye a (-PI4..PI4)
	Angle Projection::ViewAngle(int viewX) const noexcept
	{
		auto relativeX = (viewX - m_midPointX);
		auto fractionX = static_cast<float>(relativeX) / m_midPointX;
		return MathCache::instance().ArcTan(fractionX * PI4);
	}

	// vertical screen position for given distance and height difference
	int Projection::ViewY(float distance, float height) const noexcept
	{
		auto dc = (m_viewHeight * 30.0f) / distance;
		auto hf = fabsf(height / 23.0f);
		auto dy = static_cast<int>(dc*hf);
		if (height > 0)
		{
			return m_midPointY - dy;
		}
		return m_midPointY + dy;
	}

	// scaling factor for texturing
	float Projection::TextureScale(float distance, float height) const noexcept
	{
		auto dc = (m_viewHeight * 30.0f) / distance;
		auto hf = fabsf(height / 23.0f);
		return dc * hf;
	}

	// trim angles for a visible span
	bool Projection::NormalizeViewAngleSpan(Angle &startAngle, Angle &endAngle) noexcept
	{
		if (abs(AngleDist(startAngle, endAngle)) > PI)
		{
			// crosses behind the player, wrap over the back edge
			if (startAngle < -PI2)
			{
				startAngle = PI2;
			}
			else if (startAngle > PI2)
			{
				startAngle = -PI2;
			}
			else if (endAngle < -PI2)
			{
				endAngle = PI2;
			}
			else if (endAngle > PI2)
			{
				endAngle = -PI2;
			}
		}

		if (startAngle < -PI4 && endAngle < -PI4)
		{
			return false;
		}
		if (endAngle > PI4 && startAngle > PI4)
		{
			return false;
		}

		return true;
	}

	Projection::~Projection()
	{
	}
}