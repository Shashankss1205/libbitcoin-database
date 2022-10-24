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
#include "../test.hpp"
#include "../storage.hpp"

BOOST_AUTO_TEST_SUITE(manager_tests)

// slabs

BOOST_AUTO_TEST_CASE(manager__size__empty_slab__zero)
{
    test::storage file;
    const manager<uint32_t, 1> instance(file);
    BOOST_REQUIRE(is_zero(instance.size()));
}

BOOST_AUTO_TEST_CASE(manager__size__non_empty_slab__expected)
{
    constexpr auto expected = 42u;
    const data_chunk buffer(expected, 0xff);
    test::storage file(buffer);

    // Slab sizing is byte-based (arbitrary, links are file offsets).
    const manager<uint32_t, zero> instance(file);
    BOOST_REQUIRE_EQUAL(instance.size(), expected);
}

BOOST_AUTO_TEST_CASE(manager__allocate__eof_slab__eof_unchanged)
{
    const data_chunk buffer;
    test::storage file(buffer);
    manager<uint32_t, zero> instance(file);
    BOOST_REQUIRE_EQUAL(instance.allocate(instance.eof), instance.eof);
    BOOST_REQUIRE_EQUAL(instance.size(), zero);
}

BOOST_AUTO_TEST_CASE(manager__allocate__empty_slab__expected)
{
    constexpr auto expected = 42u;
    const data_chunk buffer;
    test::storage file(buffer);
    manager<uint32_t, zero> instance(file);
    BOOST_REQUIRE_EQUAL(instance.allocate(expected), zero);
    BOOST_REQUIRE_EQUAL(instance.size(), expected);
}

BOOST_AUTO_TEST_CASE(manager__allocate__non_empty_slab__expected)
{
    constexpr auto expected = 42u;
    const data_chunk buffer(to_half(expected), 0xff);
    test::storage file(buffer);
    manager<uint32_t, zero> instance(file);
    BOOST_REQUIRE_EQUAL(instance.allocate(to_half(expected)), to_half(expected));
    BOOST_REQUIRE_EQUAL(instance.size(), expected);
}

BOOST_AUTO_TEST_CASE(manager__truncate__eof_slab__false_unchanged)
{
    const data_chunk buffer;
    test::storage file(buffer);
    manager<uint32_t, zero> instance(file);
    BOOST_REQUIRE(!instance.truncate(instance.eof));
    BOOST_REQUIRE_EQUAL(instance.size(), zero);
}

BOOST_AUTO_TEST_CASE(manager__truncate__overflow_slab__false_unchanged)
{
    constexpr auto size = 42u;
    const data_chunk buffer(size, 0xff);
    test::storage file(buffer);
    manager<uint32_t, zero> instance(file);
    BOOST_REQUIRE(!instance.truncate(add1(size)));
    BOOST_REQUIRE_EQUAL(instance.size(), size);
}

BOOST_AUTO_TEST_CASE(manager__truncate__half_full_slab__true_changed)
{
    constexpr auto size = 42u;
    constexpr auto half = to_half(size);
    const data_chunk buffer(size, 0xff);
    test::storage file(buffer);
    manager<uint32_t, zero> instance(file);
    BOOST_REQUIRE(instance.truncate(half));
    BOOST_REQUIRE_EQUAL(instance.size(), half);

    // Can "truncate" to capacity limit.
    BOOST_REQUIRE(instance.truncate(size));
    BOOST_REQUIRE_EQUAL(instance.size(), size);
}

BOOST_AUTO_TEST_CASE(manager__get__eof_slab__eof)
{
    constexpr auto size = 14u;
    const data_chunk buffer(size, 0xff);
    test::storage file(buffer);
    manager<uint16_t, zero> instance(file);
    BOOST_REQUIRE_EQUAL(instance.size(), size);
    BOOST_REQUIRE(!instance.get(instance.eof));
}

BOOST_AUTO_TEST_CASE(manager__get__slab__expected)
{
    constexpr auto size = 16u;
    const data_chunk buffer
    {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f
    };

    test::storage file(buffer);
    manager<uint16_t, zero> instance(file);
    BOOST_REQUIRE_EQUAL(instance.size(), size);
    BOOST_REQUIRE_EQUAL(*instance.get(0)->data(), 0x00_u8);
    BOOST_REQUIRE_EQUAL(*instance.get(1)->data(), 0x01_u8);
    BOOST_REQUIRE_EQUAL(*instance.get(2)->data(), 0x02_u8);
    BOOST_REQUIRE_EQUAL(*instance.get(9)->data(), 0x09_u8);
}

// records

BOOST_AUTO_TEST_CASE(manager__size__empty_record__zero)
{
    test::storage file;
    const manager<uint32_t, 42> instance(file);
    BOOST_REQUIRE(is_zero(instance.size()));
}

