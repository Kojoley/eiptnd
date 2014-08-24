#ifndef CORE_HPP
#define CORE_HPP

#include <boost/application.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/program_options/variables_map.hpp>

#include "log.hpp"

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

private:
  /// Logger instance and attributes.
  logging::logger log_;

  /// Application context.
  boost::application::context& context_;

  /// Variables map.
  boost::program_options::variables_map& vm_;

  /// Threads number.
  const std::size_t thread_pool_size_;

  /// Boost.Asio Proactor.
  boost::asio::io_service io_service_;
};

} // namespace eiptnd

#endif // CORE_HPP
