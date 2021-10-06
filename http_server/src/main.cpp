#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <stdexcept>
#include <string>
#include <map>
#include <iterator>
#include <sstream>
#include <algorithm>

#include "ports.hpp"

#include "Master.hpp"

constexpr char CONFIG_FILE_PATH [] = "/etc/httpd.conf";
constexpr char DOCUMENT_ROOT_NAME [] = "DOCUMENT_ROOT";
constexpr char CPU_LIMIT_NAME [] = "CPU_LIMIT";

constexpr unsigned int hash(const char *s, int off = 0) {  // Used just for switch()
    return !s[off] ? 5381 : (hash(s, off+1)*33) ^ s[off];
}

void  toUpperCase(std::string& str)
{
    std::transform(str.begin(), str.end(), str.begin(), ::toupper);
}

bool ParseConfigFile(const char* filepath) {
    std::ifstream conffile(filepath);
    if (conffile.bad() || conffile.eof()) {
        return false;
    }

    setenv(DOCUMENT_ROOT_NAME, "./static/", 1);
    setenv(CPU_LIMIT_NAME, "4", 1);

    std::string line;
    while (std::getline(conffile, line)) {
        std::istringstream iss(line);
        std::vector<std::string> tokens{std::istream_iterator<std::string>{iss},
                                        std::istream_iterator<std::string>{}};
        toUpperCase(tokens[0]);
        switch (hash(tokens[0].c_str())) {
            case hash(DOCUMENT_ROOT_NAME):
                setenv(DOCUMENT_ROOT_NAME, tokens[1].c_str(), 1);
                break;
            case hash(CPU_LIMIT_NAME):
                setenv(CPU_LIMIT_NAME, tokens[1].c_str(), 1);
                break;
        }
    }
    return true;
}

int main(int argc, char* argv[]) {
    std::map<std::string, int> ports;
    ports["external"] = EXTERNAL_PORT;

    if (!ParseConfigFile(CONFIG_FILE_PATH)) {
        std::cerr << "Could not find config file "
                  << CONFIG_FILE_PATH << std::endl;
        return 1;
    }

    try {
        Master master(ports, std::atoi(std::getenv("CPU_LIMIT")));  // TODO: check if CPU_LIMIT is a valid number + make these constants or something
        master.Start();
        std::cout << std::endl
                  << "Starting server at http://127.0.0.1:" << EXTERNAL_PORT << "/" << std::endl
                  << "Quit the server with CONTROL-C." << std::endl << std::endl;

        while (true) {  // Run indefinitely until ctrl+c (or other signal) is sent
            sleep(10000);
        }
    } catch (const std::exception &e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }
}
