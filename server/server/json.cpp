#include "json.h"

using namespace rapidjson;

dmkJson::dmkJson()
{
	mDoc.SetObject();
}

bool dmkJson::Parse(const std::string& str)
{
	return !mDoc.Parse(str.c_str()).HasParseError();
}

const std::string dmkJson::Extract(const std::string& key)
{
	std::string val;
	ExtractIfExists(key, val);
	return val;
}

dmkJson dmkJson::ExtractObject(const std::string& key)
{
	dmkJson obj;
	if (mDoc.HasMember(key))
	{
		bool success = obj.Parse(Extract(key));
		if (!success)
			obj.Add("error", "failed to parse");
	}
	return obj;
}

float dmkJson::ExtractFloat(const std::string& key)
{
	if (mDoc.HasMember(key))
		return mDoc[key].GetFloat();
	return -1;
}

int dmkJson::ExtractInt(const std::string& key)
{
	if (mDoc.HasMember(key))
		return mDoc[key].GetInt();
	return -1;
}

bool dmkJson::ExtractIfExists(const std::string& key, std::string& val)
{
	if (mDoc.HasMember(key))
	{
		switch (mDoc[key].GetType())
		{
		case kStringType:
			val = mDoc[key].GetString();
			break;
		case kFalseType:
		case kTrueType:
			val = mDoc[key].GetBool() ? "1" : "0";
			break;
		case kNullType:
			val = "null";
			break;
		case kNumberType:
			if (mDoc[key].IsFloat())
				val = std::to_string(mDoc[key].GetFloat());
			else if (mDoc[key].IsDouble())
				val = std::to_string(mDoc[key].GetDouble());
			else if (mDoc[key].IsInt())
				val = std::to_string(mDoc[key].GetInt());
			break;
		case kObjectType:
		{
			StringBuffer buffer;
			Writer<StringBuffer> writer(buffer);
			mDoc[key].Accept(writer);
			val = buffer.GetString();
			break;
		}

		default:
			break;
		}
		return true;
	}
	return false;
}

bool dmkJson::ExtractList(const std::string& key, std::list<std::string>& list)
{
	if (!mDoc.HasMember(key) || !mDoc[key].IsArray())
		return false;

	const Value& a = mDoc[key];
	for (SizeType i = 0; i < a.Size(); ++i)
		list.push_back(a[i].GetString());

	return true;
}

bool dmkJson::Add(const std::string& key, const std::string& val, bool replaceExisting)
{
	if (replaceExisting && mDoc.HasMember(key))
		Remove(key);
	auto& a = mDoc.GetAllocator();
	mDoc.AddMember(Value(key, a).Move(), Value(val, a).Move(), a);
	return true;
}

bool dmkJson::Remove(const std::string& key)
{
	return mDoc.RemoveMember(key);
}

const std::string dmkJson::toString()
{
	StringBuffer buffer;
	Writer<StringBuffer> writer(buffer);
	mDoc.Accept(writer);
	return buffer.GetString();
}