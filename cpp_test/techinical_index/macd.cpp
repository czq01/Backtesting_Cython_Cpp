# include <base.hpp>
# include <vector>
# include <string>


BarData get_fake_data(int price, int idx, std::string syb="SA2309.XZCE") {
    std::string datetime="2023-01-01 12:12:12", date="2023-01-01";
    std::string symbol = syb;
    BarData p(symbol.c_str(), datetime.c_str());
    p.close = price+idx*idx;
    p.high = price + 5;
    p.low = price-5;
    p.open = price-1;
    return p;
}

void test_one() {
    ArrayManager<MACD, RSI, BBAND> p (50, {26, 12}, {14}, {40, 2.0});
    for (int i=0; i<60; i++) {
        BarData sr = get_fake_data(2350, i);
        p.update_bar(sr);
        if (p.is_inited("SA2309.XZCE")) {
            auto && [macd, dif, dea] = p.get_index<MACD>("SA2309.XZCE");
            printf("%d, %lf, %lf, %lf\n", i, macd, dif, dea);
            auto r = p.get_param<RSI>();
            // printf("%d\n", r.period);
        }
    }
}



void test_two() {
    ParamsMap<MACD, RSI, BBAND> p (50, {26, 12}, {14}, {40, 12});
    auto t = p.get_val<BBAND>();
    printf("%d\n",t.N);
}

void test_three() {
    IndexManager<MACD, RSI, BBAND> p (50, {26, 12}, {14}, {40, 2.0});
    for (int i=0; i<60; i++) {
        BarData sr = get_fake_data(2350, i);
        p.update_bar(sr);
        if (p.is_inited()) {
            auto && [macd, dif, dea] = p.get_index<BBAND>();
            printf("%d, %lf, %lf, %lf\n", i, macd, dif, dea);
            auto r = p.get_param<RSI>();
            // printf("%d\n", r.period);
        }
    }
}

int main() {
    printf("test1 on several symbols:\n");
    test_one();
    test_two();
    printf("test3 on one symbols:\n");

    test_three();
    return 0;
}