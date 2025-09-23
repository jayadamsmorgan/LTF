#include "cmd_parser.h"

#include "ltf_init.h"
#include "ltf_logs.h"
#include "ltf_target.h"
#include "ltf_test.h"

#include <stdlib.h>

int main(int argc, char **argv) {

    // Parse cli options
    cmd_category cmd = cmd_parser_parse(argc, argv);

    switch (cmd) {
    case CMD_INIT:
        return ltf_init();
    case CMD_TEST:
        return ltf_test();
    case CMD_LOGS_INFO:
        return ltf_logs_info();
    case CMD_TARGET_ADD:
        return ltf_target_add();
    case CMD_TARGET_REMOVE:
        return ltf_target_remove();
    case CMD_HELP:
        // Should be already handled in cmd_parser
        return EXIT_SUCCESS;
    case CMD_UNKNOWN:
        // Should be already handled in cmd_parser
        return EXIT_FAILURE;
    }
}
