#include <string>
#include <exception>
#include <meta>
#include <print>

#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>

#include "router.cpp"
#include "annotations.cpp"

namespace boing
{

    namespace net = boost::asio;
    namespace beast = boost::beast;
    namespace http = beast::http;
    using tcp = net::ip::tcp;

    template <std::meta::info inf>
    class webserver
    {
        router app;
        session_manager session_manager_;

        constexpr static auto ctx = std::meta::access_context::current();
        constexpr static auto ns_members = std::define_static_array(std::meta::members_of(inf, ctx));
        constexpr static std::size_t ns_members_size = ns_members.size();

    public:
        webserver()
        {
            constexpr static auto ns_members_indices = make_indices_array<ns_members_size>();
            template for (constexpr auto i : ns_members_indices)
            {
                // NAMESPACE MEMBERS
                constexpr auto m = ns_members[i];
                constexpr auto m_annotations = std::define_static_array(std::meta::annotations_of(m));
                if constexpr (m_annotations.size() > 0)
                {
                    constexpr static auto m_anno = m_annotations[0];
                    constexpr static auto m_anno_type = std::meta::type_of(m_anno);
                    constexpr static auto ctrl_type = ^^controller<0>;
                    if constexpr (std::meta::template_of(m_anno_type) == std::meta::template_of(ctrl_type))
                    {
                        // MEMBERS WITH CONTROLLER ANNOTATION
                        using ctrl_anno_type = [:m_anno_type:];
                        constexpr auto ctrl_obj = std::meta::extract<ctrl_anno_type>(m_anno);
                        // CONTROLLER PATH
                        const auto ctrl_path = ctrl_obj.path;
                        // CONTROLLER INSTANCE
                        static typename[:m:] m_instance;

                        constexpr static auto class_members = std::define_static_array(std::meta::members_of(m, ctx));
                        constexpr static auto class_members_indices = make_indices_array<class_members.size()>();
                        template for (constexpr auto j : class_members_indices)
                        {
                            // CLASS MEMBERS
                            constexpr auto cm = class_members[j];
                            if constexpr (std::meta::is_function(cm))
                            {
                                constexpr auto cm_annotations = std::define_static_array(std::meta::annotations_of(cm));
                                if constexpr (cm_annotations.size() > 0)
                                {
                                    constexpr static auto fn_anno = cm_annotations[0];
                                    constexpr static auto fn_anno_type = std::meta::type_of(fn_anno);
                                    constexpr static auto get_type = ^^GET<0>;
                                    constexpr static auto post_type = ^^POST<0>;
                                    if constexpr (std::meta::template_of(fn_anno_type) == std::meta::template_of(get_type))
                                    {
                                        // GET annotation
                                        using get_anno_type = [:fn_anno_type:];
                                        constexpr auto get_obj = std::meta::extract<get_anno_type>(fn_anno);
                                        const auto get_path = get_obj.path;

                                        // RT
                                        std::string full_path = ctrl_path;
                                        full_path += get_path;

                                        if constexpr (std::meta::is_static_member(cm))
                                        {
                                            app.add_route(http::verb::get, full_path, [:cm:]);
                                        }
                                        else
                                        {
                                            app.add_route(http::verb::get, full_path, [](context &ctx)
                                                          { m_instance.[:cm:](ctx); });
                                        }
                                    }
                                    else if constexpr (std::meta::template_of(fn_anno_type) == std::meta::template_of(post_type))
                                    {
                                        // POST ANNOTATION
                                        using post_anno_type = [:fn_anno_type:];
                                        constexpr auto post_obj = std::meta::extract<post_anno_type>(fn_anno);
                                        const auto post_path = post_obj.path;

                                        // RT
                                        std::string full_path = ctrl_path;
                                        full_path += post_path;

                                        if constexpr (std::meta::is_static_member(cm))
                                        {
                                            app.add_route(http::verb::post, full_path, [:cm:]);
                                        }
                                        else
                                        {
                                            app.add_route(http::verb::post, full_path, [](context &ctx)
                                                          { m_instance.[:cm:](ctx); });
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        void start(const std::string_view address = "0.0.0.0", unsigned short port = 8080)
        {
            try
            {
                auto const ip_address = net::ip::make_address(address);
                net::io_context ioc{1};

                // Pass the app logic into the listener
                net::co_spawn(ioc, do_listen({ip_address, port}, app, session_manager_), net::detached);

                std::println("Boing webserver running on {}:{}", address, port);
                ioc.run();
            }
            catch (std::exception const &e)
            {
                std::println("Error: {}", e.what());
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
                    // current_session->visit_count++;

                    // 2. Prepare Response
                    http::response<http::string_body> res{http::status::ok, req.version()};
                    res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
                    res.keep_alive(req.keep_alive());

                    if (is_new_session)
                    {
                        res.set(http::field::set_cookie, "sid=" + sid + "; HttpOnly; Path=/");
                    }

                    // 3. Dispatch to Router
                    context ctx{req, res, current_session, ""};
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

        template <std::size_t N>
        consteval static std::array<std::size_t, N> make_indices_array()
        {
            std::array<std::size_t, N> a{};
            for (std::size_t i = 0; i < N; i++)
            {
                a[i] = i;
            }
            return a;
        }
    };
}