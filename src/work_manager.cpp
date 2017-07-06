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
#include <sstream>

#include <boost/asio.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>

#include <miner/configuration.hpp>
#include <miner/getwork.hpp>
#include <miner/getwork_work.hpp>
#include <miner/http_transport.hpp>
#include <miner/logger.hpp>
#include <miner/stack_impl.hpp>
#include <miner/statistics.hpp>
#include <miner/stratum.hpp>
#include <miner/stratum_connection.hpp>
#include <miner/stratum_work.hpp>
#include <miner/utility.hpp>
#include <miner/work_manager.hpp>

using namespace miner;

work_manager::work_manager(stack_impl & owner)
    : stack_impl_(owner)
    , strand_(owner.io_service())
    , timer_(owner.io_service())
    , timer_check_work_hosts_(owner.io_service())
    , work_host_index_(0)
    , getwork_current_block_(-1)
    , timer_getwork_poll_(owner.io_service())
    , timer_statistics_(owner.io_service())
{
    // ...
}

void work_manager::start()
{
    if (configuration::instance().work_hosts().size() > 0)
    {
        log_info(
            "Work manager is switching to (primary) work host " <<
            work_host_index_ << "."
        );
        
        if (
            configuration::instance().work_host_type() ==
            configuration::work_host_type_getwork
            )
        {
            getwork::instance().set_host(
                configuration::instance().work_hosts()[
                work_host_index_].second.first
            );
            getwork::instance().set_port(
                configuration::instance().work_hosts()[
                work_host_index_].second.second
            );
            getwork::instance().set_username(
                configuration::instance().work_hosts()[
                work_host_index_].first.first
            );
            getwork::instance().set_password(
                configuration::instance().work_hosts()[
                work_host_index_].first.second
            );
        }
        else if (
            configuration::instance().work_host_type() ==
            configuration::work_host_type_stratum
            )
        {
            stratum::instance().set_host(
                configuration::instance().work_hosts()[
                work_host_index_].second.first
            );
            stratum::instance().set_port(
                configuration::instance().work_hosts()[
                work_host_index_].second.second
            );
            stratum::instance().set_username(
                configuration::instance().work_hosts()[
                work_host_index_].first.first
            );
            stratum::instance().set_password(
                configuration::instance().work_hosts()[
                work_host_index_].first.second
            );
        }
        else
        {
            throw std::runtime_error("invalid work_host_type");
        }
    }
    else
    {
        throw std::runtime_error("no work hosts");
    }
    
    timer_.expires_from_now(std::chrono::seconds(8));
    timer_.async_wait(std::bind(
        &work_manager::tick, this, std::placeholders::_1)
    );
    
    /**
     * Send a (getwork) RPC getblockcount request to get the best block height.
     */
    if (
        configuration::instance().work_host_type() ==
        configuration::work_host_type_getwork
        )
    {
        timer_getwork_poll_.expires_from_now(std::chrono::seconds(0));
        timer_getwork_poll_.async_wait(std::bind(
            &work_manager::tick_getwork_poll, this, std::placeholders::_1)
        );
    }
    else
    {
        connect();
    }
    
    timer_statistics_.expires_from_now(std::chrono::seconds(60));
    timer_statistics_.async_wait(std::bind(
        &work_manager::tick_statistics, this, std::placeholders::_1)
    );
}

void work_manager::stop()
{
    timer_.cancel();
    timer_check_work_hosts_.cancel();
    timer_getwork_poll_.cancel();
    timer_statistics_.cancel();
    
    if (
        configuration::instance().work_host_type() ==
        configuration::work_host_type_stratum
        )
    {
        for (auto & i : stratum_connections_)
        {
            if (auto j = i.lock())
            {
                j->stop();
            }
        }
    }
}

void work_manager::set_getwork_work(const std::shared_ptr<getwork_work> & val)
{
    m_getwork_work = val;
    
    auto self(shared_from_this());
    
    stack_impl_.io_service().post(strand_.wrap(
        [this, self] ()
        {
            stack_impl_.handle_work(m_getwork_work);
        })
    );
}

