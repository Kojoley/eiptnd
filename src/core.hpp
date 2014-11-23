#ifndef CORE_HPP
#define CORE_HPP

#include "log.hpp"
#include "tcp_server.hpp"
#include "plugin_factory.hpp"

#include <vector>
#include <boost/application/context.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/program_options/variables_map.hpp>
#include <boost/property_tree/ptree_fwd.hpp>

typedef std::vector<std::string> string_vector;

namespace eiptnd {

class plugin_factory;
class translator_manager;

class core
{
public:
  core(boost::application::context& context);
  ~core();

  /// Handles run signal.
  int operator()();

  /// Handles stop signal.
  bool stop();

  boost::shared_ptr<boost::asio::io_service> get_ios() { return io_service_; }
  plugin_factory& get_pf() { return *plugin_factory_; }

private:
  /// Heart of service.
  void run();

  /// Load settings from configuration file.
  void load_settings();

  /// Logger instance and attributes.
  logging::logger log_;

  /// Application context.
  boost::application::context& context_;

  /// Variables map.
  boost::program_options::variables_map& vm_;

  /// Boost.Asio Proactor.
  boost::shared_ptr<boost::asio::io_service> io_service_;

  ///
  boost::shared_ptr<plugin_factory> plugin_factory_;

  /// Settings tree.
  boost::property_tree::ptree settings_;

  ///
  std::vector<boost::weak_ptr<tcp_server> > listeners_;
};

} // namespace eiptnd

#endif // CORE_HPP
