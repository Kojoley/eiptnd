#include "config.hpp"
#include "core.hpp"

#include <iostream>
#include <string>
#include <boost/application.hpp>
#include <boost/program_options.hpp>

#if defined(BOOST_WINDOWS_API)
#  include "windows/setup/service_setup.hpp"
#endif

int main(int argc, char* argv[])
{
  namespace app = boost::application;
  namespace po = boost::program_options;

  po::variables_map vm;

  po::options_description general("General options");
  general.add_options()
    ("help,h", "show this help message")
    ("version,v", "output the version information and exit")
    ("foreground,F", "run in foreground mode")
#if not defined(BOOST_OS_WINDOWS)
    ("pid-file", po::value<std::string>()
                   ->default_value(DEFAULT_PID_PATH)
                   ->value_name("path"), "pid file location")
#endif
    ("config-file", po::value<std::string>()
                   ->default_value(DEFAULT_CONFIG_PATH)
                   ->value_name("path"), "config file location")
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
    std::cout << DAEMON_VERSION << std::endl;

    return EXIT_SUCCESS;
  }

  std::cout << DAEMON_INFO "\n" << std::endl;

  if (vm.count("help")) {
    std::cout << desc << std::endl;
    return EXIT_SUCCESS;
  }

  int result = EXIT_SUCCESS;
  boost::system::error_code ec;

  app::context app_ctx;
  app_ctx.insert<app::path>(
    boost::make_shared<app::path_default_behaviour>(argc, argv));

#if defined(BOOST_WINDOWS_API)
  /// TODO: Install into System32
  boost::shared_ptr<app::path> pt = app_ctx.find<app::path>();
  boost::filesystem::path exec_path = pt->executable_path_name();

  if (vm.count("install")) {
    app::example::install_windows_service(
      app::setup_arg(DAEMON_NAME_SHORT),
      app::setup_arg(DAEMON_NAME_FULL),
      app::setup_arg(DAEMON_DESCRIPTION),
      app::setup_arg(exec_path)).install(ec);

    if (ec) {
      std::cerr << ec.message() << std::endl;
      return EXIT_FAILURE;
    }
  }

  if (vm.count("uninstall")) {
    app::example::uninstall_windows_service(
      app::setup_arg(DAEMON_NAME_FULL),
      app::setup_arg(exec_path)).uninstall(ec);

    if (ec) {
      std::cerr << ec.message() << std::endl;
      return EXIT_FAILURE;
    }
  }
#endif

  eiptnd::core app_core(app_ctx, vm);
  //app::auto_handler<eiptnd::core> app(app_ctx, vm);

  app::handler<>::callback termination_callback
    = boost::bind<bool>(&eiptnd::core::stop, &app_core);

  app_ctx.insert<app::termination_handler>(
    boost::make_shared<
      app::termination_handler_default_behaviour>(termination_callback));

  if (vm.count("foreground")) {
    result = app::launch<app::common>(app_core, app_ctx, ec);
  }
  else {
    std::cout << "Deamonizing..." << std::endl;
    result = app::launch<app::server>(app_core, app_ctx, ec);
  }

  if (ec) {
    std::cerr << "[main::Error] " << ec.message()
              << " (" << ec.value() << ")" << std::endl;
    result = EXIT_FAILURE;
  }

  std::cout << "Bye!" << std::endl;

  return result;
}