void work_manager::set_stratum_work(const std::shared_ptr<stratum_work> & val)
{
    m_stratum_work = val;
    
    auto self(shared_from_this());
    
    stack_impl_.io_service().post(strand_.wrap(
        [this, self] ()
        {
            stack_impl_.handle_work(m_stratum_work);
        })
    );
}

void work_manager::submit_getwork_work(
    const std::shared_ptr<getwork_work> & val
    )
{
    for (auto i = 0; i < val->data().size(); i++)
    {
        utility::le32enc(&val->data()[i], val->data()[i]);
    }

    std::vector<uint8_t> bytes(
        reinterpret_cast<std::uint8_t *> (&val->data()[0]),
        reinterpret_cast<std::uint8_t *> (&val->data()[0]) +
        val->data().size() * sizeof(std::uint32_t)
    );
    
    auto param = utility::to_hex(bytes);
    
    std::string body =
        "{\"method\": \"getwork\", \"params\": [ \"" + param +
        "\" ], \"id\":1}\r\n"
    ;
    
    log_info("body = " << body);
    
    /**
     * Allocate the http_transport.
     */
    std::shared_ptr<http_transport> t =
        std::make_shared<http_transport>(stack_impl_.io_service(),
        strand_, "http://" + getwork::instance().host())
    ;
    
    /**
     * The headers.
     */
    std::map<std::string, std::string> headers;
    
    /**
     * Set the content-length.
     */
    headers["content-length"] = body.size();
    
    /**
     * Set the headers.
     */
    t->headers() = headers;

    /**
     * Set the body.
     */
    t->set_request_body(body);

    auto self(shared_from_this());
    
    /**
     * Start the transport.
     */
    t->start(
        [this, self](boost::system::error_code ec,
            std::shared_ptr<http_transport> t)
        {
            if (ec)
            {
                std::cerr <<
                    "http_transport request failed, message = " <<
                    ec.message() <<
                std::endl;
            }
            else
            {
                if (t->status_code() == 200)
                {
                    boost::property_tree::ptree pt;
                    
                    std::map<std::string, std::string> result;
                    
                    try
                    {
                        std::stringstream ss;

                        ss << t->response_body();
                    
                        read_json(ss, pt);

                        auto result =
                            pt.get("result", std::to_string(false)) == "true"
                        ;

                        if (result == true)
                        {
                            getwork::instance().set_shares_accepted(
                                getwork::instance().shares_accepted() + 1
                            );
                        }
                        else
                        {
                            getwork::instance().set_shares_rejected(
                                getwork::instance().shares_rejected() + 1
                            );
                        }
                        
                        const auto & shares_accepted =
                            getwork::instance().shares_accepted()
                        ;
                        const auto & shares_rejected =
                            getwork::instance().shares_rejected()
                        ;

                        log_info(
                            "Getwork connection handled getwork result, " <<
                            shares_accepted << "/" << shares_accepted +
                            shares_rejected << " (" << 100.0f * shares_accepted /
                            (shares_accepted + shares_rejected) << "%) " <<
                            (result ? "accepted" : "rejected") << " at " <<
                            std::fixed << std::setprecision(2) <<
                            statistics::instance().hashes_per_second() << " H/s."
                        );
                        
                        timer_getwork_poll_.expires_from_now(
                            std::chrono::seconds(0)
                        );
                        timer_getwork_poll_.async_wait(std::bind(
                            &work_manager::tick_getwork_poll, this,
                            std::placeholders::_1)
                        );
                    }
                    catch (std::exception & e)
                    {
                        log_error(
                            "Work manager (getwork) failed to parse "
                            "JSON-RPC response, what = " << e.what() << "."
                        );
                    }
                }
                else
                {
                    log_error(
                        "Work manager http transport request failed, status "
                        "code = " << t->status_code() << "."
                    );
                }
            }
        }
    , getwork::instance().port());
}

