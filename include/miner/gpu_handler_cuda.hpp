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

#ifndef MINER_GPU_HANDLER_CUDA_HPP
#define MINER_GPU_HANDLER_CUDA_HPP

#include <memory>

#include <miner/gpu_handler.hpp>

namespace miner {

    class gpu;
    
    /**
     * Implements a GPU handler for CUDA.
     */
    class gpu_handler_cuda : public gpu_handler
    {
        public:
        
            /**
             * Constructor
             * @param owner The gpu.
             */
            explicit gpu_handler_cuda(std::shared_ptr<gpu> owner)
                : gpu_handler(owner)
            {
                // ...
            }
        
            /**
             * Starts
             */
            virtual void start()
            {
                assert(0);
            }
        
            /**
             * Stops
             */
            virtual void stop()
            {
                assert(0);
            }
        
            /**
             * The read handler.
             * @param buf The buffer.
             * @param len The length.
             */
            virtual void on_read(const char * buf, const std::size_t & len)
            {
                assert(0);
            }
        
            /**
             * Sets that new work is available.
             * @param val The value.
             */
            virtual void set_has_new_work(const bool & val)
            {
                assert(0);
            }
        
            /**
             * Sets that new work is available.
             * @param val The value.
             */
            virtual void set_needs_work_restart(const bool & val)
            {
                assert(0);
            }
        
            /**
             * Runs the loop.
             */
            virtual void run()
            {
                assert(0);
            }
        
        private:
        
            // ...
        
        protected:
        
            // ...
    };
    
} // namespace miner

#endif // MINER_GPU_HANDLER_CUDA_HPP
