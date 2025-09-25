#pragma once
#include "saxio/log/logger.hpp"
#include "saxio/common/util/singleton.hpp"

namespace saxio::log {
inline auto& console = util::Singleton<Logger>::instance();
}