void work_manager::submit_stratum_work(
    const std::shared_ptr<stratum_work> & val
    )
{
    /**
     * Update the statistics.
     */
    stack_impl_.update_statistics();
    
    auto time = utility::to_hex(val->time());
    
    std::uint32_t nonce_little = utility::le32dec(&val->data()[19]);

    auto nonce = utility::to_hex(
        reinterpret_cast<std::uint8_t *>(&nonce_little),
        reinterpret_cast<std::uint8_t *>(&nonce_little) + sizeof(std::uint32_t)
    );
    
    auto extranonce2 = utility::to_hex(val->extranonce2());

    std::string json_line =
        "{\"params\": [\"" + val->worker_name() +
        "\", \"" + val->job_id() + "\", \"" + extranonce2 + "\", \"" + time +
        "\", \"" + nonce + "\"], \"id\": 4, \"method\": \"mining.submit\"}\n"
    ;

    for (auto & i : stratum_connections_)
    {
        if (auto j = i.lock())
        {
            j->write(json_line);
        }
    }
}

void work_manager::connect()
{
    if (
        configuration::instance().work_host_type() ==
        configuration::work_host_type_getwork
        )
    {
        log_info("Work manager is connecting (getwork).");
        
        /**
         * The JSON (getwork) request.
         */
        std::string json_request_getwork =
            "{\"method\": \"getwork\", \"params\": [], \"id\":0}\r\n"
        ;
    
        /**
         * Allocate the http_transport.
         */
        std::shared_ptr<http_transport> t =
            std::make_shared<http_transport>(stack_impl_.io_service(),
            strand_, "http://" + getwork::instance().host())
        ;
        
        /**
         * The headers.
         */
        std::map<std::string, std::string> headers;
        
        /**
         * The body.
         */
        std::string body;
        
        /**
         * Set the body.
         */
        body = json_request_getwork;
        
        /**
         * Set the content-length.
         */
        headers["content-length"] = body.size();
        
        /**
         * Set the headers.
         */
        t->headers() = headers;
    
        /**
         * Set the body.
         */
        t->set_request_body(body);
    
        auto self(shared_from_this());
        
        /**
         * Start the transport.
         */
        t->start(
            [this, self](boost::system::error_code ec,
                std::shared_ptr<http_transport> t)
            {
                if (ec)
                {
                    log_error(
                        "Work manager (getwork) connection to " <<
                        getwork::instance().host() << ":" <<
                        getwork::instance().port() << " failed (" <<
                        ec.message() << ")."
                    );
                    
                    /**
                     * Try to set a backup work hosts if possible.
                     */
                    if (
                        configuration::instance().work_hosts().size() >
                        work_host_index_ + 1 && work_host_index_ <= 1
                        )
                    {
                        /**
                         * Increment the work index.
                         */
                        ++work_host_index_;
                        
                        log_info(
                            "Work manager is switching to (backup) work "
                            "host " << work_host_index_ << "."
                        );
                        
                        /**
                         * Start the check work hosts timer.
                         */
                        timer_check_work_hosts_.expires_from_now(
                            std::chrono::seconds(8)
                        );
                        timer_check_work_hosts_.async_wait(std::bind(
                            &work_manager::tick_check_work_hosts, this,
                            std::placeholders::_1)
                        );
                        
                        /**
                         * Switch to the backup host.
                         */
                        getwork::instance().set_host(
                            configuration::instance().work_hosts()[
                            work_host_index_].second.first
                        );
                        getwork::instance().set_port(
                            configuration::instance().work_hosts()[
                            work_host_index_].second.second
                        );
                        getwork::instance().set_username(
                            configuration::instance().work_hosts()[
                            work_host_index_].first.first
                        );
                        getwork::instance().set_password(
                            configuration::instance().work_hosts()[
                            work_host_index_].first.second
                        );
                        
                        /**
                         * The next getwork request will use the backup host
                         * while the timer will probe the primary host until
                         * it is reachable and if so switch to it.
                         */
                    }
                    else
                    {
                        log_info(
                            "Work manager (primary) (getwork) work host " <<
                            work_host_index_ << " is down, will retry."
                        );
                        
                        /**
                         * Start the check work hosts timer.
                         */
                        timer_check_work_hosts_.expires_from_now(
                            std::chrono::seconds(60)
                        );
                        timer_check_work_hosts_.async_wait(std::bind(
                            &work_manager::tick_check_work_hosts, this,
                            std::placeholders::_1)
                        );
                    }
                }
                else
                {
                    if (t->status_code() == 200)
                    {
                        boost::property_tree::ptree pt;
                        
                        std::map<std::string, std::string> result;
                        
                        try
                        {
                            std::stringstream ss;

                            ss << t->response_body();
                        
                            read_json(ss, pt);
                            
                            auto & pt_result = pt.get_child("result");
                            
                            auto & pt_data = pt_result.get_child("data");
                            auto & pt_target = pt_result.get_child("target");
                            
                            auto hex_data = pt_data.get<std::string> ("");
                            auto hex_target = pt_target.get<std::string> ("");
  
                            auto data = utility::from_hex(hex_data);
                            
                            assert(data.size() == 128);
                            
                            auto target = utility::from_hex(hex_target);
                            
                            assert(target.size() == 32);
                            
                            auto work = std::make_shared<getwork_work> (
                                data, target
                            );
                            
                            if (work)
                            {
                                set_getwork_work(work);
                            }
                        }
                        catch (std::exception & e)
                        {
                            log_error(
                                "Work manager (getwork) failed to parse "
                                "JSON-RPC response, what = " << e.what() << "."
                            );
                        }
                    }
                    else
                    {
                        log_error(
                            "Work manager http transport request failed, "
                            "status code = " << t->status_code() << "."
                        );
                    }
                }
            }
        , getwork::instance().port());
    }
    else if (
        configuration::instance().work_host_type() ==
        configuration::work_host_type_stratum
        )
    {
        log_info("Work manager is connecting (stratum).");
        
        /**
         * Allocate a stratum_connection.
         */
        auto c = std::make_shared<stratum_connection> (stack_impl_);
        
        /**
         * Retain the stratum_connection.
         */
        stratum_connections_.push_back(c);
        
        /**
         * Start the stratum_connection.
         */
        c->start();
    }
    else
    {
        throw std::runtime_error("invalid work_host_type");
    }
}

