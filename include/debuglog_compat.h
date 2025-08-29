#pragma once

// If the logging library doesn't provide LOG_SET_OPTION, make it a no-op.
#ifndef LOG_SET_OPTION
#define LOG_SET_OPTION(_file, _line, _func) do { } while (0)
#endif