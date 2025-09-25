#pragma once
#include <cassert>

#ifdef NEED_SAXIO_LOG
#include "saxio/log.hpp"
inline auto& debug_logger = saxio::log::console;

#define SET_LOG_LEVEL(level) debug_logger.setLevel(level)

#define LOG_DEBUG(...) debug_logger.debug(__VA_ARGS__)
#define LOG_INFO(...) debug_logger.info(__VA_ARGS__)
#define LOG_WARN(...) debug_logger.warning(__VA_ARGS__)
#define LOG_ERROR(...) debug_logger.error(__VA_ARGS__)

#else

#define SET_LOG_LEVEL(level)

#define LOG_DEBUG(...)
#define LOG_INFO(...)
#define LOG_WARN(...)
#define LOG_ERROR(...)

#endif