#include "Server.hpp"

int main()
{
    Server server("127.0.0.1", 8080); // Use dummy values for now
    server.start();
    return 0;
}
