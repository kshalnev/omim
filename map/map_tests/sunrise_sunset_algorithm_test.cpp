#include "testing/testing.hpp"

#include "map/sunrise_sunset_algorithm.hpp"

#include "base/logging.hpp"
#include "base/timegm.hpp"

// Test site for sunrise and sunset is
// http://voshod-solnca.ru/

namespace
{

time_t constexpr kTimeEqualityRangeSec = 10 * 60; // 10 minutes

bool TimesEqual(time_t examinedTime, time_t sampleTime, time_t range = kTimeEqualityRangeSec)
{
  bool const res = examinedTime >= (sampleTime - range / 2) &&
                   examinedTime <= (sampleTime + range / 2);
  if (!res)
  {
    string const examined = ctime(&examinedTime);
    string const sample = ctime(&sampleTime);
    LOG(LINFO, ("Times are not equal: examined time", examined, "sample time", sample));
  }
  return res;
}

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

} // namespace

UNIT_TEST(SunriseSunsetAlgorithm_Moscow_April)
{
  // Moscow (utc +3), date 2015/4/12:
  // Sunrise utc time: 2015/4/12,01:55 (local time: 2015/4/12,4:55)
  // Sunset utc time: 2015/4/12,17:07 (local time: 2015/4/12,20:05)
  double const lat = 55.7522222;
  double const lon = 37.6155556;

  time_t sunrise, sunset;
  TEST(CalculateSunriseSunsetTime(2015, 4, 12, lat, lon, sunrise, sunset), ());
  TEST(TimesEqual(sunrise, MakeUtcTime(2015, 4, 12, 1, 55)), ());
  TEST(TimesEqual(sunset, MakeUtcTime(2015, 4, 12, 17, 7)), ());
}

UNIT_TEST(SunriseSunsetAlgorithm_Moscow_July)
{
  // Moscow (utc +3), date 2015/7/13:
  // Sunrise utc time: 2015/7/13,00:07 (local time: 2015/7/13,3:07)
  // Sunset utc time: 2015/7/13,19:05 (local time: 2015/7/13,22:05)
  double const lat = 55.7522222;
  double const lon = 37.6155556;

  time_t sunrise, sunset;
  TEST(CalculateSunriseSunsetTime(2015, 7, 13, lat, lon, sunrise, sunset), ());
  TEST(TimesEqual(sunrise, MakeUtcTime(2015, 7, 13, 0, 7)), ());
  TEST(TimesEqual(sunset, MakeUtcTime(2015, 7, 13, 19, 5)), ());
}

UNIT_TEST(SunriseSunsetAlgorithm_Moscow_September)
{
  // Moscow (utc +3), date 2015/9/17:
  // Sunrise utc time: 2015/9/17,2:28 (local time: 2015/9/17,5:28)
  // Sunset utc time: 2015/9/17,16:23 (local time: 2015/9/17,19:23)
  double const lat = 55.7522222;
  double const lon = 37.6155556;

  time_t sunrise, sunset;
  TEST(CalculateSunriseSunsetTime(2015, 9, 17, lat, lon, sunrise, sunset), ());
  TEST(TimesEqual(sunrise, MakeUtcTime(2015, 9, 17, 2, 28)), ());
  TEST(TimesEqual(sunset, MakeUtcTime(2015, 9, 17, 16, 23)), ());
}

UNIT_TEST(SunriseSunsetAlgorithm_Moscow_December)
{
  // Moscow (utc +3), date 2015/12/25:
  // Sunrise utc time: 2015/12/25,05:12 (local time: 2015/12/25,8:12)
  // Sunset utc time: 2015/12/25,13:48 (local time: 2015/12/25,16:48)
  double const lat = 55.7522222;
  double const lon = 37.6155556;

  time_t sunrise, sunset;
  TEST(CalculateSunriseSunsetTime(2015, 12, 25, lat, lon, sunrise, sunset), ());
  TEST(TimesEqual(sunrise, MakeUtcTime(2015, 12, 25, 5, 12)), ());
  TEST(TimesEqual(sunset, MakeUtcTime(2015, 12, 25, 13, 48)), ());
}

