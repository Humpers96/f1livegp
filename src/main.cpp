#include <curl/curl.h>
#include <ftxui/dom/elements.hpp>
#include <ftxui/dom/node.hpp>
#include <ftxui/dom/table.hpp>
#include <ftxui/screen/color.hpp>
#include <ftxui/screen/screen.hpp>
#include <nlohmann/json.hpp>

#include <chrono>
#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <sstream>

#include "f1-helpers.h"
#include "openf1.hpp"
#include "utils.hpp"

using namespace nlohmann;
using namespace ftxui;

/**
 * general execution order:
 * - req latest track details
 *   - req weather?
 * - req positions
 *   - first 20 entries are starting positions
 *   - anything following is a position *change*
 *   - so to get positions, get the first 20 and the apply changes 
 * - req tyres & age (possibly needs to be done after race start?)
 *
 * main req loop:
 * - timestamp = green flag + 0.25 or 0.5 seconds 
 *   - this has to be way larger, 5-10 seconds
 * 
 * - req position post timestamp
 * - req intervals (interval, gap to leader) post timestamp
 * // these two first
 * 
 * - req laps (sector 1/2/3, duration, lap number, is out lap) post timestamp
 * - req stints (compound, tyre age, lap start) post timestamp
 * - req race control (unsure what this will look like) post timestamp
 * - push requests into buffer(s) to process
 *   - reverse iterate through json and update drivers with req data via driver number
 *     - find first date post timestamp and parse everything after 
 *     - update timestamp to larger of latest date found or timestamp plus 5-10 seconds
 * // need some frame over the api to create/queue request and parse results based on
 * // complete driver sets, discrete sets & stallable and non-stallable requests
 * 	- drip feed results to the ui
 * 
 * - at some point requests should be separated out into failable & non-failable
 *   - failed requests should be skipped and data should be updated next loop
 * 	 - if a request doesn't return a full driver list it should be skipped, possibly skipping a full loop
 * 
 * req notes:
 * - do NOT forget about URI encoding ffs
 *   - should be simple enough to do manually
 *   - if not, should consider boost
 *     - would really rather not consider boost
 *
 *
 * main widgets:
 * - headers
 *   - gp title
 *   - country -- location -- track (check for matches & hide duplicates)
 *   - weather somewhere here?
 * - table
 * - race control ticker
 */



// -- request functions
// - curl & json
void send_and_write_curl_req(CURL *curl, const std::string &uri, void *buff)
{
    curl_easy_setopt(curl, CURLOPT_URL, uri.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, buff);
    curl_easy_perform(curl);
};

json try_curl_to_json(CURL *curl, const std::string &uri)
{
    try
    {
        std::string response;
        send_and_write_curl_req(curl, uri, &response);
        return json::parse(response);
    }
    catch (const json::exception &e)
    {
        std::cerr << "unable to parse response to json (" << uri << ")\n" << e.what() << '\n';
        return json{};
    }

    return json{};
}

json wait_for_response(CURL* curl, const std::string& url/*, milliseconds timeout = 1000*/)
{
	json ret = try_curl_to_json(curl, url);

	while (ret.empty())
	{
		std::this_thread::sleep_for(1s);
		ret = try_curl_to_json(curl, url);
	}

	return ret;
}

// - requests to f1 objects
std::unique_ptr<track> get_latest_track_info(CURL* curl)
{
	//const std::string meeting_url = "https://api.openf1.org/v1/meetings?meeting_key=latest";
	const std::string meeting_url = openf1::build_request_string(openf1::endpoint::MEETINGS);
	json track_js = wait_for_response(curl, meeting_url);

	return std::make_unique<track>(*track_js.begin());
}

std::unique_ptr<weather> get_latest_weather(CURL* curl)
{
	//const std::string weather_url = "https://api.openf1.org/v1/weather?session_key=latest";
	const std::string weather_url = openf1::build_request_string(openf1::endpoint::WEATHER);
	json weather_js = wait_for_response(curl, weather_url);

	return std::make_unique<weather>(weather_js.back());
}

std::unique_ptr<std::vector<driver>> get_latest_drivers(CURL* curl)
{
	//const std::string drivers_url = "https://api.openf1.org/v1/drivers?session_key=latest";
	const std::string drivers_url = openf1::build_request_string(openf1::endpoint::DRIVERS);
	json drivers_js = wait_for_response(curl, drivers_url);

	std::vector<driver> drivers_vec;
	for (const auto& js : drivers_js)
	{
		drivers_vec.emplace_back(js);
	}

	return std::make_unique<std::vector<driver>>(drivers_vec);
}

