#include "session.cpp"
#include "session_manager.cpp"

#include <boost/beast/http.hpp>

#ifndef BOING_CONTEXT
#define BOING_CONTEXT

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
    };
}
#endif