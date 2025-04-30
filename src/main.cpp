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
void decorate_table(ftxui::Table &table)
{
    using namespace ftxui;

    table.SelectColumn(0).DecorateCells(align_right);                   // align driver numbers right
    table.SelectColumns(1, -1).DecorateCells(hcenter);                  // centre body cells
    table.SelectColumn(1).DecorateCells(size(WIDTH, EQUAL, 5));         // NAME width
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

json get_drivers_json()
{
    json j;
    return j;
}

json get_positions_json()
{
    json j;
    return j;
}

json get_laps_json()
{
    json j;
    return j;
}

json get_intervals_json()
{
    json j;
    return j;
}

json get_stints_json()
{
    json j;
    return j;
}

void print_json(const json &js)
{
    std::cout << std::setw(4) << js << std::endl;
}

/**
 * general execution order:
 * - req latest track details
 *   - req weather?
 * - req drivers (number, acronym)
 * - req position
 * - req tyres & age (possibly needs to be done after race start?)
 * - poll race control for green flag
 *   - retain timestamp of green flag for first lap/sector 1 calcs
 *
 * main req loop:
 * - req position
 * - req intervals (interval, gap to leader)
 * - req laps (sector 1/2/3, duration, lap number, is out lap)
 * - req stints (compound, tyre age, lap start)
 * - req race control (unsure what this will look like)
 *
 * req notes:
 * - best to filter by "after this timestamp" to prevent having to make 20 separate reqs per driver
 * - need to find best way to filter to only latest capture
 * - do NOT forget about URI encoding ffs
 *   - should be simple enough to do manually
 *   - if not, should consider boost
 *     - would really rather not consider boost
 *
 * main widgets:
 * - headers
 *   - gp title
 *   - country -- location -- track (check for matches & hide duplicates)
 *   - weather somewhere here?
 * - table
 * - race control ticker
 */

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

ftxui::Table create_drivers_table(const std::vector<driver>& drivers)
{
	std::vector<std::vector<std::string>> table_rows{
        {"POS", "NAME", "INTERVAL", "TO LEAD", "SECTOR 1", "SECTOR 2", "SECTOR 3", "LAST LAP", "BEST LAP", "PIT/OUT", "TYRES", "TYRE AGE"},
    };

	milliseconds thirty_seconds(32343);
	milliseconds ninety_seconds(92345);

    for (int i = 0; i < 20; i++)
    {
		const auto& driver = drivers[i];
        table_rows.push_back(
			{
				std::to_string(i + 1),
				driver.name,
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

	auto table = ftxui::Table(table_rows);
	decorate_table(table);

	return table;
}

int main()
{
    using namespace ftxui;

    CURL *curl = curl_easy_init();
    if (!curl)
        return -1;

	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curlutils::write_cb);

	auto location = get_latest_track_info(curl);
	auto drivers = get_latest_drivers(curl);

    auto location_header_strings = location->to_strings();
    std::vector<Element> location_hbox;

    for (auto it = location_header_strings.begin(); it != location_header_strings.end(); ++it)
    {
        location_hbox.push_back(text(*it));
        if (it != location_header_strings.end() - 1)
            location_hbox.push_back(text(" -- "));
    }

    auto header = vbox(text(location->broadcast_name) | inverted, hbox(location_hbox));
	Table table = create_drivers_table(*drivers);

    // auto table = Table({
    //   { "POS",  "NAME", "INTERVAL",   "TO LEAD",    "SECTOR 1", "SECTOR 2", "SECTOR 3", "LATEST LAP", "FASTEST LAP",  "PIT/OUT",  "TYRES",  "TYRE AGE" },
    //   { "1",    "XXX",  "+0:00.000",  "+0:00.000",  "00.000",   "00.000",   "00.000",   "0:00.000",   "0:00.000",     "IN PITS",   "(S)" ,  "00 LAPS"  },
    //   { "2",    "XXX",  "+0:00.000",  "+0:00.000",  "00.000",   "00.000",   "00.000",   "0:00.000",   "0:00.000",     "OUT LAP",   "(S)" ,  "00 LAPS"  },
    //   { "3",    "XXX",  "+0:00.000",  "+0:00.000",  "00.000",   "00.000",   "00.000",   "0:00.000",   "0:00.000",     "IN PITS",   "(S)" ,  "00 LAPS"  },
    //   { "4",    "XXX",  "+0:00.000",  "+0:00.000",  "00.000",   "00.000",   "00.000",   "0:00.000",   "0:00.000",     "OUT LAP",   "(S)" ,  "00 LAPS"  },
    //   { "5",    "XXX",  "+0:00.000",  "+0:00.000",  "00.000",   "00.000",   "00.000",   "0:00.000",   "0:00.000",     "IN PITS",   "(S)" ,  "00 LAPS"  },
    //   { "6",    "XXX",  "+0:00.000",  "+0:00.000",  "00.000",   "00.000",   "00.000",   "0:00.000",   "0:00.000",     "OUT LAP",   "(S)" ,  "00 LAPS"  },
    //   { "7",    "XXX",  "+0:00.000",  "+0:00.000",  "00.000",   "00.000",   "00.000",   "0:00.000",   "0:00.000",     "IN PITS",   "(S)" ,  "00 LAPS"  },
    //   { "8",    "XXX",  "+0:00.000",  "+0:00.000",  "00.000",   "00.000",   "00.000",   "0:00.000",   "0:00.000",     "OUT LAP",   "(S)" ,  "00 LAPS"  },
    //   { "9",    "XXX",  "+0:00.000",  "+0:00.000",  "00.000",   "00.000",   "00.000",   "0:00.000",   "0:00.000",     "IN PITS",   "(S)" ,  "00 LAPS"  },
    //   { "10",   "XXX",  "+0:00.000",  "+0:00.000",  "00.000",   "00.000",   "00.000",   "0:00.000",   "0:00.000",     "OUT LAP",   "(S)" ,  "00 LAPS"  },
    //   { "11",   "XXX",  "+0:00.000",  "+0:00.000",  "00.000",   "00.000",   "00.000",   "0:00.000",   "0:00.000",     "IN PITS",   "(S)" ,  "00 LAPS"  },
    //   { "12",   "XXX",  "+0:00.000",  "+0:00.000",  "00.000",   "00.000",   "00.000",   "0:00.000",   "0:00.000",     "OUT LAP",   "(S)" ,  "00 LAPS"  },
    //   { "13",   "XXX",  "+0:00.000",  "+0:00.000",  "00.000",   "00.000",   "00.000",   "0:00.000",   "0:00.000",     "IN PITS",   "(S)" ,  "00 LAPS"  },
    //   { "14",   "XXX",  "+0:00.000",  "+0:00.000",  "00.000",   "00.000",   "00.000",   "0:00.000",   "0:00.000",     "OUT LAP",   "(S)" ,  "00 LAPS"  },
    //   { "15",   "XXX",  "+0:00.000",  "+0:00.000",  "00.000",   "00.000",   "00.000",   "0:00.000",   "0:00.000",     "IN PITS",   "(S)" ,  "00 LAPS"  },
    //   { "16",   "XXX",  "+0:00.000",  "+0:00.000",  "00.000",   "00.000",   "00.000",   "0:00.000",   "0:00.000",     "OUT LAP",   "(S)" ,  "00 LAPS"  },
    //   { "17",   "XXX",  "+0:00.000",  "+0:00.000",  "00.000",   "00.000",   "00.000",   "0:00.000",   "0:00.000",     "IN PITS",   "(S)" ,  "00 LAPS"  },
    //   { "18",   "XXX",  "+0:00.000",  "+0:00.000",  "00.000",   "00.000",   "00.000",   "0:00.000",   "0:00.000",     "OUT LAP",   "(S)" ,  "00 LAPS"  },
    //   { "19",   "XXX",  "+0:00.000",  "+0:00.000",  "00.000",   "00.000",   "00.000",   "0:00.000",   "0:00.000",     "IN PITS",   "(S)" ,  "00 LAPS"  },
    //   { "20",   "XXX",  "+0:00.000",  "+0:00.000",  "00.000",   "00.000",   "00.000",   "0:00.000",   "0:00.000",     "OUT LAP",   "(S)" ,  "00 LAPS"  },
    // });

    auto document = vbox(header, table.Render());

    auto screen = Screen::Create(Dimension::Fit(document, /*extend_beyond_screen=*/true));
    Render(screen, document);
    screen.Print();
    std::cout << std::endl;

    curl_easy_cleanup(curl);

    return 0;
}