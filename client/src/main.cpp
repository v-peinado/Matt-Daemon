#include <exception>
#include <iostream>
#include "BenAfk.hpp"
#include "ArgParser.hpp"

int main(int argc, char **argv)
{
    try
    {
        auto options = ArgParser::parse(argc, argv);
        BenAfk client(options);
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