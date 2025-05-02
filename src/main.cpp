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

#include "f1-helpers.h"
#include "openf1.hpp"
#include "utils.hpp"

using namespace nlohmann;
using namespace ftxui;

/**
 * general execution order:
 * - req latest track details
 *   - req weather?
 * - req position
 * - req tyres & age (possibly needs to be done after race start?)
 * - poll race control for green flag
 *   - retain timestamp of green flag for first lap/sector 1 calcs
 *
 * main req loop:
 * - timestamp = green flag + 0.25 or 0.5 seconds 
 * - req position post timestamp
 * - req intervals (interval, gap to leader) post timestamp
 * - req laps (sector 1/2/3, duration, lap number, is out lap) post timestamp
 * - req stints (compound, tyre age, lap start) post timestamp
 * - req race control (unsure what this will look like) post timestamp
 * - reverse iterate through json and update drivers with req data via driver number
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

// - requests to f1 objects
std::unique_ptr<track> get_latest_track_info(CURL* curl)
{
	const std::string track_url = "https://api.openf1.org/v1/meetings?meeting_key=latest";
	json track_js = try_curl_to_json(curl, track_url);

	while (track_js.empty())
	{
		std::this_thread::sleep_for(1s);
		track_js = try_curl_to_json(curl, track_url);
	}

	return std::make_unique<track>(*track_js.begin());
}

std::unique_ptr<std::vector<driver>> get_latest_drivers(CURL* curl)
{
	const std::string drivers_url = "https://api.openf1.org/v1/drivers?session_key=latest";
	json drivers_js = try_curl_to_json(curl, drivers_url);

	while (drivers_js.empty())
	{
		std::this_thread::sleep_for(1s);
		drivers_js = try_curl_to_json(curl, drivers_url);
	}
	
	std::vector<driver> drivers_vec;
	for (const auto& js : drivers_js)
	{
		drivers_vec.emplace_back(js);
	}

	return std::make_unique<std::vector<driver>>(drivers_vec);
}

std::vector<driver> get_latest_drivers_vec(CURL* curl)
{
	const std::string drivers_url = "https://api.openf1.org/v1/drivers?session_key=latest";
	json drivers_js = try_curl_to_json(curl, drivers_url);

	while (drivers_js.empty())
	{
		std::this_thread::sleep_for(1s);
		drivers_js = try_curl_to_json(curl, drivers_url);
	}
	
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
Element create_location_header(const track& location)
{
	auto location_header_strings = location.to_strings();
    std::vector<Element> location_hbox;

    for (auto it = location_header_strings.begin(); it != location_header_strings.end(); ++it)
    {
        location_hbox.push_back(text(*it));
        if (it != location_header_strings.end() - 1)
            location_hbox.push_back(text(" -- "));
    }

    return vbox(text(location.broadcast_name) | inverted, hbox(location_hbox));
}

int main()
{
	// curl boot
    CURL *curl = curl_easy_init();
    if (!curl)
        return -1;

	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curlutils::write_cb); // curl write callback

	// -- get track & driver info from openf1
	auto location = get_latest_track_info(curl);

	// create map of drivers numbers to drivers 
	std::map<int, driver> driver_map;
	{
		auto driver_vec = get_latest_drivers_vec(curl);
		for (auto& driver : driver_vec)
		{
			driver_map[driver.no] = std::move(driver);
		}
	}

	auto drivers = get_latest_drivers(curl); // remove this
	
    // auto table = Table({
    //   { "POS",  "NAME", 		 "INTERVAL",   "TO LEAD",    "SECTOR 1", "SECTOR 2", "SECTOR 3", "LATEST LAP", "FASTEST LAP",  "PIT/OUT",  "TYRES",  "TYRE AGE" },
    //   { "1",    "XXX | XXX",  "+0:00.000",  "+0:00.000",  "00.000",   "00.000",   "00.000",   "0:00.000",   "0:00.000",     "IN PITS",   "(S)" ,  "00 LAPS"  },
    //   { "2",    "XXX | XXX",  "+0:00.000",  "+0:00.000",  "00.000",   "00.000",   "00.000",   "0:00.000",   "0:00.000",     "OUT LAP",   "(S)" ,  "00 LAPS"  },
    //   { "3",    "XXX | XXX",  "+0:00.000",  "+0:00.000",  "00.000",   "00.000",   "00.000",   "0:00.000",   "0:00.000",     "IN PITS",   "(S)" ,  "00 LAPS"  },
    //   { "4",    "XXX | XXX",  "+0:00.000",  "+0:00.000",  "00.000",   "00.000",   "00.000",   "0:00.000",   "0:00.000",     "OUT LAP",   "(S)" ,  "00 LAPS"  },
    //   { "5",    "XXX | XXX",  "+0:00.000",  "+0:00.000",  "00.000",   "00.000",   "00.000",   "0:00.000",   "0:00.000",     "IN PITS",   "(S)" ,  "00 LAPS"  },
    //   { "6",    "XXX | XXX",  "+0:00.000",  "+0:00.000",  "00.000",   "00.000",   "00.000",   "0:00.000",   "0:00.000",     "OUT LAP",   "(S)" ,  "00 LAPS"  },
    //   { "7",    "XXX | XXX",  "+0:00.000",  "+0:00.000",  "00.000",   "00.000",   "00.000",   "0:00.000",   "0:00.000",     "IN PITS",   "(S)" ,  "00 LAPS"  },
    //   { "8",    "XXX | XXX",  "+0:00.000",  "+0:00.000",  "00.000",   "00.000",   "00.000",   "0:00.000",   "0:00.000",     "OUT LAP",   "(S)" ,  "00 LAPS"  },
    //   { "9",    "XXX | XXX",  "+0:00.000",  "+0:00.000",  "00.000",   "00.000",   "00.000",   "0:00.000",   "0:00.000",     "IN PITS",   "(S)" ,  "00 LAPS"  },
    //   { "10",   "XXX | XXX",  "+0:00.000",  "+0:00.000",  "00.000",   "00.000",   "00.000",   "0:00.000",   "0:00.000",     "OUT LAP",   "(S)" ,  "00 LAPS"  },
    //   { "11",   "XXX | XXX",  "+0:00.000",  "+0:00.000",  "00.000",   "00.000",   "00.000",   "0:00.000",   "0:00.000",     "IN PITS",   "(S)" ,  "00 LAPS"  },
    //   { "12",   "XXX | XXX",  "+0:00.000",  "+0:00.000",  "00.000",   "00.000",   "00.000",   "0:00.000",   "0:00.000",     "OUT LAP",   "(S)" ,  "00 LAPS"  },
    //   { "13",   "XXX | XXX",  "+0:00.000",  "+0:00.000",  "00.000",   "00.000",   "00.000",   "0:00.000",   "0:00.000",     "IN PITS",   "(S)" ,  "00 LAPS"  },
    //   { "14",   "XXX | XXX",  "+0:00.000",  "+0:00.000",  "00.000",   "00.000",   "00.000",   "0:00.000",   "0:00.000",     "OUT LAP",   "(S)" ,  "00 LAPS"  },
    //   { "15",   "XXX | XXX",  "+0:00.000",  "+0:00.000",  "00.000",   "00.000",   "00.000",   "0:00.000",   "0:00.000",     "IN PITS",   "(S)" ,  "00 LAPS"  },
    //   { "16",   "XXX | XXX",  "+0:00.000",  "+0:00.000",  "00.000",   "00.000",   "00.000",   "0:00.000",   "0:00.000",     "OUT LAP",   "(S)" ,  "00 LAPS"  },
    //   { "17",   "XXX | XXX",  "+0:00.000",  "+0:00.000",  "00.000",   "00.000",   "00.000",   "0:00.000",   "0:00.000",     "IN PITS",   "(S)" ,  "00 LAPS"  },
    //   { "18",   "XXX | XXX",  "+0:00.000",  "+0:00.000",  "00.000",   "00.000",   "00.000",   "0:00.000",   "0:00.000",     "OUT LAP",   "(S)" ,  "00 LAPS"  },
    //   { "19",   "XXX | XXX",  "+0:00.000",  "+0:00.000",  "00.000",   "00.000",   "00.000",   "0:00.000",   "0:00.000",     "IN PITS",   "(S)" ,  "00 LAPS"  },
    //   { "20",   "XXX | XXX",  "+0:00.000",  "+0:00.000",  "00.000",   "00.000",   "00.000",   "0:00.000",   "0:00.000",     "OUT LAP",   "(S)" ,  "00 LAPS"  },
    // });

	// -- create main ftxui widgets
	Element header = create_location_header(*location);
	Table table = create_drivers_table(*drivers);

	// -- draw ui to terminal
	draw_elements_to_term({ header, table.Render() });

	// -- curl cleanup
    curl_easy_cleanup(curl);

    return 0;
}