#pragma once

using Angle = float;

namespace rtdoom
{
// output display size (scaled framebuffer)
constexpr int   s_displayX           = 1280;
constexpr int   s_displayY           = 800;
constexpr float s_multisamplingLevel = 0.5f;
constexpr float s_minDistance        = 1.0f;
constexpr float s_minScale           = 0.025f;
constexpr float s_lightnessFactor    = 1500.0f;

constexpr Angle PI  = 3.14159265359f;
constexpr Angle PI2 = PI / 2.0f;
constexpr Angle PI4 = PI / 4.0f;
} // namespace rtdoom
