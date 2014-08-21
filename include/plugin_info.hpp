#ifndef PLUGIN_INFO_H
#define PLUGIN_INFO_H

#include "plugin_api.hpp"

#include <boost/application/shared_library.hpp>

namespace eiptnd {

typedef boost::shared_ptr<boost::application::shared_library> plugin_library_ptr;

class plugin_info
  : private boost::noncopyable
{
public:
  explicit plugin_info(plugin_library_ptr lib, plugin_api_ptr_fn ctor,
                       const std::string& name, const std::string& ver)
    : name(name)
    , version(ver)
    , library_(lib)
    , creator_(ctor)
  {}

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
  plugin_api_ptr operator()() const
  {
      return creator_();
  }

  /// Public plugin information.
  const std::string name;
  const std::string version;

private:
  /*BOOST_MOVABLE_BUT_NOT_COPYABLE(plugin_info);*/

  /// Holds pointer to library which contains plugin
  plugin_library_ptr library_;

  /// Stores boost::function which is constructs plugin
  plugin_api_ptr_fn creator_;
};

typedef boost::shared_ptr<plugin_info> plugin_info_ptr;

} // namespace eiptnd

#endif // PLUGIN_INFO_H
