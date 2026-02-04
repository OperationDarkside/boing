#include "webserver.cpp"
#include "annotations.cpp"

namespace endpoints
{
    using namespace boing;
    struct[[= controller("/test")]] A
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
    // constexpr int i = server.test();
    server.start();
}