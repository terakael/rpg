#pragma once
#include <map>
#include <memory>
#include "json.h"

struct Stat
{
	Stat(int id, std::string name, int exp) : id(id), name(name), exp(exp) {}
	int id;
	std::string name;
	int exp;
};
typedef std::shared_ptr<Stat> Stat_ptr;

class Stats
{
public:
	Stats(const int playerId);
	void Set(const int statId, const std::string& name, const int exp);
	dmkJson getJson();
private:
	std::map<int, Stat_ptr> mStats;
	int mPlayerId;
	
};
