#include "engine.hpp"
#include <iostream>
#include <functional>
#include <vector>
#include <cassert>

struct TestCase {
    const char* name;
    std::function<void()> fn;
};

int main() {
    std::vector<TestCase> tests = {
        {"Test 0: Simple order lookup", []() {
            Orderbook ob;
            assert(!order_exists(ob, 1));
            assert(match_order(ob, Order{1,100,10,Side::SELL}) == 0);
            assert(order_exists(ob, 1));
            auto o = lookup_order_by_id(ob, 1);
            assert(o.id == 1 && o.price == 100 && o.quantity == 10 && o.side == Side::SELL);
        }},
        {"Test 1: Simple match and modify", []() {
            Orderbook ob;
            assert(match_order(ob, Order{1,100,10,Side::SELL}) == 0);
            assert(match_order(ob, Order{2,100,5,Side::BUY}) == 1);
            assert(lookup_order_by_id(ob,1).quantity == 5);
            modify_order_by_id(ob,1,0);
            assert(!order_exists(ob,1));
        }},
        {"Test 2: Multiple matches across price levels", []() {
            Orderbook ob;
            match_order(ob, Order{3,90,5,Side::SELL});
            match_order(ob, Order{4,95,5,Side::SELL});
            assert(match_order(ob, Order{5,100,8,Side::BUY}) == 2);
            assert(lookup_order_by_id(ob,4).quantity == 2);
            modify_order_by_id(ob,4,1);
            assert(lookup_order_by_id(ob,4).quantity == 1);
            modify_order_by_id(ob,4,0);
            assert(!order_exists(ob,4));
        }},
        {"Test 3: Sell order matching buy orders", []() {
            Orderbook ob;
            match_order(ob, Order{6,100,10,Side::BUY});
            assert(match_order(ob, Order{7,100,4,Side::SELL}) == 1);
            assert(lookup_order_by_id(ob,6).quantity == 6);
            assert(match_order(ob, Order{8,90,7,Side::SELL}) == 1);
            assert(!order_exists(ob,6));
            assert(lookup_order_by_id(ob,8).quantity == 1);
        }},
        {"Test 4: Full fill buy order exact match", []() {
            Orderbook ob;
            match_order(ob, Order{20,100,10,Side::SELL});
            assert(match_order(ob, Order{21,100,10,Side::BUY}) == 1);
            assert(!order_exists(ob,20));
        }},
        {"Test 5: Partial fill buy across multiple sells", []() {
            Orderbook ob;
            match_order(ob, Order{22,95,4,Side::SELL});
            match_order(ob, Order{23,100,6,Side::SELL});
            assert(match_order(ob, Order{24,100,8,Side::BUY}) == 2);
            assert(lookup_order_by_id(ob,23).quantity == 2);
        }},
        {"Test 6: Modify nonexistent order", []() {
            Orderbook ob;
            match_order(ob, Order{25,100,10,Side::BUY});
            modify_order_by_id(ob,999,0);
            assert(order_exists(ob,25));
        }},
        {"Test 7: Partial modification", []() {
            Orderbook ob;
            match_order(ob, Order{26,100,10,Side::SELL});
            assert(lookup_order_by_id(ob,26).quantity == 10);
            modify_order_by_id(ob,26,1);
            assert(lookup_order_by_id(ob,26).quantity == 1);
            modify_order_by_id(ob,26,0);
            assert(!order_exists(ob,26));
        }},
        {"Test 8: Partial fill sell across multiple buys", []() {
            Orderbook ob;
            match_order(ob, Order{27,100,5,Side::BUY});
            match_order(ob, Order{28,95,5,Side::BUY});
            assert(match_order(ob, Order{29,90,7,Side::SELL}) == 2);
            assert(lookup_order_by_id(ob,28).quantity == 3);
            assert(!order_exists(ob,27));
        }},
        {"Test 9: Exact price mismatch no fill", []() {
            Orderbook ob;
            match_order(ob, Order{30,105,5,Side::SELL});
            match_order(ob, Order{31,100,5,Side::BUY});
            assert(order_exists(ob,30) && order_exists(ob,31));
        }},
        {"Test 10: Multiple partial fills same level", []() {
            Orderbook ob;
            match_order(ob, Order{32,100,4,Side::SELL});
            match_order(ob, Order{33,100,6,Side::SELL});
            assert(match_order(ob, Order{34,100,8,Side::BUY}) == 2);
            assert(!order_exists(ob,32));
            assert(lookup_order_by_id(ob,33).quantity == 2);
        }},
        {"Test 11: Integrity after multiple ops", []() {
            Orderbook ob;
            match_order(ob, Order{35,100,10,Side::BUY});
            match_order(ob, Order{36,100,5,Side::SELL});
            assert(lookup_order_by_id(ob,35).quantity == 5);
            match_order(ob, Order{37,95,3,Side::SELL});
            assert(lookup_order_by_id(ob,35).quantity == 2);
            modify_order_by_id(ob,35,0);
            match_order(ob, Order{38,100,2,Side::SELL});
            assert(order_exists(ob,38));
        }},
        {"Test 12: FIFO ordering", []() {
            Orderbook ob;
            match_order(ob, Order{39,100,5,Side::BUY});
            match_order(ob, Order{40,100,5,Side::BUY});
            assert(match_order(ob, Order{41,95,3,Side::SELL}) == 1);
            assert(lookup_order_by_id(ob,39).quantity == 2);
        }},
        {"Test 13: Full match sell exact", []() {
            Orderbook ob;
            match_order(ob, Order{42,100,10,Side::BUY});
            assert(match_order(ob, Order{43,100,10,Side::SELL}) == 1);
            assert(!order_exists(ob,42));
        }},
        {"Test 14: Modify no change", []() {
            Orderbook ob;
            match_order(ob, Order{50,100,10,Side::SELL});
            modify_order_by_id(ob,50,10);
            assert(lookup_order_by_id(ob,50).quantity == 10);
        }},
        {"Test 15: Modify after partial fill", []() {
            Orderbook ob;
            match_order(ob, Order{51,100,10,Side::BUY});
            match_order(ob, Order{52,100,4,Side::SELL});
            assert(lookup_order_by_id(ob,51).quantity == 6);
            modify_order_by_id(ob,51,3);
            assert(lookup_order_by_id(ob,51).quantity == 3);
            assert(match_order(ob, Order{53,90,3,Side::SELL}) == 1);
            assert(!order_exists(ob,51));
        }},
        {"Test 16: Modify preserves FIFO", []() {
            Orderbook ob;
            match_order(ob, Order{54,100,5,Side::SELL});
            match_order(ob, Order{55,100,5,Side::SELL});
            modify_order_by_id(ob,54,3);
            assert(match_order(ob, Order{56,100,4,Side::BUY}) == 2);
            assert(!order_exists(ob,54));
            assert(lookup_order_by_id(ob,55).quantity == 4);
        }},
        {"Test 17: Multiple modifications", []() {
            Orderbook ob;
            match_order(ob, Order{57,100,12,Side::BUY});
            modify_order_by_id(ob,57,8);
            assert(lookup_order_by_id(ob,57).quantity == 8);
            modify_order_by_id(ob,57,5);
            assert(lookup_order_by_id(ob,57).quantity == 5);
            assert(match_order(ob, Order{58,100,5,Side::SELL}) == 1);
            assert(!order_exists(ob,57));
        }},
        {"Test 18: Modify zero removes", []() {
            Orderbook ob;
            match_order(ob, Order{60,100,10,Side::BUY});
            modify_order_by_id(ob,60,0);
            assert(!order_exists(ob,60));
        }},
        {"Test 19: Volume no orders", []() {
            Orderbook ob;
            assert(get_volume_at_level(ob,Side::BUY,100) == 0);
            assert(get_volume_at_level(ob,Side::SELL,100) == 0);
        }},
        {"Test 20: Volume single order", []() {
            Orderbook ob;
            match_order(ob, Order{100,100,10,Side::SELL});
            assert(get_volume_at_level(ob,Side::SELL,100) == 10);
            assert(get_volume_at_level(ob,Side::BUY,100) == 0);
        }},
        {"Test 21: Volume multiple same level", []() {
            Orderbook ob;
            match_order(ob, Order{101,100,5,Side::SELL});
            match_order(ob, Order{102,100,7,Side::SELL});
            assert(get_volume_at_level(ob,Side::SELL,100) == 12);
        }},
        {"Test 22: Volume different levels", []() {
            Orderbook ob;
            match_order(ob, Order{103,100,10,Side::BUY});
            match_order(ob, Order{104,101,5,Side::BUY});
            assert(get_volume_at_level(ob,Side::BUY,100) == 10);
            assert(get_volume_at_level(ob,Side::BUY,101) == 5);
        }},
        {"Test 23: Volume after partial fill", []() {
            Orderbook ob;
            match_order(ob, Order{105,100,10,Side::SELL});
            match_order(ob, Order{106,100,4,Side::BUY});
            assert(get_volume_at_level(ob,Side::SELL,100) == 6);
        }},
        {"Test 24: Volume after cancellation", []() {
            Orderbook ob;
            match_order(ob, Order{107,100,10,Side::BUY});
            modify_order_by_id(ob,107,0);
            assert(get_volume_at_level(ob,Side::BUY,100) == 0);
        }},
        {"Test 25: Complex sell modifications", []() {
            Orderbook ob;
            match_order(ob, Order{200,100,10,Side::SELL});
            match_order(ob, Order{201,100,20,Side::SELL});
            match_order(ob, Order{202,101,15,Side::SELL});
            modify_order_by_id(ob,200,5);
            assert(get_volume_at_level(ob,Side::SELL,100) == 25);
            assert(get_volume_at_level(ob,Side::SELL,101) == 15);
        }},
        {"Test 26: Complex buy scenario", []() {
            Orderbook ob;
            match_order(ob, Order{300,100,20,Side::BUY});
            match_order(ob, Order{301,100,10,Side::BUY});
            assert(get_volume_at_level(ob,Side::BUY,100) == 30);
            match_order(ob, Order{302,100,15,Side::SELL});
            assert(get_volume_at_level(ob,Side::BUY,100) == 15);
            modify_order_by_id(ob,301,5);
            assert(get_volume_at_level(ob,Side::BUY,100) == 10);
        }},
        {"Test 27: Complex sell side scenario", []() {
            Orderbook ob;
            match_order(ob, Order{400,100,30,Side::SELL});
            match_order(ob, Order{401,100,20,Side::SELL});
            assert(get_volume_at_level(ob,Side::SELL,100) == 50);
            modify_order_by_id(ob,400,0);
            assert(get_volume_at_level(ob,Side::SELL,100) == 20);
            match_order(ob, Order{402,100,15,Side::SELL});
            assert(get_volume_at_level(ob,Side::SELL,100) == 35);
            match_order(ob, Order{403,101,10,Side::SELL});
            assert(get_volume_at_level(ob,Side::SELL,101) == 10);
            modify_order_by_id(ob,401,10);
            assert(get_volume_at_level(ob,Side::SELL,100) == 25);
        }},
        {"Test 28: All-encompassing scenario", []() {
            Orderbook ob;
            match_order(ob, Order{500,100,20,Side::BUY});
            match_order(ob, Order{501,100,15,Side::BUY});
            match_order(ob, Order{502,99,10,Side::BUY});
            match_order(ob, Order{503,100,25,Side::SELL});
            assert(get_volume_at_level(ob,Side::BUY,100) == 10);
            assert(get_volume_at_level(ob,Side::BUY,99) == 10);
            match_order(ob, Order{504,102,30,Side::SELL});
            match_order(ob, Order{505,101,10,Side::SELL});
            modify_order_by_id(ob,502,5);
            modify_order_by_id(ob,504,20);
            match_order(ob, Order{506,102,15,Side::BUY});
            assert(get_volume_at_level(ob,Side::BUY,100) == 10);
            assert(get_volume_at_level(ob,Side::BUY,99) == 5);
            assert(get_volume_at_level(ob,Side::SELL,102) == 15);
            assert(get_volume_at_level(ob,Side::SELL,101) == 0);
        }}
    };

    int passed = 0;
    for (auto &tc : tests) {
        std::cout << tc.name << " ... ";
        tc.fn();
        std::cout << "OK\n";
        ++passed;
    }
    std::cout << passed << " of " << tests.size() << " tests passed\n";
    return 0;
}