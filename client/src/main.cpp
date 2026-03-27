#include <exception>
#include <iostream>
#include "BenAfk.hpp"

int main()
{
    try
    {
        BenAfk::Config config; 
        BenAfk client(config);
        client.init();
        client.run();
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
        return 1;
    }
    return 0;
}