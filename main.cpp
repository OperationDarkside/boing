#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <iostream>
#include <string>
#include <unordered_map>
#include <map>
#include <functional>
#include <optional>
//#include <uuid.h> // You might need a uuid lib, or use a simple random string

namespace net = boost::asio;
namespace beast = boost::beast;
namespace http = beast::http;
using tcp = net::ip::tcp;

// --- FRAMEWORK CORE COMPONENTS ---

/**
 * SessionData: Stores user-specific information.
 * In a real app, this might store User IDs, permissions, or shopping cart IDs.
 */
struct SessionData {
    std::string user_id;
    int visit_count = 0;
};

/**
 * SessionManager: Handles the lifecycle of cookies and session lookups.
 */
class SessionManager {
    std::unordered_map<std::string, SessionData> sessions_;
public:
    std::string create_session() {
        std::string sid = "sess_" + std::to_string(rand()); // Simplified ID generation
        sessions_[sid] = SessionData{"guest", 0};
        return sid;
    }

    SessionData* get_session(const std::string& sid) {
        if (sessions_.find(sid) != sessions_.end()) return &sessions_[sid];
        return nullptr;
    }
};

/**
 * Context: Passed to every route handler.
 * Encapsulates the Request, Response, and Session info so the user 
 * doesn't have to deal with Beast internals directly.
 */
struct Context {
    const http::request<http::string_body>& req;
    http::response<http::string_body>& res;
    SessionData* session;

    void text(std::string body) {
        res.body() = std::move(body);
        res.set(http::field::content_type, "text/plain");
    }

    void html(std::string body) {
        res.body() = std::move(body);
        res.set(http::field::content_type, "text/html");
    }
};

/**
 * Router: Maps HTTP Methods + Paths to specific Handler functions.
 */
class Router {
    using Handler = std::function<void(Context&)>;
    std::map<std::pair<http::verb, std::string>, Handler> routes_;

public:
    void add_route(http::verb method, std::string path, Handler handler) {
        routes_[{method, path}] = std::move(handler);
    }

    void route(Context& ctx) {
        auto it = routes_.find({ctx.req.method(), std::string(ctx.req.target())});
        if (it != routes_.end()) {
            it->second(ctx);
        } else {
            ctx.res.result(http::status::not_found);
            ctx.text("404 Not Found");
        }
    }
};

// --- SESSION HANDLING HELPERS ---

std::string get_session_id_from_cookie(const http::request<http::string_body>& req) {
    auto cookie_it = req.find(http::field::cookie);
    if (cookie_it != req.end()) {
        std::string cookie_val{cookie_it->value()};
        // Simple parser looking for "sid="
        size_t pos = cookie_val.find("sid=");
        if (pos != std::string::npos) return cookie_val.substr(pos + 4); 
    }
    return "";
}

// --- SESSION LOGIC ---

net::awaitable<void> do_session(tcp::socket socket, Router& router, SessionManager& sm) {
    beast::flat_buffer buffer;
    try {
        for (;;) {
            http::request<http::string_body> req;
            co_await http::async_read(socket, buffer, req, net::use_awaitable);

            // 1. Handle Sessions
            std::string sid = get_session_id_from_cookie(req);
            SessionData* current_session = sm.get_session(sid);
            bool is_new_session = false;

            if (!current_session) {
                sid = sm.create_session();
                current_session = sm.get_session(sid);
                is_new_session = true;
            }
            current_session->visit_count++;

            // 2. Prepare Response
            http::response<http::string_body> res{http::status::ok, req.version()};
            res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
            res.keep_alive(req.keep_alive());
            
            if (is_new_session) {
                res.set(http::field::set_cookie, "sid=" + sid + "; HttpOnly; Path=/");
            }

            // 3. Dispatch to Router
            Context ctx{req, res, current_session};
            router.route(ctx);

            // 4. Finalize and Write
            res.prepare_payload();
            co_await http::async_write(socket, res, net::use_awaitable);

            if (res.need_eof()) break;
        }
    } catch (std::exception const& e) {
        // Log error
    }
    beast::error_code ec;
    socket.shutdown(tcp::socket::shutdown_send, ec);
}

// --- LISTENER ---

net::awaitable<void> do_listen(tcp::endpoint endpoint, Router& router, SessionManager& sm) {
    auto executor = co_await net::this_coro::executor;
    tcp::acceptor acceptor(executor, endpoint);

    for (;;) {
        tcp::socket socket = co_await acceptor.async_accept(net::use_awaitable);
        // Pass references to router and session manager to the session coroutine
        net::co_spawn(executor, do_session(std::move(socket), router, sm), net::detached);
    }
}

// --- MAIN APPLICATION ---

int main() {
    Router app;
    SessionManager session_manager;

    // Define Routes like a Framework
    app.add_route(http::verb::get, "/", [](Context& ctx) {
        ctx.html("<h1>Welcome</h1><p>Check /stats to see session tracking.</p>");
    });

    app.add_route(http::verb::get, "/stats", [](Context& ctx) {
        std::string msg = "You have visited this site " + 
                          std::to_string(ctx.session->visit_count) + " times!";
        ctx.text(msg);
    });

    try {
        unsigned short port = 8080;
        auto const address = net::ip::make_address("0.0.0.0");
        net::io_context ioc{1};
        
        // Pass the app logic into the listener
        net::co_spawn(ioc, do_listen({address, port}, app, session_manager), net::detached);

        std::cout << "Framework running on http://localhost:" << port << std::endl;
        ioc.run();
    } catch (std::exception const& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
}