#include "log.hpp"

#include <string>
#include <boost/log/attributes.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/keywords/order.hpp>
#include <boost/log/keywords/ordering_window.hpp>
#include <boost/log/support/date_time.hpp>
#include <boost/make_shared.hpp>
#include <boost/utility/empty_deleter.hpp>

/*#include <boost/log/utility/setup/formatter_parser.hpp>
#include <boost/log/utility/setup/filter_parser.hpp>*/

namespace eiptnd {
namespace logging {

template <typename CharT, typename TraitsT>
inline std::basic_ostream<CharT, TraitsT>& operator<< (
  std::basic_ostream<CharT, TraitsT>& os, severity_level level)
{
  static const char* strings[] = {
    "",
    "CRIT",
    "ERROR",
    "WARN",
    "NOTIFY",
    "NORMAL",
    "INFO",
    "DEBUG",
    "TRACE"
  };

  os << strings[level / 0x200];

  if (level % 0x200) {
    os << ":" << static_cast<std::size_t>(level);
  }

  return os;
}

/*template <typename CharT, typename TraitsT>
inline std::basic_istream<CharT, TraitsT>& operator>> (
    std::basic_istream<CharT, TraitsT>& strm, const severity_level& lvl)
{
    int n = normal;
    os >> n;
    if (n >= normal && n <= critical)
        lvl = static_cast< severity_level >(n);
    else
        lvl = normal;
    return os;
}*/

boost::shared_ptr<sink_t>
init_logging()
{
  namespace attrs = boost::log::attributes;
  namespace expr = boost::log::expressions;

  /*boost::log::register_simple_formatter_factory<logging::severity_level, char>("Severity");
  boost::log::register_simple_filter_factory<logging::severity_level, char>("Severity");*/

  boost::shared_ptr<boost::log::core> core = boost::log::core::get();

  boost::shared_ptr<logging::sinks::text_ostream_backend> backend =
    boost::make_shared<logging::sinks::text_ostream_backend>();
  backend->add_stream(
    boost::shared_ptr<std::ostream>(&std::clog, boost::empty_deleter()));
  backend->auto_flush(true);

  boost::shared_ptr<sink_t> sink_ = boost::make_shared<sink_t>(
    backend,
    logging::keywords::order =
      boost::log::make_attr_ordering("RecordID", std::less<std::size_t>()),
    logging::keywords::ordering_window = boost::posix_time::seconds(1)
  );
  core->add_sink(sink_);
  core->add_global_attribute("TimeStamp", attrs::local_clock());
  core->add_global_attribute("RecordID", attrs::counter<std::size_t>());

  //sink->set_filter(expr::attr< severity_level >("Severity") >= warning);
  sink_->set_formatter(
    /// TODO: Use Boost.Karma for formatting?
    expr::format("[%1%] <%2%>\t[%3%] - %4%")
      % expr::format_date_time<boost::posix_time::ptime>("TimeStamp", "%H:%M:%S.%f")
      % expr::attr<logging::severity_level>("Severity")
      % expr::attr<std::string>("Channel")
      % expr::smessage
  );
  return sink_;
}

} // namespace log
} // namespace eiptnd
