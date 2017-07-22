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

#ifndef MINER_STATISTICS_HPP
#define MINER_STATISTICS_HPP

namespace miner {

    /**
     * Implements statistics.
     */
    class statistics
    {
        public:
        
            /**
             * Constructor
             */
            statistics();
        
            /**
             * The singleton accessor.
             */
            static statistics & instance();
        
            /**
             * Sets the hashes per second.
             * @param val The value.
             */
            void set_hashes_per_second(const double & val);
        
            /**
             * The hashes per second.
             */
            const double & hashes_per_second() const;
        
            /**
             * Sets the hashes per second (LAN).
             * @param val The value.
             */
            void set_hashes_per_second_lan(const double & val);
        
            /**
             * The hashes per second (LAN).
             */
            const double & hashes_per_second_lan() const;
        
        private:
        
            /**
             * The hashes per second.
             */
            double m_hashes_per_second;
        
            /**
             * The hashes per second (LAN).
             */
            double m_hashes_per_second_lan;
        
        protected:
      
            // ...
    };
    
} // namespace miner

#endif // MINER_STATISTICS_HPP
