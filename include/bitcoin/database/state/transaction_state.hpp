/**
 * Copyright (c) 2011-2017 libbitcoin developers (see AUTHORS)
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
#ifndef LIBBITCOIN_DATABASE_TRANSACTION_STATE_HPP
#define LIBBITCOIN_DATABASE_TRANSACTION_STATE_HPP

#include <cstdint>

namespace libbitcoin {
namespace database {

// Stored txs are verified or protected by valid header PoW, states are:
// TODO: compress into position using flag for indexed and sentinal for pool.
enum class transaction_state : uint8_t
{
    /// Confirmable if forks match, height is forks, position unused.
    pooled = 0,

    /// Confirmed in header index, height is forks, position unused.
    candidate = 1,

    /// Confirmed in block index, height and position are block values.
    confirmed = 2
};

} // namespace database
} // namespace libbitcoin

#endif
