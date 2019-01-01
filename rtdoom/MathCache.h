#pragma once

#include "rtdoom.h"

namespace rtdoom
{
	// singleton with precalculated trigonometric functions
	class MathCache
	{
	protected:
		MathCache();

		constexpr static bool s_useCache = true;
		constexpr static size_t s_precision = 16384;

		int sign(float v) const;
		float FastArcTan(float x) const;
		float _atan2f(float y, float x) const;

		std::array<float, s_precision + 1> _tan;
		std::array<float, s_precision + 1> _atan;
		std::array<float, s_precision + 1> _cos;

	public:
		static const MathCache& instance();

		float ArcTan(float x) const;
		float ArcTan(float dy, float dx) const;
		float Cos(float x) const;
		float Tan(float x) const;
	};
}
