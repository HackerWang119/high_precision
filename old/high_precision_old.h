#pragma once
#ifndef HIGH_PRECISION_H
#define HIGH_PRECISION_H

#include <string>
#include <iostream>
#include <fstream>
namespace hacker_wang{
    class int_hp{
    private:
        std::string digit; //the first is the symbol
        friend bool align(int_hp& a,int_hp& b);
        friend bool align(int_hp& a,const size_t& n);
        friend bool trim(int_hp& a);
        friend bool cmp_abs(const int_hp& a,const int_hp& b);
    public:
        int_hp();
        int_hp(const long long n);
        size_t size() const;
        char& operator[](const size_t& n);
        const char& operator[](const size_t& n) const;
        int_hp& operator++();
        int_hp operator++(int);
        int_hp& operator--();
        int_hp operator--(int);
        friend int_hp abs(const int_hp a);
        friend std::istream& operator>>(std::istream& is,int_hp& hp);
        friend std::ostream& operator<<(std::ostream& os,const int_hp& hp);
        friend std::ifstream& operator>>(std::ifstream& is,int_hp& hp);
        friend std::ofstream& operator<<(std::ofstream& os,const int_hp& hp);
        friend bool operator==(const int_hp& a,const int_hp& b);
        friend bool operator!=(const int_hp& a,const int_hp& b);
        friend bool operator>(const int_hp& a,const int_hp& b);
        friend bool operator<(const int_hp& a,const int_hp& b);
        friend bool operator>=(const int_hp& a,const int_hp& b);
        friend bool operator<=(const int_hp& a,const int_hp& b);
        friend int_hp operator+(const int_hp a,const int_hp b);
        friend int_hp operator-(const int_hp a);
        friend int_hp operator-(const int_hp a,const int_hp b);
        friend int_hp operator*(const int_hp& a,const int_hp& b);
        friend int_hp operator/(const int_hp a,const int_hp b);
        friend int_hp operator%(const int_hp& a,const int_hp& b);
        friend int_hp operator^(const int_hp a,const int_hp b);
        friend int_hp operator+=(int_hp& a,const int_hp& b);
        friend int_hp operator-=(int_hp& a,const int_hp& b);
        friend int_hp operator*=(int_hp& a,const int_hp& b);
        friend int_hp operator/=(int_hp& a,const int_hp& b);
        friend int_hp operator%=(int_hp& a,const int_hp& b);
        friend int_hp operator^=(int_hp& a,const int_hp& b);
    };
}

#endif
