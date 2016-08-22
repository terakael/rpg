#include "stats.h"

Stats::Stats(const int playerId)
	: mPlayerId(playerId)
{

}

void Stats::Set(const int statId, const std::string& name, const int exp)
{
	mStats[statId] = std::make_shared<Stat>(statId, name, exp);
}

dmkJson Stats::getJson()
{
	dmkJson json;

	for (const auto& iter : mStats)
		json.Add(iter.second->name, iter.second->exp);

	return json;
}