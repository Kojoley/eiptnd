#include "core.hpp"

#include "tcp_server.hpp"
#include "empty_ptree.hpp"
#include "log.hpp"

#include <boost/bind.hpp>
#include <boost/foreach.hpp>
#include <boost/container/flat_set.hpp>
#include <boost/core/null_deleter.hpp>
#include <boost/log/attributes.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/keywords/order.hpp>
#include <boost/log/keywords/ordering_window.hpp>
#include <boost/log/support/date_time.hpp>
#include <boost/log/sinks/async_frontend.hpp>
#include <boost/log/sinks/text_ostream_backend.hpp>
#include <boost/log/sinks/unbounded_ordering_queue.hpp>
#include <boost/make_shared.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/range/adaptor/map.hpp>

namespace eiptnd {

typedef boost::log::sinks::asynchronous_sink<
  boost::log::sinks::text_ostream_backend,
  boost::log::sinks::unbounded_ordering_queue<
    boost::log::attribute_value_ordering<
      std::size_t,
      std::less<std::size_t>
    >
  >
> sink_t;

void
init_logging()
{
  namespace attrs = boost::log::attributes;
  namespace expr = boost::log::expressions;

  /*boost::log::register_simple_formatter_factory<logging::severity_level, char>("Severity");
  boost::log::register_simple_filter_factory<logging::severity_level, char>("Severity");*/

  boost::shared_ptr<boost::log::core> core = boost::log::core::get();

  boost::shared_ptr<boost::log::sinks::text_ostream_backend> backend =
    boost::make_shared<boost::log::sinks::text_ostream_backend>();
  backend->add_stream(
    boost::shared_ptr<std::ostream>(&std::clog, boost::null_deleter()));
  backend->auto_flush(true);

  boost::shared_ptr<sink_t> sink_ = boost::make_shared<sink_t>(
    backend,
    boost::log::keywords::order =
      boost::log::make_attr_ordering("RecordID", std::less<std::size_t>()),
    boost::log::keywords::ordering_window = boost::posix_time::milliseconds(250)
  );
  core->add_sink(sink_);
  core->add_global_attribute("TimeStamp", attrs::local_clock());
  core->add_global_attribute("RecordID", attrs::counter<std::size_t>());

  //sink_->set_filter(expr::attr<logging::severity_level>("Severity") <= logging::warning);
  sink_->set_formatter(
    expr::format("[%1%] <%2%>\t[%3%] - %4%")
      % expr::format_date_time<boost::posix_time::ptime>("TimeStamp", "%H:%M:%S.%f")
      % expr::attr<logging::severity_level>("Severity")
      % expr::attr<std::string>("Channel")
      % expr::smessage
  );
}

namespace app = boost::application;

core::core(app::context& context)
  : log_(boost::log::keywords::channel = "core")
  , context_(context)
  , vm_(*context.find<boost::program_options::variables_map>())
{
}

core::~core()
{
  BOOST_AUTO(logging_core, boost::log::core::get());
  logging_core->flush();
  logging_core->remove_all_sinks();
}

int
core::operator()()
{
#if defined(BOOST_OS_WINDOWS)
  /// Default working dictory for windows services is system32 (or SysWOW64)
  if (context_.find<app::run_mode>()->mode() == app::server::mode()) {
    boost::filesystem::current_path(context_.find<app::path>()->executable_path());
  }
#endif

  int ret = EXIT_SUCCESS;
  try {
    init_logging();
    load_settings();
    run();
  }
  catch (const boost::exception& e) {
    BOOST_LOG_SEV(log_, logging::critical)
      << boost::diagnostic_information(e);

    BOOST_LOG_SEV(log_, logging::global)
      << "Shutting down after an unrecoverable error";

    stop();
    ret = EXIT_FAILURE;
  }

  return ret;
}

bool
core::stop()
{
  BOOST_LOG_SEV(log_, logging::notify) << "Caught signal to stop";

  /// TODO: Temporary solution for correct destruction
  BOOST_LOG_SEV(log_, logging::trace) << "Freeing io_service";
  io_service_.reset();

  BOOST_LOG_SEV(log_, logging::trace) << "Freeing listeners";
  BOOST_FOREACH(const boost::weak_ptr<tcp_server>& listener, listeners_) {
    BOOST_AUTO(p, listener.lock());
    if (p) {
      p->cancel();
    }
  }

  BOOST_LOG_SEV(log_, logging::trace)
    << "Cleanup is done. Waiting for io_service release...";

  return true; // return true to stop, false to ignore
}

void
core::run()
{
  BOOST_LOG_SEV(log_, logging::info)
      << "Working directory: " << boost::filesystem::current_path();

  std::size_t thread_pool_size = vm_["num-threads"].as<std::size_t>();
  io_service_ = boost::make_shared<boost::asio::io_service>(thread_pool_size);
  plugin_factory_ = boost::make_shared<plugin_factory>(boost::ref(*this));
  plugin_factory_->load_settings(
      settings_.get_child("plugin", empty_ptree<boost::property_tree::ptree>()));

  translator_manager& tm = plugin_factory_->get_tm();
  BOOST_AUTO(it, tm.list_port() | boost::adaptors::map_keys);
  boost::container::flat_set<unsigned short> tcp_ports(it.begin(), it.end());

  string_vector bind_list = vm_["bind"].as<string_vector>();

  try {
    BOOST_FOREACH(const std::string& address, bind_list) {
      BOOST_FOREACH(const unsigned short port_num, tcp_ports) {
        BOOST_LOG_SEV(log_, logging::normal)
          << "Create TCP server at " << address << ":" << port_num;

        boost::shared_ptr<tcp_server> listener =
            boost::make_shared<tcp_server>(boost::ref(*this), address, port_num);
        listeners_.push_back(listener);
        listener->start_accept();
      }
    }
  }
  catch (const boost::system::system_error& e) {
    BOOST_LOG_SEV(log_, logging::critical)
      << e.what() << " (" << e.code().value() << ")";

    throw;
  }

  boost::thread_group threads;
  for (std::size_t i = 0; i < thread_pool_size; ++i) {
    threads.create_thread(
        boost::bind(&boost::asio::io_service::run, io_service_));
  }
  threads.join_all();

  BOOST_LOG_SEV(log_, logging::notify) << "All threads are done";
}

void
core::load_settings()
{
  namespace json_parser = boost::property_tree::json_parser;

  const std::string filename = vm_["config-file"].as<std::string>();
  BOOST_LOG_SEV(log_, logging::info)
    << "Loading settings from configuration file " << filename;

  try {
    json_parser::read_json(filename, settings_);
  }
  catch (const json_parser::json_parser_error& e) {
    BOOST_LOG_SEV(log_, logging::critical) << e.what();

    throw;
  }
}

} // namespace eiptnd
