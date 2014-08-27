#include "core.hpp"

#include <boost/bind.hpp>
#include <boost/foreach.hpp>
#include <boost/container/vector.hpp>
#include <boost/container/list.hpp>
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

#include "tcp_server.hpp"
#include "log.hpp"

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

boost::shared_ptr<sink_t>
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
    boost::log::keywords::ordering_window = boost::posix_time::seconds(1)
  );
  core->add_sink(sink_);
  core->add_global_attribute("TimeStamp", attrs::local_clock());
  core->add_global_attribute("RecordID", attrs::counter<std::size_t>());

  //sink->set_filter(expr::attr<logging::severity_level>("Severity") >= warning);
  sink_->set_formatter(
    expr::format("[%1%] <%2%>\t[%3%] - %4%")
      % expr::format_date_time<boost::posix_time::ptime>("TimeStamp", "%H:%M:%S.%f")
      % expr::attr<logging::severity_level>("Severity")
      % expr::attr<std::string>("Channel")
      % expr::smessage
  );

  return sink_;
}

namespace app = boost::application;

core::core(app::context& context, boost::program_options::variables_map& vm)
  : log_(boost::log::keywords::channel = "core")
  , context_(context)
  , vm_(vm)
  , thread_pool_size_(vm_["num-threads"].as<std::size_t>())
  , io_service_(thread_pool_size_)
  , plugin_factory_()
{
}

int
core::operator()()
{
  boost::shared_ptr<app::path> pt = context_.find<app::path>();
  boost::shared_ptr<sink_t> sink_ = init_logging();

  plugin_factory_.load_dir(pt->executable_path());

  /// TODO: Replace with intrusive list
  typedef boost::shared_ptr<tcp_server> server_ptr;
  boost::container::list<server_ptr> servers;

  string_vector bind_list = vm_["bind"].as<string_vector>();

  std::vector<unsigned short> tcp_ports;
  tcp_ports.push_back(3333);
  tcp_ports.push_back(4444);

  try {
    BOOST_FOREACH(const std::string& address, bind_list) {
      BOOST_FOREACH(const unsigned short port_num, tcp_ports) {
        BOOST_LOG_SEV(log_, logging::normal)
          << "Create TCP server at " << address << ":" << port_num;
        servers.emplace_back(
            boost::make_shared<tcp_server>(
                boost::ref(*this), address, port_num));
      }
      //BOOST_FOREACH(std::string port, udp_ports) {
      //}
    }
  }
  catch (const boost::system::system_error& e) {
    BOOST_LOG_SEV(log_, logging::critical)
      << e.what() << " (" << e.code().value() << ")";
  }

  boost::thread_group threads;
  for (std::size_t i = 0; i < thread_pool_size_; ++i) {
    threads.create_thread(
        boost::bind(&boost::asio::io_service::run, &io_service_));
  }
  threads.join_all();

  BOOST_LOG_SEV(log_, logging::notify) << "All threads are done";

  sink_->flush();

  return EXIT_SUCCESS;
}

bool
core::stop()
{
  BOOST_LOG_SEV(log_, logging::notify) << "Caught signal to stop";
  io_service_.stop();
  return true; // return true to stop, false to ignore
}

} // namespace eiptnd
