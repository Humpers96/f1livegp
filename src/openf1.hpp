#pragma once
#include <string>

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

enum class param
{
    DATE,
    DRIVERNUMBER,
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

std::string param_to_string(const param &p)
{
    switch (p)
    {
    case param::DATE:
        return "date";
        break;
    case param::DRIVERNUMBER:
        return "driver_number";
        break;
    default:
        break;
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

std::string build_request_string(const endpoint& ep, std::string session_key = "latest", std::string post_date = "")
{
	std::string ret;

	if (ep == endpoint::MEETINGS)
		ret = openf1_query_base_url(ep) + "meeting_key=" + session_key;
	else
		ret = openf1_query_base_url(ep) + "session_key=" + session_key;

	if (!post_date.empty())
		ret += '&' + "date" + right_angle_uri() + post_date;

	return ret;
}
} // namespace openf1