#include "stats.h"
#include "player.h"

Player::Player(const int id, const std::string& name)
	: mId(id)
	, mName(name)
	, mPosX(0)
	, mPosY(0)
{
}

void Player::SetStats(const std::shared_ptr<Stats>& stats)
{
	mStats = stats;
}

void Player::SetPos(const int x, const int y)
{
	mPosX = x;
	mPosY = y;
}

const dmkJson Player::getJson() const
{
	dmkJson json;

	json.Add("id", mId);
	json.Add("name", mName);
	json.Add("stats", mStats->getJson());

	return json;
}