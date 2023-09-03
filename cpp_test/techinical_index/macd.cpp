# include <structure.hpp>
# include <vector>
# include <string>


Series_plus get_fake_data(int price, int idx) {
    std::string datetime="2023-01-01 12:12:12", date="2023-01-01";
    std::string symbol="SA239.XZCE";
    Series_plus p(symbol);
    p.close = price+idx*idx;
    p.high = price + 5;
    p.low = price-5;
    p.open = price-1;
    return p;
}

int main() {
    double a = NAN;
    ArrayManager p(45);
    MACD_Calculator macd_cal(p);
    for (int i=0; i<60; i++) {
        Series_plus sr = get_fake_data(2350, i);
        p.update_bar(sr);
        if (macd_cal.is_inited) {
            printf("%d, %lf, %lf, %lf\n", i, macd_cal.MACD, macd_cal.DIF, macd_cal.DEA);
        }
    }
    return 0;
}