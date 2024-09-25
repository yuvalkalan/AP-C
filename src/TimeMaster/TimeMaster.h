#pragma once
#include <tuple>
const int MONTHS_LEN[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

class Date
{
private:
    int m_year, m_month, m_day;
    int m_start_year, m_start_month, m_start_day;
    int m_month_delta, m_day_delta;

public:
    Date(int year, int month, int day);
    bool is_leap() const;
    int get_day() const;
    int get_month() const;
    int get_year() const;
    Date &operator+=(int other);
    bool is_equal(const Date &date2) const;
    int month_delta() const;
    bool is_bigger(const Date &date2) const;
};

std::tuple<int, int, int, int, int, int, bool> calculate_time_dif(std::tuple<int, int, int, int, int, int> datetime1, std::tuple<int, int, int, int, int, int> datetime2);