#ifndef CORE_HPP
#define CORE_HPP

#include "log.hpp"
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
  explicit core(boost::application::context& context, boost::program_options::variables_map& vm);

  /// Handles run signal.
  int operator()();

  /// Handles stop signal.
  bool stop();

  boost::asio::io_service& get_ios() { return io_service_; }
  plugin_factory& get_pf() { return plugin_factory_; }

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

  /// Thread group size (initialization parameter of io_service).
  const std::size_t thread_pool_size_;

  /// Boost.Asio Proactor.
  boost::asio::io_service io_service_;

  ///
  plugin_factory plugin_factory_;

  /// Settings tree.
  boost::property_tree::ptree settings_;
};

} // namespace eiptnd

#endif // CORE_HPP
