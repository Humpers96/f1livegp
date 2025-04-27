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

  private:

};

void from_json(const nlohmann::json& j, driver& d)
{
  j.at("name_acronym").get_to(d.name);
  j.at("driver_number").get_to(d.no);
}