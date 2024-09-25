#include "TimeMaster.h"

Date::Date(int year, int month, int day)
    : m_year(year), m_month(month - 1), m_day(day - 1), m_start_year(year), m_start_month(month - 1), m_start_day(day - 1),
      m_month_delta(0), m_day_delta(0) {}
bool Date::is_leap() const
{
    return m_year % 4 == 0 && (m_year % 100 != 0 || m_year % 400 == 0);
}
int Date::get_day() const
{
    return m_day + 1;
}
int Date::get_month() const
{
    return m_month + 1;
}
int Date::get_year() const
{
    return m_year;
}
Date &Date::operator+=(int other)
{
    for (int i = 0; i < other; i++)
    {
        m_day += 1;
        if (m_day >= MONTHS_LEN[m_month])
        {
            if (!(m_day == 28 && m_month == 1 && is_leap()))
            {
                m_day = 0;
                m_month += 1;
                m_month_delta += 1;
            }
        }
        if (m_month == 12)
        {
            m_day = 0;
            m_month = 0;
            m_year += 1;
        }
    }
    return *this;
}
bool Date::is_equal(const Date &date2) const
{
    return get_year() == date2.get_year() && get_month() == date2.get_month() && get_day() == date2.get_day();
}
int Date::month_delta() const
{
    return m_month_delta;
}
bool Date::is_bigger(const Date &date2) const
{
    if (get_year() > date2.get_year())
    {
        return true;
    }
    else if (get_year() == date2.get_year())
    {
        if (get_month() > date2.get_month())
        {
            return true;
        }
        else if (get_month() == date2.get_month())
        {
            return get_day() > date2.get_day();
        }
    }
    return false;
}

std::tuple<int, int, int, int, int, int, bool> calculate_time_dif(std::tuple<int, int, int, int, int, int> datetime1, std::tuple<int, int, int, int, int, int> datetime2)
{
    int year1 = std::get<0>(datetime1), month1 = std::get<1>(datetime1), day1 = std::get<2>(datetime1), hour1 = std::get<3>(datetime1), minute1 = std::get<4>(datetime1), second1 = std::get<5>(datetime1);
    int year2 = std::get<0>(datetime2), month2 = std::get<1>(datetime2), day2 = std::get<2>(datetime2), hour2 = std::get<3>(datetime2), minute2 = std::get<4>(datetime2), second2 = std::get<5>(datetime2);

    Date date1(year1, month1, day1);
    Date date2(year2, month2, day2);

    bool is_reverse = false;
    if (date1.is_bigger(date2))
    {
        std::swap(year1, year2);
        std::swap(month1, month2);
        std::swap(day1, day2);
        std::swap(hour1, hour2);
        std::swap(minute1, minute2);
        std::swap(second1, second2);
        date1 = Date(year1, month1, day1);
        date2 = Date(year2, month2, day2);
        is_reverse = true;
    }

    int counter = 0;
    while (!date1.is_equal(date2))
    {
        date1 += 1;
        counter += 1;
    }

    int seconds = second2 - second1;
    int minutes = minute2 - minute1;
    int hours = hour2 - hour1;
    int days = day2 - day1;
    int years = date1.month_delta() / 12, months = date1.month_delta() % 12;

    if (seconds < 0)
    {
        seconds += 60;
        minutes -= 1;
    }
    if (minutes < 0)
    {
        minutes += 60;
        hours -= 1;
    }
    if (hours < 0)
    {
        hours += 24;
        days -= 1;
    }
    if (days < 0)
    {
        days += MONTHS_LEN[date1.get_month() - 1];
        months -= 1;
    }
    if (months < 0)
    {
        months += 12;
        years -= 1;
    }

    if (is_reverse)
    {
        return std::make_tuple(-years, -months, -days, -hours, -minutes, -seconds, is_reverse);
    }
    else
    {
        return std::make_tuple(years, months, days, hours, minutes, seconds, is_reverse);
    }
}