std::vector<driver> get_latest_drivers_vec(CURL* curl)
{
	// const std::string drivers_url = "https://api.openf1.org/v1/drivers?session_key=latest";
	const std::string drivers_url = openf1::build_request_string(openf1::endpoint::DRIVERS);
	json drivers_js = wait_for_response(curl, drivers_url);

	std::vector<driver> drivers_vec;
	for (const auto& js : drivers_js)
	{
		drivers_vec.emplace_back(js);
	}

	return drivers_vec;
}

// -- ftxui functions
void draw_elements_to_term(const std::vector<Element>& ui)
{
	auto document = vbox(ui);

    auto screen = Screen::Create(Dimension::Fit(document, /*extend_beyond_screen=*/true));
    Render(screen, document);
    screen.Print();
    std::cout << std::endl;
}

// - table
void decorate_table(Table &table)
{
    table.SelectColumn(0).DecorateCells(align_right);                   // align driver numbers right
    table.SelectColumns(1, -1).DecorateCells(hcenter);                  // centre body cells
    table.SelectColumn(1).DecorateCells(size(WIDTH, EQUAL, 11));         // NAME width
    table.SelectColumns(2, 3).DecorateCells(size(WIDTH, EQUAL, 11));    // INTERVAL/LEAD width
    table.SelectColumns(7, 8).DecorateCells(size(WIDTH, EQUAL, 10));    // LAST/BEST LAP width
    table.SelectColumn(9).DecorateCells(size(WIDTH, EQUAL, 9));         // PIT/OUT width

    table.SelectRow(0).SeparatorVertical(LIGHT);
    table.SelectRow(0).Border(HEAVY);
    table.SelectColumn(0).Border(HEAVY);
    table.SelectAll().Border(HEAVY);

    auto first_column = table.SelectRectangle(0, 0, 1, -1);
    auto middle_columns = table.SelectRectangle(1, 10, 1, -1);
    auto last_column = table.SelectRectangle(11, 11, 1, -1);

    first_column.DecorateCellsAlternateRow(bgcolor(Color::Grey23), 2, 0);
    first_column.DecorateCellsAlternateRow(bgcolor(Color::Grey30), 2, 1);
    middle_columns.DecorateAlternateRow(bgcolor(Color::Grey23), 2, 0);
    middle_columns.DecorateAlternateRow(bgcolor(Color::Grey30), 2, 1);
    last_column.DecorateCellsAlternateRow(bgcolor(Color::Grey23), 2, 0);
    last_column.DecorateCellsAlternateRow(bgcolor(Color::Grey30), 2, 1);
}

Table create_drivers_table(const std::vector<driver>& drivers)
{
	std::vector<std::vector<std::string>> table_rows{
        {"POS", "DRIVER", "INTERVAL", "TO LEAD", "SECTOR 1", "SECTOR 2", "SECTOR 3", "LAST LAP", "BEST LAP", "PIT/OUT", "TYRES", "TYRE AGE"},
    };

	milliseconds thirty_seconds(32343);
	milliseconds ninety_seconds(92345);

    for (int i = 0; i < 20; i++)
    {
		const auto& driver = drivers[i];
        table_rows.push_back(
			{
				std::to_string(i + 1),
				driver.name + " [" + driver.team + ']',
				"+" + strutils::ms_to_long_string(ninety_seconds),
				"+" + strutils::ms_to_long_string(ninety_seconds),
				strutils::ms_to_long_string(thirty_seconds),
				strutils::ms_to_long_string(thirty_seconds),
				strutils::ms_to_long_string(thirty_seconds),
				strutils::ms_to_long_string(ninety_seconds),
				strutils::ms_to_long_string(ninety_seconds),
				"IN PITS", 
				"(S)", 
				"00 LAPS"
			}
		);
	}

	auto table = Table(table_rows);
	decorate_table(table);

	return table;
}

// - headers
Element create_location_header(const track& location, const weather& conditions)
{
	auto location_header_strings = location.to_strings();
    std::vector<Element> location_hbox;

    for (auto it = location_header_strings.begin(); it != location_header_strings.end(); ++it)
    {
        location_hbox.push_back(text(*it));
        if (it != location_header_strings.end() - 1)
            location_hbox.push_back(text(" -- "));
    }

	std::ostringstream oss;

	oss << std::setw(4) << conditions.air_temp;
	std::string air_temp{ "Air Temp: " + oss.str() + ' ' };

	oss << conditions.track_temp;
	std::string track_temp{ "Track temp: " + oss.str() + ' ' };
	
	auto upper = hbox(text(location.broadcast_name), filler(), text(air_temp)) | inverted;
	auto lower = hbox(hbox(location_hbox), filler(), text(track_temp));

    return vbox(upper, lower);
}

