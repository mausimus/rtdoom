#pragma once

#include "FrameBuffer.h"
#include "MapDef.h"

namespace rtdoom
{
	// class holding working structures used in drawing a single frame
	class Frame
	{
	public:
		struct Span
		{
			constexpr Span(int s, int e) : s{ s }, e{ e } {}
			constexpr Span() : Span{ 0, 0 } {}
			bool isVisible() const { return (s != 0 || e != 0); };
			int s;
			int e;
		};

		struct Plane
		{
			Plane(float h, int s, int e, std::string textureName, float lightLevel) : h{ h }, s{ s }, e{ e }, textureName{ textureName }, lightLevel{ lightLevel } {}
			bool isSky() const { return isnan(h); }
			std::string textureName;
			float lightLevel;
			float h;
			int s;
			int e;
		};

		const int m_width;
		const int m_height;

		// list of horizontal screen spans where isSolid walls have already been drawn (completely occluded)
		std::list<Span> m_occlusion;

		// screen height where the last floor/ceilings have been drawn up to so far
		std::vector<int> m_floorClip;
		std::vector<int> m_ceilClip;

		// spaces between walls with floors and ceilings
		std::vector<std::vector<Plane>> m_floorPlanes;
		std::vector<std::vector<Plane>> m_ceilingPlanes;

		// height of the current view point based on sector the player is in
		std::vector<Sector> m_drawnSectors;

		// returns horizontal screen spans where the segment is visible and updates occlusion table
		std::vector<Span> ClipHorizontalSegment(int startX, int endX, bool isSolid);

		// returns the vertical screen span where the column is visible and updates occlussion table
		Span ClipVerticalSegment(int x, int ceilingProjection, int floorProjection, bool isSolid,
			const float* ceilingHeight, const float* floorHeight, const std::string& ceilingTexture, const std::string& floorTexture, float lightLevel);

		bool IsHorizontallyOccluded() const;
		bool IsVerticallyOccluded(int x) const;

		Frame(const FrameBuffer& frameBuffer);
		~Frame();
	};
}
