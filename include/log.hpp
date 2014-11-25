#ifndef LOG_HPP
#define LOG_HPP

#include <boost/algorithm/string.hpp>
#include <boost/log/core.hpp>
#include <boost/log/sources/severity_channel_logger.hpp>
#include <boost/log/sources/channel_logger.hpp>
#include <boost/log/sources/record_ostream.hpp>
#include <boost/log/utility/record_ordering.hpp>

namespace eiptnd {
namespace logging {

enum severity_level {
  flood    = 0,
  trace    = 1,
  debug    = 2,
  info     = 3,
  normal   = 4,
  notify   = 5,
  warning  = 6,
  error    = 7,
  critical = 8,
  global   = 9,
  silence  = 10
};

static const char* level_strings[] = {
  "FLOOD",
  "TRACE",
  "DEBUG",
  "INFO",
  "NORMAL",
  "NOTIFY",
  "WARNING",
  "ERROR",
  "CRITICAL",
  "GLOBAL",
  "SILENCE",
  NULL
};

template <typename CharT, typename TraitsT>
inline std::basic_ostream<CharT, TraitsT>& operator<< (
  std::basic_ostream<CharT, TraitsT>& os, const severity_level level)
{
  return os << level_strings[level];
}

template <typename CharT, typename TraitsT>
inline std::basic_istream<CharT, TraitsT>& operator>> (
  std::basic_istream<CharT, TraitsT>& is, severity_level& level)
{
  std::basic_string<CharT, TraitsT> value;

  is >> value;
  boost::to_upper(value);

  for (std::size_t i = 0; level_strings[i]; ++i) {
    if (level_strings[i] == value) {
      level = static_cast<severity_level>(i);
      break;
    }
  }

  return is;
}

typedef boost::log::sources::severity_channel_logger_mt<severity_level> logger_mt;
typedef boost::log::sources::severity_channel_logger<severity_level> logger;

} // namespace logging
} // namespace eiptnd

#endif // LOG_HPP
