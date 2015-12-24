#pragma once

#include "std/ctime.hpp"
#include "std/string.hpp"

/// Calculates timestamps of the sunrise and sunset for a specified date
/// in a specified location.
/// @param year - year, since 0, like 2015
/// @param month - month, 1-jan...12-dec
/// @param day - day of month 1...31
/// @param latitude - latutude, -90...+90 degrees
/// @param longitude - longitude, -180...+180 degrees
/// @param sunriseUtc - output param, UTC time of sunrise
/// @param sunsetUtc - output param, UTC time of sunset
/// @returns true on success and false on error
/// @note date year/month/day is specified for the interesting point latituda/longitude
/// @note for polar day diff between sunset and sunrise is 24h - all day sun,
/// and for polar night sunrise = sunset.
bool CalculateSunriseSunsetTime(int year, int month, int day,
                                double latitude, double longitude,
                                time_t & sunriseUtc, time_t & sunsetUtc);

/// Calculates timestamps of the sunrise and sunset for a specified time
/// in a specified location.
/// @param timeUtc - utc time
/// @param latitude - latutude, -90...+90 degrees
/// @param longitude - longitude, -180...+180 degrees
/// @param sunriseUtc - output param, UTC time of sunrise
/// @param sunsetUtc - output param, UTC time of sunset
/// @returns true on success and false on error
/// @note if sunrise/sunset occur before the specified time then next sunrise and sunset are returned.
/// @note for polar day diff between sunset and sunrise is 24h - all day sun,
/// and for polar night sunrise = sunset.
bool CalculateSunriseSunsetTime(time_t timeUtc,
                                double latitude, double longitude,
                                time_t & sunriseUtc, time_t & sunsetUtc);

enum class DayTimeType
{
  DayTime,
  NightTime,
  PolarDay,
  PolarNight
};

string DebugPrint(DayTimeType type);

/// Calculates day time type and time limit for that day time type
/// for a specified date and for a specified location.
/// @param timeUtc - utc time
/// @param latitude - latutude, -90...+90 degrees
/// @param longitude - longitude, -180...+180 degrees
/// @param type - output, type of day time
/// @param untilUtc - output, UTC file when day time type is being changed
/// @returns true on success and false on error
bool GetDayTime(time_t timeUtc,
                double latitude, double longitude,
                DayTimeType & type, time_t & untilUtc);
