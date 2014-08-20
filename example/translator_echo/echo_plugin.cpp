#include "echo_plugin.hpp"

echo_plugin::echo_plugin()
{
  std::cout << name() << " created" << std::endl;
  //api_->connection->do_read(buf_);
}

echo_plugin::~echo_plugin()
{
  std::cout << name() << " destroyed" << std::endl;
}

void echo_plugin::handle_start()
{
  handle_write();
}

void echo_plugin::handle_read(std::size_t bytes_transferred)
{
  //std::cout << "*handle_read*";
  api_->do_write(boost::asio::buffer(buffer_, bytes_transferred));
  std::cout << std::string(buffer_.c_array()).substr(0, bytes_transferred) << std::endl;
}

void echo_plugin::handle_write()
{
  //std::cout << "*handle_write*";
  api_->do_read_some(boost::asio::buffer(buffer_));
}
