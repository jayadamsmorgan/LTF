#include "ltf_log_level.h"

#include <string.h>

static const char *ltf_log_level_str_map[] = {"CRITICAL", "ERROR", "WARNING",
                                              "INFO",     "DEBUG", "TRACE"};
const char *ltf_log_level_to_str(ltf_log_level level) {
    if (level < 0)
        return NULL;
    if (level > LTF_LOG_LEVEL_TRACE)
        return NULL;
    return ltf_log_level_str_map[level];
}

ltf_log_level ltf_log_level_from_str(const char *str) {
    if (!strcasecmp(str, "c") || !strcasecmp(str, "critical"))
        return LTF_LOG_LEVEL_CRITICAL;
    if (!strcasecmp(str, "e") || !strcasecmp(str, "error"))
        return LTF_LOG_LEVEL_ERROR;
    if (!strcasecmp(str, "w") || !strcasecmp(str, "warning"))
        return LTF_LOG_LEVEL_WARNING;
    if (!strcasecmp(str, "i") || !strcasecmp(str, "info"))
        return LTF_LOG_LEVEL_INFO;
    if (!strcasecmp(str, "d") || !strcasecmp(str, "debug"))
        return LTF_LOG_LEVEL_DEBUG;
    if (!strcasecmp(str, "t") || !strcasecmp(str, "trace"))
        return LTF_LOG_LEVEL_TRACE;

    return -1;
}
