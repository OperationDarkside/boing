#include <array>
#include <functional>
#include <meta>
#include <print>
#include <ranges>
#include <span>
#include <string_view>
#include <tuple>
#include <vector>

#include "annotations.cpp"

namespace boing
{
    constexpr static std::array<std::meta::info, 3> valid_class_annotation_types{std::meta::template_of(std::meta::type_of(^^controller<0>)), std::meta::type_of(^^auto_controller), std::meta::template_of(std::meta::type_of(^^rest_controller<0>))};
    constexpr static std::array<std::meta::info, 3> valid_fn_annotation_types{std::meta::template_of(std::meta::type_of(^^GET<0>)), std::meta::template_of(std::meta::type_of(^^POST<0>))};

    struct refl_anno
    {
        std::string_view name;
        // std::string_view path;
        std::meta::info type;
    };

    struct relf_fn_param
    {
        std::string_view name;
        std::meta::info type;
        std::vector<refl_anno> annotations;
    };

    struct relf_member_fn
    {
        std::string_view name;
        std::meta::info fn_type;
        std::meta::info return_type;
        std::vector<refl_anno> annotations;
        std::vector<relf_fn_param> params;
    };

    struct refl_class
    {
        std::string_view name;
        std::meta::info class_type;
        std::vector<refl_anno> annotations;
        std::vector<relf_member_fn> member_fns;
    };

    struct refl_route
    {
        std::string_view class_name;
        std::meta::info class_type;
        std::vector<refl_anno> class_annotations;

        std::string_view fn_name;
        std::meta::info fn;
        std::meta::info fn_return_type;
        std::vector<refl_anno> fn_annotations;
        std::vector<relf_fn_param> fn_params;
    };

    /*
    consteval bool has_annotation_path(std::meta::info inf) {
        auto ctx = std::meta::access_context::current();
        auto members = std::meta::nonstatic_data_members_of(inf, ctx);
        for (auto m : members) {
            if (std::meta::remove_cvref(std::meta::type_of(m)) == ^^char[]) {
                return true;
            }
        }
        return false;
    }
    */

    consteval refl_anno get_annotation(std::meta::info inf)
    {
        auto name = std::meta::display_string_of(inf);
        auto type = std::meta::remove_cvref(std::meta::type_of(inf));
        /*
        std::string_view path = "";

        if (has_annotation_path(inf)) {
            //using anno_type = [:type:];
            auto obj = std::meta::extract<[:type:]>(inf);
            auto path = obj.path;
        }
        */

        return refl_anno{.name = name, .type = type};
    }

    consteval std::vector<refl_anno> get_annotations(std::vector<std::meta::info> &annos)
    {
        std::vector<refl_anno> result{};
        for (auto a : annos)
        {
            result.push_back(get_annotation(a));
        }
        return result;
    }

    consteval relf_fn_param get_fn_param(std::meta::info inf)
    {
        auto name = std::meta::identifier_of(inf);
        auto type = std::meta::remove_cvref(std::meta::type_of(inf));

        auto m_annotations = std::meta::annotations_of(inf);
        // constexpr static std::size_t m_annotations_size = m_annotations.size();
        auto param_annotations = get_annotations(m_annotations);

        return relf_fn_param{.name = name, .type = type, .annotations = param_annotations};
    }

    consteval std::vector<relf_fn_param> get_fn_params(std::vector<std::meta::info> &params)
    {
        std::vector<relf_fn_param> result{};
        for (auto p : params)
        {
            result.push_back(get_fn_param(p));
        }
        return result;
    }

    consteval relf_member_fn get_member_fn(std::meta::info inf)
    {
        auto name = std::meta::identifier_of(inf);
        // constexpr static auto type = std::meta::remove_cvref(std::meta::type_of(ctrl_anno));
        auto return_type = std::meta::remove_cvref(std::meta::return_type_of(inf));

        auto m_annotations = std::meta::annotations_of(inf);
        std::size_t m_annotations_size = m_annotations.size();

        auto member_annotations = get_annotations(m_annotations);

        auto params = std::meta::parameters_of(inf);
        auto fn_params = get_fn_params(params);

        return relf_member_fn{.name = name,
                              .fn_type = inf,
                              .return_type = return_type,
                              .annotations = member_annotations,
                              .params = fn_params};
    }

