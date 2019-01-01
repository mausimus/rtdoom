#pragma once

namespace rtdoom
{
	class Utils
	{
	public:
		template<typename T> static std::vector<T> LoadEntities(const std::string& fileName)
		{
			std::ifstream infile(fileName, std::ios::binary);
			if (!infile.good())
			{
				throw std::runtime_error("Unable to open file " + fileName);
			}
			std::vector<T> items;
			T v;
			while (infile.read((char *)&v, sizeof(T)))
			{
				items.push_back(v);
			};
			return items;
		}

		Utils();
		~Utils();
	};
}