void work_manager::getwork_send_getblockcount()
{
    std::string body =
        "{\"method\": \"getblockcount\", \"params\": [], \"id\":1}\r\n"
    ;
    
    /**
     * Allocate the http_transport.
     */
    std::shared_ptr<http_transport> t =
        std::make_shared<http_transport>(stack_impl_.io_service(),
        strand_, "http://" + getwork::instance().host())
    ;
    
    /**
     * The headers.
     */
    std::map<std::string, std::string> headers;
    
    /**
     * Set the content-length.
     */
    headers["content-length"] = body.size();
    
    /**
     * Set the headers.
     */
    t->headers() = headers;

    /**
     * Set the body.
     */
    t->set_request_body(body);

    auto self(shared_from_this());
    
    /**
     * Start the transport.
     */
    t->start(
        [this, self](boost::system::error_code ec,
            std::shared_ptr<http_transport> t)
        {
            if (ec)
            {
                // ...
            }
            else
            {
                if (t->status_code() == 200)
                {
                    boost::property_tree::ptree pt;
                    
                    std::map<std::string, std::string> result;
                    
                    try
                    {
                        std::stringstream ss;

                        ss << t->response_body();
         
                        read_json(ss, pt);

                        boost::property_tree::ptree pt_result;
            
                        pt_result = pt.get_child("result");
       
                        auto blocks = pt_result.get("", 0);
                        
                        /**
                         * Check if the block height has changed.
                         */
                        if (getwork_current_block_ != blocks)
                        {
                            getwork_current_block_ = blocks;
                            
                            log_info(
                                "Work manager detected (getwork) block change "
                                "to " << getwork_current_block_ << "."
                            );
                            
                            /**
                             * Abort all current work.
                             */
                            set_getwork_work(nullptr);
                            
                            /**
                             * Connect to a (getwork) work server.
                             */
                            connect();
                        }
                    }
                    catch (std::exception & e)
                    {
                        log_error(
                            "Work manager (getwork/getinfo) failed to parse "
                            "JSON-RPC response, what = " << e.what() << "."
                        );
                    }
                }
                else
                {
                    log_error(
                        "Work manager http transport request failed, status "
                        "code = " << t->status_code() << "."
                    );
                }
            }
        }
    , getwork::instance().port());
}

