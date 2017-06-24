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
#include <cstring>

#include <miner/getwork_work.hpp>
#include <miner/utility.hpp>

using namespace miner;

getwork_work::getwork_work(
    const std::vector<std::uint8_t> & data,
    const std::vector<std::uint8_t> & target
    )
{
    assert(data.size() == 128);
    
    m_data.resize(128 / sizeof(std::uint32_t), 0);
    
    std::memcpy(
        reinterpret_cast<std::uint8_t *> (&m_data[0]), &data[0], data.size()
    );

    for (auto i = 0; i < m_data.size(); i++)
    {
        m_data[i] = utility::le32dec(&m_data[i]);
    }
    
    assert(m_data.size() == (128 / sizeof(std::uint32_t)));
    
    m_target.resize(32 / sizeof(std::uint32_t), 0);
    
    std::memcpy(
        reinterpret_cast<std::uint8_t *> (&m_target[0]), &target[0],
        target.size()
    );
    
    for (std::size_t i = 0; i < m_target.size(); i++)
    {
        m_target[i] = utility::le32dec(&m_target[i]);
    }
    
    assert(m_target.size() == (32 / sizeof(std::uint32_t)));
}

std::vector<std::uint32_t> & getwork_work::data()
{
    return m_data;
}

const std::vector<std::uint32_t> & getwork_work::target() const
{
    return m_target;
}