UNIT_TEST(SunriseSunsetAlgorithm_Moscow_NewYear_1)
{
  // Moscow (utc +3), date 2016/1/1:
  // Sunrise utc time: 2016/1/1,5:13 (local time: 2016/1/1,8:13)
  // Sunset utc time: 2016/1/1,13:54 (local time: 2016/1/1,16:54)
  double const lat = 55.7522222;
  double const lon = 37.6155556;

  time_t sunrise, sunset;
  TEST(CalculateSunriseSunsetTime(2016, 1, 1, lat, lon, sunrise, sunset), ());
  TEST(TimesEqual(sunrise, MakeUtcTime(2016, 1, 1, 5, 13)), ());
  TEST(TimesEqual(sunset, MakeUtcTime(2016, 1, 1, 13, 54)), ());
}

UNIT_TEST(SunriseSunsetAlgorithm_Moscow_NewYear_2)
{
  // Moscow (utc +3), local time 2016/1/1,2:0, utc time 2015/12/31,23:0
  // Sunrise utc time: 2016/1/1,5:13 (local time: 2016/1/1,8:13)
  // Sunset utc time: 2016/1/1,13:54 (local time: 2016/1/1,16:54)
  double const lat = 55.7522222;
  double const lon = 37.6155556;

  time_t sunrise, sunset;
  TEST(CalculateSunriseSunsetTime(MakeUtcTime(2015, 12, 31, 23, 0), lat, lon, sunrise, sunset), ());
  TEST(TimesEqual(sunrise, MakeUtcTime(2016, 1, 1, 5, 13)), ());
  TEST(TimesEqual(sunset, MakeUtcTime(2016, 1, 1, 13, 54)), ());
}

UNIT_TEST(SunriseSunsetAlgorithm_GetDayTime_Moscow_December)
{
  // Moscow (utc +3), 2015/12/24
  // prev sunset utc 2015/12/23,13:46
  // sunrise utc 2015/12/24,5:12
  // sunset utc 2015/12/24,13:37
  // next sunrise utc 2015/12/24,5:13
  double const lat = 55.7522222;
  double const lon = 37.6155556;

  DayTimeType type;
  time_t until;

  // before sunrise but after prev sunset
  TEST(GetDayTime(MakeUtcTime(2015, 12, 23, 17, 0), lat, lon, type, until), ());
  TEST_EQUAL(type, DayTimeType::NightTime, ());
  TEST(TimesEqual(until, MakeUtcTime(2015, 12, 24, 5, 12)), ());

  // between sunrise and sunset
  TEST(GetDayTime(MakeUtcTime(2015, 12, 24, 10, 53), lat, lon, type, until), ());
  TEST_EQUAL(type, DayTimeType::DayTime, ());
  TEST(TimesEqual(until, MakeUtcTime(2015, 12, 24, 13, 47)), ());

  // after sunset and before next sunrise
  TEST(GetDayTime(MakeUtcTime(2015, 12, 24, 16, 30), lat, lon, type, until), ());
  TEST_EQUAL(type, DayTimeType::NightTime, ());
  TEST(TimesEqual(until, MakeUtcTime(2015, 12, 25, 5, 13)), ());
}

UNIT_TEST(SunriseSunsetAlgorithm_Paris_NewYear)
{
  // Paris (utc +1), local time 2016/1/1,0:30, utc time 2015/12/31,23:30
  // Sunrise utc time: 2016/1/1,7:08 (local time: 2016/1/1,8:08)
  // Sunset utc time: 2016/1/1,16:41 (local time: 2016/1/1,17:41)
  double const lat = 48.875649;
  double const lon = 2.344428;

  time_t sunrise, sunset;
  TEST(CalculateSunriseSunsetTime(MakeUtcTime(2015, 12, 31, 23, 30), lat, lon, sunrise, sunset), ());
  TEST(TimesEqual(sunrise, MakeUtcTime(2016, 1, 1, 7, 8)), ());
  TEST(TimesEqual(sunset, MakeUtcTime(2016, 1, 1, 16, 41)), ());
}

UNIT_TEST(SunriseSunsetAlgorithm_Honolulu_February)
{
  // Honolulu (utc -10), date 2015/2/12:
  // Sunrise utc time: 2015/2/12,16:42 (local time: 2015/2/12,6:42)
  // Sunset utc time: 2015/2/13,4:51 (local time: 2015/2/12,18:51)
  double const lat = 21.307431;
  double const lon = -157.848568;

  time_t sunrise, sunset;
  TEST(CalculateSunriseSunsetTime(2015, 2, 12, lat, lon, sunrise, sunset), ());
  TEST(TimesEqual(sunrise, MakeUtcTime(2015, 2, 12, 16, 42)), ());
  TEST(TimesEqual(sunset, MakeUtcTime(2015, 2, 13, 4, 51)), ());
}

