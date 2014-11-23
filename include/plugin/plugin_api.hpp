#ifndef PLUGIN_API_HPP
#define PLUGIN_API_HPP

#ifndef BOOST_MSVC
#  define MANGLING_MODE extern "C"
#else
#  define MANGLING_MODE
#endif

#define LIBRARY_API MANGLING_MODE BOOST_SYMBOL_EXPORT

#include <boost/function.hpp>
#include <boost/make_shared.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/shared_ptr.hpp>

namespace eiptnd {
namespace plugin_api {


typedef enum {
  PLUGIN_TRANSLATOR,
  PLUGIN_DISPATCHER
} plugin_type;

typedef boost::function<void(bool)> authenticate_callback;
typedef boost::function<void(bool)> process_data_callback;


class interface
{
public:
  virtual ~interface() {}
  virtual plugin_type type() = 0;
  virtual const char* uid() = 0;
  virtual const char* name() = 0;
  virtual const char* version() = 0;
  virtual void load_settings(const boost::property_tree::ptree&) {}
};

typedef boost::shared_ptr<interface> create_shared();

} // plugin_api

typedef boost::shared_ptr<plugin_api::interface> plugin_interface_ptr;

} // namespace eiptnd

#define DECLARE_PLUGIN(T)                         \
  LIBRARY_API eiptnd::plugin_interface_ptr        \
  create_shared(void)                             \
  { return boost::make_shared<T>(); }             \
// DECLARE_PLUGIN

#endif // PLUGIN_API_HPP
