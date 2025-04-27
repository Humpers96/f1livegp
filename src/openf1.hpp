#pragma once
#include <string>

namespace openf1
{
    const std::string openf1 = "https://api.openf1.org/v1/";

    enum class endpoint
    {
        DRIVERS,
        POSITION,
        LAPS,
        INTERVALS,
        STINTS,
        RACECONTROL
    };

    enum class param
    {
        DATE,
        DRIVERNUMBER,
    };

    std::string endpoint_to_string(const endpoint& ep)
    {
        switch (ep)
        {
            case endpoint::DRIVERS:
                return "drivers";
            break;
            case endpoint::POSITION:
                return "position";
            break;
            case endpoint::LAPS:
                return "laps";
            break;
            case endpoint::INTERVALS:
                return "intervals";
            break;
            case endpoint::STINTS:
                return "stints";
            break;
            case endpoint::RACECONTROL:
                return "race_control";
            break;
            default:
            break;
        }
    }

    std::string param_to_string(const param& p)
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

    std::string openf1_string(const endpoint& ep)
    {
        return openf1 + endpoint_to_string(ep);
    }

    std::string left_angle_uri()
    {
        return "%3C";
    }

    std::string right_angle_uri()
    {
        return "%3E";
    }

    std::string openf1_query_base_url(const endpoint& ep)
    {
        return openf1_string(ep) + '?'; 
    }
}