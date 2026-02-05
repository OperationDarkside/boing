#include "webserver.cpp"
#include "annotations.cpp"

namespace endpoints
{
    using namespace boing;
    struct[[= controller("/")]] root
    {

        [[= GET("")]] static void greeting(context &ctx)
        {
            ctx.html("<h2>Welcome to Boing</h2><p>Try our <a href=\"/stats\">/stats</a> page</p>");
        }

        [[= GET("stats")]] void stats(context &ctx)
        {
            std::string msg = "<p>You have visited this site " +
                          std::to_string(ctx.session_->visit_count++) + " times!</p>";
            ctx.html(msg);
        }
    };

    struct[[= controller("/test")]] test
    {

        [[= GET("/hello")]] static void hello(context &ctx)
        {
            ctx.text("Hello from within the end of points!");
        }

        [[= GET("/olla")]] void olla(context &ctx)
        {
            ctx.text("¡Hola desde el final de los puntos!");
        }
    };
}

int main()
{
    boing::webserver<^^endpoints> server{};
    server.start();
}