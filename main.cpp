#include "webserver.cpp"
#include "annotations.cpp"
//#include "route_scanner.cpp"

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
        int counter = 1;

        [[= GET("/hello")]] static void hello(context &ctx)
        {
            ctx.text("Hello from within the end of points!");
        }

        [[= GET("/count")]] void count(context &ctx)
        {
            std::string msg = "<h3>Counter: " +
                              std::to_string(counter++) + "</h3>";
            ctx.html(msg);
        }

        [[= GET("/olla")]] void olla(context &ctx)
        {
            ctx.text("¡Hola desde el final de los puntos!");
        }
    };

    struct[[= auto_controller()]] api
    {
        static void rest(context &ctx)
        {
            ctx.text("Hello from the Rest API!");
        }

        void bla(context &ctx)
        {
            ctx.text("bla bla bla bla");
        }
    };

    struct[[= rest_controller("/rest")]] rest_test
    {
        // STATIC
        [[= GET("/test")]]
        static std::string rest(int a, int b)
        {
            return std::to_string(a + b);
        }

        [[= GET("/pest")]]
        static std::string pest(float a, std::string inter, int b)
        {
            return std::to_string(a) + inter + std::to_string(b);
        }

        // NON-STATIC
        int visit_count = 0;
        [[= GET("/fest")]]
        std::string fest(float a, int b)
        {
            std::string result = "a + b + visits = ";
            return std::to_string(a + b + visit_count++);
        }

    };
}

int main()
{
    boing::webserver<^^endpoints> server{};
    server.start();
}