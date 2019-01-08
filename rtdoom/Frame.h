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
			Plane(float h, const std::string& textureName, float lightLevel, int height) : h{ h }, textureName{ textureName }, lightLevel{ lightLevel },
				spans(height) {}
			bool isSky() const { return isnan(h); }
			const std::string& textureName;
			const float lightLevel;
			const float h;
			std::vector<std::vector<Span>> spans;

			void addSpan(int x, int sy, int ey);
		};

		const int m_width;
		const int m_height;

		// list of horizontal screen spans where isSolid walls have already been drawn (completely occluded)
		std::list<Span> m_occlusion;

		// screen height where the last floor/ceilings have been drawn up to so far
		std::vector<int> m_floorClip;
		std::vector<int> m_ceilClip;

		// numer of drawn segments
		int m_numSegments = 0;
		int m_numFloorPlanes = 0;
		int m_numCeilingPlanes = 0;

		// spaces between walls with floors and ceilings
		std::deque<Plane> m_floorPlanes;
		std::deque<Plane> m_ceilingPlanes;

		// add vertical span to existing planes
		void MergeIntoPlane(std::deque<Plane>& planes, float height, const std::string& textureName, float lightLevel, int x, int sy, int ey);

		// returns horizontal screen spans where the mapSegment is visible and updates occlusion table
		std::vector<Span> ClipHorizontalSegment(int startX, int endX, bool isSolid);

		// returns the vertical screen span where the column is visible and updates occlussion table
		Span ClipVerticalSegment(int x, int ceilingProjection, int floorProjection, bool isSolid,
			const float* ceilingHeight, const float* floorHeight, const std::string& ceilingTexture, const std::string& floorTexture, float lightLevel);

		bool IsSpanVisible(int x, int sy, int ey) const;
		bool IsHorizontallyOccluded() const;
		bool IsVerticallyOccluded(int x) const;

		Frame(const FrameBuffer& frameBuffer);
		~Frame();
	};
}
