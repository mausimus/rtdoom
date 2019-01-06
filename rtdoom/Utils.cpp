#include "pch.h"
#include "Utils.h"

namespace rtdoom
{
	std::string Utils::MakeString(const char data[8])
	{
		char fullName[9];
		fullName[8] = 0;
		memcpy(fullName, data, 8);
		for (int i = 0; i < 8; i++)
		{
			fullName[i] = toupper(fullName[i]);
		}
		return std::string(fullName);
	}

	int Utils::Clip(int v, int max)
	{
		while (v < 0)
		{
			v += max;
		}
		v %= max;
		return v;
	}

	Utils::Utils()
	{
	}

	Utils::~Utils()
	{
	}
}