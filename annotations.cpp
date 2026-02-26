#ifndef BOING_ANNOTATIONS
#define BOING_ANNOTATIONS

#include <cstddef>

namespace boing
{
    // Controllerrrrrr
    template <std::size_t N>
    struct controller
    {
        char path[N];

        constexpr controller(const char (&input)[N] = "")
        {
            for (std::size_t i = 0; i < N; ++i)
                path[i] = input[i];
        }
    };

    // Creates routing paths from the class and member-function names
    struct auto_controller{};

    template <std::size_t N>
    struct GET
    {
        char path[N];

        constexpr GET(const char (&input)[N] = "")
        {
            for (std::size_t i = 0; i < N; ++i)
                path[i] = input[i];
        }
    };

    template <std::size_t N>
    struct POST
    {
        char path[N];

        constexpr POST(const char (&input)[N] = "")
        {
            for (std::size_t i = 0; i < N; ++i)
                path[i] = input[i];
        }
    };

    template <std::size_t N>
    struct rest_controller{
        char path[N];

        constexpr rest_controller(const char (&input)[N] = "")
        {
            for (std::size_t i = 0; i < N; ++i)
                path[i] = input[i];
        }
    };

    template<typename T>
    struct POST_BODY {
        T value;
    };
}

#endif