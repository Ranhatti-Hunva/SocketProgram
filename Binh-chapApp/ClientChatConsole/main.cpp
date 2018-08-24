#include "clientchat.h"

int main()
{

    ClientChat client("localhost","8096");
    client.initClient();
    client.createSocket();
    client.mainLoop();
    client.cleanUp();
    return 0;
}
