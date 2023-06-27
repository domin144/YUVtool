/*
 * Copyright 2016 Dominik Wójt
 *
 * This file is part of YUVtool.
 *
 * YUVtool is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * YUVtool is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with YUVtool.  If not, see <http://www.gnu.org/licenses/>.
 *
 */
#ifndef CACHE_H
#define CACHE_H

#include <yuv/trees_and_heaps.h>

#include <limits>
#include <map>
#include <numeric>
#include <optional>

namespace YUV_tool {
/*----------------------------------------------------------------------------*/
/* Cache class.
 *
 * Manages set of pointers to TData. Cache holds 0 to cache_size pointers at
 * any given time.
 *
 * Pointer not referenced for the most iterations is removed first. Pointers are
 * referenced by push, get_and_update and update_key. Pointer can be retrieved
 * without registering a reference using get function.
 *
 * Pointer push, pop, reference and get have logarithmic complexity in
 * the number of pointers being held. */
/*----------------------------------------------------------------------------*/
template<typename TKey, typename TData>
class Cache
{
public:
    using Key = TKey;
    using Data = TData;
    using Time = int;

private:
    struct Slot
    {
        Index m_index_in_heap = -1;
        Time m_last_reference = 0;
        Data m_data{};
        Key m_key{};
    };

    struct Heap_element
    {
        Slot *m_slot;

        /* comparison by time of last reference */
        bool operator<(const Heap_element& rhs) const
        {
            return m_slot->m_last_reference < rhs.m_slot->m_last_reference;
        }

        bool operator>(const Heap_element& rhs) const
        {
            return rhs < *this;
        }

        friend void swap(Heap_element &lhs, Heap_element &rhs)
        {
            std::swap(lhs.m_slot, rhs.m_slot);
            std::swap(lhs.m_slot->m_index_in_heap, rhs.m_slot->m_index_in_heap);
        }
    };

private:
    std::vector<Slot> m_slots;
    std::map<Key, Slot*> m_map;
    Heap<Heap_element, std::greater<Heap_element>> m_heap;
    std::vector<Slot*> m_free_slots;
    Time m_time;

public:
    Cache(const Index cache_size) :
        m_slots(cache_size),
        m_time(0)
    {
        m_heap.reserve(cache_size);

        m_free_slots.reserve(cache_size);
        for(Slot &slot : m_slots)
            m_free_slots.push_back(&slot);
    }

    Index get_cache_size() const
    {
        return m_slots.size();
    }

    Index get_used_slots_count() const
    {
        return m_heap.get_size();
    }

    std::optional<std::reference_wrapper<Data>> get(const Key &key)
    {
        return get_or_update(key, false);
    }

    void update_key(const Key &key)
    {
        Data *data = get_or_update(key, true);
        my_assert(data, "trying to update absent key");
    }

    /* more efficient if done at once */
    std::optional<std::reference_wrapper<Data>> get_and_update(const Key &key)
    {
        return get_or_update(key, true);
    }

    Data pop()
    {
        my_assert(m_map.size() != 0, "trying to pop empty cache");
        const Heap_element heap_element = m_heap.pop();
        Slot *const slot = heap_element.m_slot;
        my_assert(
                    slot->m_index_in_heap == m_heap.get_size(),
                    "invalid index in heap");

        /* remove key from the map */
        const Key key = slot->m_key;
        m_map.erase(key);

        Data result = std::move(slot->m_data);

        /* clear the values in slot */
        *slot = Slot();
        m_free_slots.push_back(slot);

        const Index map_size = m_map.size();
        my_assert(map_size == m_heap.get_size(), "heap - map size mismatch");

        return std::move(result);
    }

    void push(const TKey &key, Data &&data)
    {
        if(m_map.find(key) != m_map.end())
        {
            my_assert(false, "trying to push data already present in cache");
        }
        else
        {
            const Index current_size = m_map.size();
            my_assert(
                        current_size < get_cache_size(),
                        "trying to push more data than cache can hold");

            Slot *const slot = m_free_slots.back();
            m_free_slots.pop_back();

            m_map[key] = slot;

            slot->m_data = std::move(data);
            slot->m_key = key;

            slot->m_index_in_heap = m_heap.get_size();
            Heap_element heap_element;
            heap_element.m_slot = slot;
            m_heap.push(heap_element);

            update(slot);
        }
    }

    bool is_full() const
    {
        return m_free_slots.empty();
    }

    bool is_empty() const
    {
        return m_heap.is_empty();
    }

private:
    std::optional<std::reference_wrapper<Data>> get_or_update(
        const Key& key, const bool do_update)
    {
        const auto iter = m_map.find(key);
        if(iter != m_map.end())
        {
            Slot *slot = iter->second;
            if(do_update)
                update(slot);
            return slot->m_data;
        }
        else
        {
            return std::nullopt;
        }
    }

    void update(Slot *slot)
    {
        slot->m_last_reference = m_time;
        m_heap.update(slot->m_index_in_heap);
        increment_time();
    }

    void increment_time()
    {
        if(m_time == std::numeric_limits<Time>::max())
        {
            struct Ordering_element {
                Index index;
                Time last_reference;
            };
            std::vector<Ordering_element> slot_order {m_slots.size()};
            std::transform(
                m_slots.begin(),
                m_slots.end(),
                slot_order.begin(),
                [i {Index {0}}](const auto& slot) mutable {
                    const Ordering_element result {i, slot.m_last_reference};
                    ++i;
                    return result;
                });

            std::sort(
                slot_order.begin(),
                slot_order.end(),
                [](const auto& lhs, const auto& rhs) {
                    return lhs.last_reference < rhs.last_reference;
                });

            m_time = 0;
            std::for_each(
                slot_order.begin(),
                slot_order.end(),
                [this](const auto& order) {
                    m_slots[order.index].m_last_reference = m_time;
                    ++m_time;
                });
        }
        else
        {
            ++m_time;
        }
    }
};
/*----------------------------------------------------------------------------*/
} /* namespace YUV_tool */

#endif /* CACHE_H */
