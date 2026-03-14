#include <string>
#include <exception>
#include <meta>
#include <print>
#include <tuple>
#include <array>
#include <ranges>
#include <algorithm>
#include <optional>

#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/algorithm/string.hpp>

#include "router.cpp"
#include "annotations.cpp"
#include "extern/CppJsonMagic/json_magic.cpp"

constexpr static bool is_debug = true;

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
        std::vector<std::string> all_endpoint_paths{};

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

        constexpr static auto ctx = std::meta::access_context::current();
        constexpr static auto ns_members = std::define_static_array(std::meta::members_of(inf, ctx));

        consteval static auto get_endpoint_types()
        {
            std::vector<std::meta::info> tuple_members{};
            template for (constexpr auto m : ns_members)
            {
                if constexpr (std::meta::is_type(m) && std::meta::is_complete_type(m) && std::meta::is_class_type(m) && std::meta::is_default_constructible_type(m))
                {
                    tuple_members.push_back(m);
                }
            }
            return std::define_static_array(tuple_members);
        }
        constexpr static auto endpoint_types = get_endpoint_types();
        constexpr static auto endpoint_types_indices = make_indices_array<endpoint_types.size()>();

        constexpr static auto tuple_instance_types = std::meta::substitute(
            ^^std::tuple, endpoint_types);
        typename[:tuple_instance_types:] m_instances{};

    public:
        webserver()
        {
            template for (constexpr auto i : endpoint_types_indices)
            {
                // NAMESPACE MEMBERS
                constexpr auto m = endpoint_types[i];

                constexpr auto m_annotations = std::define_static_array(std::meta::annotations_of(m));
                if constexpr (!m_annotations.empty())
                {
                    if constexpr (is_debug)
                    {
                        std::println("m: {}", std::meta::identifier_of(m));
                    }

                    constexpr static auto m_anno = m_annotations[0];
                    constexpr static auto m_anno_type = std::meta::remove_cvref(std::meta::type_of(m_anno));

                    constexpr static auto class_members = std::define_static_array(std::meta::members_of(m, ctx));
                    constexpr static auto class_members_indices = make_indices_array<class_members.size()>();

                    if constexpr (std::meta::has_template_arguments(m_anno_type))
                    {
                        constexpr static auto ctrl_type = ^^controller<0>;
                        constexpr static auto rest_ctrl_type = ^^rest_controller<0>;
                        if constexpr (std::meta::template_of(m_anno_type) == std::meta::template_of(ctrl_type))
                        {
                            // MEMBERS WITH CONTROLLER ANNOTATION
                            using ctrl_anno_type = [:m_anno_type:];
                            constexpr auto ctrl_obj = std::meta::extract<ctrl_anno_type>(m_anno);
                            // CONTROLLER PATH
                            const auto ctrl_path = ctrl_obj.path;

                            if constexpr (is_debug)
                            {
                                std::println("ctrl_path: {}", ctrl_path);
                            }

                            template for (constexpr auto cm : class_members)
                            {
                                // CLASS MEMBERS
                                if constexpr (std::meta::is_function(cm) && std::meta::is_public(cm) && std::meta::is_user_provided(cm) && !std::meta::is_pure_virtual(cm))
                                {
                                    constexpr auto cm_annotations = std::define_static_array(std::meta::annotations_of(cm));
                                    if constexpr (cm_annotations.size() > 0)
                                    {
                                        constexpr static auto fn_anno = cm_annotations[0];
                                        constexpr static auto fn_anno_type = std::meta::remove_cvref(std::meta::type_of(fn_anno));
                                        constexpr static auto get_type = ^^GET<0>;
                                        constexpr static auto post_type = ^^POST<0>;
                                        constexpr static bool is_get = std::meta::template_of(fn_anno_type) == std::meta::template_of(get_type);
                                        constexpr static bool is_post = std::meta::template_of(fn_anno_type) == std::meta::template_of(post_type);
                                        if constexpr (is_get || is_post)
                                        {
                                            using anno_type = [:fn_anno_type:];
                                            constexpr auto anno_obj = std::meta::extract<anno_type>(fn_anno);
                                            const auto anno_path = anno_obj.path;

                                            // RT
                                            std::string full_path = ctrl_path;
                                            full_path += anno_path;
                                            all_endpoint_paths.emplace_back(full_path);

                                            if constexpr (is_debug)
                                            {
                                                std::println("full_path: {}", full_path);
                                            }

                                            http::verb method;
                                            if constexpr (is_get)
                                            {
                                                method = http::verb::get;
                                            }
                                            else
                                            {
                                                method = http::verb::post;
                                            }

                                            if constexpr (std::meta::is_static_member(cm))
                                            {
                                                app.add_route(method, full_path, [:cm:]);
                                            }
                                            else
                                            {
                                                auto &inst = std::get<i>(m_instances);
                                                constexpr static auto mf = &[:cm:];
                                                auto func = [&inst, mf](context &ctx)
                                                { (inst.*mf)(ctx); };
                                                app.add_route(method, full_path, func);
                                            }
                                        }
                                    }
                                }
                            }
                        }
                        else if constexpr (std::meta::template_of(m_anno_type) == std::meta::template_of(rest_ctrl_type))
                        {
                            // MEMBERS WITH CONTROLLER ANNOTATION
                            using rest_ctrl_anno_type = [:m_anno_type:];
                            constexpr auto rest_ctrl_obj = std::meta::extract<rest_ctrl_anno_type>(m_anno);
                            // REST CONTROLLER PATH
                            const auto rest_ctrl_path = rest_ctrl_obj.path;

                            if constexpr (is_debug)
                            {
                                std::println("rest_ctrl_path: {}", rest_ctrl_path);
                            }

                            template for (constexpr auto j : class_members_indices)
                            {
                                // CLASS MEMBERS
                                constexpr auto cm = class_members[j];
                                if constexpr (std::meta::is_function(cm) && std::meta::is_public(cm) && std::meta::is_user_provided(cm) && !std::meta::is_pure_virtual(cm))
                                {
                                    constexpr auto cm_annotations = std::define_static_array(std::meta::annotations_of(cm));
                                    if constexpr (cm_annotations.size() > 0)
                                    {
                                        constexpr static auto fn_anno = cm_annotations[0];
                                        constexpr static auto fn_anno_type = std::meta::remove_cvref(std::meta::type_of(fn_anno));
                                        constexpr static auto get_type = ^^GET<0>;
                                        constexpr static auto post_type = ^^POST<0>;
                                        constexpr static bool is_get = std::meta::template_of(fn_anno_type) == std::meta::template_of(get_type);
                                        constexpr static bool is_post = std::meta::template_of(fn_anno_type) == std::meta::template_of(post_type);
                                        if constexpr (!(is_get || is_post))
                                        {
                                            continue;
                                        }
                                        // GET annotation
                                        using get_anno_type = [:fn_anno_type:];
                                        constexpr auto get_obj = std::meta::extract<get_anno_type>(fn_anno);
                                        const auto get_path = get_obj.path;

                                        // RT
                                        std::string full_path = rest_ctrl_path;
                                        full_path += get_path;
                                        all_endpoint_paths.emplace_back(full_path);

                                        if constexpr (is_debug)
                                        {
                                            std::println("full_path: {}", full_path);
                                        }

                                        // DECONSTRUCT MEMBER FUNCTION
                                        constexpr auto ret_type = std::meta::return_type_of(cm);
                                        constexpr auto ret_type_name = std::meta::display_string_of(ret_type);
                                        if constexpr (is_debug)
                                        {
                                            std::println("ret_type_name: {}", ret_type_name);
                                        }
                                        constexpr static auto fn_parameters = std::define_static_array(parameters_of(cm));
                                        constexpr static auto params_indices = make_indices_array<fn_parameters.size()>();

                                        constexpr auto tuple_refl = std::meta::substitute(
                                            ^^std::tuple, fn_parameters | std::views::transform(std::meta::type_of));

                                        http::verb v;
                                        if constexpr (is_get)
                                        {
                                            v = http::verb::get;
                                        }
                                        else
                                        {
                                            v = http::verb::post;
                                        }

                                        auto &inst = std::get<i>(m_instances);
                                        constexpr static auto mf = &[:cm:];

                                        app.add_route(v, full_path, [this, &inst, mf](context &ctx)
                                                      {
                                                    typename[:tuple_refl:] args{};
                                                    
                                                    template for (constexpr auto ki : params_indices)
                                                    {
                                                        
                                                        constexpr auto fp = fn_parameters[ki];
                                                        constexpr auto fp_name = std::meta::identifier_of(fp);
                                                        constexpr auto fp_type = std::meta::type_of(fp);
                                                        constexpr auto fp_type_cleaned = std::meta::remove_cvref(fp_type);

                                                        //constexpr static auto fp_type_name = std::define_static_string(std::meta::display_string_of(fp_type));
                                                        //constexpr auto fp_type_cleaned_name = std::define_static_string(std::meta::display_string_of(fp_type_cleaned));
                                                        //std::println("fp_type_name: {} - fp_type_cleaned_name: {}", fp_type_name, fp_type_cleaned_name);

                                                        constexpr static bool has_templ_args = has_template_arguments(fp_type_cleaned);
                                                        constexpr static bool is_post_body = has_templ_args && template_of(fp_type_cleaned) == template_of(^^boing::POST_BODY<int>);
                                                        //std::println("is_post_body: {}", is_post_body);
                                                        
                                                        if constexpr (is_post_body) {
                                                            // ChatGPT 5.2                                                            
                                                            constexpr static auto body_type = template_arguments_of(fp_type_cleaned)[0];
                                                            typename [:body_type:] obj{};
                                                            json_magic::deserialize_value(obj, ctx.req.body());

                                                            typename [:fp_type_cleaned:] wrapped{ std::move(obj) };
                                                            std::get<ki>(args) = std::move(wrapped);
                                                        } else {
                                                            
                                                            if (ctx.params.contains(fp_name))
                                                            {
                                                                auto it = *ctx.params.find(fp_name);
                                                                auto val = it.value;
                                                                
                                                                constexpr static bool is_optional = has_templ_args && template_of(fp_type_cleaned) == template_of(^^std::optional<int>);
                                                                //std::println("is_optional: {}", is_optional);
                                                                
                                                                if constexpr(is_optional) {
                                                                    constexpr static auto opt_type = template_arguments_of(fp_type_cleaned)[0];
                                                                    typename [:fp_type_cleaned:] opt_val = deserialize_get_param<opt_type>(val);
                                                                    std::get<ki>(args) = opt_val;
                                                                } else {
                                                                    std::get<ki>(args) = deserialize_get_param<fp_type_cleaned>(val);
                                                                }
                                                                
                                                            } else {
                                                                std::get<ki>(args) = {};
                                                            }
                                                        
                                                        }
                                                            
                                                    }
                                                    
                                                    auto [... params] = args;
                                                    typename [:ret_type:] result;

                                                    
                                                    if constexpr (std::meta::is_static_member(cm)) {
                                                        result = [:cm:](params...);
                                                    } else {
                                                        result = (inst.*mf)(params...);
                                                    }

                                                    std::string str_json = json_magic::serialize_value(result);

                                                    ctx.json(str_json); });
                                    }
                                }
                            }
                        }
                    }
                    else
                    {
                        constexpr auto auto_ctrl_type = ^^auto_controller;

                        if constexpr (m_anno_type == auto_ctrl_type)
                        {
                            // IS AUTO_CONTROLLER
                            constexpr auto class_name = std::meta::identifier_of(m);

                            template for (constexpr auto cm : class_members)
                            {
                                // CLASS MEMBERS
                                if constexpr (std::meta::is_function(cm) && std::meta::is_public(cm) && std::meta::is_user_provided(cm) && !std::meta::is_pure_virtual(cm))
                                {
                                    constexpr auto class_member_name = std::meta::identifier_of(cm);
                                    std::string full_path = "/";
                                    full_path += class_name;
                                    full_path += "/";
                                    full_path += class_member_name;
                                    all_endpoint_paths.emplace_back(full_path);

                                    if constexpr (is_debug)
                                    {
                                        std::println("full_path: {}", full_path);
                                    }

                                    if constexpr (std::meta::is_static_member(cm))
                                    {
                                        app.add_route(http::verb::get, full_path, [:cm:]); // Should default to POST?
                                    }
                                    else
                                    {
                                        auto &inst = std::get<i>(m_instances);
                                        constexpr static auto mf = &[:cm:];
                                        auto func = [&inst, mf](context &ctx)
                                        { (inst.*mf)(ctx); };
                                        app.add_route(http::verb::get, full_path, func);
                                    }
                                }
                            }
                        }
                    }
                }
            }

            std::string result{};
            for(const auto& e : all_endpoint_paths) {
                result += "<a href=\"";
                result += e;
                result += "\">";
                result += e;
                result += "</a><br>";
            }
            app.add_route(http::verb::get, "/endpoints", [result](context &ctx) { 
                ctx.html(result);
            });
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
        template <std::meta::info fp_type_cleaned>
        auto deserialize_get_param(const std::string &param_val)
        {
            // 1. Strings & String Views
            if constexpr (fp_type_cleaned == std::meta::dealias(^^std::string) || fp_type_cleaned == std::meta::dealias(^^std::string_view))
            {
                return param_val;
            }
            // 2. Booleans
            else if constexpr (fp_type_cleaned == ^^bool)
            {
                std::string lower_val = boost::algorithm::to_lower_copy(param_val); // Implement a quick to_lower
                return (lower_val == "true" || lower_val == "1" || lower_val == "yes");
            }
            // 3. Floating points (use traits to catch float, double, long double)
            else if constexpr (std::is_floating_point_v<typename[:fp_type_cleaned:]>)
            {
                if constexpr (fp_type_cleaned == ^^float)
                    return std::stof(param_val);
                else
                    return std::stod(param_val); // double and others
            }
            // 4. Integers (catch int, long, uint64_t, size_t, etc.)
            else if constexpr (std::is_integral_v<typename[:fp_type_cleaned:]>)
            {
                if constexpr (std::is_signed_v<typename[:fp_type_cleaned:]>)
                {
                    // signed integers
                    return static_cast<typename[:fp_type_cleaned:]>(std::stoll(param_val));
                }
                else
                {
                    // unsigned integers
                    return static_cast<typename[:fp_type_cleaned:]>(std::stoull(param_val));
                }
            }
            else
            {
                static_assert(false, "unknown type");
            }
        }

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
                    context ctx{req, res, current_session, "", {}};
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