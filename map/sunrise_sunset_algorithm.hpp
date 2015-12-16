#pragma once

#include "std/string.hpp"

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

/// Calculates timestamps of the sunrise and sunset for a specified date
/// @param timestampUtc - time in UTC, only year, month and day are interesting
/// @param latitude - latutude, -90...+90 degrees
/// @param longitude - longitude, -180...+180 degrees
/// @param sunriseUtc - output param, UTC time of sunrise
/// @param sunsetUtc - output param, UTC time of sunset
/// @returns returns true and fills output if output has been calculated,
/// false and does not touch output if sunrise/sunset cannot be calculated for a specified date
bool CalculateSunriseSunsetTime(time_t timestampUtc,
                                double latitude, double longitude,
                                time_t & sunriseUtc, time_t & sunsetUtc);
