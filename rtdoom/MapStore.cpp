#include "pch.h"
#include "MapStore.h"
#include "Utils.h"

using namespace std::literals::string_literals;

namespace rtdoom
{
	MapStore::MapStore()
	{
	}

	void MapStore::Load(const std::string& mapFolder)
	{
		m_vertexes = Utils::LoadEntities<Vertex>(mapFolder + "\\vertexes.lmp"s);
		m_lineDefs = Utils::LoadEntities<LineDef>(mapFolder + "\\linedefs.lmp"s);
		m_sideDefs = Utils::LoadEntities<SideDef>(mapFolder + "\\sidedefs.lmp"s);
		m_sectors = Utils::LoadEntities<Sector>(mapFolder + "\\sectors.lmp"s);
		m_subSectors = Utils::LoadEntities<SubSector>(mapFolder + "\\ssectors.lmp"s);
		m_nodes = Utils::LoadEntities<Node>(mapFolder + "\\nodes.lmp"s);
		m_segments = Utils::LoadEntities<Segment>(mapFolder + "\\segs.lmp"s);
		m_things = Utils::LoadEntities<Thing>(mapFolder + "\\things.lmp"s);
	}

	void MapStore::Load(const std::map<std::string, std::vector<char>>& mapLumps)
	{
		m_vertexes = Utils::LoadEntities<Vertex>(mapLumps.at("VERTEXES"s));
		m_lineDefs = Utils::LoadEntities<LineDef>(mapLumps.at("LINEDEFS"s));
		m_sideDefs = Utils::LoadEntities<SideDef>(mapLumps.at("SIDEDEFS"s));
		m_sectors = Utils::LoadEntities<Sector>(mapLumps.at("SECTORS"s));
		m_subSectors = Utils::LoadEntities<SubSector>(mapLumps.at("SSECTORS"s));
		m_nodes = Utils::LoadEntities<Node>(mapLumps.at("NODES"s));
		m_segments = Utils::LoadEntities<Segment>(mapLumps.at("SEGS"s));
		m_things = Utils::LoadEntities<Thing>(mapLumps.at("THINGS"s));
	}

	void MapStore::GetStartingPosition(signed short &x, signed short &y, unsigned short &a) const
	{
		auto foundPlayer = find_if(m_things.cbegin(), m_things.cend(), [](const Thing& t) { return t.type == 1; });
		if (foundPlayer == m_things.cend())
		{
			throw std::runtime_error("No player starting location on map");
		}
		const Thing& playerThing = *foundPlayer;
		x = playerThing.x;
		y = playerThing.y;
		a = playerThing.a;
	}

	MapStore::~MapStore()
	{
	}
}