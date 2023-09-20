# include <structure.hpp>
# include <vector>
# include <string>


BarData get_fake_data(int price, int idx, std::string syb="SA2309.XZCE") {
    std::string datetime="2023-01-01 12:12:12", date="2023-01-01";
    std::string symbol = syb;
    BarData p(symbol.c_str());
    p.close = price+(idx*idx);
    p.high = price + 5;
    p.low = price-5;
    p.open = price-1;
    return p;
}

void test_one() {
    ArrayManager p(50);
    std::vector<std::string> symbols{"SA2309.XZCE", "SA2311.XZCE"};
    for (int i=0; i<60; i++) {
        for (auto && symbol: symbols) {
            BarData sr = get_fake_data(2350, i, symbol);
            p.update_bar(sr);
            if (p.is_inited(sr.symbol)) {
                auto std = p.get_std(sr.symbol);
                printf("%s, %d, %lf\n", sr.symbol, i, std);
            }
        }
    }
}


int main() {
    printf("test1 on one symbols:\n");
    test_one();
    return 0;
}