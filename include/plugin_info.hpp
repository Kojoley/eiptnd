#ifndef PLUGIN_INFO_H
#define PLUGIN_INFO_H

#include <iostream>

#include <boost/plugin.hpp>

#include <boost/lambda/bind.hpp>
#include <boost/lambda/construct.hpp>

#include "plugin_api.hpp"

namespace eiptnd {

typedef boost::shared_ptr<boost::plugin::shared_library> plugin_library_ptr;
typedef std::string puid_t;

class plugin_info
  : private boost::noncopyable
{
public:
  explicit plugin_info(const boost::filesystem::path& path_name)
    : library_(path_name)
    , creator_(library_.get<create_shared_fn>("create_plugin"))
  {
    BOOST_AUTO(instance, creator_());
    ptype_ = instance->ptype();
    puid_ = instance->uid();
    name_ = instance->name();
    version_ = instance->version();
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

  const boost::plugin::shared_library& library() const { return library_; }
  plugin_api::plugin_type ptype() const { return ptype_; }
  puid_t puid() const { return puid_; }
  std::string name() const { return name_; }
  std::string version() const { return version_; }

private:
  /*BOOST_MOVABLE_BUT_NOT_COPYABLE(plugin_info);*/

  /// Holds pointer to library which contains plugin
  //plugin_library_ptr library_;
  boost::plugin::shared_library library_;

  /// Stores boost::function which is constructs plugin
  plugin_interface_ptr_fn creator_;
  //create_shared_fn creator_;

  /// Public plugin information.
  plugin_api::plugin_type ptype_;
  puid_t puid_;
  std::string name_;
  std::string version_;
};

typedef boost::shared_ptr<plugin_info> plugin_info_ptr;

} // namespace eiptnd

#endif // PLUGIN_INFO_H
