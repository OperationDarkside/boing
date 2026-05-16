#ifndef BOING_ROUTER
#define BOING_ROUTER

#include <functional>
#include <map>
#include <string>
#include <print>

#include <boost/beast/http.hpp>
#include <boost/url.hpp>

#include "context.cpp"

namespace boing
{
    namespace http = boost::beast::http;
    /**
     * Router: Maps HTTP Methods + Paths to specific Handler functions.
     */
    template <is_session session>
    class router
    {
        using handler = std::function<void(context<session> &)>;
        std::map<std::pair<http::verb, std::string>, handler> routes_{};

    public:
        void add_route(http::verb method, std::string path, handler handler_)
        {
            routes_[{method, path}] = std::move(handler_);
        }

        void route(context<session> &ctx)
        {
            const auto url_view_res = boost::urls::parse_origin_form(ctx.req.target());
            if (url_view_res.has_value())
            {
                const auto &url_view = url_view_res.value();
                const auto path = url_view.encoded_path();
                // std::println("path: {}", path);
                if (url_view.has_query())
                {
                    const auto query = url_view.encoded_query();
                    ctx.query_params = std::string(query);
                    ctx.params = url_view.params();
                    //auto a = *ctx.params.find("bla");
                    //a.
                    /*
                    const auto params = url_view.params();
                    for(const auto& p : params) {
                        std::println("key: {} - value: {}", p.key, p.value);
                    }
                    std::println("query: {}", ctx.query_params);
                    */
                }

                auto it = routes_.find({ctx.req.method(), std::string(path)});
                if (it != routes_.end())
                {
                    it->second(ctx); // Call endpoint
                }
                else
                {
                    std::println("Router: Route \"{}\" not found", std::string(ctx.req.target()));
                    ctx.res.result(http::status::not_found);
                    ctx.text("404 Not Found");
                }
            }
            else
            {
                const auto ec = url_view_res.error();
                std::println("Router: Route \"{}\" could not be parsed. Error: \"{}\"", std::string(ctx.req.target()), ec.what());
                ctx.res.result(http::status::bad_request);
                ctx.text("400 url could not be parsed");
            }
        }
    };
}
#endif