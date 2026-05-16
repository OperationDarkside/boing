#ifndef BOING_SESSION_MANAGER
#define BOING_SESSION_MANAGER

#include <string>
#include <unordered_map>
#include <chrono>

#include "session.cpp"

namespace boing
{
    /**
     * SessionManager: Handles the lifecycle of cookies and session lookups.
     */
    template <is_session session>
    class session_manager
    {
        std::unordered_map<std::string, session> sessions_{};

    public:
        std::string create_session()
        {
            std::string sid = "sess_" + std::to_string(rand()); // Simplified ID generation
            sessions_[sid] = session{.session_id = sid, .last_active = std::chrono::steady_clock::now()};
            return sid;
        }

        session *get_session(const std::string &sid)
        {
            if (sessions_.find(sid) != sessions_.end())
            {
                return &sessions_[sid];
            }
            return nullptr;
        }
    };
}
#endif