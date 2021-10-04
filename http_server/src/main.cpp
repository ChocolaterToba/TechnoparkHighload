#include <iostream>
#include <stdexcept>
#include <string>
#include <map>

#include "ports.hpp"

#include "Master.hpp"

int main(int argc, char* argv[]) {
    try {
        std::map<std::string, int> ports;
        ports["external"] = EXTERNAL_PORT;
        Master master(ports, 4);  // make dependant on cores' amount
        master.Start();
        std::cout << std::endl
                  << "Starting server at http://127.0.0.1:" << EXTERNAL_PORT << "/" << std::endl
                  << "Quit the server with CONTROL-C." << std::endl << std::endl;

        while (true) {  // Run indefinitely until ctrl+c (or other signal) is sent
            sleep(10000);
        }
    } catch (const std::exception &e) {
        std::cerr << e.what() << std::endl;
    }
}
