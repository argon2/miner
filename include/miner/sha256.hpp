/*
 * Copyright (c) 2014-2017 BM-2cVZag8xxXdPC9BsLewmUotg6TBB8T2yTk
 *
 * This file is part of Miner++.
 *
 * Miner++ is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License with
 * additional permissions to the one published by the Free Software
 * Foundation, either version 3 of the License, or (at your option)
 * any later version. For more information see LICENSE.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MINER_SHA256_HPP
#define MINER_SHA256_HPP

#include <cstdint>
#include <cstring>
#include <string>

/**
 * Performs sha256 hash by reversing the output.
 * @param buf The buffer.
 * @param len The length.
 * @param digest The digest.
 */
void sha256(
    const std::uint8_t * buf, const std::size_t & len, std::uint8_t * digest
);

/**
 * Performs sha256d hash.
 * @param buf The buffer.
 * @param len The length.
 * @param digest The digest.
 */
void sha256d(
    const std::uint8_t * buf, const std::size_t & len, std::uint8_t * digest
);

/**
 * Converts a 32-bit hash into a string representation.
 */
std::string sha256_to_string(const std::uint8_t * digest);

#endif // MINER_SHA256_HPP
