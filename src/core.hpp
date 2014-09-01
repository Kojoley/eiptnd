#ifndef CORE_HPP
#define CORE_HPP

#include "log.hpp"
#include "plugin_factory.hpp"
#include "request_router.hpp"

#include <boost/application/context.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/program_options/variables_map.hpp>

typedef std::vector<std::string> string_vector;

namespace eiptnd {

class core
{
public:
  core(boost::application::context& context, boost::program_options::variables_map& vm);

  /// Handles run signal.
  int operator()();

  /// Handles stop signal.
  bool stop();

  boost::asio::io_service& get_ios() { return io_service_; }
  plugin_factory& get_pf() { return plugin_factory_; }
  request_router& get_rr() { return request_router_; }

private:
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

  ///
  request_router request_router_;
};

} // namespace eiptnd

#endif // CORE_HPP
