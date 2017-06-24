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

#include <cassert>

#include <miner/argon2.h>
#include <miner/getwork_work.hpp>
#include <miner/hash.hpp>
#include <miner/logger.hpp>
#include <miner/stratum_work.hpp>
#include <miner/utility.hpp>

using namespace miner;

bool hash::scan(
    const configuration::proof_of_work_type_t & type,
    std::shared_ptr<stratum_work> & work,
    const std::uint32_t & max_nonce, std::uint64_t & hashes_done,
    std::uint32_t & nonce, bool & restart, bool & has_new_work
    )
{
    if (type == configuration::proof_of_work_type_argon2d)
    {
        return scan_argon2d(
            work, max_nonce, hashes_done, nonce, restart, has_new_work
        );
    }

    log_error("Hash got invalid Proof-of-Work type = " << type << ".");
    
    return false;
}

bool hash::scan(
    const configuration::proof_of_work_type_t & type,
    std::shared_ptr<getwork_work> & work,
    const std::uint32_t & max_nonce, std::uint64_t & hashes_done,
    std::uint32_t & nonce, bool & restart, bool & has_new_work
    )
{
    if (type == configuration::proof_of_work_type_argon2d)
    {
        return scan_argon2d(
            work, max_nonce, hashes_done, nonce, restart, has_new_work
        );
    }

    log_error("Hash got invalid Proof-of-Work type = " << type << ".");
    
    return false;
}


bool hash::check(const std::uint32_t * hash, const std::uint32_t * target)
{
	auto ret = true;

    for (auto i = 7; i >= 0; i--)
    {
        if (hash[i] > target[i])
        {
            ret = false;
            
            break;
        }
        
        if (hash[i] < target[i])
        {
            ret = true;
            
            break;
        }
    }

	if (true)
    {
		std::uint32_t hash_be[8];
        
        std::uint32_t target_be[8];

		for (auto i = 0; i < 8; i++)
        {
			utility::be32enc(&hash_be[i], hash[7 - i]);
			utility::be32enc(&target_be[i], target[7 - i]);
		}
        
        log_info(
            "Hash check: \n\thash: " <<
            utility::to_hex(
            reinterpret_cast<std::uint8_t *> (&hash_be[0]),
            reinterpret_cast<std::uint8_t *> (&hash_be[0]) + 32) <<
            "\n\t" << "target: " << utility::to_hex(
            reinterpret_cast<std::uint8_t *> (&target_be[0]),
            reinterpret_cast<std::uint8_t *> (&target_be[0]) + 32) <<
            "\n\tresult: " << (ret ? "good" : "bad")
        );
	}

	return ret;
}

static std::string sha256_to_string(const std::uint8_t * digest)
{
    char ret[32 * 2 + 1];
    
    for (unsigned i = 0; i < 32; i++)
    {
        sprintf(
            ret + i * 2, "%02x", digest[32 - i - 1]
        );
    }
    
    return std::string(ret, ret + 32 * 2);
}

static std::vector<uint8_t> hash_argon2d(
    const std::string & password, const std::string & salt,
    std::string & encoded_out
    )
{
    std::vector<uint8_t> ret(32);

    enum { p = 8 };
    
    enum { encoded_length = 108 };
    
    encoded_out.resize(encoded_length);
    
    auto code = argon2_hash(
        1, 1 << p, 1, password.c_str(), password.length(), salt.c_str(),
        salt.length(), &ret[0], 32, const_cast<char *> (encoded_out.c_str()),
        encoded_length, Argon2_d, ARGON2_VERSION_NUMBER
    );
    
    if (code != ARGON2_OK)
    {
        throw std::runtime_error(
            "Hash, argon2d failed, code = " + std::to_string(code) + "."
        );
    }
 
    return ret;
}

