#pragma once

#include <cstdint>
#include <list>
#include <map>

enum class Side : uint8_t { BUY, SELL };

using Id = uint32_t;
using Price = uint16_t;
using Quantity = uint16_t;

struct Order {
  Id id; 
  Price price;
  Quantity quantity;
  Side side;
};

struct Orderbook {
  std::map<Price, std::list<Order>, std::greater<Price>> buyOrders;
  std::map<Price, std::list<Order>> sellOrders;
};

extern "C" {

uint32_t match_order(Orderbook &orderbook, const Order &incoming);
void modify_order_by_id(Orderbook &orderbook, Id order_id,
                        Quantity new_quantity);
uint32_t get_volume_at_level(Orderbook &orderbook, Side side,
                             Price quantity);


Order lookup_order_by_id(Orderbook &orderbook, Id order_id);
bool order_exists(Orderbook &orderbook, Id order_id);
Orderbook *create_orderbook();
}
