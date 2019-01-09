#pragma once

namespace rtdoom
{
	class Helpers
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

		template<typename T> static std::vector<T> LoadEntities(const std::vector<char>& data)
		{
			size_t dataIndex = 0;
			std::vector<T> items;
			T v;
			while (dataIndex < data.size())
			{
				memcpy(&v, data.data() + dataIndex, sizeof(T));
				items.push_back(v);
				dataIndex += sizeof(T);
			}
			return items;
		}

		template<typename T> static void LoadEntity(const std::vector<char>& data, T * entity)
		{
			memcpy(entity, data.data(), sizeof(T));
		}

		static std::string MakeString(const char data[8]);

		static int Clip(int v, int max);

		Helpers();
		~Helpers();
	};
}
