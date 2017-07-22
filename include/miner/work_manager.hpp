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

#ifndef MINER_WORK_MANAGER_HPP
#define MINER_WORK_MANAGER_HPP

#include <cstdint>
#include <map>
#include <vector>

#include <boost/asio.hpp>

namespace miner {

    class getwork_work;
    class stack_impl;
    class stratum_connection;
    class stratum_work;
    
    /**
     * Implements a work manager (gathers work via stratum and other mining
     * protocols).
     */
    class work_manager
        : public std::enable_shared_from_this<work_manager>
    {
        public:
        
            /**
             * Constructor
             * @param owner The stack_impl.
             */
            explicit work_manager(stack_impl & owner);
        
            /**
             * Starts
             */
            void start();
        
            /**
             * Stops
             */
            void stop();
        
            /**
             * Sets the getwork_work.
             * @param val The value.
             */
            void set_getwork_work(const std::shared_ptr<getwork_work> & val);
        
            /**
             * Sets the stratum_work
             * @param val The value.
             */
            void set_stratum_work(const std::shared_ptr<stratum_work> & val);
        
            /**
             * Submits (getwork) work with a solution.
             * @param val The work.
             */
            void submit_getwork_work(const std::shared_ptr<getwork_work> & val);
        
            /**
             * Submits (stratum) work with a solution.
             * @param val The work.
             */
            void submit_stratum_work(const std::shared_ptr<stratum_work> & val);
        
        private:
        
            /**
             * Connects to the work server.
             */
            void connect();
        
            /**
             * Sends a (getwork) getblockcount RPC request.
             */
            void getwork_send_getblockcount();
        
            /**
             * The timer handler.
             * @param ec The boost::system::error_code.
             */
            void tick(const boost::system::error_code & ec);
        
            /**
             * The check work hosts timer handler.
             * @param ec The boost::system::error_code.
             */
            void tick_check_work_hosts(const boost::system::error_code & ec);
        
            /**
             * The getwork poll timer handler.
             * @param ec The boost::system::error_code.
             */
            void tick_getwork_poll(const boost::system::error_code & ec);
        
            /**
             * The statistics timer handler.
             * @param ec The boost::system::error_code.
             */
            void tick_statistics(const boost::system::error_code & ec);
        
            /**
             * UDP receive handler.
             * @param ec The boost::system::error_code.
             * @param len The length.
             */
            void handle_udp_multicast_receive_from(
                const boost::system::error_code & ec, const size_t & len
            );
        
            /**
             * UDP sendto handler.
             * @param ec The boost::system::error_code.
             */
            void handle_udp_multicast_send_to(
                const boost::system::error_code & ec
            );
        
            /**
             * The getwork_work.
             */
            std::shared_ptr<getwork_work> m_getwork_work;
        
            /**
             * The stratum_work.
             */
            std::shared_ptr<stratum_work> m_stratum_work;

        protected:
        
            /**
             * The stack_impl.
             */
            stack_impl & stack_impl_;
        
            /**
             * The boost::asio::strand.
             */
            boost::asio::strand strand_;
            
            /**
             * The timer.
             */
            boost::asio::basic_waitable_timer<
                std::chrono::steady_clock
            > timer_;
        
            /**
             * The stratum connections.
             */
            std::vector<
                std::weak_ptr<stratum_connection>
            > stratum_connections_;
        
            /**
             * The currently selected work host index.
             */
            std::uint32_t work_host_index_;
        
            /**
             * The check work hosts timer.
             */
            boost::asio::basic_waitable_timer<
                std::chrono::steady_clock
            > timer_check_work_hosts_;
        
            /**
             * The current (getwork) block height.
             */
            std::int32_t getwork_current_block_;
        
            /**
             * The getwork poll timer timer.
             */
            boost::asio::basic_waitable_timer<
                std::chrono::steady_clock
            > timer_getwork_poll_;
        
            /**
             * The statistics timer timer.
             */
            boost::asio::basic_waitable_timer<
                std::chrono::steady_clock
            > timer_statistics_;
        
            /**
             * The local boost::asio::ip::udp::socket.
             */
            boost::asio::ip::udp::socket udp_socket_multicast_;
        
            /**
             * The remote boost::asio::ip::udp::socket.
             */
            boost::asio::ip::udp::endpoint udp_endpoint_sender_;
        
            /**
             * The maximum length of a multicast message.
             */
            enum { maximum_multicast_length = 1024 };
        
            /**
             * The multicast data.
             */
            char multicast_data_[maximum_multicast_length];
        
            /**
             * The multicast port.
             */
            enum { multicast_port = 9702 };
        
            /**
             * The multicast identifier.
             */
            std::uint32_t multicast_id_;
        
            /**
             * The multicast statistics.
             */
            std::map<
                std::string,
                std::pair<std::time_t, std::string>
            > multicast_statistics_;
        
            /**
             * The last time we have broadcast multicast statistics.
             */
            std::time_t time_last_multicast_send_;
    };
    
} // namespace miner

#endif // MINER_WORK_MANAGER_HPP
