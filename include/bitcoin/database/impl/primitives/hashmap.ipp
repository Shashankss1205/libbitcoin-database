/**
 * Copyright (c) 2011-2023 libbitcoin developers (see AUTHORS)
 *
 * This file is part of libbitcoin.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef LIBBITCOIN_DATABASE_PRIMITIVES_HASHMAP_IPP
#define LIBBITCOIN_DATABASE_PRIMITIVES_HASHMAP_IPP

#include <algorithm>
#include <bitcoin/system.hpp>
#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {

TEMPLATE
CLASS::hashmap(storage& header, storage& body, const Link& buckets) NOEXCEPT
  : head_(header, buckets), manager_(body)
{
}

// not thread safe
// ----------------------------------------------------------------------------

TEMPLATE
bool CLASS::create() NOEXCEPT
{
    Link count{};
    return head_.create() && head_.get_body_count(count) &&
        manager_.truncate(count);
}

TEMPLATE
bool CLASS::close() NOEXCEPT
{
    return head_.set_body_count(manager_.count());
}

TEMPLATE
bool CLASS::backup() NOEXCEPT
{
    return head_.set_body_count(manager_.count());
}

TEMPLATE
bool CLASS::restore() NOEXCEPT
{
    Link count{};
    return head_.verify() && head_.get_body_count(count) &&
        manager_.truncate(count);
}

TEMPLATE
bool CLASS::verify() const NOEXCEPT
{
    Link count{};
    return head_.verify() && head_.get_body_count(count) &&
        (count == manager_.count());
}

// sizing
// ----------------------------------------------------------------------------

TEMPLATE
bool CLASS::enabled() const NOEXCEPT
{
    return head_.buckets() > one;
}

TEMPLATE
size_t CLASS::buckets() const NOEXCEPT
{
    return head_.buckets();
}

TEMPLATE
size_t CLASS::head_size() const NOEXCEPT
{
    return head_.size();
}

TEMPLATE
size_t CLASS::body_size() const NOEXCEPT
{
    return manager_.size();
}

TEMPLATE
Link CLASS::count() const NOEXCEPT
{
    return manager_.count();
}

// query interface
// ----------------------------------------------------------------------------

TEMPLATE
code CLASS::get_fault() const NOEXCEPT
{
    return manager_.get_fault();
}

TEMPLATE
size_t CLASS::get_space() const NOEXCEPT
{
    return manager_.get_space();
}

TEMPLATE
code CLASS::reload() NOEXCEPT
{
    return manager_.reload();
}

// query interface
// ----------------------------------------------------------------------------

TEMPLATE
Link CLASS::top(const Link& link) const NOEXCEPT
{
    if (link >= head_.buckets())
        return {};

    return head_.top(link);
}

TEMPLATE
bool CLASS::exists(const Key& key) const NOEXCEPT
{
    return !first(key).is_terminal();
}

TEMPLATE
Link CLASS::first(const Key& key) const NOEXCEPT
{
    // This denormalization below is a clone of iterator::to_match(link).
    // The significant performance benefit from avoiding iterator construct.
    ////return it(key).self();

    const auto ptr = manager_.get();
    if (!ptr)
        return {};

    auto link = head_.top(key);
    while (!link.is_terminal())
    {
        // get element offset (fault)
        const auto offset = ptr->offset(iterator::link_to_position(link));
        if (is_null(offset))
            return {};

        // element key matches (found)
        const auto key_ptr = std::next(offset, Link::size);
        if (is_zero(std::memcmp(key.data(), key_ptr, array_count<Key>)))
            return link;

        // set next element link (loop)
        link = system::unsafe_array_cast<uint8_t, Link::size>(offset);
    }

    return link;
}

TEMPLATE
typename CLASS::iterator CLASS::it(const Key& key) const NOEXCEPT
{
    // Expensive, dropped from 27% to 1% from first() removal and head optimization.
    // key is passed and retained by reference, origin must remain in scope.
    return { manager_.get(), head_.top(key), key };
}

TEMPLATE
Link CLASS::allocate(const Link& size) NOEXCEPT
{
    return manager_.allocate(size);
}

TEMPLATE
Key CLASS::get_key(const Link& link) NOEXCEPT
{
    const auto ptr = manager_.get(link);
    if (!ptr || system::is_lesser(ptr->size(), index_size))
        return {};

    return system::unsafe_array_cast<uint8_t, array_count<Key>>(std::next(
        ptr->begin(), Link::size));
}

TEMPLATE
template <typename Element, if_equal<Element::size, Size>>
bool CLASS::get(const Link& link, Element& element) const NOEXCEPT
{
    using namespace system;
    const auto ptr = manager_.get(link);
    if (!ptr)
        return false;

    iostream stream{ *ptr };
    reader source{ stream };
    source.skip_bytes(index_size);

    if constexpr (!is_slab) { source.set_limit(Size); }
    return element.from_data(source);
}

TEMPLATE
template <typename Element, if_equal<Element::size, Size>>
bool CLASS::get(const iterator& it, Element& element) const NOEXCEPT
{
    using namespace system;
    const auto ptr = it.get();
    if (!ptr)
        return false;

    // This denormalization is a clone of hashmap::get(link, element).
    // The significant performance benefit from avoiding mempry_ptr construct.
    // This is also neccessary to avoid deadlock when holding an iterator.
    const auto buffer = ptr->offset(iterator::link_to_position(it.self()));

    iostream stream{ buffer, buffer_size };
    reader source{ stream };
    source.skip_bytes(index_size);

    if constexpr (!is_slab) { source.set_limit(Size); }
    return element.from_data(source);
}

TEMPLATE
template <typename Element, if_equal<Element::size, Size>>
bool CLASS::set(const Link& link, const Element& element) NOEXCEPT
{
    using namespace system;
    const auto ptr = manager_.get(link);
    if (!ptr)
        return false;

    iostream stream{ *ptr };
    finalizer sink{ stream };
    sink.skip_bytes(index_size);

    if constexpr (!is_slab) { sink.set_limit(Size); }
    return element.to_data(sink);
}

TEMPLATE
template <typename Element, if_equal<Element::size, Size>>
Link CLASS::set_link(const Element& element) NOEXCEPT
{
    Link link{};
    if (!set_link(link, element))
        return {};
    
    return link;
}

TEMPLATE
template <typename Element, if_equal<Element::size, Size>>
bool CLASS::set_link(Link& link, const Element& element) NOEXCEPT
{
    link = allocate(element.count());
    return set(link, element);
}

TEMPLATE
template <typename Element, if_equal<Element::size, Size>>
Link CLASS::put_link(const Key& key, const Element& element) NOEXCEPT
{
    Link link{};
    if (!put_link(link, key, element))
        return {};

    return link;
}

TEMPLATE
template <typename Element, if_equal<Element::size, Size>>
bool CLASS::put_link(Link& link, const Key& key,
    const Element& element) NOEXCEPT
{
    using namespace system;
    const auto count = element.count();
    link = allocate(count);
    const auto ptr = manager_.get(link);
    if (!ptr)
        return false;

    iostream stream{ *ptr };
    finalizer sink{ stream };
    sink.skip_bytes(Link::size);
    sink.write_bytes(key);
    sink.set_finalizer([this, link, index = head_.index(key), ptr]() NOEXCEPT
    {
        auto& next = unsafe_array_cast<uint8_t, Link::size>(ptr->begin());
        return head_.push(link, next, index);
    });

    if constexpr (!is_slab) { sink.set_limit(Size * count); }
    return element.to_data(sink) && sink.finalize();
}

TEMPLATE
template <typename Element, if_equal<Element::size, Size>>
bool CLASS::put(const Key& key, const Element& element) NOEXCEPT
{
    return !put_link(key, element).is_terminal();
}

TEMPLATE
template <typename Element, if_equal<Element::size, Size>>
bool CLASS::put(const Link& link, const Key& key,
    const Element& element) NOEXCEPT
{
    using namespace system;
    const auto count = element.count();
    const auto ptr = manager_.get(link);
    if (!ptr)
        return false;

    iostream stream{ *ptr };
    finalizer sink{ stream };
    sink.skip_bytes(Link::size);
    sink.write_bytes(key);

    // The finalizer provides deferred index commit following serialization.
    sink.set_finalizer([this, link, index = head_.index(key), ptr]() NOEXCEPT
    {
        auto& next = unsafe_array_cast<uint8_t, Link::size>(ptr->begin());
        return head_.push(link, next, index);
    });

    if constexpr (!is_slab) { sink.set_limit(Size * count); }
    return element.to_data(sink) && sink.finalize();
}

TEMPLATE
bool CLASS::commit(const Link& link, const Key& key) NOEXCEPT
{
    const auto ptr = manager_.get(link);
    if (!ptr)
        return false;

    // Set element search key.
    system::unsafe_array_cast<uint8_t, array_count<Key>>(
        std::next(ptr->begin(), Link::size)) = key;

    // Commit element to search index.
    auto& next = system::unsafe_array_cast<uint8_t, Link::size>(ptr->begin());
    return head_.push(link, next, head_.index(key));
}

TEMPLATE
Link CLASS::commit_link(const Link& link, const Key& key) NOEXCEPT
{
    if (!commit(link, key))
        return {};

    return link;
}

} // namespace database
} // namespace libbitcoin

#endif
