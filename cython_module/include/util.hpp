
#ifndef __UTIL_OWN
#define __UTIL_OWN
#include <type_traits>
#include <chrono>
#include "structure.hpp"



/* Test time cost of function*/
template <class Fn,class ...Args>
long long time_test(Fn&& func, Args&& ...args) {
    auto start_time = std::chrono::high_resolution_clock::now();
    func(std::forward<Args>(args)...);
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time);
    return duration.count();
}

constexpr double sqrtNewtonRaphson(double x, double curr, double prev){
	return curr == prev
			? curr
			: sqrtNewtonRaphson(x, 0.5 * (curr + x / curr), curr);
}

constexpr double sqrt_const(double x)
{
	return x >= 0 && x < std::numeric_limits<double>::infinity()
		? sqrtNewtonRaphson(x, x, 0)
		: std::numeric_limits<double>::quiet_NaN();
}


/* Get Mean Value of numbers*/
constexpr double mean(const std::vector<double>& v) {
    return std::accumulate(v.begin(), v.end(), 0.0) / v.size();
}

/* Calculate the standard deviation of a vector */
constexpr double stddev(const std::vector<double>& v, double mean) {
    double sum = 0.0;
    for(double x : v) {
        double diff = x - mean;
        sum += diff * diff;
    }
    return sqrt_const(sum / (v.size() - 1));
}

template <size_t N>
constexpr double stddev(const double (&v)[N], double mean) {
    double sum = 0.0;
    for(double x : v) {
        double diff = x - mean;
        sum += diff * diff;
    }
    return sqrt_const(sum / (N-1));
}

// Calculate the Sharpe Ratio
constexpr double sharpe_ratio(const std::vector<double>& balance, double risk_free_rate, double period_year_count) {
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
    return (mean_return - risk_free_rate) / stddev_return * sqrt_const(adjust_val);
}

#endif