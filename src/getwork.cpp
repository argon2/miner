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

#include <miner/getwork.hpp>

using namespace miner;

getwork::getwork()
    : m_port(0)
    , m_shares_accepted(0)
    , m_shares_rejected(0)
{
    // ...
}

getwork & getwork::instance()
{
    static getwork g_getwork;
    
    return g_getwork;
}

void getwork::set_username(const std::string & val)
{
    m_username = val;
}

const std::string & getwork::username() const
{
    return m_username;
}

void getwork::set_password(const std::string & val)
{
    m_password = val;
}

const std::string & getwork::password() const
{
    return m_password;
}

void getwork::set_host(const std::string & val)
{
    m_host = val;
}

const std::string & getwork::host() const
{
    return m_host;
}

void getwork::set_port(const std::uint16_t & val)
{
    m_port = val;
}

const std::uint16_t & getwork::port() const
{
    return m_port;
}

void getwork::set_shares_accepted(const std::uint32_t & val)
{
    m_shares_accepted = val;
}

const std::uint32_t & getwork::shares_accepted() const
{
    return m_shares_accepted;
}

void getwork::set_shares_rejected(const std::uint32_t & val)
{
    m_shares_rejected = val;
}

const std::uint32_t & getwork::shares_rejected() const
{
    return m_shares_rejected;
}
