#ifndef LTF_LOG_LEVEL_H
#define LTF_LOG_LEVEL_H

typedef enum {
    LTF_LOG_LEVEL_CRITICAL = 0,
    LTF_LOG_LEVEL_ERROR = 1,
    LTF_LOG_LEVEL_WARNING = 2,
    LTF_LOG_LEVEL_INFO = 3,
    LTF_LOG_LEVEL_DEBUG = 4,
    LTF_LOG_LEVEL_TRACE = 5,
} ltf_log_level;

ltf_log_level ltf_log_level_from_str(const char *str);

const char *ltf_log_level_to_str(ltf_log_level level);

#endif // LTF_LOG_LEVEL_H
