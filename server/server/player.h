#pragma once
#include <memory>
#include "json.h"

class Stats;
class Player
{
public:
	Player(const int id, const std::string& name);
	void SetStats(const std::shared_ptr<Stats>& stats);
	void SetPos(const int x, const int y);
	const std::string& GetName() const { return mName; }
	const dmkJson getJson() const;

private:
	std::shared_ptr<Stats> mStats;
	int mPosX;
	int mPosY;
	int mId;
	std::string mName;
};