#pragma once
#include <iostream>
#include <string>

namespace ArgParser
{
    struct parseStruct   // en c++ struct ya es un tipo no hace falta definirlo, y deja inicialiczarlo directamnte
    {
        std::string host = "127.0.0.1";
        int port = 4242;
    };
    parseStruct parse(int argc, char** argv);
    void printHelp();
}