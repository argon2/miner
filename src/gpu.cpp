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

#include <miner/gpu.hpp>

#include <cassert>
#include <chrono>
#include <iomanip>

#include <miner/configuration.hpp>
#include <miner/getwork_work.hpp>
#include <miner/gpu.hpp>
#include <miner/gpu_handler.hpp>
#include <miner/gpu_handler_cuda.hpp>
#include <miner/gpu_handler_opencl.hpp>
#include <miner/hash.hpp>
#include <miner/logger.hpp>
#include <miner/stack_impl.hpp>
#include <miner/stratum_work.hpp>
#include <miner/utility.hpp>
#include <miner/work_manager.hpp>

using namespace miner;

gpu::gpu(
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

void gpu::start()
{
    log_debug("GPU is starting.");
    
    state_ = state_started;
    
    /**
     * Configure (the default) handler.
     */
    if (true)
    {
        /**
         * Allocate the handler per the model of device.
         */
        m_gpu_handler = std::make_shared<gpu_handler_opencl> (
            shared_from_this()
        );
    }
    else
    {
        /**
         * Allocate the handler per the model of device.
         */
        m_gpu_handler = std::make_shared<gpu_handler_cuda> (
            shared_from_this()
        );
    }
    
    /**
     * Start the gpu_handler.
     */
    if (m_gpu_handler)
    {
        m_gpu_handler->start();
    }
    
    /**
     * Allocate the std::thread.
     */
    thread_ = std::thread(&gpu::loop, shared_from_this());
}

void gpu::stop()
{
    log_debug("GPU is stopping.");
    
    state_ = state_stopped;
    
    /**
     * Stop the gpu_handler.
     */
    if (m_gpu_handler)
    {
        m_gpu_handler->stop();
    }
    
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

void gpu::set_work(const std::shared_ptr<getwork_work> & val)
{
    log_info("GPU " << this << " got new (getwork) work.");

    /**
     * If the work is null we need to restart.
     */
    if (val == 0)
    {
        needs_work_restart_ = true;
        
        mutex_work_.lock();
        
        m_getwork_work = val;
        
        mutex_work_.unlock();
        
        if (m_gpu_handler)
        {
            m_gpu_handler->set_has_new_work(needs_work_restart_);
        }
    }
    else
    {
        mutex_work_.lock();
        
        if (m_getwork_work == 0 || m_getwork_work->data() != val->data())
        {
            /**
             * Set the work it will then be picked up in the next hashing loop.
             */
            m_getwork_work = val;
            
            has_new_work_ = true;
            
            if (m_gpu_handler)
            {
                m_gpu_handler->set_has_new_work(has_new_work_);
            }
        }
        else
        {
            /**
             * Set the work it will then be picked up in the next hashing loop.
             */
            m_getwork_work = val;
        }
        
        mutex_work_.unlock();
    }
}

void gpu::set_work(const std::shared_ptr<stratum_work> & val)
{
    log_info("GPU " << this << " got new (stratum) work.");

    /**
     * If the work is null we need to restart.
     */
    if (val == 0)
    {
        needs_work_restart_ = true;
        
        mutex_work_.lock();
        
        m_stratum_work = val;
        
        mutex_work_.unlock();
        
        if (m_gpu_handler)
        {
            m_gpu_handler->set_has_new_work(needs_work_restart_);
        }
    }
    else
    {
        mutex_work_.lock();
        
        if (m_stratum_work == 0 || m_stratum_work->job_id() != val->job_id())
        {
            /**
             * Set the work it will then be picked up in the next hashing loop.
             */
            m_stratum_work = val;
            
            has_new_work_ = true;
            
            if (m_gpu_handler)
            {
                m_gpu_handler->set_has_new_work(has_new_work_);
            }
        }
        else
        {
            /**
             * Set the work it will then be picked up in the next hashing loop.
             */
            m_stratum_work = val;
        }
        
        mutex_work_.unlock();
    }
}

std::shared_ptr<stratum_work> gpu::work()
{
    std::lock_guard<std::recursive_mutex> l1(mutex_work_);
    
    return
        m_stratum_work ? std::make_shared<stratum_work> (*m_stratum_work) : 0
    ;
}

void gpu::loop()
{
    std::uint32_t
        nonce_end = 0xffffffffU / m_id_maximum * (m_id + 1) - 0x20
    ;
    
    (void)nonce_end;

    std::srand(static_cast<std::uint32_t> (std::clock()));
    
    while (state_ == state_started)
    {
        if (m_gpu_handler)
        {
            m_gpu_handler->run();
        }
        else
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        }
    }
}

const double & gpu::hashes_per_second() const
{
    return m_hashes_per_second;
}
