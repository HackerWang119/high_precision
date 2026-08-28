#include "high_precision.h"
#include <algorithm>

namespace hacker_wang {
    bool align(int_hp& a,int_hp& b){
        if(a.size()==b.size())
            return false;
        else{
            while(a.size()>b.size())
                b.digit.push_back('0');
            while(b.size()>a.size())
                a.digit.push_back('0');
            return true;
        }
    }

    bool align(int_hp& a,const size_t& n){
        if(a.size()-1>=n)
            return false;
        else{
            while(a.size()-1<n)
                a.digit.push_back('0');
            return true;
        }
    }

    bool trim(int_hp& a){
        if(*(a.digit.end()-1)!='0')
            return false;
        else{
            auto i=a.digit.end()-1;
            while(*i=='0' && i!=a.digit.begin()+1)
                i--;
            a.digit.erase(i+1,a.digit.end());
            return true;
        }
    }

    bool cmp_abs(const int_hp& a,const int_hp& b){
        if(a.size()>b.size())
            return true;
        else if(a.size()<b.size())
            return false;
        else{
            unsigned long long i=a.size()-1;
            while(a[i]==b[i] && i>0)
                i--;
            return a[i]>b[i];
        }
    }

    int_hp::int_hp(){
        digit="00";
    }

    int_hp::int_hp(long long n){
        digit.reserve(20);
        if(n<0){
            digit.push_back('1');
            n=-n;
        }else
            digit.push_back('0');
        do{
            digit.push_back('0'+(n%10));
            n/=10;
        }while(n>0);
    }

    size_t int_hp::size() const{
        return digit.size();
    }

    char& int_hp::operator[](const size_t& n){
        return digit[n];
    }

    const char& int_hp::operator[](const size_t& n) const {
        return digit[n];
    }

    int_hp& int_hp::operator++() {
    *this+=1;
    return *this;
    }

    int_hp int_hp::operator++(int) {
        int_hp temp=*this;
        ++(*this);
        return temp;
    }

    int_hp& int_hp::operator--() {
        *this-=1;
        return *this;
    }

    int_hp int_hp::operator--(int) {
        int_hp temp=*this;
        --(*this);
        return temp;
    }

    int_hp abs(int_hp a){
        a[0]='0';
        return a;
    }

    std::istream& operator>>(std::istream& is,int_hp& hp){
        std::string temp;
        is>>temp;
        if(temp[0]=='-'){
            hp.digit="1";
            hp.digit.append(temp.rbegin(),temp.rend()-1);
        }else if(temp[0]=='+'){
            hp.digit="0";
            hp.digit.append(temp.rbegin(),temp.rend()-1);
        }else{
            hp.digit="0";
            hp.digit.append(temp.rbegin(),temp.rend());
        }
        return is;
    }

    std::ostream& operator<<(std::ostream& os,const int_hp& hp){
        std::string temp;
        if(hp[0]=='1')
            temp.push_back('-');
        temp.append(hp.digit.rbegin(),hp.digit.rend()-1);
        return (os<<temp);
    }

    std::ifstream& operator>>(std::ifstream& is,int_hp& hp){
        std::string temp;
        is>>temp;
        if(temp[0]=='-'){
            hp.digit="1";
            hp.digit.append(temp.rbegin(),temp.rend()-1);
        }else if(temp[0]=='+'){
            hp.digit="0";
            hp.digit.append(temp.rbegin(),temp.rend()-1);
        }else{
            hp.digit="0";
            hp.digit.append(temp.rbegin(),temp.rend());
        }
        return is;
    }

    std::ofstream& operator<<(std::ofstream& os,const int_hp& hp){
        std::string temp;
        if(hp[0]=='1')
            temp.push_back('-');
        temp.append(hp.digit.rbegin(),hp.digit.rend()-1);
        os<<temp;
        return os;
    }

    bool operator==(const int_hp& a,const int_hp& b){
        return a.digit==b.digit;
    }

    bool operator!=(const int_hp& a,const int_hp& b){
        return !(a==b);
    }

    bool operator>(const int_hp& a,const int_hp& b){  //可以改进，使得最多经过2次if判断
        if(a[0]=='0')
            if(b[0]=='1')
                return true;
            else
                return cmp_abs(a,b);
        if(a[0]=='1')
            if(b[0]=='0')
                return false;
            else
                return !cmp_abs(a,b);
    }

