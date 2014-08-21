#ifndef LOG_HPP
#define LOG_HPP

#include <boost/log/core.hpp>
#include <boost/log/sources/severity_channel_logger.hpp>
#include <boost/log/sources/channel_logger.hpp>
#include <boost/log/sources/record_ostream.hpp>
#include <boost/log/sinks/async_frontend.hpp>
#include <boost/log/sinks/text_ostream_backend.hpp>
#include <boost/log/sinks/unbounded_ordering_queue.hpp>
#include <boost/log/utility/record_ordering.hpp>

namespace eiptnd {
namespace logging {

enum severity_level {
  silence  = 0x000,
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
    std::basic_ostream<CharT, TraitsT>& strm, const severity_level level);

typedef boost::log::sources::severity_channel_logger_mt<severity_level> logger;
namespace keywords = boost::log::keywords;
namespace sinks = boost::log::sinks;

typedef sinks::asynchronous_sink<
  sinks::text_ostream_backend,
  sinks::unbounded_ordering_queue<
    boost::log::attribute_value_ordering<
      std::size_t,
      std::less<std::size_t>
    >
  >
> sink_t;

boost::shared_ptr<sink_t> init_logging();

} // namespace logging
} // namespace eiptnd

#endif // LOG_HPP
