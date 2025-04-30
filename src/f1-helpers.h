#pragma once
#include <chrono>
#include <nlohmann/json.hpp>

using namespace std::chrono;

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

    session sesh;
};

class track
{
  public:
    std::string broadcast_name;
    std::string country;
    std::string location;
    std::string track_name;

    std::vector<std::string> to_strings()
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

void from_json(const nlohmann::json &js, driver &dr)
{
    js.at("name_acronym").get_to(dr.name);
    js.at("driver_number").get_to(dr.no);
}

void from_json(const nlohmann::json &js, track &tr)
{
    js.at("meeting_official_name").get_to(tr.broadcast_name);
    js.at("country_name").get_to(tr.country);
    js.at("location").get_to(tr.location);
    js.at("circuit_short_name").get_to(tr.track_name);
}