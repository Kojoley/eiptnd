#ifndef PLUGIN_API_H
#define PLUGIN_API_H

#define LIBRARY_API extern "C" BOOST_SYMBOL_EXPORT

#include <boost/asio/buffer.hpp>
#include <boost/asio/streambuf.hpp>
#include <boost/function.hpp>
#include <boost/make_shared.hpp>
#include <boost/plugin/alias.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/signals2/signal.hpp>
#include <boost/typeof/typeof.hpp>

namespace eiptnd {

namespace plugin_api {

typedef enum {
  PLUGIN_TRANSLATOR,
  PLUGIN_DISPATCHER
} plugin_type;

typedef boost::signals2::signal<void(bool)>::slot_type authenticate_callback;
typedef boost::signals2::signal<void(bool)>::slot_type process_data_callback;

struct api
{
  boost::function<void(boost::asio::streambuf&, const std::string& delim)> do_read_until;
  boost::function<void(const boost::asio::mutable_buffer&)> do_read_some;
  boost::function<void(const boost::asio::const_buffer&)> do_write;

  boost::function<void(std::string, std::string, authenticate_callback)> authenticate;
  boost::function<void(boost::shared_ptr<boost::property_tree::ptree>, process_data_callback)> process_data;
};

class interface
{
public:
  virtual ~interface() {}
  virtual plugin_type type() = 0;
  virtual const char* uid() = 0;
  virtual const char* name() = 0;
  virtual const char* version() = 0;
};

class translator
  : public interface
{
public:
  virtual plugin_type type() { return PLUGIN_TRANSLATOR; }
  virtual void handle_start() = 0;
  virtual void handle_read(std::size_t bytes_transferred) = 0;
  virtual void handle_write() = 0;
  void setup_api(boost::shared_ptr<api> p) { api_ = p; }
protected:
  boost::shared_ptr<api> api_;
};

class dispatcher
  : public interface
{
public:
  virtual plugin_type type() { return PLUGIN_DISPATCHER; }
  virtual void handle_process_data(boost::shared_ptr<boost::property_tree::ptree> tree, process_data_callback callback) = 0;
};

boost::shared_ptr<interface> create_shared();

} // plugin_api

typedef boost::shared_ptr<plugin_api::interface> plugin_interface_ptr;
typedef boost::function<plugin_interface_ptr()> plugin_interface_ptr_fn;
typedef BOOST_TYPEOF(&plugin_api::create_shared) create_shared_fn;

typedef boost::shared_ptr<plugin_api::translator> plugin_translator_ptr;

} // namespace eiptnd

#define DECLARE_PLUGIN(T)                         \
  LIBRARY_API eiptnd::plugin_interface_ptr        \
  create_shared(void)                             \
  { return boost::make_shared<T>(); }             \
  BOOST_PLUGIN_ALIAS(create_shared, create_plugin)
// DECLARE_PLUGIN

#endif // PLUGIN_API_H
