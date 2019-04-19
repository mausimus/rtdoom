#include "pch.h"
#include "MathCache.h"

namespace rtdoom
{
MathCache::MathCache()
{
    for(size_t i = 0; i <= s_precision; i++)
    {
        _tan[i]  = tanf(i * PI4 / s_precision);
        _atan[i] = atanf(i * PI4 / s_precision);
        _cos[i]  = cosf(i * 2.0f * PI / s_precision);
        _sin[i]  = sinf(i * 2.0f * PI / s_precision);
    }
}

int MathCache::sign(float v) const
{
    return v < 0 ? 1 : 0;
}

float MathCache::FastArcTan(float x) const
{
    return PI4 * x - x * (x - 1) * (0.2447f + 0.0663f * x);
}

float MathCache::_atan2f(float y, float x) const
{
    if(x == 1.0f)
    {
        return atanf(y);
    }

    uint32_t m = 2 * sign(x) + sign(y);
    if(y == 0)
    {
        switch(m)
        {
        case 0:
        case 1:
            return y; /* atan(+-0,+anything)=+-0 */
        case 2:
            return PI; /* atan(+0,-anything) = pi */
        case 3:
            return -PI; /* atan(-0,-anything) =-pi */
        }
    }

    if(x == 0)
    {
        return m & 1 ? -PI2 : PI2;
    }

    float z;
    auto  v = fabsf(y / x);
    if(v < 1)
    {
        z = FastArcTan(v);
    }
    else
    {
        z = atanf(v);
    }
    switch(m)
    {
    case 0:
        return z; /* atan(+,+) */
    case 1:
        return -z; /* atan(-,+) */
    case 2:
        return PI - z; /* atan(+,-) */
    default:
        return z - PI; /* atan(-,-) */
    }
}

float MathCache::ArcTan(float x) const
{
    if constexpr(!s_useCache)
    {
        return atanf(x);
    }
    auto v = static_cast<size_t>(fabsf(x) / PI4 * s_precision);
    auto s = x < 0 ? -1 : 1;
    return _atan[v % (s_precision + 1)] * s;
}

float MathCache::ArcTan(float dy, float dx) const
{
    if constexpr(!s_useCache)
    {
        return atan2f(dy, dx);
    }
    return _atan2f(dy, dx);
}

float MathCache::Cos(float x) const
{
    if constexpr(!s_useCache)
    {
        return cosf(x);
    }
    while(x < 2 * PI)
    {
        x += 2 * PI;
    }
    auto v = static_cast<size_t>(x / (2 * PI) * s_precision);
    return _cos[v % (s_precision + 1)];
}

float MathCache::Sin(float x) const
{
    if constexpr(!s_useCache)
    {
        return sinf(x);
    }
    while(x < 2 * PI)
    {
        x += 2 * PI;
    }
    auto v = static_cast<size_t>(x / (2 * PI) * s_precision);
    return _sin[v % (s_precision + 1)];
}

float MathCache::Tan(float x) const
{
    if constexpr(!s_useCache)
    {
        return tanf(x);
    }
    auto v = static_cast<size_t>(fabsf(x) / PI4 * s_precision);
    auto s = x < 0 ? -1 : 1;
    return _tan[v % (s_precision + 1)] * s;
}

const MathCache& MathCache::instance()
{
    const static MathCache cache;
    return cache;
}
} // namespace rtdoom
