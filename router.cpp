#include <functional>
#include <map>
#include <string>

#include <boost/beast/http.hpp>

#include "context.cpp"

#ifndef BOING_ROUTER
#define BOING_ROUTER

namespace boing
{
    namespace http = boost::beast::http;
    /**
     * Router: Maps HTTP Methods + Paths to specific Handler functions.
     */
    class router
    {
        using handler = std::function<void(context &)>;
        std::map<std::pair<http::verb, std::string>, handler> routes_;

    public:
        void add_route(http::verb method, std::string path, handler handler_)
        {
            routes_[{method, path}] = std::move(handler_);
        }

        void route(context &ctx)
        {
            auto it = routes_.find({ctx.req.method(), std::string(ctx.req.target())});
            if (it != routes_.end())
            {
                it->second(ctx);
            }
            else
            {
                ctx.res.result(http::status::not_found);
                ctx.text("404 Not Found");
            }
        }
    };
}
#endif