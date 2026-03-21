# boing web framework

## A demonstration of C++ 26 reflection and annotations for web development

### What this repo is **NOT**
- **production ready**: Use at your own risk. Expect frequent breakages.
- **stable**: The complier's (mostly GCC trunk) reflection and annotation implementation is still in its early development so this project might refuse to compile from one day to another.
- **tested**: This is mostly an exploration of the new features and their limits. If you want tests, contribute some!

### What it tries to be

Enable web development that feels like java's spring web.

### How to setup

You need at least a working version of g++ (GCC) 16.0.1 20260313 (experimental). Look at the cmake file for more info.

### How to start:

Give the webserver a reflection of a namespace as template parameter:

```cpp
int main()
{
    boing::webserver<^^endpoints> server{};
    server.start();
}
```

### How to implement an endpoint

Put an annotated class in that namespace to create an endpoint and receive web requests

```cpp
struct User {
    std::string username;
};

namespace endpoints
{
  using namespace boing;

  struct[[= rest_controller("/rest")]] rest_test
  {
    // STATIC
    [[= GET("/static_calc")]]
    static std::string rest(int a, int b)
    {
      return std::to_string(a + b);
    }

    // NON-STATIC
    int visit_count = 0;
    [[= POST("/post_test")]]
    std::string fest(float a, int b, boing::POST_BODY<User> body)
    {
      std::string result = "username ";
      result += body.value.username;
      result += " visited ";
      result += std::to_string(visit_count++);
      result += " times.<br>Oh and the result is ";
      result += std::to_string(a + b);
      return result;
    }
  }
}
```

### That's it. Reflection does the rest.

### Supported features:
- **controller** class annotation: Makes this class an endpoint. It takes a path as parameter (can be an empty string). Member functions need to be annotated with GET or POST to receive web requests.
- **GET** and **POST**: Member function annotations, who take a path as parameter (can be an empty string). Free functions are not supported yet.
- **auto_controller**: Same as **controller**, but the full request path is created from the class and member function name. It's just added convenience enabled by C++ 26.
- **rest_controller**: Similar to **controller**, but the member functions don't just take a request context as parameter, but GET and POST request data is serialized via C++ 26 magic and conveniently injected as member function parameters.
- **auto_rest_controller**: (TODO) **auto_controller** + **rest_controller** for the ultimate convenience.
- The "/endpoints" path is reserved and returns all available endpoints for easier discovery and testing.