BOOST_AUTO_TEST_CASE(manager__size__1_record__expected)
{
    constexpr auto count = 1u;
    constexpr auto limit = 5u;
    constexpr auto expected = count * (sizeof(uint32_t) + limit);
    const data_chunk buffer(expected, 0xff);
    test::storage file(buffer);

    // Record sizing is record count-based (links are record counters).
    const manager<uint32_t, limit> instance(file);
    BOOST_REQUIRE_EQUAL(instance.size(), 1u);
}

BOOST_AUTO_TEST_CASE(manager__size__33_record__expected)
{
    constexpr auto count = 33u;
    constexpr auto limit = 5u;
    constexpr auto expected = count * (sizeof(uint32_t) + limit);
    const data_chunk buffer(expected, 0xff);
    test::storage file(buffer);
    const manager<uint32_t, limit> instance(file);
    BOOST_REQUIRE_EQUAL(instance.size(), 33u);
}

BOOST_AUTO_TEST_CASE(manager__allocate__eof_empty_record__eof_unchanged)
{
    const data_chunk buffer;
    test::storage file(buffer);
    manager<uint16_t, 5u> instance(file);
    BOOST_REQUIRE_EQUAL(instance.allocate(instance.eof), instance.eof);
    BOOST_REQUIRE_EQUAL(instance.size(), zero);
}

BOOST_AUTO_TEST_CASE(manager__allocate__eof_non_empty_record__expected)
{
    const data_chunk buffer(7, 0xff);
    test::storage file(buffer);
    manager<uint16_t, 5u> instance(file);
    BOOST_REQUIRE_EQUAL(instance.allocate(1), 1u);
    BOOST_REQUIRE_EQUAL(instance.size(), 2u);
    BOOST_REQUIRE_EQUAL(instance.allocate(instance.eof), instance.eof);
    BOOST_REQUIRE_EQUAL(instance.size(), 2u);
}

BOOST_AUTO_TEST_CASE(manager__allocate__empty_record__expected)
{
    const data_chunk buffer;
    test::storage file(buffer);
    manager<uint16_t, 5u> instance(file);
    BOOST_REQUIRE_EQUAL(instance.allocate(1), 0u);
    BOOST_REQUIRE_EQUAL(instance.size(), 1u);
    BOOST_REQUIRE_EQUAL(instance.allocate(2), 1u);
    BOOST_REQUIRE_EQUAL(instance.size(), 3u);
}

BOOST_AUTO_TEST_CASE(manager__allocate__non_empty_record__expected)
{
    const data_chunk buffer(7, 0xff);
    test::storage file(buffer);
    manager<uint16_t, 5u> instance(file);
    BOOST_REQUIRE_EQUAL(instance.allocate(1), 1u);
    BOOST_REQUIRE_EQUAL(instance.size(), 2u);
    BOOST_REQUIRE_EQUAL(instance.allocate(2), 2u);
    BOOST_REQUIRE_EQUAL(instance.size(), 4u);
}

BOOST_AUTO_TEST_CASE(manager__truncate__eof_record__false_unchanged)
{
    const data_chunk buffer;
    test::storage file(buffer);
    manager<uint16_t, 5u> instance(file);
    BOOST_REQUIRE(!instance.truncate(instance.eof));
    BOOST_REQUIRE_EQUAL(instance.size(), zero);
}

BOOST_AUTO_TEST_CASE(manager__truncate__overflow_record__false_unchanged)
{
    const data_chunk buffer(7, 0xff);
    test::storage file(buffer);
    manager<uint16_t, 5u> instance(file);
    BOOST_REQUIRE_EQUAL(instance.size(), 1u);
    BOOST_REQUIRE(!instance.truncate(2));
    BOOST_REQUIRE_EQUAL(instance.size(), 1u);
}

BOOST_AUTO_TEST_CASE(manager__truncate__half_full_record__true_changed)
{
    const data_chunk buffer(14, 0xff);
    test::storage file(buffer);
    manager<uint16_t, 5u> instance(file);
    BOOST_REQUIRE_EQUAL(instance.size(), 2u);
    BOOST_REQUIRE(instance.truncate(1));
    BOOST_REQUIRE_EQUAL(instance.size(), 1u);

    // Can "truncate" to capacity limit.
    BOOST_REQUIRE(instance.truncate(2));
    BOOST_REQUIRE_EQUAL(instance.size(), 2u);
}

BOOST_AUTO_TEST_CASE(manager__get__eof_record__eof)
{
    const data_chunk buffer(14, 0xff);
    test::storage file(buffer);
    manager<uint16_t, 5u> instance(file);
    BOOST_REQUIRE_EQUAL(instance.size(), 2u);
    BOOST_REQUIRE(!instance.get(instance.eof));
}

BOOST_AUTO_TEST_CASE(manager__get__record__expected)
{
    const data_chunk buffer
    {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f
    };

    test::storage file(buffer);
    manager<uint16_t, 6u> instance(file);
    BOOST_REQUIRE_EQUAL(instance.size(), 2u);
    BOOST_REQUIRE_EQUAL(*instance.get(0)->data(), 0x00_u8);
    BOOST_REQUIRE_EQUAL(*instance.get(1)->data(), 0x08_u8);
}

BOOST_AUTO_TEST_SUITE_END()
