#pragma once

#include "rtdoom.h"
#include "MapStore.h"

namespace rtdoom
{
	struct Point
	{
		constexpr Point(float x, float y) : x{ x }, y{ y } {}
		
		float x;
		float y;
	};

	struct Vertex : Point
	{
		constexpr Vertex(float x, float y) : Point{ x, y } {}
		constexpr Vertex(int x, int y) : Point{ static_cast<float>(x), static_cast<float>(y) } {}
	};

	struct Location : Point
	{
		constexpr Location(float x, float y, float z) : Point{ x, y }, z{ z } {}
		
		float z;
	};

	struct Thing : Location
	{
		constexpr Thing(float x, float y, float z, Angle a) : Location{ x, y, z }, a{ a } {}
		
		Angle a;
	};

	struct Line
	{
		constexpr Line(Vertex s, Vertex e) : s{ s }, e{ e } {}
		
		Vertex s;
		Vertex e;
	};

	struct Sector
	{
		constexpr Sector(float floorHeight, float ceilingHeight, bool isSky) :
			floorHeight{ floorHeight }, ceilingHeight{ ceilingHeight }, isSky{ isSky } {}
		constexpr Sector() : Sector{ 0, 0, false } {}

		Sector(const MapStore::Sector& s) :
			floorHeight{ static_cast<float>(s.floorHeight) },
			ceilingHeight{ static_cast<float>(s.ceilingHeight) },
			isSky{ strcmp(s.ceilingTexture, "F_SKY1") == 0 } {}

		float floorHeight;
		float ceilingHeight;
		bool isSky;
	};

	struct Segment : Line
	{
		constexpr Segment(Vertex s, Vertex e, bool isSolid, Sector frontSector, Sector backSector) :
			Line{ s, e }, isSolid{ isSolid }, frontSector{ frontSector }, backSector{ backSector } {}

		bool isSolid;
		Sector frontSector;
		Sector backSector;

		const bool isHorizontal = s.y == e.y;
		const bool isVertical = s.x == e.x;
	};

	struct Vector
	{
		constexpr Vector(Angle a, float d) : a{ a }, d{ d } {}
		constexpr Vector() : Vector{ 0, 0 } {}

		Angle a;
		float d;
	};
}