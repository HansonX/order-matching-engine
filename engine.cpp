#include "engine.hpp"

#include <list>
#include <map>
#include <optional>
#include <stdexcept>
#include <cstdint>
#include <algorithm>
#include <functional>
#include <optional>

// Core matching helper: walk through price levels in ordersMap and fill order
// until quantity reaches zero or no more qualifying price levels remain
template <typename OrderMap, typename Condition>
static uint32_t process_orders(Order &order,
                               OrderMap &ordersMap,
                               Condition cond) {
    uint32_t matchCount = 0;
    auto it = ordersMap.begin();
    while (it != ordersMap.end() && order.quantity > 0 && cond(it->first, order.price)) {
        auto &ordersAtPrice = it->second;
        auto listIt = ordersAtPrice.begin();
        while (listIt != ordersAtPrice.end() && order.quantity > 0) {
            Quantity trade = std::min<Quantity>(order.quantity, listIt->quantity);
            order.quantity -= trade;
            listIt->quantity -= trade;
            ++matchCount;

            if (listIt->quantity == 0) {
                listIt = ordersAtPrice.erase(listIt);
            } else {
                ++listIt;
            }
        }
        if (ordersAtPrice.empty()) {
            it = ordersMap.erase(it);
        } else {
            ++it;
        }
    }
    return matchCount;
}

// Modify or cancel an order in one side of the book
template <typename OrderMap>
static bool modify_order_in_map(OrderMap &ordersMap,
                                Id order_id,
                                Quantity new_quantity) {
    for (auto mapIt = ordersMap.begin(); mapIt != ordersMap.end();) {
        auto &orderList = mapIt->second;
        for (auto listIt = orderList.begin(); listIt != orderList.end();) {
            if (listIt->id == order_id) {
                if (new_quantity == 0) {
                    listIt = orderList.erase(listIt);
                } else {
                    listIt->quantity = new_quantity;
                    return true;
                }
            } else {
                ++listIt;
            }
        }
        if (orderList.empty()) {
            mapIt = ordersMap.erase(mapIt);
        } else {
            ++mapIt;
        }
    }
    return false;
}

// Lookup an order by ID in one side of the book
template <typename OrderMap>
static std::optional<Order> lookup_order_in_map(const OrderMap &ordersMap,
                                                Id order_id) {
    for (const auto &[price, orderList] : ordersMap) {
        for (const auto &order : orderList) {
            if (order.id == order_id) {
                return order;
            }
        }
    }
    return std::nullopt;
}

extern "C" {

// Match an incoming order against the book
uint32_t match_order(Orderbook &orderbook, const Order &incoming) {
    Order order = incoming;
    uint32_t matchCount = 0;

    if (order.side == Side::BUY) {
        // match sell orders priced ≤ limit
        matchCount = process_orders(order,
                                    orderbook.sellOrders,
                                    [](Price p, Price limit) { return p <= limit; });
        if (order.quantity > 0) {
            orderbook.buyOrders[order.price].push_back(order);
        }
    } else {
        // match buy orders priced ≥ limit
        matchCount = process_orders(order,
                                    orderbook.buyOrders,
                                    [](Price p, Price limit) { return p >= limit; });
        if (order.quantity > 0) {
            orderbook.sellOrders[order.price].push_back(order);
        }
    }

    return matchCount;
}

// Modify or cancel an existing order by ID
void modify_order_by_id(Orderbook &orderbook,
                        Id order_id,
                        Quantity new_quantity) {
    if (modify_order_in_map(orderbook.buyOrders, order_id, new_quantity)) {
        return;
    }
    modify_order_in_map(orderbook.sellOrders, order_id, new_quantity);
}

// Sum total quantity at a given price level
uint32_t get_volume_at_level(Orderbook &orderbook,
                             Side side,
                             Price price) {
    uint32_t total = 0;
    if (side == Side::BUY) {
        auto &bookSide = orderbook.buyOrders;
        auto it = bookSide.find(price);
        if (it != bookSide.end()) {
            for (const auto &o : it->second) {
                total += o.quantity;
            }
        }
    } else {
        auto &bookSide = orderbook.sellOrders;
        auto it = bookSide.find(price);
        if (it != bookSide.end()) {
            for (const auto &o : it->second) {
                total += o.quantity;
            }
        }
    }
    return total;
}
Order lookup_order_by_id(Orderbook &orderbook, Id order_id) {
    if (auto opt = lookup_order_in_map(orderbook.buyOrders, order_id))  return *opt;
    if (auto opt = lookup_order_in_map(orderbook.sellOrders, order_id)) return *opt;
    throw std::runtime_error("Order not found");
}

bool order_exists(Orderbook &orderbook, Id order_id) {
    return lookup_order_in_map(orderbook.buyOrders, order_id).has_value() ||
           lookup_order_in_map(orderbook.sellOrders, order_id).has_value();
}

Orderbook *create_orderbook() {
    return new Orderbook();
}

}
