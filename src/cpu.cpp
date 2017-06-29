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
#include <chrono>
#include <iomanip>

#include <miner/configuration.hpp>
#include <miner/cpu.hpp>
#include <miner/cpu_handler.hpp>
#include <miner/getwork_work.hpp>
#include <miner/hash.hpp>
#include <miner/logger.hpp>
#include <miner/stack_impl.hpp>
#include <miner/stratum_work.hpp>
#include <miner/utility.hpp>
#include <miner/work_manager.hpp>

using namespace miner;

cpu::cpu(
    stack_impl & owner, const std::uint32_t & id, const std::uint32_t & id_max
    )
    : m_id(id)
    , m_id_maximum(id_max)
    , m_hashes_per_second(0.0)
    , m_hash_counter(0)
    , state_(state_none)
    , stack_impl_(owner)
    , has_new_work_(false)
    , needs_work_restart_(false)
{
    // ...
}

void cpu::start()
{
    log_debug("CPU is starting.");
    
    state_ = state_started;
    
    /**
     * Allocate the std::thread.
     */
    thread_ = std::thread(&cpu::loop, shared_from_this());
}

void cpu::stop()
{
    log_debug("CPU is stopping.");
    
    state_ = state_stopped;
    
    /**
     * Set that we need to restart work to abort the inner mining loops.
     */
    needs_work_restart_ = true;
    
    /**
     * Set the hashes per second.
     */
    m_hashes_per_second = 0.0f;

    /**
     * Set the hash counter.
     */
    m_hash_counter = 0;
    
    try
    {
        /**
         * Join the std::thread.
         */
        thread_.join();
    }
    catch (std::exception & e)
    {
        // ...
    }
}

void cpu::set_work(const std::shared_ptr<getwork_work> & val)
{
    log_debug("CPU " << this << " got new (getwork) work.");

    /**
     * If the work is null we need to restart.
     */
    if (val == 0)
    {
        needs_work_restart_ = true;
        
        mutex_work_.lock();
        
        m_getwork_work = val;
        
        mutex_work_.unlock();
    }
    else
    {
        if (m_getwork_work == nullptr || m_getwork_work->data() != val->data())
        {
            has_new_work_ = true;
            
            mutex_work_.lock();
            
            /**
             * Set the work it will then be picked up in the next hashing loop.
             */
            m_getwork_work = val;
            
            mutex_work_.unlock();
        }
        else
        {
            has_new_work_ = true;
            
            mutex_work_.lock();
            
            /**
             * Set the work it will then be picked up in the next hashing loop.
             */
            m_getwork_work = val;
            
            mutex_work_.unlock();
        }
    }
}

void cpu::set_work(const std::shared_ptr<stratum_work> & val)
{
    log_debug("CPU " << this << " got new (stratum) work.");

    /**
     * If the work is null we need to restart.
     */
    if (val == 0)
    {
        needs_work_restart_ = true;
        
        mutex_work_.lock();
        
        m_stratum_work = val;
        
        mutex_work_.unlock();
    }
    else
    {
        if (m_stratum_work == nullptr || m_stratum_work->job_id() != val->job_id())
        {
            mutex_work_.lock();
            
            has_new_work_ = true;
            
            /**
             * Set the work it will then be picked up in the next hashing loop.
             */
            m_stratum_work = val;

            mutex_work_.unlock();
        }
        else
        {
            has_new_work_ = true;
            
            mutex_work_.lock();
            
            /**
             * Set the work it will then be picked up in the next hashing loop.
             */
            m_stratum_work = val;
            
            mutex_work_.unlock();
        }
    }
}

