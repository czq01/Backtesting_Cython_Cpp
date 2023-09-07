# include <structure.hpp>
# include <vector>
# include <string>


Series_plus get_fake_data(int price, int idx, std::string syb="SA2309.XZCE") {
    std::string datetime="2023-01-01 12:12:12", date="2023-01-01";
    std::string symbol = syb;
    Series_plus p(symbol);
    p.close = price+idx*idx;
    p.high = price + 5;
    p.low = price-5;
    p.open = price-1;
    return p;
}

void test_one() {
    ArrayManager p(45);
    Calculator_MACD macd_cal(p);
    std::vector<std::string> symbols{"SA2309.XZCE", "SA2311.XZCE"};
    for (int i=0; i<60; i++) {
        for (auto && symbol: symbols) {
            Series_plus sr = get_fake_data(2350, i, symbol);
            p.update_bar(sr);
            if (macd_cal.is_inited(sr.symbol)) {
                auto&&[MACD, DIF, DEA] = macd_cal.get_val(sr.symbol);
                printf("%s, %d, %lf, %lf, %lf\n", sr.symbol.c_str(),i, MACD, DIF, DEA);
            }
        }
    }
}

void test_two() {
    double a = NAN;
    SingleArrayManager p(45);
    SingleCalculator_MACD macd_cal(p);
    for (int i=0; i<60; i++) {
        Series_plus sr = get_fake_data(2350, i);
        p.update_bar(sr);
        if (macd_cal.is_inited) {
            auto&&[MACD, DIF, DEA] = macd_cal.get_val();
            printf("%d, %lf, %lf, %lf\n", i, MACD, DIF, DEA);
        }
    }
}


int main() {
    printf("test1 on one symbols:\n");
    test_one();
    printf("test2 on Several symbols:\n");
    test_two();
    return 0;
}