#include "sunrise_sunset_algorithm.hpp"

#include "base/math.hpp"
#include "base/timegm.hpp"

namespace
{

// Sun's zenith for sunrise/sunset
//   offical      = 90 degrees 50' = 90,83333333333333 degrees
//   civil        = 96 degrees
//   nautical     = 102 degrees
//   astronomical = 108 degrees
double constexpr kZenith = 96;

inline double NormalizeAngle(double a)
{
  double res = fmod(a, 360.);
  if (res < 0)
    res += 360.;
  return res;
}

inline double NormalizeHour(double h)
{
  double res = fmod(h, 24.);
  if (res < 0)
    res += 24.;
  return res;
}

}  // namespace

string DebugPrint(DayEventType type)
{
  switch (type)
  {
  case DayEventType::Sunrise: return "Sunrise";
  case DayEventType::Sunset: return "Sunset";
  }
  ASSERT(false, ());
  return string();
}

bool CalculateDayEventTime(int year, int month, int day,
                           double latitude, double longitude,
                           DayEventType type,
                           int & hour, int & minute, int & second)
{
  // Source: http://williams.best.vwh.net/sunrise_sunset_algorithm.htm

  // 1. first calculate the day of the year

  double const N1 = floor(275. * month / 9.);
  double const N2 = floor((month + 9.) / 12.);
  double const N3 = (1. + floor((year - 4. * floor(year / 4.) + 2.) / 3.));
  double const N = N1 - (N2 * N3) + day - 30.;

  // 2. convert the longitude to hour value and calculate an approximate time

  double const lngHour = longitude / 15;

  double t = 0;
  switch (type)
  {
  case DayEventType::Sunrise: t = N + ((6 - lngHour) / 24); break;
  case DayEventType::Sunset: t = N + ((18 - lngHour) / 24); break;
  }

  // 3. calculate the Sun's mean anomaly

  double const M = (0.9856 * t) - 3.289;

  // 4. calculate the Sun's true longitude

  double L = M + (1.916 * sin(my::DegToRad(M))) + (0.020 * sin(2 * my::DegToRad(M))) + 282.634;
  // NOTE: L potentially needs to be adjusted into the range [0,360) by adding/subtracting 360
  L = NormalizeAngle(L);

  // 5a. calculate the Sun's right ascension

  double RA = my::RadToDeg( atan(0.91764 * tan(my::DegToRad(L))) );
  // NOTE: RA potentially needs to be adjusted into the range [0,360) by adding/subtracting 360
  RA = NormalizeAngle(RA);

  // 5b. right ascension value needs to be in the same quadrant as L

  double const Lquadrant = (floor( L / 90)) * 90;
  double const RAquadrant = (floor(RA / 90)) * 90;
  RA = RA + (Lquadrant - RAquadrant);

  // 5c. right ascension value needs to be converted into hours

  RA = RA / 15;

  // 6. calculate the Sun's declination

  double sinDec = 0.39782 * sin(my::DegToRad(L));
  double cosDec = cos(asin(sinDec));

  // 7a. calculate the Sun's local hour angle

  double cosH = (cos(my::DegToRad(kZenith)) - (sinDec * sin(my::DegToRad(latitude)))) / (cosDec * cos(my::DegToRad(latitude)));

  switch (type)
  {
  case DayEventType::Sunrise:
    if (cosH > 1)
      return false; // the sun never rises on this location (on the specified date)
    break;
  case DayEventType::Sunset:
    if (cosH < -1)
      return false; // the sun never sets on this location (on the specified date)
    break;
  }

  // 7b. finish calculating H and convert into hours

  double H = 0;
  switch (type)
  {
  case DayEventType::Sunrise: H = 360 - my::RadToDeg(acos(cosH)); break;
  case DayEventType::Sunset: H = my::RadToDeg(acos(cosH)); break;
  }

  H = H / 15;

  // 8. calculate local mean time of rising/setting

  double T = H + RA - (0.06571 * t) - 6.622;

  // 9. adjust back to UTC

  double UT = T - lngHour;
  // NOTE: UT potentially needs to be adjusted into the range [0,24) by adding/subtracting 24
  UT = NormalizeHour(UT);

  // UT - is a hour with fractional part of date year/month/day, in range of [0;24)

  hour = floor(UT); // [0;24)
  minute = floor((UT - hour) * 60); // [0;60)
  second = fmod(floor(UT * 60 * 60) /* number of seconds from 0:0 to UT */, 60); // [0;60)

  return true;
}

bool CalculateSunriseSunsetTime(int year, int month, int day,
                                double latitude, double longitude,
                                time_t & sunriseUtc, time_t & sunsetUtc)
{
  int h, m, s;
  if (!CalculateDayEventTime(year, month, day, latitude, longitude, DayEventType::Sunrise, h, m, s))
    return false;

  tm sunrise = {};
  sunrise.tm_year = year - 1900;
  sunrise.tm_mon = month - 1;
  sunrise.tm_mday = day;
  sunrise.tm_hour = h;
  sunrise.tm_min = m;
  sunrise.tm_sec = s;

  if (!CalculateDayEventTime(year, month, day, latitude, longitude, DayEventType::Sunset, h, m, s))
    return false;

  tm sunset = {};
  sunset.tm_year = year - 1900;
  sunset.tm_mon = month - 1;
  sunset.tm_mday = day;
  sunset.tm_hour = h;
  sunset.tm_min = m;
  sunset.tm_sec = s;

  sunriseUtc = base::TimeGM(sunrise);
  sunsetUtc = base::TimeGM(sunset);
  return true;
}

