#ifndef BOING_SESSION_MANAGER
#define BOING_SESSION_MANAGER

#include <string>
#include <unordered_map>

#include "session.cpp"

namespace boing
{
    /**
     * SessionManager: Handles the lifecycle of cookies and session lookups.
     */
    class session_manager
    {
        std::unordered_map<std::string, session> sessions_;

    public:
        std::string create_session()
        {
            std::string sid = "sess_" + std::to_string(rand()); // Simplified ID generation
            sessions_[sid] = session{"guest", 0};
            return sid;
        }

        session *get_session(const std::string &sid)
        {
            if (sessions_.find(sid) != sessions_.end())
                return &sessions_[sid];
            return nullptr;
        }
    };
}
#endif