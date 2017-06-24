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

#include <miner/configuration.hpp>

using namespace miner;

configuration::configuration()
    : m_work_host_type(work_host_type_none)
    , m_device_type(device_type_none)
    , m_proof_of_work_type(proof_of_work_type_none)
    , m_device_cores(1)
{
    // ...
}

configuration & configuration::instance()
{
    static configuration g_configuration;
            
    return g_configuration;
}

const configuration::work_host_type_t & configuration::work_host_type() const
{
    return m_work_host_type;
}

void configuration::set_work_host_type(
    const configuration::work_host_type_t & val
    )
{
    m_work_host_type = val;
}

void configuration::set_device_type(const configuration::device_type_t & val)
{
    m_device_type = val;
}

const configuration::device_type_t & configuration::device_type() const
{
    return m_device_type;
}

void configuration::set_proof_of_work_type(
    const configuration::proof_of_work_type_t & val
    )
{
    m_proof_of_work_type = val;
}

const configuration::proof_of_work_type_t &
    configuration::proof_of_work_type() const
{
    return m_proof_of_work_type;
}

std::vector< std::pair< std::pair<std::string, std::string>,
    std::pair<std::string, std::uint16_t> > > & configuration::work_hosts()
{
    return m_work_hosts;
}

void configuration::set_device_cores(const std::uint32_t & val)
{
    m_device_cores = val;
}

const std::uint32_t & configuration::device_cores() const
{
    return m_device_cores;
}
