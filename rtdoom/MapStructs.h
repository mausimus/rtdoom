#pragma once

#include "rtdoom.h"
#include "Helpers.h"
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
		Thing(float x, float y, float z, Angle a) : Location{ x, y, z }, a{ a }, type{ -1 }, sectorId{ -1 }, textureName{ "" } {}

		Angle a;
		int thingId;
		int sectorId;
		int type;
		float distance;
		std::string textureName;

		bool operator < (const Thing& str) const
		{
			return (distance > str.distance);
		}
	};

	struct Line
	{
		constexpr Line(Vertex s, Vertex e) : s{ s }, e{ e } {}

		Vertex s;
		Vertex e;
	};

	struct Sector
	{
		Sector() : sectorId{ -1 } {}

		Sector(int sectorId, const MapStore::Sector& s) :
			sectorId{ sectorId },
			floorHeight{ static_cast<float>(s.floorHeight) },
			ceilingHeight{ static_cast<float>(s.ceilingHeight) },
			ceilingTexture{ Helpers::MakeString(s.ceilingTexture) },
			floorTexture{ Helpers::MakeString(s.floorTexture) },
			lightLevel{ s.lightLevel / 255.0f },
			isSky{ strcmp(s.ceilingTexture, "F_SKY1") == 0 } {}

		int sectorId;
		std::string ceilingTexture;
		std::string floorTexture;
		float floorHeight;
		float ceilingHeight;
		float lightLevel;
		bool isSky;
	};

	struct Side
	{
		Side(Sector sector, const std::string lowerTexture, const std::string middleTexture, const std::string upperTexture, int xOffset, int yOffset) :
			sector{ sector }, lowerTexture{ lowerTexture }, middleTexture{ middleTexture }, upperTexture{ upperTexture }, xOffset{ xOffset }, yOffset{ yOffset }
		{
		}

		Sector sector;
		int xOffset;
		int yOffset;
		std::string lowerTexture;
		std::string upperTexture;
		std::string middleTexture;
	};

	struct Segment : Line
	{
		Segment(Vertex s, Vertex e, bool isSolid, Side frontSide, Side backSide, int xOffset, bool lowerUnpegged, bool upperUnpegged) :
			Line{ s, e }, isSolid{ isSolid }, frontSide{ frontSide }, backSide{ backSide }, xOffset{ xOffset },
			lowerUnpegged{ lowerUnpegged }, upperUnpegged{ upperUnpegged },
			length{ sqrt((e.x - s.x) * (e.x - s.x) + (e.y - s.y) * (e.y - s.y)) } {}

		bool isSolid;
		Side frontSide;
		Side backSide;
		int xOffset;
		bool lowerUnpegged;
		bool upperUnpegged;
		float length;

		bool isHorizontal = s.y == e.y;
		bool isVertical = s.x == e.x;
	};

	struct Vector
	{
		constexpr Vector(Angle a, float d) : a{ a }, d{ d } {}
		constexpr Vector() : Vector{ 0, 0 } {}

		Angle a;
		float d;
	};
}