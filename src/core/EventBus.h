#pragma once
#include <functional>
#include <vector>
#include <typeindex>
#include <unordered_map>
#include <algorithm>
#include <cstdint>

namespace materializr {

class EventBus {
public:
    using SubscriptionId = uint64_t;

    template<typename EventT>
    SubscriptionId subscribe(std::function<void(const EventT&)> handler) {
        auto id = ++m_nextId;
        auto& subs = m_subscribers[std::type_index(typeid(EventT))];
        subs.push_back({id, [handler = std::move(handler)](const void* e) {
            handler(*static_cast<const EventT*>(e));
        }});
        return id;
    }

    void unsubscribe(SubscriptionId id) {
        for (auto& [type, subs] : m_subscribers) {
            subs.erase(
                std::remove_if(subs.begin(), subs.end(),
                    [id](const Sub& s) { return s.id == id; }),
                subs.end());
        }
    }

    template<typename EventT>
    void publish(const EventT& event) {
        auto it = m_subscribers.find(std::type_index(typeid(EventT)));
        if (it == m_subscribers.end()) return;
        // Copy the vector in case a handler modifies subscriptions
        auto snapshot = it->second;
        for (const auto& sub : snapshot) {
            sub.handler(&event);
        }
    }

private:
    struct Sub {
        SubscriptionId id;
        std::function<void(const void*)> handler;
    };
    std::unordered_map<std::type_index, std::vector<Sub>> m_subscribers;
    SubscriptionId m_nextId = 0;
};

} // namespace materializr
