/**
 * Copyright (c) 2011-2022 libbitcoin developers (see AUTHORS)
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
#ifndef LIBBITCOIN_DATABASE_PRIMITIVES_ARRAYMAP_IPP
#define LIBBITCOIN_DATABASE_PRIMITIVES_ARRAYMAP_IPP

#include <bitcoin/system.hpp>
#include <bitcoin/database/define.hpp>

namespace libbitcoin {
namespace database {
    
TEMPLATE
CLASS::arraymap(storage& body) NOEXCEPT
  : body_(body)
{
}

// query interface
// ----------------------------------------------------------------------------

TEMPLATE
template <typename Record, if_equal<Record::size, Size>>
bool CLASS::get(const Link& link, Record& record) const NOEXCEPT
{
    auto source = at(link);
    if (!source)
        return false;

    // Use of stream pointer can be eliminated by cloning at() here.
    // RECORD.FROM_DATA MUST NOT EXTEND SOURCE LIFETIME - DEADLOCK RISK.
    return record.from_data(*source);
}

TEMPLATE
template <typename Record, if_equal<Record::size, Size>>
bool CLASS::put(const Record& record) NOEXCEPT
{
    auto sink = push(record.count());
    if (!sink)
        return false;

    // Use of stream pointer can be eliminated by cloning push() here.
    // RECORD.TO_DATA MUST NOT EXTEND SOURCE LIFETIME - DEADLOCK RISK.
    return record.to_data(*sink);
}

// protected
// ----------------------------------------------------------------------------

TEMPLATE
reader_ptr CLASS::at(const Link& link) const NOEXCEPT
{
    if (link.is_terminal())
        return {};

    const auto ptr = body_.get(link_to_position(link));
    if (!ptr)
        return {};

    const auto source = std::make_shared<reader>(ptr);
    if constexpr (!is_slab) { source->set_limit(Size); }
    return source;
}

TEMPLATE
writer_ptr CLASS::push(const Link& size) NOEXCEPT
{
    const auto value = system::possible_narrow_cast<size_t>(size.value);
    BC_ASSERT(is_slab || !system::is_multiply_overflow(value, Size));
    BC_ASSERT(!size.is_terminal());

    const auto item = body_.allocate(link_to_position(size));
    if (item == storage::eof)
        return {};

    const auto ptr = body_.get(item);
    if (!ptr)
        return {};

    const auto sink = std::make_shared<writer>(ptr);
    if constexpr (is_slab) { sink->set_limit(value); }
    if constexpr (!is_slab) { sink->set_limit(value * Size); }
    return sink;
}

// private
// ----------------------------------------------------------------------------

TEMPLATE
constexpr size_t CLASS::link_to_position(const Link& link) NOEXCEPT
{
    const auto value = system::possible_narrow_cast<size_t>(link.value);
    BC_ASSERT(is_slab || !system::is_multiply_overflow(value, Size));

    if constexpr (is_slab) { return value; }
    if constexpr (!is_slab) { return value * Size; }
}

} // namespace database
} // namespace libbitcoin

#endif