std::vector<json> parse_to_latest_set(const json& response_json)
{
	std::vector<json> ret;

	// std::pair<int, bool> driver_numbers[] = {
	// 	{ 1,  false }, { 4,  false }, { 5,  false }, { 6,  false }, { 7,  false },
	// 	{ 10, false }, { 12, false }, { 14, false }, { 16, false }, { 18, false },
	// 	{ 22, false }, { 23, false }, { 27, false }, { 30, false }, { 31, false },
	// 	{ 44, false }, { 55, false }, { 63, false }, { 81, false }, { 87, false }
	// };

	auto last_json_object = response_json.rbegin();
	std::string latest_response_date;

	try
	{
		latest_response_date = last_json_object->at("date");
	}
	catch(const std::exception& e)
	{	
		std::cerr << "unable to parse date." << std::endl;
		std::cerr << e.what() << '\n';
		return std::vector<json>{};
	}

	auto json_it = response_json.rbegin();
	auto set_start_it = json_it;
	int current_set = 0;

	for (json_it; json_it != response_json.rend(); ++json_it)
	{
		std::string json_date{};

		try
		{
			json_date = json_it->at("date");
		}
		catch(const std::exception& e)
		{
			std::cerr << "unable to parse date" << std::endl;
			return std::vector<json>{};
		}

		if (json_date == latest_response_date)
		{
			current_set++;
		}
		else
		{
			current_set = 0;
			set_start_it = json_it;
			continue;
		}

		if (current_set == 20)
		{
			break;
		}
	}

	if (current_set != 20)
		return std::vector<json>{};

	return std::vector<json>(json_it, std::next(json_it, -20));
}

int main()
{
	// curl boot
    CURL *curl = curl_easy_init();
    if (!curl)
        return -1;

	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curlutils::write_cb); // curl write callback

	// -- get latest track, weather & driver info from openf1
	auto location = get_latest_track_info(curl);
	auto conditions = get_latest_weather(curl);

	// create map of drivers numbers to drivers 
	std::map<int, driver> driver_map;
	{
		auto driver_vec = get_latest_drivers_vec(curl);
		for (auto& driver : driver_vec)
		{
			driver_map[driver.no] = std::move(driver);
		}
	}

	// read positions since beginning of latest session 
	const std::string position_url = "https://api.openf1.org/v1/position?session_key=latest";
	json pos_js = wait_for_response(curl, position_url);

	// quick and dirty update of the postions of drivers in the driver map
	auto update_pos = [](std::map<int, driver>& driver_map, const json& positions)
	{	
		int total_json = positions.size();
		int processed = 0;

		for (const auto& position : positions)
		{
			try
			{
				int driver_no, new_position;

				driver_no = position.at("driver_number");
				new_position = position.at("position");

				driver_map[driver_no].sesh.position = new_position;
				processed++;
			}
			catch(const std::exception& e)
			{
				std::cerr << "error parsing positions\n" << processed << '/' << total_json << " positions updated." << std::endl;
				std::cerr << e.what() << '\n';
			}
		}
	};
	update_pos(driver_map, pos_js);

	json intervals = wait_for_response(curl, openf1::build_request_string(openf1::endpoint::INTERVALS));

	// std::vector<json> most_recent_driver_intervals;

	// std::pair<int, bool> driver_numbers[] = {
	// 	{1, false}, {4, false}, {5, false}, {6, false}, {7, false},
	// 	{10, false}, {12, false}, {14, false}, {16, false}, {18, false},
	// 	{22, false}, {23, false}, {27, false}, {30, false}, {31, false},
	// 	{44, false}, {55, false}, {63, false}, {81, false}, {87, false}
	// };

	// for (const auto& interval : intervals)
	// {

	// }

	

	// represents the driver objects that will be displayed in the table
	std::vector<driver> driver_display_vec;
	for (auto& driver : driver_map)
	{
		driver_display_vec.push_back(driver.second);
	}

	// sort display drivers by their positions
	std::sort(driver_display_vec.begin(), driver_display_vec.end(), [](const driver& dr, const driver& other)
	{
		return dr.sesh.position < other.sesh.position;
	});

	// -- create main ftxui widgets
	Element header = create_location_header(*location, *conditions);
	Table table = create_drivers_table(driver_display_vec);

	// -- draw ui to terminal
	draw_elements_to_term({ header, table.Render() });

	// -- curl cleanup
    curl_easy_cleanup(curl);

    return 0;
}