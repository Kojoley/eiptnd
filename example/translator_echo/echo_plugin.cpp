#include "echo_plugin.hpp"

#include <boost/property_tree/json_parser.hpp> /// for create_escapes

namespace eiptnd {

echo_plugin::echo_plugin()
  : log_(boost::log::keywords::channel = uid())
{
  BOOST_LOG_SEV(log_, logging::trace) << name() << " created";
}

echo_plugin::~echo_plugin()
{
  BOOST_LOG_SEV(log_, logging::trace) << name() << " destroyed";

  boost::log::core::get()->flush(); /// FIXME: At reworking logging
}

void echo_plugin::handle_start()
{
  handle_write();
}

void echo_plugin::handle_read(std::size_t bytes_transferred)
{
  std::string data(buffer_.c_array(), 0, bytes_transferred);
  BOOST_LOG_SEV(log_, logging::trace)
      << "recieved " << bytes_transferred << " bytes: "
      << boost::property_tree::json_parser::create_escapes(data);

  api_->do_write(boost::asio::buffer(buffer_, bytes_transferred));
}

void echo_plugin::handle_write()
{
  api_->do_read_some(boost::asio::buffer(buffer_));
}

} // namespace eiptnd
