
#include <type_traits>
#include "structure.hpp"

#ifndef __UTIL_OWN
#define __UTIL_OWN

/* Fast Find if Main Contract Changed. */
class ChangeMain {
    const char (*change)[11];
public:

    ChangeMain(const char (*SA_change)[11]): change(SA_change) {}

    bool is_change(const std::string& val) {
        bool flag = (val.size()==11)&&(
            (val[3]==(*change)[3])&&(val[5]==(*change)[5])&&(val[6]==(*change)[6])
                    &&(val[8]==(*change)[8])&&(val[9]==(*change)[9]));
        // time is moving at one direction
        change += (val[3]>(*change)[3])||(val[5]<=(*change)[5])||(val[6]<=(*change)[6])
                                ||(val[8]>(*change)[8])||(val[9]>(*change)[9]);
        return flag;
    }

    std::string get_next_date() {
        return std::string(*change);
    }
};

/* Test time cost of function*/
template <class Fn,class ...Args>
long long time_test(Fn&& func, Args&& ...args) {
    auto start_time = std::chrono::high_resolution_clock::now();
    func(std::forward<Args>(args)...);
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time);
    return duration.count();
}


/* Get Mean Value of numbers*/
inline double mean(const std::vector<double>& v) {
    return std::accumulate(v.begin(), v.end(), 0.0) / v.size();
}

/* Calculate the standard deviation of a vector */
double stddev(const std::vector<double>& v, double mean) {
    double sum = 0.0;
    for(double x : v) {
        double diff = x - mean;
        sum += diff * diff;
    }
    return std::sqrt(sum / (v.size() - 1));
}

template <size_t N>
constexpr double stddev(const double (&v)[N], double mean) {
    double sum = 0.0;
    for(double x : v) {
        double diff = x - mean;
        sum += diff * diff;
    }
    return std::sqrt(sum / (N-1));
}

// Calculate the Sharpe Ratio
double sharpe_ratio(const std::vector<double>& balance, double risk_free_rate, double period_year_count) {
    // Calculate the returns
    // const int return_size = balance.size()-1;
    const int return_size = 15;
    // std::vector<double> _CACHE_ALIGN returns(return_size, 0);
    double returns[15];
    double adjust_val = static_cast<double>(return_size+1)/period_year_count;
    for(int i = 0; i < return_size; i++) {
        returns[i] = ((balance[i+1] - balance[i]) / (balance[i]+20000));
    }

    // Calculate the mean and standard deviation of returns
    double mean_return = balance[return_size]/return_size/20000;
    double stddev_return = stddev(returns, mean_return);

    // Calculate the Sharpe Ratio
    return (mean_return - risk_free_rate) / stddev_return * sqrt(adjust_val);
}


#endif