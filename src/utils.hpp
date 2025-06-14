#pragma once
#include <chrono>
#include <iomanip>
#include <sstream>

using namespace std::chrono;
namespace strutils
{	
void print_json(const nlohmann::json &js)
{
    std::cout << std::setw(4) << js << std::endl;
}

milliseconds time_point_to_ms(const system_clock::time_point &tp)
{
    return duration_cast<milliseconds>(tp.time_since_epoch());
}

time_point<system_clock> ms_to_time_point(const milliseconds &ms)
{
    return time_point<system_clock>(ms);
}

std::string time_point_to_ISO8601(const system_clock::time_point &tp)
{
    std::time_t time = system_clock::to_time_t(tp);
    milliseconds ms = duration_cast<milliseconds>(tp.time_since_epoch()) % 1000;

    std::stringstream ss;
    ss << std::setfill('0') << std::setw(3);
    ss << std::put_time(std::gmtime(&time), "%Y-%m-%dT%H:%M:%S.") << ms.count();

    return ss.str();
}

std::string ms_to_ISO8601(const milliseconds& ms)
{
    return time_point_to_ISO8601(ms_to_time_point(ms));
}

std::string time_now_as_ISO8601()
{
    return time_point_to_ISO8601(system_clock::now());
}

// milliseconds to 00.000
std::string ms_to_short_string(milliseconds ms, int remove_minutes = 0)
{
	int seconds = ms.count() / 1000;
	int milliseconds = ms.count() % 1000;
	
	std::ostringstream os;
	os << std::setw(2) << std::setfill('0') << seconds - (remove_minutes * 60) << '.' << std::setw(3) << milliseconds;
	
	return os.str();
}

// milliseconds to 0:00.000
std::string ms_to_long_string(milliseconds ms)
{
	int minutes = ms.count() / 60000;
	std::ostringstream os;

	if (minutes > 0)
		os << std::setw(1) << minutes << ':';

	os << ms_to_short_string(ms, minutes);
	
	return os.str();
}
} // namespace strutils

namespace curlutils
{
size_t write_cb(void *contents, size_t size_in, size_t nmemb_in, void *user_data = nullptr)
{
    size_t size = size_in * nmemb_in;
    std::string *str_ptr = static_cast<std::string *>(user_data);

    if (!str_ptr)
        return 0;

    str_ptr->append(static_cast<char *>(contents), size);

    return size;
}
} // namespace curlutils