void cpu::loop()
{
    if (
        configuration::instance().work_host_type() ==
        configuration::work_host_type_getwork
        )
    {
        std::uint32_t
            nonce_end = 0xffffffffU / m_id_maximum * (m_id + 1) - 0x20
        ;

        std::srand(static_cast<std::uint32_t> (std::clock()));
        
        while (state_ == state_started)
        {
            mutex_work_.lock();
            
            auto work =
                m_getwork_work ?
                std::make_shared<getwork_work> (*m_getwork_work) : 0
            ;
            
            mutex_work_.unlock();
            
            /**
             * If we have work and it can be generated then start hashing.
             */
            if (work)
            {
                auto is_new_work = true;
                
                while (
                    state_ == state_started && has_new_work_ == false &&
                    needs_work_restart_ == false && work
                    )
                {
                    if (work->data()[19] >= nonce_end)
                    {
                        // ...
                    }
                    
                    /**
                     * The number of hashes.
                     */
                    std::uint64_t hashes = 0;
                
                    /**
                     * The nonce.
                     */
                    std::uint32_t nonce = 0;
                    
                    /**
                     * The maximum nonce.
                     */
                    std::uint32_t nonce_max = 0xffff0000;
                    
                    std::int64_t max64 = 60;

                    max64 *= m_hashes_per_second;
                    
                    if (max64 <= 0)
                    {
                         max64 = 0xffff0000;
                    }

                    if (is_new_work)
                    {
                        is_new_work = false;
                        
                        work->data()[19] = 0xffffffffU / m_id_maximum * m_id;
                    }
                    else
                    {
                        /**
                         * Increment the nonce.
                         */
                        work->data()[19]++;
                    }

                    if (work->data()[19] + max64 > nonce_end)
                    {
                        nonce_max = nonce_end;
                    }
                    else
                    {
                        nonce_max = static_cast<std::uint32_t> (
                            work->data()[19] + max64
                        );
                    }
                    
                    /**
                     * Get the milliseconds since epoch.
                     */
                    auto milliseconds1 = std::chrono::duration_cast<
                        std::chrono::milliseconds> (
                        std::chrono::system_clock::now().time_since_epoch()
                    ).count();

                    /**
                     * Scan for a solution to the work.
                     */
                    auto result = hash::scan(
                        configuration::instance().proof_of_work_type(),
                        work, nonce_max, hashes, nonce,
                        needs_work_restart_, has_new_work_
                    );

                    /**
                     * Check the result.
                     */
                    if (result == true)
                    {
                        assert(work->data()[19] == nonce);
                        
                        /**
                         * Submit the work.
                         */
                        stack_impl_.get_work_manager()->submit_getwork_work(
                            work
                        );
                        
                        /**
                         * Keep searching for solutions.
                         */
                    }
                    else
                    {
                        /**
                         * Keep searching for solutions.
                         */
                    }

                    m_hash_counter += hashes;
                    
                    auto milliseconds2 = std::chrono::duration_cast<
                        std::chrono::milliseconds> (
                        std::chrono::system_clock::now().time_since_epoch()
                    ).count();

                    if (milliseconds2 - milliseconds1 >= 1000)
                    {
                        m_hashes_per_second =
                            1000.0 * m_hash_counter /
                            (milliseconds2 - milliseconds1)
                        ;

                        m_hash_counter = 0;

                        log_info(
                            "CPU hashing " <<
                            std::fixed << std::setprecision(2) <<
                            m_hashes_per_second << " H/s."
                        );
                    }
                }
                
                log_info(
                    "CPU " << std::this_thread::get_id() <<
                    " is switching to new (getwork) work."
                );
                
                has_new_work_ = false;
                needs_work_restart_ = false;
            }
            else
            {
                log_debug(
                    "CPU is " << std::this_thread::get_id() << " waiting for "
                    "(getwork) work."
                );
                
                std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            }
        }

    }
    else if (
        configuration::instance().work_host_type() ==
        configuration::work_host_type_stratum
        )
    {
        std::uint32_t
            nonce_end = 0xffffffffU / m_id_maximum * (m_id + 1) - 0x20
        ;

        std::srand(static_cast<std::uint32_t> (std::clock()));
        
        while (state_ == state_started)
        {
            mutex_work_.lock();
            
            auto work =
                m_stratum_work ?
                std::make_shared<stratum_work> (*m_stratum_work) : 0
            ;
            
            mutex_work_.unlock();
            
            /**
             * If we have work and it can be generated then start hashing.
             */
            if (work && work->generate())
            {
                auto is_new_work = true;
                
                while (
                    state_ == state_started && has_new_work_ == false &&
                    needs_work_restart_ == false && work
                    )
                {
                    if (work->data()[19] >= nonce_end)
                    {
                        /**
                         * :TODO: Increment extranaonce2.
                         */
                        
                        /**
                         * Generate new work.
                         */
                        work->generate();
                    }
                    
                    /**
                     * The number of hashes.
                     */
                    std::uint64_t hashes = 0;
                
                    /**
                     * The nonce.
                     */
                    std::uint32_t nonce = 0;
                    
                    /**
                     * The maximum nonce.
                     */
                    std::uint32_t nonce_max = 0xffff0000;
                    
                    std::int64_t max64 = 60;

                    max64 *= m_hashes_per_second;
                    
                    if (max64 <= 0)
                    {
                        /**
                         * :TODO: Pick proper max scans.
                         */
                        max64 = 0xffff0000;
                    }

                    if (is_new_work)
                    {
                        is_new_work = false;
                        
                        work->data()[19] = 0xffffffffU / m_id_maximum * m_id;
                    }
                    else
                    {
                        /**
                         * Increment the nonce.
                         */
                        work->data()[19]++;
                    }

                    if (work->data()[19] + max64 > nonce_end)
                    {
                        nonce_max = nonce_end;
                    }
                    else
                    {
                        nonce_max = static_cast<std::uint32_t> (
                            work->data()[19] + max64
                        );
                    }
                    
                    /**
                     * Get the milliseconds since epoch.
                     */
                    auto milliseconds1 = std::chrono::duration_cast<
                        std::chrono::milliseconds> (
                        std::chrono::system_clock::now().time_since_epoch()
                    ).count();

                    /**
                     * Scan for a solution to the work.
                     */
                    auto result = hash::scan(
                        configuration::instance().proof_of_work_type(),
                        work, nonce_max, hashes, nonce,
                        needs_work_restart_, has_new_work_
                    );

                    /**
                     * Check the result.
                     */
                    if (result == true)
                    {
                        assert(work->data()[19] == nonce);
                        
                        /**
                         * Submit the work.
                         */
                        stack_impl_.get_work_manager()->submit_stratum_work(work);
                        
                        /**
                         * Keep searching for solutions.
                         */
                    }
                    else
                    {
                        /**
                         * Keep searching for solutions.
                         */
                    }

                    m_hash_counter += hashes;
                    
                    auto milliseconds2 = std::chrono::duration_cast<
                        std::chrono::milliseconds> (
                        std::chrono::system_clock::now().time_since_epoch()
                    ).count();

                    if (milliseconds2 - milliseconds1 >= 1000)
                    {
                        m_hashes_per_second =
                            1000.0 * m_hash_counter /
                            (milliseconds2 - milliseconds1)
                        ;

                        m_hash_counter = 0;

                        log_info(
                            "CPU hashing " <<
                            std::fixed << std::setprecision(2) <<
                            m_hashes_per_second << " H/s."
                        );
                    }
                }
                
                log_info(
                    "CPU " << std::this_thread::get_id() <<
                    " is switching to new (stratum) work."
                );
                
                has_new_work_ = false;
                needs_work_restart_ = false;
            }
            else
            {
                log_debug(
                    "CPU is " << std::this_thread::get_id() << " waiting for "
                    "(stratum) work."
                );
                
                std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            }
        }
    }
}

const double & cpu::hashes_per_second() const
{
    return m_hashes_per_second;
}
