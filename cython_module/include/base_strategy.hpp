#ifndef __BASE_STRATEGY__
#define __BASE_STRATEGY__

#include <type_traits>
#include <Python.h>

class BarData;

template <class Child>
class BaseStrategy {
private:
    void _make_order(const BarData& record) noexcept {
        if (!this->pos) {
            if (this->short_sig) {
                this->on_trade(-1, record);
            } else if (this->buy_sig) {
                this->on_trade(1, record);
            }
        }
        else if (this->pos>0) {
            if (this->short_sig) {
                this->on_trade(0, record);
                this->on_trade(-1, record);
            }
            else if (this->sell_sig) {
                this->on_trade(0, record);
            }
        } else {
            if (this->buy_sig) {
                this->on_trade(0, record);
                this->on_trade(1, record);
            } else if (this->cover_sig) {
                this->on_trade(0, record);
            }
        }
    }

    void _manage_pos(const BarData& bar) noexcept {
        this->earn += static_cast<double>(this->pos*20)*(bar.close-this->_last_price);
        this->balance = this->earn-this->fee-this->slip+this->earn_change;
        this->_max_balance = std::max(this->balance, this->_max_balance);
        this->drawdown = std::max(0.0, this->_max_balance-this->balance);
        this->_last_price = bar.close;
        static_cast<Child*>(this)->manage_pos(bar);
    }

public:
    int pos;
    double earn, earn_change, slip;
    double long_close_line, short_close_line;
    double _max_balance, _last_price, balance, fee, drawdown;
    // double pos_high, pos_low;
    bool buy_sig, short_sig, cover_sig, sell_sig, last_sig;

    BaseStrategy():
        _max_balance(0), _last_price (0) {
        pos = earn = 0;
        earn_change = balance = 0;
        slip = fee = drawdown = 0;
        cover_sig=false;
        short_sig=buy_sig = sell_sig=false;
    }

    template <class ...Args>
    void renew(Args && ...args) noexcept {
        _max_balance = _last_price = 0.0;
        pos = 0;
        earn = earn_change = balance = 0.0;
        slip = fee = drawdown = 0.0;
        cover_sig = short_sig= buy_sig = sell_sig=false;
        static_cast<Child*>(this)->set_params(std::forward<Args>(args)...);
        static_cast<Child*>(this)->child_renew();
    }

    // -----------------------
    // User Reload Functions
    // -----------------------

    // renew the child variable.
    // will be called whenever starting a new backtest process.
    // treat it as Construction Function.
    void child_renew() noexcept {}

    void manage_pos(const BarData& bar) noexcept {}

    // You can use this, or just simply define function with same signature in Child to hide this.
    void on_bar(const BarData& record) noexcept{
        this->_manage_pos(record);
        if (static_cast<Child*>(this)->is_domain_change(record)) {
            return static_cast<Child*>(this)->on_domain_change(record);
        }
        static_cast<Child*>(this)->get_sig(record);
        this->_make_order(record);
        static_cast<Child*>(this)->get_next_min_bounds(record);
    }

    void on_trade(int target,const BarData& bar) noexcept {
        int volume = abs(target-this->pos);
        this->fee += volume*8.01;
        this->slip += volume*20;
        this->earn += static_cast<double>(this->pos*20)*(bar.close-this->_last_price);
        this->pos = target;
        static_cast<Child*>(this)->on_trade(target, bar);
    }

    void on_domain_change(const BarData& record) noexcept {
        this->on_trade(0, record);
        this->sell_sig = this->cover_sig = pos>0;
        this->cover_sig = this->sell_sig = pos<0;
    }

    // Check if it is time for Domain Contract Changes.
    bool is_domain_change(const BarData&) {
        return false;
    }

    // Load Sufficient Data Before calling on_bar (Usually for calculate bounds)
    // Return false to stop loading process and proceed to on_bar
    bool loading_data(const BarData&) noexcept {return false;}

    template <class ...Args>
    void set_params(Args ...args) noexcept {}

    template <> // PyListObject * ONLY
    void set_params(PyObject* args) noexcept {}

    void get_next_min_bounds(const BarData& record) noexcept {}

    void get_sig(const BarData& record) noexcept {}


};

#endif