UNIT_TEST(SunriseSunsetAlgorithm_Honolulu_July)
{
  // Honolulu (utc -10). For date 2015/7/13:
  // Sunrise utc time: 2015/7/13,15:33 (local time: 2015/7/13,5:33)
  // Sunset utc time: 2015/7/14,5:42 (local time: 2015/7/13,19:42)
  double const lat = 21.307431;
  double const lon = -157.848568;

  time_t sunrise, sunset;
  TEST(CalculateSunriseSunsetTime(2015, 7, 13, lat, lon, sunrise, sunset), ());
  TEST(TimesEqual(sunrise, MakeUtcTime(2015, 7, 13, 15, 33)), ());
  TEST(TimesEqual(sunset, MakeUtcTime(2015, 7, 14, 5, 42)), ());
}

UNIT_TEST(SunriseSunsetAlgorithm_Honolulu_December)
{
  // Honolulu (utc -10). For date 2015/12/23:
  // Sunrise utc time: 2015/12/23,16:43 (local time: 2015/12/23,6:43)
  // Sunset utc time: 2015/12/24,4:21 (local time: 2015/12/23,18:21)
  double const lat = 21.307431;
  double const lon = -157.848568;

  time_t sunrise, sunset;
  TEST(CalculateSunriseSunsetTime(2015, 12, 23, lat, lon, sunrise, sunset), ());
  TEST(TimesEqual(sunrise, MakeUtcTime(2015, 12, 23, 16, 43)), ());
  TEST(TimesEqual(sunset, MakeUtcTime(2015, 12, 24, 4, 21)), ());
}

UNIT_TEST(SunriseSunsetAlgorithm_Melbourne_Ferbuary)
{
  // Melbourne (utc +11). For date 2015/2/12:
  // Sunrise utc time: 2015/2/11,19:18 (local time: 2015/2/12,6:18)
  // Sunset utc time: 2015/2/12,9:52 (local time: 2015/2/12,20:52)
  double const lat = -37.829188;
  double const lon = 144.957976;

  time_t sunrise, sunset;
  TEST(CalculateSunriseSunsetTime(2015, 2, 12, lat, lon, sunrise, sunset), ());
  TEST(TimesEqual(sunrise, MakeUtcTime(2015, 2, 11, 19, 18)), ());
  TEST(TimesEqual(sunset, MakeUtcTime(2015, 2, 12, 9, 52)), ());
}

UNIT_TEST(SunriseSunsetAlgorithm_Melbourne_NewYear)
{
  // Melbourne (utc +11). For date 2016/1/1:
  // Sunrise utc time: 2015/12/31,18:30
  // Sunset utc time: 2016/1/1,10:17
  double const lat = -37.829188;
  double const lon = 144.957976;

  time_t sunrise, sunset;
  TEST(CalculateSunriseSunsetTime(2016, 1, 1, lat, lon, sunrise, sunset), ());
  TEST(TimesEqual(sunrise, MakeUtcTime(2015, 12, 31, 18, 30)), ());
  TEST(TimesEqual(sunset, MakeUtcTime(2016, 1, 1, 10, 17)), ());
}

UNIT_TEST(SunriseSunsetAlgorithm_GetDayTime_Melbourne_August)
{
  // Melbourne (utc +11), 2015/8/12
  // prev sunset utc 2015/8/11,8:8
  // sunrise utc 2015/8/11,20:43
  // sunset utc 2015/8/12,8:9
  // next sunrise utc 2015/8/12,20:42
  double const lat = -37.829188;
  double const lon = 144.957976;

  DayTimeType type;
  time_t until;

  // before sunrise but after prev sunset
  TEST(GetDayTime(MakeUtcTime(2015, 8, 11, 15, 0), lat, lon, type, until), ());
  TEST_EQUAL(type, DayTimeType::NightTime, ());
  TEST(TimesEqual(until, MakeUtcTime(2015, 8, 11, 20, 43)), ());

  // between sunrise and sunset
  TEST(GetDayTime(MakeUtcTime(2015, 8, 11, 21, 0), lat, lon, type, until), ());
  TEST_EQUAL(type, DayTimeType::DayTime, ());
  TEST(TimesEqual(until, MakeUtcTime(2015, 8, 12, 8, 9)), ());

  // after sunset but before next sunrise
  TEST(GetDayTime(MakeUtcTime(2015, 8, 12, 10, 0), lat, lon, type, until), ());
  TEST_EQUAL(type, DayTimeType::NightTime, ());
  TEST(TimesEqual(until, MakeUtcTime(2015, 8, 12, 20, 42)), ());
}

