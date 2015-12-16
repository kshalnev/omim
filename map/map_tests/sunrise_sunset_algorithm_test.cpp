#include "testing/testing.hpp"

#include "map/sunrise_sunset_algorithm.hpp"

#include "base/logging.hpp"
#include "base/timegm.hpp"

// Test site for sunrise sunset is
// http://voshod-solnca.ru/

namespace
{

time_t constexpr kSecondsPerHour = 60 * 60;

time_t MakeUtcTime(int year, int month, int day, int hour = 0, int min = 0, int sec = 0)
{
  tm t = {};
  t.tm_year = year - 1900;
  t.tm_mon = month - 1;
  t.tm_mday = day;
  t.tm_hour = hour;
  t.tm_min = min;
  t.tm_sec = sec;
  return base::TimeGM(t);
}

time_t LocalTimeToUtc(int utcOffset, int year, int month, int day, int hour = 0, int min = 0, int sec = 0)
{
  return MakeUtcTime(year, month, day, hour, min, sec) - utcOffset * kSecondsPerHour;
}

} // namespace

UNIT_TEST(SunriseSunsetAlgorithm_Moscow_December)
{
  double const lat = 55.7522222;
  double const lon = 37.6155556;
  int const utcOffset = +3;

  time_t sunrise, sunset;
  TEST(CalculateSunriseSunsetTime(2015, 12, 21, lat, lon, sunrise, sunset), ());

  TEST_GREATER_OR_EQUAL(sunrise, LocalTimeToUtc(utcOffset, 2015, 12, 21, 8, 5), ());
  TEST_LESS_OR_EQUAL(sunrise, LocalTimeToUtc(utcOffset, 2015, 12, 21, 8, 15), ());

  TEST_GREATER_OR_EQUAL(sunset, LocalTimeToUtc(utcOffset, 2015, 12, 21, 16, 40), ());
  TEST_LESS_OR_EQUAL(sunset, LocalTimeToUtc(utcOffset, 2015, 12, 21, 16, 50), ());
}

UNIT_TEST(SunriseSunsetAlgorithm_Moscow_April)
{
  double const lat = 55.7522222;
  double const lon = 37.6155556;
  int const utcOffset = +3;

  time_t sunrise, sunset;
  TEST(CalculateSunriseSunsetTime(2015, 4, 12, lat, lon, sunrise, sunset), ());

  TEST_GREATER_OR_EQUAL(sunrise, LocalTimeToUtc(utcOffset, 2015, 4, 12, 4, 50), ());
  TEST_LESS_OR_EQUAL(sunrise, LocalTimeToUtc(utcOffset, 2015, 4, 12, 5, 0), ());

  TEST_GREATER_OR_EQUAL(sunset, LocalTimeToUtc(utcOffset, 2015, 4, 12, 20, 0), ());
  TEST_LESS_OR_EQUAL(sunset, LocalTimeToUtc(utcOffset, 2015, 4, 12, 20, 10), ());
}

UNIT_TEST(SunriseSunsetAlgorithm_Moscow_July)
{
  double const lat = 55.7522222;
  double const lon = 37.6155556;
  int const utcOffset = +3;

  time_t sunrise, sunset;
  TEST(CalculateSunriseSunsetTime(2015, 7, 13, lat, lon, sunrise, sunset), ());

  TEST_GREATER_OR_EQUAL(sunrise, LocalTimeToUtc(utcOffset, 2015, 7, 13, 3, 0), ());
  TEST_LESS_OR_EQUAL(sunrise, LocalTimeToUtc(utcOffset, 2015, 7, 13, 3, 10), ());

  TEST_GREATER_OR_EQUAL(sunset, LocalTimeToUtc(utcOffset, 2015, 7, 13, 22, 0), ());
  TEST_LESS_OR_EQUAL(sunset, LocalTimeToUtc(utcOffset, 2015, 7, 13, 22, 10), ());
}

UNIT_TEST(SunriseSunsetAlgorithm_Moscow_September)
{
  double const lat = 55.7522222;
  double const lon = 37.6155556;
  int const utcOffset = +3;

  time_t sunrise, sunset;
  TEST(CalculateSunriseSunsetTime(2015, 9, 16, lat, lon, sunrise, sunset), ());

  TEST_GREATER_OR_EQUAL(sunrise, LocalTimeToUtc(utcOffset, 2015, 9, 16, 5, 15), ());
  TEST_LESS_OR_EQUAL(sunrise, LocalTimeToUtc(utcOffset, 2015, 9, 16, 5, 30), ());

  TEST_GREATER_OR_EQUAL(sunset, LocalTimeToUtc(utcOffset, 2015, 9, 16, 19, 20), ());
  TEST_LESS_OR_EQUAL(sunset, LocalTimeToUtc(utcOffset, 2015, 9, 16, 19, 30), ());
}