    consteval std::vector<relf_member_fn> get_member_fns(std::vector<std::meta::info> &members)
    {
        std::vector<relf_member_fn> result{};
        for (auto m : members)
        {
            if (std::meta::is_function(m) && std::meta::is_public(m) &&
                std::meta::is_user_provided(m) && !std::meta::is_pure_virtual(m))
            {
                result.push_back(get_member_fn(m));
            }
        }
        return result;
    }

    consteval refl_class make_refl_class(std::meta::info inf)
    {
        auto ctx = std::meta::access_context::current();
        auto name = std::meta::display_string_of(inf);
        auto type = std::meta::remove_cvref(inf);
        auto members = std::meta::members_of(inf, ctx);
        std::size_t members_size = members.size();
        // constexpr static auto members_indices = make_indices_array<members_size>();

        auto m_annotations = std::meta::annotations_of(inf);
        std::size_t m_annotations_size = m_annotations.size();

        auto class_annotations = get_annotations(m_annotations);
        auto member_fns_vec = get_member_fns(members);
        // constexpr static auto member_fns_arr = to_array(member_fns_vec);

        return refl_class{.name = name,
                          .class_type = type,
                          .annotations = class_annotations,
                          .member_fns = member_fns_vec};
    }

    consteval auto get_all_refl_class(auto members)
    {
        std::vector<refl_class> result{};
        for (auto &m : members)
        {
            result.push_back(make_refl_class(m));
        }
        return result;
    }

    template<std::size_t N>
    consteval bool has_valid_annotations(const std::vector<std::meta::info> &annos, const std::array<std::meta::info, N>& valid_annos)
    {
        for (const auto a : annos)
        {
            auto cleaned_anno_type = std::meta::remove_cvref(std::meta::type_of(a));
            if(std::meta::has_template_arguments(cleaned_anno_type)) {
                cleaned_anno_type = std::meta::template_of(cleaned_anno_type);
            }
            for (const auto vca : valid_annos)
            {
                if (cleaned_anno_type == vca)
                {
                    return true;
                }
            }
        }
        return false;
    }

    consteval auto filter_ns_members_for_user_anno_classes(const std::vector<std::meta::info> &members)
    {
        std::vector<std::meta::info> filtered_members{};
        for (const auto &m : members)
        {
            if (std::meta::is_type(m) && std::meta::is_complete_type(m) && std::meta::is_class_type(m) && std::meta::is_default_constructible_type(m))
            {
                auto annos = std::meta::annotations_of(m);
                if (!annos.empty() && has_valid_annotations(annos, valid_class_annotation_types))
                {
                    filtered_members.push_back(m);
                }
            }
        }
        return filtered_members;
    }

    consteval auto get_valid_routes(std::meta::info inf)
    {
        bool is_ns = std::meta::is_namespace(inf);
        static_assert(is_ns, "Parameter is not a reflection of a namespace. Write <^^my_ns>.");

        auto ctx = std::meta::access_context::current();
        auto ns_members = std::meta::members_of(inf, ctx);

        auto filtered_ns_members = filter_ns_members_for_user_anno_classes(ns_members);

        auto refl_classes = get_all_refl_class(filtered_ns_members);

        // TODO convert refl_classes to std::vector<refl_route>, check for valid function annotations, check for duplicate routes, create actual routes from refl_route's


        std::vector<const char *> result{};

        for (auto &m : refl_classes)
        {
            // auto m = refl_classes[i];
            // auto member_data = make_refl_class(m);
            auto name = std::define_static_string(m.name);
            result.push_back(name);
        }

        // auto member_data = make_refl_class(inf);
        // std::array<std::string_view, 1> result{};
        // result[0] = std::define_static_string(member_data.name);

        return std::define_static_array(result);
    }

    template <std::meta::info inf>
    consteval auto get_member_names()
    {
        // TODO check if is namespace
        auto ctx = std::meta::access_context::current();
        auto ns_members = std::meta::members_of(inf, ctx);
        // auto ns_members_size = std::meta::members_of(inf, ctx).size();
        auto refl_classes = get_all_refl_class(ns_members);
        std::vector<const char *> result{};

        for (auto &m : refl_classes)
        {
            // auto m = refl_classes[i];
            // auto member_data = make_refl_class(m);
            auto name = std::define_static_string(m.name);
            result.push_back(name);
        }

        // auto member_data = make_refl_class(inf);
        // std::array<std::string_view, 1> result{};
        // result[0] = std::define_static_string(member_data.name);

        return std::define_static_array(result);
    }
}