void work_manager::tick(const boost::system::error_code & ec)
{
    if (ec)
    {
        // ...
    }
    else
    {
        /**
         * The timer timeout in seconds.
         */
        auto timeout = 8;
        
        if (
            configuration::instance().work_host_type() ==
            configuration::work_host_type_getwork
            )
        {
            log_info("Work manager is fetching new (getwork) work.");
            
            /**
             * Abort all current work.
             */
            set_getwork_work(nullptr);
            
            /**
             * Connect to a (getwork) work server.
             */
            connect();
            
            /**
             * We fetch (getwork) work every 60 seconds.
             */
            timeout = 60;
        }
        else if (
            configuration::instance().work_host_type() ==
            configuration::work_host_type_stratum
            )
        {
            /**
             * If the connections are empty we need a reconnect.
             */
            auto needs_reconnect = stratum_connections_.size() == 0;
            
            auto connections = 0;
            
            for (auto & i : stratum_connections_)
            {
                if (auto j = i.lock())
                {
                    connections++;
                }
            }
            
            log_debug(
                "Work manager tick, connections = " << connections << "."
            );
            
            if (needs_reconnect || connections == 0)
            {
                log_error("Work manager disconnected.");

                /**
                 * Abort work.
                 */
                set_stratum_work(nullptr);

                /**
                 * Try to set a backup work hosts if possible.
                 */
                if (
                    configuration::instance().work_hosts().size() >
                    work_host_index_ + 1
                    )
                {
                    /**
                     * Increment the work index.
                     */
                    ++work_host_index_;
                    
                    log_info(
                        "Work manager is switching to (backup) work host " <<
                        work_host_index_ << "."
                    );
                    
                    /**
                     * Start the check work hosts timer.
                     */
                    timer_check_work_hosts_.expires_from_now(
                        std::chrono::seconds(60)
                    );
                    timer_check_work_hosts_.async_wait(std::bind(
                        &work_manager::tick_check_work_hosts, this,
                        std::placeholders::_1)
                    );
                    
                    stratum::instance().set_host(
                        configuration::instance().work_hosts()[
                        work_host_index_].second.first
                    );
                    stratum::instance().set_port(
                        configuration::instance().work_hosts()[
                        work_host_index_].second.second
                    );
                    stratum::instance().set_username(
                        configuration::instance().work_hosts()[
                        work_host_index_].first.first
                    );
                    stratum::instance().set_password(
                        configuration::instance().work_hosts()[
                        work_host_index_].first.second
                    );
            
                    /**
                     * Connect to a work server.
                     */
                    connect();
                }
                else
                {
                    log_info(
                        "Work manager (primary) work host " <<
                        work_host_index_ << " is down, will retry."
                    );
                    
                    /**
                     * Start the check work hosts timer.
                     */
                    timer_check_work_hosts_.expires_from_now(
                        std::chrono::seconds(60)
                    );
                    timer_check_work_hosts_.async_wait(std::bind(
                        &work_manager::tick_check_work_hosts, this,
                        std::placeholders::_1)
                    );
                }
                
                /**
                 * Set the timeout to N to allow for work host check to complete.
                 */
                timeout = 90;
                
                assert(timeout > 60);
            }
        }
        else
        {
            throw std::runtime_error("invalid work_host_type");
        }

        /**
         * Update the statistics.
         */
        stack_impl_.update_statistics();
    
        timer_.expires_from_now(std::chrono::seconds(timeout));
        timer_.async_wait(std::bind(
            &work_manager::tick, this, std::placeholders::_1)
        );
    }
}

