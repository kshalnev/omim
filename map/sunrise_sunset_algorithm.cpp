#include "sunrise_sunset_algorithm.hpp"

#include "base/assert.hpp"
#include "base/math.hpp"
#include "base/logging.hpp"
#include "base/timegm.hpp"

namespace
{

// Sun's zenith for sunrise/sunset
//   offical      = 90 degrees 50' = 90,83333333333333 degrees
//   civil        = 96 degrees
//   nautical     = 102 degrees
//   astronomical = 108 degrees
double constexpr kZenith = 96;

time_t constexpr kOneDaySeconds = 24 * 60 * 60;

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

inline bool IsLeapYear(int year)
{
  return year % 4 == 0 && (year % 100 != 0 || year % 400 == 0);
}

inline int DaysOfMonth(int year, int month)
{
  ASSERT_GREATER_OR_EQUAL(month, 1, ());
  ASSERT_LESS_OR_EQUAL(month, 12, ());
  int const february = IsLeapYear(year) ? 29 : 28;
  int const daysPerMonth[12] = { 31, february, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
  return daysPerMonth[month - 1];
}

void NextDay(int & year, int & month, int & day)
{
  if (day < DaysOfMonth(year, month))
  {
    ++day;
    return;
  }
  if (month < 12)
  {
    day = 1;
    ++month;
    return;
  }
  day = 1;
  month = 1;
  ++year;
}

void PrevDay(int & year, int & month, int & day)
{
  if (day > 1)
  {
    --day;
    return;
  }
  if (month > 1)
  {
    --month;
    day = DaysOfMonth(year, month);
    return;
  }
  --year;
  month = 12;
  day = 31;
}

enum class DayEventType
{
  Sunrise,
  Sunset,
  PolarDay,
  PolarNight
};

DayEventType CalculateDayEventTime(int year, int month, int day,
                                   double latitude, double longitude,
                                   bool sunrise,
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
  if (sunrise)
    t = N + ((6 - lngHour) / 24);
  else
    t = N + ((18 - lngHour) / 24);

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

  // if cosH > 1 then sun is never rises on this location on specified date (polar night)
  // if cosH < -1 then sun is never sets on this location on specified date (polar day)
  if (cosH < -1 || cosH > 1)
  {
    hour = sunrise ? 0 : 23;
    minute = sunrise ? 0 : 59;
    second = sunrise ? 0 : 59;
    return (cosH < -1) ? DayEventType::PolarDay : DayEventType::PolarNight;
  }

  // 7b. finish calculating H and convert into hours

  double H = 0;
  if (sunrise)
    H = 360 - my::RadToDeg(acos(cosH));
  else
    H = my::RadToDeg(acos(cosH));

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

  return sunrise ? DayEventType::Sunrise : DayEventType::Sunset;
}

DayEventType CalculateDayEventTime(int year, int month, int day,
                                   double latitude, double longitude,
                                   bool sunrise,
                                   time_t & timestampUtc)
{
  int h, m, s;
  DayEventType res = CalculateDayEventTime(year, month, day, latitude, longitude, sunrise, h, m, s);
  timestampUtc = base::TimeGM(year, month, day, h, m, s);
  return res;
}

} // namespace

pair<time_t, time_t> CalculateSunriseSunsetTime(int year, int month, int day,
                                                double latitude, double longitude)
{
  ASSERT_GREATER_OR_EQUAL(month, 1, ());
  ASSERT_LESS_OR_EQUAL(month, 12, ());
  ASSERT_GREATER_OR_EQUAL(day, 1, ());
  ASSERT_LESS_OR_EQUAL(day, DaysOfMonth(year, month), ());

  time_t sunriseUtc, sunsetUtc;
  DayEventType sunriseRes = CalculateDayEventTime(year, month, day, latitude, longitude, true /* sunrise */, sunriseUtc);
  DayEventType sunsetRes = CalculateDayEventTime(year, month, day, latitude, longitude, false /* sunrise */, sunsetUtc);

  // Edge cases: polar day and polar night
  if (sunriseRes == DayEventType::PolarDay || sunsetRes == DayEventType::PolarDay)
  {
    // Polar day: 24 hours of sun
    sunriseUtc = base::TimeGM(year, month, day, 0, 0, 0);
    sunsetUtc = sunriseUtc + kOneDaySeconds;
    return make_pair(sunriseUtc, sunsetUtc);
  }
  else if (sunriseRes == DayEventType::PolarNight || sunsetRes == DayEventType::PolarNight)
  {
    // Polar night: 0 secs of sun
    sunriseUtc = base::TimeGM(year, month, day, 0, 0, 0);
    sunsetUtc = sunriseUtc;
    return make_pair(sunriseUtc, sunsetUtc);
  }

  if (sunsetUtc < sunriseUtc)
  {
    // Process line of date changing.
    // Local date fits to two utc dates (current utc date and previous utc date).

    if (longitude > 0)
    {
      PrevDay(year, month, day);
      CalculateDayEventTime(year, month, day, latitude, longitude, true /* sunrise */, sunriseUtc);
    }
    else if (longitude < 0)
    {
      NextDay(year, month, day);
      CalculateDayEventTime(year, month, day, latitude, longitude, false /* sunrise */, sunsetUtc);
    }
  }

  ASSERT_LESS(sunriseUtc, sunsetUtc, ());
  return make_pair(sunriseUtc, sunsetUtc);
}

pair<time_t, time_t> CalculateSunriseSunsetTime(time_t timeUtc, double latitude, double longitude)
{
  tm const * const t = gmtime(&timeUtc);
  if (nullptr == t)
  {
    LOG(LERROR, ("gmtime has failed for time", timeUtc));
    return make_pair(0, 0);
  }

  auto sunriseSunsetUtc = CalculateSunriseSunsetTime(t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
                                                     latitude, longitude);

  if (sunriseSunsetUtc.second < timeUtc)
  {
    sunriseSunsetUtc.first += kOneDaySeconds;
    sunriseSunsetUtc.second += kOneDaySeconds;
  }

  return sunriseSunsetUtc;
}

pair<DayTimeType, time_t> GetDayTime(time_t timeUtc, double latitude, double longitude)
{
  auto sunriseSunsetUtc = CalculateSunriseSunsetTime(timeUtc, latitude, longitude);

  // Edge cases: polar day and polar night
  if (sunriseSunsetUtc.first == sunriseSunsetUtc.second)
  {
    // Polar night: 0 secs of sun
    return make_pair(DayTimeType::PolarNight, timeUtc + kOneDaySeconds);
  }
  else if (sunriseSunsetUtc.second == (sunriseSunsetUtc.first + kOneDaySeconds))
  {
    // Polar day: 24 hours of sun
    return make_pair(DayTimeType::PolarDay, timeUtc + kOneDaySeconds);
  }

  if (timeUtc < sunriseSunsetUtc.first)
  {
    // (time) (sunrise) (sunset) ---> time axis
    return make_pair(DayTimeType::NightTime, sunriseSunsetUtc.first);
  }
  else if (timeUtc < sunriseSunsetUtc.second)
  {
    // (sunrise) (time) (sunset) ---> time axis
    return make_pair(DayTimeType::DayTime, sunriseSunsetUtc.second);
  }
  else
  {
    // (sunrise) (sunset) (time) | (sunrise) (sunset) ---> time axis
    sunriseSunsetUtc = CalculateSunriseSunsetTime(timeUtc + kOneDaySeconds, latitude, longitude);
    return make_pair(DayTimeType::NightTime, sunriseSunsetUtc.first);
  }
}

string DebugPrint(DayTimeType type)
{
  switch (type)
  {
  case DayTimeType::DayTime: return "DayTime";
  case DayTimeType::NightTime: return "NightTime";
  case DayTimeType::PolarDay: return "PolarDay";
  case DayTimeType::PolarNight: return "PolarNight";
  }
  return string();
}
