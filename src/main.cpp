#include <iostream>
#include <string>
#include <boost/application.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/bind.hpp>
#include <boost/container/vector.hpp>
#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/program_options.hpp>
#include <boost/unordered_map.hpp>

#if defined(BOOST_WINDOWS_API)
#  include "windows/setup/service_setup.hpp"
#endif

#include "config.hpp"

#include "plugin_api.hpp"

#include "plugin_factory.hpp"
#include "tcp_server.hpp"
#include "log.hpp"


typedef std::vector<std::string> string_vector;


namespace application = boost::application;
namespace po = boost::program_options;

namespace eiptnd {

class srvd
{
public:

  srvd(application::context& context, boost::program_options::variables_map& vm)
    : context_(context)
    , vm_(vm)
    , thread_pool_size_(vm_["num-threads"].as<std::size_t>())
    , io_service_(thread_pool_size_)
    , log_(logging::keywords::channel = "default")
    , sink_(logging::init_logging())
  {
  }

  int operator()()
  {
    boost::shared_ptr<application::path> pt = context_.find<application::path>();

    plugin_factory plugin_factory_;
    plugin_factory_.load_dir(pt->executable_path());

    boost::shared_ptr<application::run_mode> modes
      = context_.find<application::run_mode>();

    if (modes->mode() == application::common::mode()) {
    }
    else if (modes->mode() == application::server::mode()) {
    }
    //context_.find<application::wait_for_termination_request>()->wait();

    /// TODO: Replace with intrusive list
    typedef boost::shared_ptr<tcp_server> server_ptr;
    boost::container::vector<server_ptr> servers;

    string_vector bind_list = vm_["bind"].as<string_vector>();

    std::vector<unsigned short> tcp_ports;
    tcp_ports.push_back(3333);
    tcp_ports.push_back(4444);
    //boost::lexical_cast<std::string>(port_num)

    BOOST_FOREACH(std::string address, bind_list) {
      BOOST_FOREACH(unsigned short port_num, tcp_ports) {
        try {
          BOOST_LOG_SEV(log_, logging::info)
            << "Create TCP server at " << address << ":" << port_num;

          server_ptr server_p(new tcp_server(io_service_, address, port_num, plugin_factory_));
          //server_ptr server_p = boost::make_shared<tcp_server>(io_service_, address, port, ec, plugin_factory_);
          //servers.emplace_back(io_service_, address, port, ec, plugin_factory_);
          servers.push_back(server_p);
        }
        catch (const boost::system::system_error& e) {
          BOOST_LOG_SEV(log_, logging::error)
            << e.what() << " (" << e.code().value() << ")";
        }
      }
      //BOOST_FOREACH(std::string port, udp_ports) {
      //}
    }

    boost::thread_group threads;
    for (std::size_t i = 0; i < thread_pool_size_; ++i) {
      /// TODO: Affinity?
      threads.create_thread(
          boost::bind(&boost::asio::io_service::run, &io_service_));
    }
    threads.join_all();

    //BOOST_LOG_SEV(log_, log::notify) << "All threads are done";
    BOOST_LOG_SEV(log_, logging::notify) << "All threads are done";

    return EXIT_SUCCESS;
  }

  bool stop()
  {
    std::cout << "stop!" << std::endl;
    io_service_.stop();
    return true; // return true to stop, false to ignore
  }

private:
  //typedef boost::unordered_map<endpoint,connection::ptr>  ep_to_con_map;
  //connections_manager conn_mgr_;
  //logger log_;
  application::context& context_;
  boost::program_options::variables_map& vm_;
  boost::asio::io_service io_service_;
  std::size_t thread_pool_size_;

  logging::logger log_;
  boost::shared_ptr<logging::sink_t> sink_;

};

} // namespace eiptnd

int main(int argc, char* argv[])
{
  std::cout << DAEMON_INFO "\n" << std::endl;

  po::variables_map vm;

  po::options_description general("General options");
  general.add_options()
    ("help,h", "show this help message")
    ("version,v", "output the version information and exit")
    ("foreground,F", "run in foreground mode")
    ("pid-file", po::value<std::string>()
                   ->default_value(DEFAULT_PID_PATH)
                   ->value_name("path"), "pid file location")
  ;

  std::size_t num_threads = std::max(1u, boost::thread::hardware_concurrency());
  po::options_description network("Network Options");
  network.add_options()
    ("bind", po::value<string_vector>()
               ->default_value(string_vector(1, "0.0.0.0"), "0.0.0.0")
               ->multitoken()->value_name("ip"), "bind address")
    ("num-threads", po::value<std::size_t>()->default_value(num_threads)
       ->value_name("N"), "number of connection handler threads count")
  ;

  po::options_description desc("Allowed Options");
  desc.add(general).add(network);

#if defined(BOOST_WINDOWS_API)
  po::options_description service("Service Options");

  service.add_options()
    ("install", "install service")
    ("uninstall", "unistall service")
  ;

  desc.add(service);
#endif

  try {
    po::store(po::command_line_parser(argc, argv)
        .options(desc).run(), vm);
    //std::ifstream ifs(DEFAULT_CONFIG_PATH));
    //if (ifs.is_open()) {
    //  po::store(po::parse_config_file(ifs, network, vm);
    //  ifs.close();
    //}
    po::notify(vm);
  }
  catch (const std::exception& e) {
    std::cerr << e.what() << std::endl;
    std::cout << desc << std::endl;
    //std::cerr << boost::diagnostic_information(e) << std::endl;
    return EXIT_FAILURE;
  }

  if (vm.count("version")) {
    return EXIT_SUCCESS;
  }

  if (vm.count("help")) {
    std::cout << desc << std::endl;
    return EXIT_SUCCESS;
  }

  int result = EXIT_SUCCESS;
  boost::system::error_code ec;

  application::context app_ctx;
  app_ctx.insert<application::path>(
    boost::make_shared<application::path_default_behaviour>(argc, argv));

#if defined(BOOST_WINDOWS_API)
  /// TODO: Install into System32
  boost::shared_ptr<application::path> pt = app_ctx.find<application::path>();
  boost::filesystem::path exec_path = pt->executable_path_name();

  if (vm.count("install")) {
    application::example::install_windows_service(
      application::setup_arg(DAEMON_NAME_SHORT),
      application::setup_arg(DAEMON_NAME_FULL),
      application::setup_arg(DAEMON_DESCRIPTION),
      application::setup_arg(exec_path)).install(ec);

    if (ec) {
      std::cout << ec.message() << std::endl;
      return EXIT_FAILURE;
    }
  }

  if (vm.count("uninstall")) {
    application::example::uninstall_windows_service(
      application::setup_arg(DAEMON_NAME_FULL),
      application::setup_arg(exec_path)).uninstall(ec);

    if (ec) {
      std::cout << ec.message() << std::endl;
      return EXIT_FAILURE;
    }
  }
#endif

  eiptnd::srvd app(app_ctx, vm);
  //application::auto_handler<srvd> app(app_ctx, vm);

  application::handler<>::callback termination_callback
    = boost::bind<bool>(&eiptnd::srvd::stop, &app);

  app_ctx.insert<application::termination_handler>(
    boost::make_shared<
      application::termination_handler_default_behaviour>(termination_callback));

  if (vm.count("foreground")) {
    result = application::launch<application::common>(app, app_ctx, ec);
  }
  else {
    result = application::launch<application::server>(app, app_ctx, ec);
  }

  if (ec) {
    std::cout << "[main::Error] " << ec.message()
              << " (" << ec.value() << ")" << std::endl;
    result = EXIT_FAILURE;
  }

  return result;
}
