#include <string>
#ifndef BOING_SESSION
#define BOING_SESSION

namespace boing
{
    /**
     * SessionData: Stores user-specific information.
     * In a real app, this might store User IDs, permissions, or shopping cart IDs.
     */
    struct session
    {
        std::string user_id;
        int visit_count = 0;
    };

}
#endif