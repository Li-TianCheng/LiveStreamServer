#include <string>
#include "Modules.h"
#include "LiveStreamServer.h"

using namespace std;

int main(int argc, char *argv[]) {
    int opt;
    string path;
    const char *optString = "c:";
    while ((opt = getopt(argc, argv, optString)) != -1) {
        if (opt == 'c') {
            path = optarg;
        }
    }
    modules::init(path);
    LiveStreamServer liveStreamServer;
    liveStreamServer.serve();
    modules::close();
    return 0;
}
