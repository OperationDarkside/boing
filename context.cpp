#ifndef BOING_CONTEXT
#define BOING_CONTEXT

#include <string>

#include "session.cpp"
#include "session_manager.cpp"

#include <boost/beast/http.hpp>
#include <boost/url.hpp>

namespace boing
{
    namespace http = boost::beast::http;
    /**
     * context: Passed to every route handler.
     * Encapsulates the Request, Response, and Session info so the user
     * doesn't have to deal with Beast internals directly.
     */
    struct context
    {
        const http::request<http::string_body> &req;
        http::response<http::string_body> &res;
        session *session_;
        std::string query_params{};
        boost::urls::params_view params{};

        void text(std::string body)
        {
            res.body() = std::move(body);
            res.set(http::field::content_type, "text/plain");
        }

        void html(std::string body)
        {
            res.body() = std::move(body);
            res.set(http::field::content_type, "text/html");
        }

        void json(std::string body)
        {
            res.body() = std::move(body);
            res.set(http::field::content_type, "application/json");
        }
    };
}
#endif