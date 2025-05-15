#pragma once
#include <string>
#include <chrono>

#include "utils.hpp"

using namespace std::chrono;
namespace openf1
{
const std::string openf1 = "https://api.openf1.org/v1/";

enum class endpoint
{
    DRIVERS,
    INTERVALS,
    LAPS,
	MEETINGS,
    POSITION,
    RACECONTROL,
    STINTS,
	WEATHER
};

std::string endpoint_to_string(const endpoint &ep)
{
    switch (ep)
    {
    case endpoint::DRIVERS:
        return "drivers";
	case endpoint::INTERVALS:
		return "intervals";
	case endpoint::LAPS:
		return "laps";
	case endpoint::MEETINGS:
	return "meetings";
    case endpoint::POSITION:
        return "position";
    case endpoint::STINTS:
        return "stints";
    case endpoint::RACECONTROL:
        return "race_control";
	case endpoint::WEATHER:
		return "weather";
    default:
	return "";
    }
}

std::string openf1_string(const endpoint &ep)
{
    return openf1 + endpoint_to_string(ep);
}

std::string left_angle_uri() { return "%3C"; }

std::string right_angle_uri() { return "%3E"; }

std::string openf1_query_base_url(const endpoint &ep)
{
    return openf1_string(ep) + '?';
}

const std::string build_req_string(const endpoint& ep, const std::string& session_key = "latest", 
    milliseconds date_from = std::chrono::milliseconds(0), 
    milliseconds date_to = std::chrono::milliseconds(0))
{
    std::string ret;

    if (ep != endpoint::MEETINGS)
        ret = openf1_query_base_url(ep) + "session_key=" + session_key;
	else
        ret = openf1_query_base_url(ep) + "meeting_key=" + session_key;

    if (date_from.count() > 0)
    {
        ret += '&' + "date" + right_angle_uri() + strutils::ms_to_ISO8601(date_from);
    }

    if (date_to.count() > 0)
    {
        ret += '&' + "date" + left_angle_uri() + strutils::ms_to_ISO8601(date_to);
    }

	return ret;
}
} // namespace openf1