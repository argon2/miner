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

#ifndef MINER_GPU_MANAGER_HPP
#define MINER_GPU_MANAGER_HPP

#include <cstdint>
#include <memory>
#include <vector>

namespace miner {

    class getwork_work;
    class gpu;
    class stack_impl;
    class stratum_work;
    
    /**
     * Implements a GPU manager.
     */
    class gpu_manager
    {
        public:
        
            /**
             * Constructor
             * @param owner The stack_impl.
             */
            explicit gpu_manager(stack_impl & owner);
        
            /**
             * Starts
             */
            void start();
        
            /**
             * Stops
             */
            void stop();
        
            /**
             * Sets the work.
             * @param val The value.
             */
            void set_work(const std::shared_ptr<getwork_work> & val);
        
            /**
             * Sets the work.
             * @param val The value.
             */
            void set_work(const std::shared_ptr<stratum_work> & val);
        
            /**
             * The GPU's.
             */
            const std::vector< std::shared_ptr<gpu> > & gpus() const;
        
        private:
        
            /**
             * The getwork_work.
             */
            std::shared_ptr<getwork_work> m_getwork_work;
        
            /**
             * The stratum_work.
             */
            std::shared_ptr<stratum_work> m_stratum_work;
        
            /**
             * The GPU's.
             */
            std::vector< std::shared_ptr<gpu> > m_gpus;
        
        protected:
        
            /**
             * The stack_impl.
             */
            stack_impl & stack_impl_;
    };
    
} // namespace miner

#endif // MINER_GPU_MANAGER_HPP
