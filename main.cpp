#include "VulkanApp.hpp"
#include <iostream>


int main() {
    VulkanApp app;

    try {
        app.Run();
    } catch( const std::runtime_error e ) {
        std::cerr << e.what() << std::endl;
        return 0;
    }

    return 0;
}
