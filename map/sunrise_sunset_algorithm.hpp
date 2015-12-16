#pragma once

#include "std/string.hpp"

enum class DayEventType
{
  Sunrise,
  Sunset
};

string DebugPrint(DayEventType type);

/// Calculates UTC time of a desired event (sunrise or sunset) for a specified date
/// @param year - year, since 0, like 2015
/// @param month - month, 1-jan...12-dec
/// @param day - day of month 1...31
/// @param latitude - latutude, -90...+90 degrees
/// @param longitude - longitude, -180...+180 degrees
/// @param type - type of desired event (sunrise or sunset)
/// @param hour - output param, UTC hour when event happens in specified day
/// @param minute - output param, UTC minute when event happens in specified day
/// @param second - output param, UTC second when event happens in specified day
/// @returns true and fill output if event is exist for specified date, or
/// returns false and does not touch output if event does not exist for specified date.
bool CalculateDayEventTime(int year, int month, int day,
                           double latitude, double longitude,
                           DayEventType type,
                           int & hour, int & minute, int & second);

/// Calculates timestamps of the sunrise and sunset for a specified date
/// @param year - year, since 0, like 2015
/// @param month - month, 1-jan...12-dec
/// @param day - day of month 1...31
/// @param latitude - latutude, -90...+90 degrees
/// @param longitude - longitude, -180...+180 degrees
/// @param sunriseUtc - output param, UTC time of sunrise
/// @param sunsetUtc - output param, UTC time of sunset
/// @returns returns true and fills output if output has been calculated,
/// false and does not touch output if sunrise/sunset cannot be calculated for a specified date
bool CalculateSunriseSunsetTime(int year, int month, int day,
                                double latitude, double longitude,
                                time_t & sunriseUtc, time_t & sunsetUtc);
