#pragma once

#include "MapDef.h"
#include "FrameBuffer.h"

namespace rtdoom
{
	class Projection
	{
	protected:
		const Thing& m_player;
		const int m_viewWidth;
		const int m_viewHeight;
		const int m_midPointX;
		const int m_midPointY;

	public:
		static Angle AngleDist(Angle a1, Angle a2) noexcept;
		static Angle NormalizeAngle(Angle angle) noexcept;
		static bool NormalizeViewAngleSpan(Angle& a1, Angle& a2) noexcept;
		static float Distance(const Point& a, const Point& b);

		int ViewX(Angle viewAngle) const noexcept;
		int ViewY(float distance, float height) const noexcept;
		float TextureScale(float distance) const noexcept;
		Angle ViewAngle(int screenX) const noexcept;
		Vector NormalVector(const Line& l) const;
		float NormalOffset(const Line& l) const;
		float Distance(const Vector& normalVector, Angle viewAngle) const;
		float Offset(const Vector& normalVector, Angle viewAngle) const;
		float PlaneDistance(int y, float height) const noexcept;
		Angle ProjectionAngle(const Point& p) const;
		float Lightness(float distance, const Segment* segment = nullptr) const;

		Projection(const Thing& player, const FrameBuffer& frameBuffer);
		~Projection();
	};
}