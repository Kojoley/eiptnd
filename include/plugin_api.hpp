#ifndef PLUGIN_API_H
#define PLUGIN_API_H

#define LIBRARY_API extern "C" BOOST_SYMBOL_EXPORT

#include <boost/shared_ptr.hpp>
#include <boost/function.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/streambuf.hpp>

#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>

//#include <boost/variant.hpp>
//#include <boost/regex.hpp>


namespace eiptnd {

namespace plugin_api {

class api
{
public:
  //typedef boost::variant</*MatchCondition,*/ char, const std::string/*, const boost::regex*/> read_condition_t;

  boost::function<void(boost::asio::streambuf&, const std::string& delim)> do_read_until;
  boost::function<void(const boost::asio::mutable_buffer&)> do_read_some;
  boost::function<void(const boost::asio::const_buffer&)> do_write;
};

class interface
{
public:
  /*interface(boost::shared_ptr<plugin_api::api> papi) : api_(papi) {};*/
  virtual ~interface(){};
  virtual const char* name() { return "N/A"; };
  virtual const char* version() { return "N/A"; };
  virtual void handle_start() = 0;
  virtual void handle_read(std::size_t bytes_transferred) = 0;
  virtual void handle_write() = 0;
  void setup_api(boost::shared_ptr<plugin_api::api> p) { api_ = p; };
protected:
  boost::shared_ptr<plugin_api::api> api_;
};

interface* create();
void destroy(plugin_api::interface* p);
boost::shared_ptr<plugin_api::interface> create_shared();

} // plugin_api

typedef boost::shared_ptr<plugin_api::interface> plugin_api_ptr;
typedef boost::function<plugin_api_ptr()> plugin_api_ptr_fn;

} // namespace eiptnd

#define DECLARE_PLUGIN(T)              \
                                       \
LIBRARY_API eiptnd::plugin_api::interface*     \
create(void)                           \
{                                      \
  return new T();                      \
}                                      \
                                       \
LIBRARY_API void                       \
destroy(eiptnd::plugin_api::interface* p)      \
{                                      \
  delete p;                            \
}                                      \
LIBRARY_API boost::shared_ptr<eiptnd::plugin_api::interface>\
create_shared(void)                                 \
{                                                   \
  return boost::make_shared<T>();                   \
}                                                   \
/// TODO: Prevent double usage
/// TODO: Using in namespaces
// DECLARE_PLUGIN

#endif // PLUGIN_API_H
