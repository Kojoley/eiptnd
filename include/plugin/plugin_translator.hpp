#ifndef PLUGIN_TRANSLATOR_HPP
#define PLUGIN_TRANSLATOR_HPP

#include "plugin/plugin_api.hpp"
#include "dptree.hpp"

#include <boost/asio/buffer.hpp>
#include <boost/asio/streambuf.hpp>
#include <boost/function.hpp>

namespace eiptnd {
namespace plugin_api {

struct api_translator
{
  boost::function<void(boost::asio::streambuf&, std::size_t minimum)> do_read_at_least;
  boost::function<void(boost::asio::streambuf&, const std::string& delim)> do_read_until;
  boost::function<void(const boost::asio::mutable_buffer&)> do_read_some;
  boost::function<void(const boost::asio::const_buffer&)> do_write;

  boost::function<void(std::string, std::string, authenticate_callback)> authenticate;
  boost::function<void(boost::shared_ptr<dptree>, process_data_callback)> process_data;
};

class translator
  : public interface
{
public:
  virtual ~translator() { api_.reset(); }
  virtual plugin_type type() { return PLUGIN_TRANSLATOR; }
  virtual void handle_start() = 0;
  virtual void handle_read(std::size_t bytes_transferred) = 0;
  virtual void handle_write() = 0;
  void setup_api(boost::shared_ptr<api_translator> p) { api_ = p; }

protected:
  boost::shared_ptr<api_translator> api_;
};

} // plugin_api

typedef boost::shared_ptr<plugin_api::translator> plugin_translator_ptr;

} // namespace eiptnd

#endif // PLUGIN_TRANSLATOR_HPP
