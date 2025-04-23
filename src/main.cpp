#include <ftxui/dom/elements.hpp>
#include <ftxui/dom/table.hpp>
#include <ftxui/screen/screen.hpp>
#include <ftxui/dom/node.hpp>
#include <ftxui/screen/color.hpp>
#include <curl/curl.h>
#include <nlohmann/json.hpp>

#include <iostream>
#include <string>
#include <vector>
#include <chrono>

#include "openf1.hpp"

typedef std::vector<std::string> stringvec;
using namespace nlohmann;

CURL* curl = curl_easy_init();

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

  std::chrono::milliseconds sector_1;
  std::chrono::milliseconds sector_2;
  std::chrono::milliseconds sector_3;
  std::chrono::milliseconds total;

  // stringvec to_stringvec();
};

class times
{
  public:

  std::chrono::milliseconds interval;
  std::chrono::milliseconds gap;

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

  const int no;
  const char* name;

  session sesh;

  private:

};

stringvec table_row(const driver& driver)
{
  return stringvec{ std::initializer_list<std::string>{
    driver.name,
    "+0:00.000" // driver.timings.interval
    "+0:00.000" // driver.timings.gap
    "00.000"    // driver.timings.sector_1
    "00.000"    // driver.timings.sector_2
    "00.000"    // driver.timings.sector_3
    "0:00.000"  // driver.timings.latest_lap
    "0:00.000"  // driver.timings.fastest_lap
    "PIT/OUT",
    "TYRES",
    "TYRE AGE"
  }};
}

void decorate_table(ftxui::Table& table)
{
  using namespace ftxui;

  table.SelectColumn(0).DecorateCells(align_right);                // align driver numbers right
  table.SelectColumns(1, -1).DecorateCells(hcenter);               // centre body cells

  table.SelectColumn(1).DecorateCells(size(WIDTH, EQUAL, 5));      // NAME width
  table.SelectColumns(2, 3).DecorateCells(size(WIDTH, EQUAL, 11)); // INTERVAL/LEAD width
  table.SelectColumn(9).DecorateCells(size(WIDTH, EQUAL, 9));      // PIT/OUT width

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

int main() {
  using namespace ftxui;

  if (!curl) return -1;

  json j;

  auto table = Table({
    { "POS",  "NAME", "INTERVAL",   "TO LEAD",    "SECTOR 1", "SECTOR 2", "SECTOR 3", "LATEST LAP", "FASTEST LAP",  "PIT/OUT",  "TYRES",  "TYRE AGE" },
    { "1",    "XXX",  "+0:00.000",  "+0:00.000",  "00.000",   "00.000",   "00.000",   "0:00.000",   "0:00.000",     "IN PITS",   "(S)" ,  "00 LAPS"  },
    { "2",    "XXX",  "+0:00.000",  "+0:00.000",  "00.000",   "00.000",   "00.000",   "0:00.000",   "0:00.000",     "OUT LAP",   "(S)" ,  "00 LAPS"  },
    { "3",    "XXX",  "+0:00.000",  "+0:00.000",  "00.000",   "00.000",   "00.000",   "0:00.000",   "0:00.000",     "IN PITS",   "(S)" ,  "00 LAPS"  },
    { "4",    "XXX",  "+0:00.000",  "+0:00.000",  "00.000",   "00.000",   "00.000",   "0:00.000",   "0:00.000",     "OUT LAP",   "(S)" ,  "00 LAPS"  },
    { "5",    "XXX",  "+0:00.000",  "+0:00.000",  "00.000",   "00.000",   "00.000",   "0:00.000",   "0:00.000",     "IN PITS",   "(S)" ,  "00 LAPS"  },
    { "6",    "XXX",  "+0:00.000",  "+0:00.000",  "00.000",   "00.000",   "00.000",   "0:00.000",   "0:00.000",     "OUT LAP",   "(S)" ,  "00 LAPS"  },
    { "7",    "XXX",  "+0:00.000",  "+0:00.000",  "00.000",   "00.000",   "00.000",   "0:00.000",   "0:00.000",     "IN PITS",   "(S)" ,  "00 LAPS"  },
    { "8",    "XXX",  "+0:00.000",  "+0:00.000",  "00.000",   "00.000",   "00.000",   "0:00.000",   "0:00.000",     "OUT LAP",   "(S)" ,  "00 LAPS"  },
    { "9",    "XXX",  "+0:00.000",  "+0:00.000",  "00.000",   "00.000",   "00.000",   "0:00.000",   "0:00.000",     "IN PITS",   "(S)" ,  "00 LAPS"  },
    { "10",   "XXX",  "+0:00.000",  "+0:00.000",  "00.000",   "00.000",   "00.000",   "0:00.000",   "0:00.000",     "OUT LAP",   "(S)" ,  "00 LAPS"  },
    { "11",   "XXX",  "+0:00.000",  "+0:00.000",  "00.000",   "00.000",   "00.000",   "0:00.000",   "0:00.000",     "IN PITS",   "(S)" ,  "00 LAPS"  },
    { "12",   "XXX",  "+0:00.000",  "+0:00.000",  "00.000",   "00.000",   "00.000",   "0:00.000",   "0:00.000",     "OUT LAP",   "(S)" ,  "00 LAPS"  },
    { "13",   "XXX",  "+0:00.000",  "+0:00.000",  "00.000",   "00.000",   "00.000",   "0:00.000",   "0:00.000",     "IN PITS",   "(S)" ,  "00 LAPS"  },
    { "14",   "XXX",  "+0:00.000",  "+0:00.000",  "00.000",   "00.000",   "00.000",   "0:00.000",   "0:00.000",     "OUT LAP",   "(S)" ,  "00 LAPS"  },
    { "15",   "XXX",  "+0:00.000",  "+0:00.000",  "00.000",   "00.000",   "00.000",   "0:00.000",   "0:00.000",     "IN PITS",   "(S)" ,  "00 LAPS"  },
    { "16",   "XXX",  "+0:00.000",  "+0:00.000",  "00.000",   "00.000",   "00.000",   "0:00.000",   "0:00.000",     "OUT LAP",   "(S)" ,  "00 LAPS"  },
    { "17",   "XXX",  "+0:00.000",  "+0:00.000",  "00.000",   "00.000",   "00.000",   "0:00.000",   "0:00.000",     "IN PITS",   "(S)" ,  "00 LAPS"  },
    { "18",   "XXX",  "+0:00.000",  "+0:00.000",  "00.000",   "00.000",   "00.000",   "0:00.000",   "0:00.000",     "OUT LAP",   "(S)" ,  "00 LAPS"  },
    { "19",   "XXX",  "+0:00.000",  "+0:00.000",  "00.000",   "00.000",   "00.000",   "0:00.000",   "0:00.000",     "IN PITS",   "(S)" ,  "00 LAPS"  },
    { "20",   "XXX",  "+0:00.000",  "+0:00.000",  "00.000",   "00.000",   "00.000",   "0:00.000",   "0:00.000",     "OUT LAP",   "(S)" ,  "00 LAPS"  },
  });

  decorate_table(table);

  auto document = table.Render();
  auto screen = Screen::Create(Dimension::Fit(document, /*extend_beyond_screen=*/true));
  Render(screen, document);
  screen.Print();
  std::cout << std::endl;

  curl_easy_cleanup(curl);

  return 0;
}