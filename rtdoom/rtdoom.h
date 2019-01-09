#pragma once

using Angle = float;

namespace rtdoom
{
	// output display size (scaled framebuffer)
	constexpr int s_displayX = 960;
	constexpr int s_displayY = 500;
	constexpr float s_multisamplingLevel = 1.0f;
	constexpr float s_minDistance = 1.0f;
	constexpr float s_lightnessFactor = 2500.0f;

	constexpr Angle PI = 3.14159265359f;
	constexpr Angle PI2 = PI / 2.0f;
	constexpr Angle PI4 = PI / 4.0f;
}
