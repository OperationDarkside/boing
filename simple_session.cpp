#ifndef BOING_SIMPLE_SESSION
#define BOING_SIMPLE_SESSION

#include <concepts>
#include <string>
#include <chrono>

/**
 * SessionData: Stores user-specific information.
 */
struct simple_session
{
    // Identity
    std::string session_id{};
    int user_id{};

    // State
    bool is_authenticated{};

    // Lifecycle
    std::chrono::steady_clock::time_point last_active{};
};
#endif // BOING_SIMPLE_SESSION