UNIT_TEST(SunriseSunsetAlgorithm_Wellington_October)
{
  // Melbourne (utc +13). For date 2015/10/20:
  // Sunrise utc time: 2015/10/19,16:57 (local time: 2015/10/20,5:57)
  // Sunset utc time: 2015/10/20,7:16 (local time: 2015/10/20,20:16)
  double const lat = -41.287481;
  double const lon = 174.774189;

  time_t sunrise, sunset;
  TEST(CalculateSunriseSunsetTime(2015, 10, 20, lat, lon, sunrise, sunset), ());
  TEST(TimesEqual(sunrise, MakeUtcTime(2015, 10, 19, 16, 57)), ());
  TEST(TimesEqual(sunset, MakeUtcTime(2015, 10, 20, 7, 16)), ());
}

UNIT_TEST(SunriseSunsetAlgorithm_BuenosAires_March)
{
  // Buenos Aires (utc -3). For date 2015/3/8:
  // Sunrise utc time: 2015/3/8,9:22 (local time: 2015/3/8,6:22)
  // Sunset utc time: 2015/3/8,22:49 (local time: 2015/3/8,19:49)
  double const lat = -34.607639;
  double const lon = -58.438095;

  time_t sunrise, sunset;
  TEST(CalculateSunriseSunsetTime(2015, 3, 8, lat, lon, sunrise, sunset), ());
  TEST(TimesEqual(sunrise, MakeUtcTime(2015, 3, 8, 9, 22)), ());
  TEST(TimesEqual(sunset, MakeUtcTime(2015, 3, 8, 22, 49)), ());
}

UNIT_TEST(SunriseSunsetAlgorithm_Seattle_May)
{
  // Seattle (utc -8). For date 2015/5/9:
  // Sunrise utc time: 2015/5/9,12:05 (local time: 2015/5/9,4:05)
  // Sunset utc time: 2015/5/10,4:08 (local time: 2015/5/9,20:08)
  double const lat = 47.597482;
  double const lon = -122.334590;

  time_t sunrise, sunset;
  TEST(CalculateSunriseSunsetTime(2015, 5, 9, lat, lon, sunrise, sunset), ());
  TEST(TimesEqual(sunrise, MakeUtcTime(2015, 5, 9, 12, 05)), ());
  TEST(TimesEqual(sunset, MakeUtcTime(2015, 5, 10, 4, 8)), ());
}

UNIT_TEST(SunriseSunsetAlgorithm_Reykjavik_May)
{
  // Reykjavik (utc 0). For date 2015/5/9:
  // Sunrise utc time: 2015/5/9,3:12 (local time: 2015/5/9,3:12)
  // Sunset utc time: 2015/5/9,23:37 (local time: 2015/5/9,23:37)
  double const lat = 64.120467;
  double const lon = -21.809448;

  time_t sunrise, sunset;
  TEST(CalculateSunriseSunsetTime(2015, 5, 9, lat, lon, sunrise, sunset), ());
  TEST(TimesEqual(sunrise, MakeUtcTime(2015, 5, 9, 3, 12)), ());
  TEST(TimesEqual(sunset, MakeUtcTime(2015, 5, 9, 23, 37)), ());
}

UNIT_TEST(SunriseSunsetAlgorithm_Reykjavik_June)
{
  // Reykjavik (utc 0). For date 2015/6/22:
  // No sunrise/sunset
  double const lat = 64.120467;
  double const lon = -21.809448;

  time_t sunrise, sunset;
  TEST_EQUAL(false, CalculateSunriseSunsetTime(2015, 6, 22, lat, lon, sunrise, sunset), ());
}