    bool operator<(const int_hp& a,const int_hp& b){
        return b>a;
    }

    bool operator>=(const int_hp& a,const int_hp& b){
        return !(a<b);
    }

    bool operator<=(const int_hp& a,const int_hp& b){
        return !(a>b);
    }

    int_hp operator+(int_hp a,int_hp b){
         if(a[0]==b[0]){
            int_hp temp;
            char carrier='0';
            align(a,b);
            align(temp,a);
            temp[0]=a[0];
            for(unsigned long long i=1;i<a.size();i++){
                temp[i]='0'+((a[i]-'0')+(b[i]-'0')+(carrier-'0'))%10;
                carrier='0'+((a[i]-'0')+(b[i]-'0')+(carrier-'0'))/10;
            }
            if(carrier!='0')
                temp.digit.push_back(carrier);
            return temp;
        }else if(a[0]=='1')
            return b-(-a);
        else
            return a-(-b);
    }

    int_hp operator-(int_hp a){
        a[0]='0'+!(bool(a[0]-'0'));
        return a;
    }

    int_hp operator-(int_hp a,int_hp b){
        if(b[0]=='1')
            return a+(-b);
        else if(a[0]=='1')
            return b-(-a);
        else if(a<b)
            return -(b-a);
        else{
            int_hp temp;
            char borrower='0';
            align(a,b);
            align(temp,a);
            for(size_t i=1;i<a.size();i++){
                if(a[i]-borrower < b[i]-'0'){
                    temp[i]='0'+(10+(a[i]-borrower)-(b[i]-'0'));
                    borrower='1';
                }else{
                    temp[i]='0'+((a[i]-borrower)-(b[i]-'0'));
                    borrower='0';
                }
            }
            trim(temp);
            return temp;
        }
    }

    int_hp operator*(const int_hp& a,const int_hp& b){
        if(a==0 || b==0)
            return 0;
        int_hp temp;
        align(temp,a.size()+b.size()-2);
        short* sum=new short[a.size()+b.size()-1] {};
        for(size_t i=1;i<a.size();i++)
            for(size_t j=1;j<b.size();j++)
                sum[i+j-2]+=(a[i]-'0')*(b[j]-'0');
        for(size_t i=0;i<a.size()+b.size()-2;i++){
            sum[i+1]+=sum[i]/10;
            temp[i+1]='0'+sum[i]%10;
        }
        if(sum[a.size()+b.size()-2] !=0)
            temp.digit.push_back('0' + sum[a.size()+b.size()-2]);
        delete[] sum;
        if(a[0]!=b[0])
            temp[0]='1';
        trim(temp);
        return temp;
    }

    int_hp operator/(int_hp a,int_hp b){
        if(b==0)
            throw std::domain_error("Divison by zero");
        if(a==0 || cmp_abs(b,a))
            return 0;
        bool sign=(a[0] != b[0]);
        a=abs(a);
        b=abs(b);
        int_hp remainder,ans;
        std::string result;
        for(size_t i=a.size()-1;i>0;i--){
            remainder=remainder*10+(a[i]-'0');
            for(short j=9;j>=0;j--)
            if(!(remainder < b*j)){
                result.push_back('0'+j);
                remainder-=b*j;
                break;
            }
        }
        ans[0]='0'+sign;
        ans.digit.erase(1);
        ans.digit.append(result.rbegin(),result.rend());
        trim(ans);
        return ans;
    }

    int_hp operator%(const int_hp& a,const int_hp& b){
        return a-(a/b)*b;
    }

    int_hp operator^(int_hp a,int_hp b){
        if(b<0)
            throw std::runtime_error("Negative exponents unsupported");
        if(b==0)
            return 1;
        while(b>1){
            a=a*a;
            b=b-1;
        }
        return a;
    }

    int_hp operator+=(int_hp& a,const int_hp& b){
        return a=a+b;
    }

    int_hp operator-=(int_hp& a,const int_hp& b){
        return a=a-b;
    }

    int_hp operator*=(int_hp& a,const int_hp& b){
        return a=a*b;
    }

    int_hp operator/=(int_hp& a,const int_hp& b){
        return a=a/b;
    }

    int_hp operator%=(int_hp& a,const int_hp& b){
        return a%b;
    }

    int_hp operator^=(int_hp& a,const int_hp& b){
        return a=a^b;
    }
}
