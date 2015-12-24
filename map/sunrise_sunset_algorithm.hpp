#pragma once

#include "std/ctime.hpp"
#include "std/string.hpp"
#include "std/utility.hpp"

/// Calculates timestamps of the sunrise and sunset for a specified date
/// in a specified location.
/// @param year - year, since 0, like 2015
/// @param month - month, 1-jan...12-dec
/// @param day - day of month 1...31
/// @param latitude - latutude, -90...+90 degrees
/// @param longitude - longitude, -180...+180 degrees
/// @returns pair where first is sunrise UTC and second is sunset UTC
/// @note date year/month/day is specified for the interesting point latituda/longitude
/// @note for polar day sunrise is set to year/month/day,0:0:0 and sunset is set to sunrise + 24h - 24h of day
/// @note for polar night sunrise and sunset both are set to year/month/day,0:0:0 - 0 sec of day
pair<time_t, time_t> CalculateSunriseSunsetTime(int year, int month, int day, double latitude, double longitude);

/// Calculates timestamps of the sunrise and sunset for a specified time
/// in a specified location.
/// @param timeUtc - UTC time
/// @param latitude - latutude, -90...+90 degrees
/// @param longitude - longitude, -180...+180 degrees
/// @returns pair where first is sunrise UTC and second is sunset UTC
/// @note if sunrise/sunset occur before the specified timeUtc then next sunrise and sunset are returned.
/// @note for polar day sunrise is set to year/month/day,0:0:0 and sunset is set to sunrise + 24h - 24h of sun
/// @note for polar night sunrise and sunset both are set to year/month/day,0:0:0 - 0 sec of sun
pair<time_t, time_t> CalculateSunriseSunsetTime(time_t timeUtc, double latitude, double longitude);

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
/// @param timeUtc - UTC time
/// @param latitude - latutude, -90...+90 degrees
/// @param longitude - longitude, -180...+180 degrees
/// @returns pair where first is day time type and second is UTC time when dayt time type is being changed
/// @note for polar day and polar night second parameter when day time type may change
pair<DayTimeType, time_t> GetDayTime(time_t timeUtc, double latitude, double longitude);
