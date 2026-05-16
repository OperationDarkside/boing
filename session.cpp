#ifndef BOING_SESSION
#define BOING_SESSION

#include <concepts>
#include <string>
#include <chrono>

namespace boing
{
    /**
     * SessionData: Stores user-specific information.
     * In a real app, this might store User IDs, permissions, or shopping cart IDs.
     */
    template <typename T>
    concept is_session = requires(T s) {
        // Identity
        { s.session_id } -> std::convertible_to<std::string &>;
        { s.user_id } -> std::convertible_to<int &>;

        // State
        { s.is_authenticated } -> std::convertible_to<bool &>;

        // Lifecycle
        { s.last_active } -> std::convertible_to<std::chrono::steady_clock::time_point &>;
    };
}
#endif