#pragma once
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include <list>

class dmkJson
{
public:
	dmkJson();
	bool Parse(const std::string& str);

	const std::string Extract(const std::string& key);
	bool ExtractIfExists(const std::string& key, std::string& val);
	bool ExtractIfExists(const std::string& key, int& val);
	dmkJson ExtractObject(const std::string& key);
	float ExtractFloat(const std::string& key);
	int ExtractInt(const std::string& key);
	bool ExtractList(const std::string& key, std::list<std::string>& list);

	bool Add(const std::string& key, const std::string& val, bool replaceExisting = true);
	bool Add(const std::string& key, const int val, bool replaceExisting = true);
	bool Add(const std::string& key, const float val, bool replaceExisting = true);
	bool Add(const std::string& key, const dmkJson& obj, bool replaceExisting = true);
	bool Remove(const std::string& key);
	const std::string toString();

private:
	rapidjson::Document mDoc;
};