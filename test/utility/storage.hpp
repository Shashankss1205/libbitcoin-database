/**
 * Copyright (c) 2011-2019 libbitcoin developers (see AUTHORS)
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
#ifndef STORAGE_HPP
#define STORAGE_HPP

#include "../test.hpp"
#include <bitcoin/system.hpp>
#include <bitcoin/database.hpp>

namespace test {

// Fake a thread safe memory map implementation.
class storage
  : public database::storage
{
public:
    DELETE4(storage);

    storage() NOEXCEPT;
    storage(data_chunk&& initial) NOEXCEPT;
    storage(const data_chunk& initial) NOEXCEPT;
    ~storage() NOEXCEPT;

    bool map() NOEXCEPT override;
    bool flush() const NOEXCEPT override;
    bool unmap() NOEXCEPT override;
    bool mapped() const NOEXCEPT override;
    size_t capacity() const NOEXCEPT override;
    size_t logical() const NOEXCEPT override;
    memory_ptr access() NOEXCEPT(false) override;
    memory_ptr resize(size_t size) NOEXCEPT(false) override;
    memory_ptr reserve(size_t size) NOEXCEPT(false) override;

private:
    bool mapped_;
    data_chunk buffer_;
    mutable upgrade_mutex mutex_;
};

}

#endif
