#include "pch.h"
#include "Helpers.h"

namespace rtdoom
{
std::string Helpers::MakeString(const char data[8])
{
    char fullName[9];
    fullName[8] = 0;
    memcpy(fullName, data, 8);
    for(int i = 0; i < 8; i++)
    {
        fullName[i] = static_cast<char>(toupper(fullName[i]));
    }
    return std::string(fullName);
}

int Helpers::Clip(int v, int max)
{
    while(v < 0)
    {
        v += max;
    }
    return v % max;
}

float Helpers::Clip(float v, float max)
{
    if(isfinite(v))
    {
        while(v < 0)
        {
            v += max;
        }
        while(v > max)
        {
            v -= max;
        }
    }
    return v;
}

Helpers::Helpers() {}

Helpers::~Helpers() {}
} // namespace rtdoom
