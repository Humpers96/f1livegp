#pragma once
#include <chrono>
#include <nlohmann/json.hpp>

using namespace std::chrono;
using namespace nlohmann;

enum class SECTOR
{
    FIRST,
    SECOND,
    THIRD
};

enum class PIT_OUT
{
    PIT,
    OUT
};

enum class TYRE
{
    SOFT,
    MEDIUM,
    HARD
};

class lap
{
  public:
    milliseconds sector_1;
    milliseconds sector_2;
    milliseconds sector_3;
    milliseconds total;

    // stringvec to_stringvec();
};

class times
{
  public:

    milliseconds interval;
    milliseconds gap;

    lap latest_lap;
    lap last_lap;
    lap fastest_lap;
};

class session
{
  public:
    int position;
    int lap;

    times timings;

    TYRE tyre;
};

class driver
{
  public:
    int no;
    std::string name;
	std::string team;
	int team_colour_hex;
    session sesh;
};

class weather
{
	public:
	float air_temp;
	float track_temp;
	bool raining;
};

class track
{
  public:
    std::string broadcast_name;
    std::string country;
    std::string location;
    std::string track_name;

    std::vector<std::string> to_strings() const
    {
        std::vector<std::string> ret;

        ret.push_back(country);
        if (location != country)
            ret.push_back(location);
        if (track_name != country && track_name != location)
            ret.push_back(track_name);

        return ret;
    }
};

namespace 
{
std::vector<std::string> team_names = {
    "Red Bull Racing",
    "McLaren",
    "Kick Sauber",
    "Racing Bulls",
    "Alpine",
    "Mercedes",
    "Aston Martin",
    "Ferrari",
    "Williams",
    "Haas F1 Team"
};

const char* shorten_team_name(std::string team_name)
{
	if (team_name == "Red Bull Racing")
	{
		return "RBR";
    } 
	else if (team_name == "McLaren")
	{
		return "MCL";
    } 
	else if (team_name == "Kick Sauber")
	{
		return "SAU";
    } 
	else if (team_name == "Racing Bulls")
	{
		return "VCA";
    } 
	else if (team_name == "Alpine")
	{
		return "ALP";
    } 
	else if (team_name == "Mercedes")
	{
		return "MER";
    } 
	else if (team_name == "Aston Martin")
	{
		return "AST";
    } 
	else if (team_name == "Ferrari")
	{
		return "FER";
    } 
	else if (team_name == "Williams")
	{
		return "WIL";
    } 
	else if (team_name == "Haas F1 Team")
	{
		return "HAA";
    }
}
}

void from_json(const json &js, driver &dr)
{
    js.at("name_acronym").get_to(dr.name);
    js.at("driver_number").get_to(dr.no);
	std::string col = js.at("team_colour");

	dr.team_colour_hex = std::stoi((std::string)js.at("team_colour"), 0, 16);
	dr.team = shorten_team_name(js.at("team_name"));
}

void from_json(const json &js, track &tr)
{
    js.at("meeting_official_name").get_to(tr.broadcast_name);
    js.at("country_name").get_to(tr.country);
    js.at("location").get_to(tr.location);
    js.at("circuit_short_name").get_to(tr.track_name);
}

void from_json(const json& js, weather &we)
{
	js.at("air_temperature").get_to(we.air_temp);
	js.at("track_temperature").get_to(we.track_temp);
	we.raining = (int)js.at("rainfall");
}