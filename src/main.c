#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <wlr/util/log.h>

#include "server.h"
#include "view.h"
#include "output.h"
#include "keyboard.h"

/* Forward-declare the init/run functions defined in server.c */
bool server_init(struct ywm_server *server);
void server_run(struct ywm_server *server, const char *startup_cmd);

int main(int argc, char *argv[]) {
    const char *startup_cmd = NULL;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-s") == 0 && i + 1 < argc) {
            startup_cmd = argv[++i];
        }
    }

    wlr_log_init(WLR_DEBUG, NULL);

    struct ywm_server server = {0};

    if (!server_init(&server)) {
        wlr_log(WLR_ERROR, "Failed to initialise ywm");
        return 1;
    }

    server_run(&server, startup_cmd);
    return 0;
}
