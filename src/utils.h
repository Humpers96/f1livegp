#pragma once
#include <chrono>
#include <sstream>
#include <iomanip>

using namespace std::chrono;
namespace utils
{
    milliseconds time_point_to_ms(const system_clock::time_point& tp)
    {
        return duration_cast<milliseconds>(tp.time_since_epoch());
    }

    time_point<system_clock> ms_to_time_point(const milliseconds& ms)
    {
        return time_point<system_clock>(ms);
    }

    std::string time_point_to_ISO8601(const system_clock::time_point& tp)
    {
        std::time_t time = system_clock::to_time_t(tp);
        milliseconds ms = duration_cast<milliseconds>(tp.time_since_epoch()) % 1000;

        std::stringstream ss;
        ss << std::setfill('0') << std::setw(3);
        ss << std::put_time(std::gmtime(&time), "%Y-%m-%dT%H:%M:%S.") << ms.count();

        return ss.str();
    }

    std::string time_now_as_string()
    {
        return time_point_to_ISO8601(system_clock::now());
    }
}