#ifndef SERVER_HPP
#define SERVER_HPP

#include "server/connection.hpp"
#include "server/request_handler.hpp"
#include "server/service_handler.hpp"

#include "util/integer_range.hpp"
#include "util/log.hpp"

#include <boost/asio.hpp>
#include <boost/bind.hpp>

#include <zlib.h>

#ifndef _WIN32
#include <sys/socket.h>
#include <sys/types.h>
#endif

#include <memory>
#include <string>
#include <thread>
#include <vector>

namespace osrm::server
{

class Server
{
  public:
    // Note: returns a shared instead of a unique ptr as it is captured in a lambda somewhere else
    static std::shared_ptr<Server> CreateServer(std::string &ip_address,
                                                int ip_port,
                                                unsigned requested_num_threads,
                                                short keepalive_timeout)
    {
        util::Log() << "http 1.1 compression handled by zlib version " << zlibVersion();
        const unsigned hardware_threads = std::max(1u, std::thread::hardware_concurrency());
        const unsigned real_num_threads = std::min(hardware_threads, requested_num_threads);
        return std::make_shared<Server>(ip_address, ip_port, real_num_threads, keepalive_timeout);
    }

    explicit Server(const std::string &address,
                    const int port,
                    const unsigned thread_pool_size,
                    const short keepalive_timeout)
        : thread_pool_size(thread_pool_size), keepalive_timeout(keepalive_timeout),
          acceptor(io_context), new_connection(std::make_shared<Connection>(
                                    io_context, request_handler, keepalive_timeout))
    {
        const auto port_string = std::to_string(port);

        boost::asio::ip::tcp::resolver resolver(io_context);
        boost::asio::ip::tcp::endpoint endpoint = *resolver.resolve(address, port_string).begin();

        acceptor.open(endpoint.protocol());
#ifdef SO_REUSEPORT
        const int option = 1;
        setsockopt(acceptor.native_handle(), SOL_SOCKET, SO_REUSEPORT, &option, sizeof(option));
#endif
        acceptor.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
        acceptor.bind(endpoint);
        acceptor.listen();

        util::Log() << "Listening on: " << acceptor.local_endpoint();

        acceptor.async_accept(
            new_connection->socket(),
            boost::bind(&Server::HandleAccept, this, boost::asio::placeholders::error));
    }

    void Run()
    {
        std::vector<std::shared_ptr<std::thread>> threads;
        for (unsigned i = 0; i < thread_pool_size; ++i)
        {
            std::shared_ptr<std::thread> thread = std::make_shared<std::thread>(
                boost::bind(&boost::asio::io_context::run, &io_context));
            threads.push_back(thread);
        }
        for (const auto &thread : threads)
        {
            thread->join();
        }
    }

    void Stop() { io_context.stop(); }

    void RegisterServiceHandler(std::unique_ptr<ServiceHandlerInterface> service_handler_)
    {
        request_handler.RegisterServiceHandler(std::move(service_handler_));
    }

  private:
    void HandleAccept(const boost::system::error_code &e)
    {
        if (!e)
        {
            new_connection->start();
            new_connection =
                std::make_shared<Connection>(io_context, request_handler, keepalive_timeout);
            acceptor.async_accept(
                new_connection->socket(),
                boost::bind(&Server::HandleAccept, this, boost::asio::placeholders::error));
        }
        else
        {
            util::Log(logERROR) << "HandleAccept error: " << e.message();
        }
    }

    RequestHandler request_handler;
    unsigned thread_pool_size;
    short keepalive_timeout;
    boost::asio::io_context io_context;
    boost::asio::ip::tcp::acceptor acceptor;
    std::shared_ptr<Connection> new_connection;
};
} // namespace osrm::server

#endif // SERVER_HPP
