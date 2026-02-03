#include <string>
#include <exception>
#include <iostream>

#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>

#include "router.cpp"

namespace boing
{

    namespace net = boost::asio;
    namespace beast = boost::beast;
    namespace http = beast::http;
    using tcp = net::ip::tcp;

    class webserver
    {
        router app;
        session_manager session_manager_;

    public:
        void start()
        {
            // Define Routes like a Framework
            app.add_route(http::verb::get, "/", [](context &ctx)
                          { ctx.html("<h1>Welcome</h1><p>Check /stats to see session tracking.</p>"); });

            app.add_route(http::verb::get, "/stats", [](context &ctx)
                          {
        std::string msg = "You have visited this site " + 
                          std::to_string(ctx.session_->visit_count) + " times!";
        ctx.text(msg); });

            try
            {
                unsigned short port = 8080;
                auto const address = net::ip::make_address("0.0.0.0");
                net::io_context ioc{1};

                // Pass the app logic into the listener
                net::co_spawn(ioc, do_listen({address, port}, app, session_manager_), net::detached);

                std::cout << "Framework running on http://localhost:" << port << std::endl;
                ioc.run();
            }
            catch (std::exception const &e)
            {
                std::cerr << "Error: " << e.what() << std::endl;
            }
        }

    private:
        // --- SESSION HANDLING HELPERS ---
        std::string get_session_id_from_cookie(const http::request<http::string_body> &req)
        {
            auto cookie_it = req.find(http::field::cookie);
            if (cookie_it != req.end())
            {
                std::string cookie_val{cookie_it->value()};
                // Simple parser looking for "sid="
                size_t pos = cookie_val.find("sid=");
                if (pos != std::string::npos)
                    return cookie_val.substr(pos + 4);
            }
            return "";
        }

        // --- SESSION LOGIC ---
        net::awaitable<void> do_session(tcp::socket socket, router &router_, session_manager &sm)
        {
            beast::flat_buffer buffer;
            try
            {
                for (;;)
                {
                    http::request<http::string_body> req;
                    co_await http::async_read(socket, buffer, req, net::use_awaitable);

                    // 1. Handle Sessions
                    std::string sid = get_session_id_from_cookie(req);
                    session *current_session = sm.get_session(sid);
                    bool is_new_session = false;

                    if (!current_session)
                    {
                        sid = sm.create_session();
                        current_session = sm.get_session(sid);
                        is_new_session = true;
                    }
                    current_session->visit_count++;

                    // 2. Prepare Response
                    http::response<http::string_body> res{http::status::ok, req.version()};
                    res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
                    res.keep_alive(req.keep_alive());

                    if (is_new_session)
                    {
                        res.set(http::field::set_cookie, "sid=" + sid + "; HttpOnly; Path=/");
                    }

                    // 3. Dispatch to Router
                    context ctx{req, res, current_session};
                    router_.route(ctx);

                    // 4. Finalize and Write
                    res.prepare_payload();
                    co_await http::async_write(socket, res, net::use_awaitable);

                    if (res.need_eof())
                        break;
                }
            }
            catch (std::exception const &e)
            {
                // Log error
            }
            beast::error_code ec;
            socket.shutdown(tcp::socket::shutdown_send, ec);
        }

        // --- LISTENER ---
        net::awaitable<void> do_listen(tcp::endpoint endpoint, router &router_, session_manager &sm)
        {
            auto executor = co_await net::this_coro::executor;
            tcp::acceptor acceptor(executor, endpoint);

            for (;;)
            {
                tcp::socket socket = co_await acceptor.async_accept(net::use_awaitable);
                // Pass references to router and session manager to the session coroutine
                net::co_spawn(executor, do_session(std::move(socket), router_, sm), net::detached);
            }
        }
    };
}