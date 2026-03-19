#include "ArgParser.hpp"
#include <vector>
#include <string>

ArgParser::parseStruct ArgParser::parse(int argc, char** argv)
{
    if(argc > 1)
    {
        std::vector<std::string> args(argv + 1, argv + argc);
        if(argc == 2 && args[0] == "--help")
            printHelp();
    }
    parseStruct returnStruct;
    return returnStruct;
}

void ArgParser::printHelp()
{
    std::cout << "Usage: [OPTIONS]\n"
              << "\nOptions:\n"
              << "  -h, --host HOST    Server hostname (default: 127.0.0.1)\n"
              << "  -p, --port PORT    Server port (default: 4242)\n"
              << "  --help             Show this help message\n"
              << "\nExample:\n"
              << "--host 192.168.1.100 --port 5000" << std::endl;
}