bool hash::scan_argon2d(
    std::shared_ptr<stratum_work> & work,
    const std::uint32_t & max_nonce, std::uint64_t & hashes_done,
    std::uint32_t & nonce, bool & restart, bool & has_new_work
    )
{
    std::uint32_t * ptr_data = const_cast<std::uint32_t *>(&work->data()[0]);
    
    const std::uint32_t * ptr_target = &work->target()[0];

    nonce = ptr_data[19] - 1;
    
    const std::uint32_t first_nonce = ptr_data[19];

    std::uint32_t hash64[8];
    
    std::uint32_t endiandata[32];

    assert(sizeof(endiandata) == 128);
    
    for (auto kk = 0; kk < 32; kk++)
    {
        utility::be32enc(&endiandata[kk], ((std::uint32_t *)ptr_data)[kk]);
    }

    auto target = ptr_target[7];

    do
    {
        ptr_data[19] = ++nonce;
 
        utility::be32enc(&endiandata[19], ptr_data[19]);

        /**
         * Unused in Stratum (for SPV node fast header verification).
         */
        std::string encoded_out;
        
        /**
         * Allocate the password buffer (entire block header).
         */
        std::vector<uint8_t> buffer_password(
            reinterpret_cast<std::uint8_t *> (&endiandata[0]),
            reinterpret_cast<std::uint8_t *> (&endiandata[0]) + 80
        );
        
        /**
         * Convert the password buffer into a hexidecimal string representation.
         */
        std::string password = utility::to_hex(buffer_password);
        
        /**
         * Allocate space to copy the hash_previous from the block header
         * hash_previous.
         */
        std::uint8_t hash_previous[32];
        
        /**
         * Skip the version and copy the block header hash_previous.
         */
        std::memcpy(
            hash_previous,
            reinterpret_cast<std::uint8_t *> (&endiandata[0]) +
            sizeof(std::uint32_t), 32
        );
        
        /**
         * Generate the salt from the last 16 bytes of the block header
         * hash_previous.
         */
        std::string salt = sha256_to_string(hash_previous).substr(48, 64);
        
        /**
         * hash_argon2d
         */
        std::vector<std::uint8_t> ret = hash_argon2d(
            password, salt, encoded_out
        );
        
        /**
         * Copy the return to the output.
         */
        std::memcpy(
            reinterpret_cast<std::uint8_t *> (&hash64[0]), &ret[0], 32
        );
  
        /**
         * First check.
         */
        if (hash64[7] <= target && check(hash64, ptr_target))
        {
            hashes_done = nonce - first_nonce + 1;

            return true;
        }
        
    } while (nonce < max_nonce && restart == false && has_new_work == false);
    
    hashes_done = nonce - first_nonce + 1;
    
    ptr_data[19] = nonce;
    
    return false;
}

bool hash::scan_argon2d(
    std::shared_ptr<getwork_work> & work,
    const std::uint32_t & max_nonce, std::uint64_t & hashes_done,
    std::uint32_t & nonce, bool & restart, bool & has_new_work
    )
{
    std::uint32_t * ptr_data = const_cast<std::uint32_t *>(&work->data()[0]);
    
    const std::uint32_t * ptr_target = &work->target()[0];

    nonce = ptr_data[19] - 1;
    
    const std::uint32_t first_nonce = ptr_data[19];

    std::uint32_t hash64[8];
    
    std::uint32_t endiandata[32];

    assert(sizeof(endiandata) == 128);
    
    for (auto kk = 0; kk < 19; kk++)
    {
        utility::be32enc(&endiandata[kk], ((std::uint32_t *)ptr_data)[kk]);
    }

    auto target = ptr_target[7];

	if (true)
    {
        std::uint32_t target_be[8];

		for (auto i = 0; i < 8; i++)
        {
			utility::be32enc(&target_be[i], ptr_target[7 - i]);
		}
        
        log_info(
            "Work Target: \n\t" << utility::to_hex(
            reinterpret_cast<std::uint8_t *> (&target_be[0]),
            reinterpret_cast<std::uint8_t *> (&target_be[0]) + 32)
        );
	}
    
    do
    {
        ptr_data[19] = ++nonce;
 
        utility::be32enc(&endiandata[19], ptr_data[19]);

        /**
         * Unused in Stratum (for SPV node fast header verification).
         */
        std::string encoded_out;
        
        /**
         * Allocate the password buffer (entire block header).
         */
        std::vector<uint8_t> buffer_password(
            reinterpret_cast<std::uint8_t *> (&endiandata[0]),
            reinterpret_cast<std::uint8_t *> (&endiandata[0]) + 80
        );
        
        /**
         * Convert the password buffer into a hexidecimal string representation.
         */
        std::string password = utility::to_hex(buffer_password);
        
        /**
         * Allocate space to copy the hash_previous from the block header
         * hash_previous.
         */
        std::uint8_t hash_previous[32];
        
        /**
         * Skip the version and copy the block header hash_previous.
         */
        std::memcpy(
            hash_previous,
            reinterpret_cast<std::uint8_t *> (&endiandata[0]) +
            sizeof(std::uint32_t), 32
        );
        
        /**
         * Generate the salt from the last 16 bytes of the block header
         * hash_previous.
         */
        std::string salt = sha256_to_string(hash_previous).substr(48, 64);
        
        /**
         * hash_argon2d
         */
        std::vector<std::uint8_t> ret = hash_argon2d(
            password, salt, encoded_out
        );
        
        /**
         * Copy the return to the output.
         */
        std::memcpy(
            reinterpret_cast<std::uint8_t *> (&hash64[0]), &ret[0], 32
        );
    
        /**
         * First check.
         */
        if (hash64[7] <= target && check(hash64, ptr_target))
        {
            hashes_done = nonce - first_nonce + 1;

            return true;
        }
        
    } while (nonce < max_nonce && restart == false && has_new_work == false);
    
    hashes_done = nonce - first_nonce + 1;
    
    ptr_data[19] = nonce;
    
    return false;
}
