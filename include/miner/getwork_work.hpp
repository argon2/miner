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

#ifndef MINER_GETWORK_WORK_HPP
#define MINER_GETWORK_WORK_HPP

#include <cstdint>
#include <vector>

namespace miner {

    /**
     * Implements getwork work.
     */
    class getwork_work
    {
        public:
        
            /**
             * Constructor
             * @param data The data.
             * @param target The target.
             */
            getwork_work(
                const std::vector<std::uint8_t> & data,
                const std::vector<std::uint8_t> & target
            );
        
            /**
             * The data.
             */
            std::vector<std::uint32_t> & data();
        
            /**
             * The target.
             */
            const std::vector<std::uint32_t> & target() const;
        
        private:
        
            /**
             * The data.
             */
            std::vector<std::uint32_t> m_data;
        
            /**
             * The target.
             */
            std::vector<std::uint32_t> m_target;
            
        protected:
        
            // ...
    };
    
} // namespace miner

#endif // MINER_GETWORK_WORK_HPP
