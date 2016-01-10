#include <G3D/G3DAll.h>

void printHelp() {
    debugPrintf("Command line: discovery [client | server]\n");
    debugPrintf("\n");
}


void runClient() {
    Discovery::ClientRef discoveryClient = Discovery::Client::createNoGui("My Game");
    debugPrintf("Running client (press any key to exit)\n");
    while (! System::consoleKeyPressed()) {
        discoveryClient->onNetwork();
        debugPrintf("\rKnown Servers:");

        const Array<Discovery::ServerDescription>& server = 
            discoveryClient->serverArray();

        for (int i = 0; i < server.size(); ++i) {
            debugPrintf("%s (%s);  ", 
                        server[i].serverName.c_str(), 
                        server[i].applicationAddress.toString().c_str());
        }
        System::sleep(1.0f / 30.0f);
    }
}


const uint16 GAME_PORT = 4808;

void runServer() {
    NetworkDevice* nd = NetworkDevice::instance();
    alwaysAssertM(nd->adapterArray().size() > 0, "No network adapters.");

    Discovery::ServerDescription description;
    description.applicationName = "My Game";
    description.maxClients = 10;
    description.serverName = "My Server (" + nd->localHostName() + ")";
    description.applicationAddress = 
        NetAddress(nd->adapterArray()[0].ip, GAME_PORT);

    Discovery::Server::Ref discoveryServer = Discovery::Server::create(description);
    
    // DiscoverServer is a Widget, so in a GApp you can just call
    // GApp::addWidget() instead of explicitly running the following
    // loop.
    debugPrintf("Running server (press any key to exit)\n");
    while (! System::consoleKeyPressed()) {
        discoveryServer->onNetwork();
        System::sleep(1.0f / 30.0f);
    }
}


int main(const int argc, const char* argv[]) {
    if (argc == 2) {
        if (argv[1] == std::string("client")) {
            runClient();
            return 0;
        } else if (argv[1] == std::string("server")) {
            runServer();
            return 0;
        }
    }

    printHelp();

    return 1;
}
