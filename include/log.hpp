#ifndef LOG_HPP
#define LOG_HPP

#include <boost/log/core.hpp>
#include <boost/log/sources/severity_channel_logger.hpp>
#include <boost/log/sources/channel_logger.hpp>
#include <boost/log/sources/record_ostream.hpp>
#include <boost/log/utility/record_ordering.hpp>

namespace eiptnd {
namespace logging {

enum severity_level {
  global  = 0x000,
  critical = 0x200,
  error    = 0x400,
  warning  = 0x600,
  notify   = 0x800,
  normal   = 0xA00,
  info     = 0xC00,
  debug    = 0xE00,
  trace    = 0x1000
};

template <typename CharT, typename TraitsT>
inline std::basic_ostream<CharT, TraitsT>& operator<< (
  std::basic_ostream<CharT, TraitsT>& os, const severity_level level)
{
  static const char* strings[] = {
    "GLOBAL",
    "CRITICAL",
    "ERROR",
    "WARNING",
    "NOTIFY",
    "NORMAL",
    "INFO",
    "DEBUG",
    "TRACE"
  };

  os << strings[level / 0x200];

  if (level % 0x200) {
    os << CharT(':') << static_cast<std::size_t>(level);
  }

  return os;
}

typedef boost::log::sources::severity_channel_logger_mt<severity_level> logger_mt;
typedef boost::log::sources::severity_channel_logger<severity_level> logger;

} // namespace logging
} // namespace eiptnd

#endif // LOG_HPP
