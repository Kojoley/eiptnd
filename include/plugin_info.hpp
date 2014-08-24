#ifndef PLUGIN_INFO_H
#define PLUGIN_INFO_H

#include "plugin_api.hpp"
#include <iostream>

#include <boost/plugin.hpp>

#include <boost/lambda/bind.hpp>
#include <boost/lambda/construct.hpp>

namespace eiptnd {

typedef unsigned short puid_t;
typedef boost::shared_ptr<boost::plugin::shared_library> plugin_library_ptr;

class plugin_info
  : private boost::noncopyable
{
public:
  explicit plugin_info(const boost::filesystem::path& path_name)
    : library_(path_name)
    , creator_(library_.get<create_shared_fn>("create_plugin"))
    , puid(4444)
    , name(creator_()->name())
    , version(creator_()->version())
  {
    /*std::cout << library_.is_loaded() << " " << library_.search_symbol("create_shared") << std::endl;

    //creator_ create_shared = library_.get_raw<create_shared_fn>("create_shared");
    creator_= boost::plugin::shared_function_alias<plugin_interface_ptr()>(path_name, "create_shared");

    BOOST_AUTO(instance, creator_());
    std::cout << instance->name() << " " << instance->version() << std::endl;*/
  }

  /*plugin_info(BOOST_RV_REF(plugin_info) x)            /// Move ctor
    : library_(boost::move(x.library_))
    , creator_(boost::move(x.creator_))
    , name(boost::move(x.name))
    , version(boost::move(x.version))
  {
  }

  plugin_info& operator=(BOOST_RV_REF(plugin_info) x) /// Move assign
  {
     return *this;
  }*/

  /// Overload call-operator with ability to construct plugin
  plugin_interface_ptr operator()() const
  {
      return creator_();
  }

private:
  /*BOOST_MOVABLE_BUT_NOT_COPYABLE(plugin_info);*/

  /// Holds pointer to library which contains plugin
  //plugin_library_ptr library_;
  boost::plugin::shared_library library_;

  /// Stores boost::function which is constructs plugin
  plugin_interface_ptr_fn creator_;
  //create_shared_fn creator_;

public:
  /// Public plugin information.
  const puid_t puid;
  const std::string name;
  const std::string version;
};

typedef boost::shared_ptr<plugin_info> plugin_info_ptr;

} // namespace eiptnd

#endif // PLUGIN_INFO_H