void work_manager::tick_check_work_hosts(const boost::system::error_code & ec)
{
    if (ec)
    {
        // ...
    }
    else
    {
        boost::asio::ip::tcp::iostream tcp_stream;
        
        /**
         * Check connectivity.
         */
        tcp_stream.connect(
            configuration::instance().work_hosts()[0].second.first,
            std::to_string(configuration::instance().work_hosts()[
            0].second.second)
        );
        
        if (!tcp_stream || tcp_stream.error())
        {
            log_info(
                "Work manager (primary) work host " <<
                configuration::instance().work_hosts()[0].second.first <<
                " check failed, will retry."
            );
            
            /**
             * Start the check work hosts timer.
             */
            timer_check_work_hosts_.expires_from_now(
                std::chrono::seconds(60)
            );
            timer_check_work_hosts_.async_wait(std::bind(
                &work_manager::tick_check_work_hosts, this,
                std::placeholders::_1)
            );
        }
        else
        {
            log_info(
                "Work manager (primary) work host check success."
            );
            
            /**
             * Close the boost::asio::ip::tcp::iostream.
             */
            tcp_stream.close();
            
            if (
                configuration::instance().work_host_type() ==
                configuration::work_host_type_getwork
                )
            {
                /**
                 * Reset the work index.
                 */
                work_host_index_ = 0;
            
                log_info(
                    "Work manager is switching to (primary) work host " <<
                    work_host_index_ << "."
                );

                getwork::instance().set_host(
                    configuration::instance().work_hosts()[
                    work_host_index_].second.first
                );
                getwork::instance().set_port(
                    configuration::instance().work_hosts()[
                    work_host_index_].second.second
                );
                getwork::instance().set_username(
                    configuration::instance().work_hosts()[
                    work_host_index_].first.first
                );
                getwork::instance().set_password(
                    configuration::instance().work_hosts()[
                    work_host_index_].first.second
                );
        
                /**
                 * Connect to a work server.
                 */
                connect();
            }
            else if (
                configuration::instance().work_host_type() ==
                configuration::work_host_type_stratum
                )
            {
                /**
                 * Disconnect all backup work hosts.
                 */
                for (auto & i : stratum_connections_)
                {
                    if (auto j = i.lock())
                    {
                        j->stop();
                    }
                }
                
                /**
                 * Reset the work index.
                 */
                work_host_index_ = 0;
            
                log_info(
                    "Work manager is switching to (primary) work host " <<
                    work_host_index_ << "."
                );

                stratum::instance().set_host(
                    configuration::instance().work_hosts()[
                    work_host_index_].second.first
                );
                stratum::instance().set_port(
                    configuration::instance().work_hosts()[
                    work_host_index_].second.second
                );
                stratum::instance().set_username(
                    configuration::instance().work_hosts()[
                    work_host_index_].first.first
                );
                stratum::instance().set_password(
                    configuration::instance().work_hosts()[
                    work_host_index_].first.second
                );
        
                /**
                 * Connect to a work server.
                 */
                connect();
            }
            else
            {
                // ...
            }
        }
    }
}

void work_manager::tick_getwork_poll(const boost::system::error_code & ec)
{
    if (ec)
    {
        // ...
    }
    else
    {
        /**
         * Check if the block height has changed.
         */
        getwork_send_getblockcount();
        
        timer_getwork_poll_.expires_from_now(std::chrono::seconds(4));
        timer_getwork_poll_.async_wait(std::bind(
            &work_manager::tick_getwork_poll, this, std::placeholders::_1)
        );
    }
}

void work_manager::tick_statistics(const boost::system::error_code & ec)
{
    if (ec)
    {
        // ...
    }
    else
    {
        /**
         * Update the statistics.
         */
        stack_impl_.update_statistics();
        
        std::uint32_t shares_accepted = 0;
        std::uint32_t shares_rejected = 0;
        
        if (
            configuration::instance().work_host_type() ==
            configuration::work_host_type_getwork
            )
        {
            shares_accepted = getwork::instance().shares_accepted();
            shares_rejected = getwork::instance().shares_rejected();
        }
        else if (
            configuration::instance().work_host_type() ==
            configuration::work_host_type_stratum
            )
        {
            shares_accepted = stratum::instance().shares_accepted();
            shares_rejected = stratum::instance().shares_rejected();
        }
        
        auto percentage = 0.0;
        
        if (shares_accepted + shares_rejected > 0)
        {
            percentage =
                100.0f * shares_accepted / (shares_accepted + shares_rejected)
            ;
        }
        
        log_info(
            "Statistics:\n " <<
            "\tAccepted: " << shares_accepted << "(" << percentage << "%)\n" <<
            "\tRejected: " << shares_rejected << "\n" <<
            std::fixed << std::setprecision(2) <<
            "\tHashrate: " << statistics::instance().hashes_per_second() <<
            " H/s."
        );
        
        timer_statistics_.expires_from_now(std::chrono::seconds(60));
        timer_statistics_.async_wait(std::bind(
            &work_manager::tick_statistics, this, std::placeholders::_1)
        );
    }
}
