#ifndef PLUGIN_INFO_HPP
#define PLUGIN_INFO_HPP

#include "plugin/plugin_api.hpp"

#include <boost/dll.hpp>
#include <boost/lambda/bind.hpp>
#include <boost/lambda/construct.hpp>
#include <boost/typeof/typeof.hpp>

namespace eiptnd {

typedef boost::shared_ptr<boost::dll::shared_library> plugin_library_ptr;
typedef std::string puid_t;

class plugin_info
  : private boost::noncopyable
{
public:
  explicit plugin_info(const boost::filesystem::path& path_name)
    : library_(path_name)
    , creator_(library_.get<plugin_api::create_shared>("create_shared"))
  {
    BOOST_AUTO(instance, creator_());
    type_ = instance->type();
    puid_ = instance->uid();
    name_ = instance->name();
    version_ = instance->version();
  }

  /// Overload call-operator with ability to construct plugin
  plugin_interface_ptr operator()() const
  {
      return creator_();
  }

  const boost::dll::shared_library& library() const { return library_; }
  plugin_api::plugin_type type() const { return type_; }
  puid_t puid() const { return puid_; }
  std::string name() const { return name_; }
  std::string version() const { return version_; }

private:
  /// Holds pointer to library which contains plugin
  boost::dll::shared_library library_;

  /// Stores plugin constructor.
  boost::function<plugin_interface_ptr()> creator_;

  /// Public plugin information.
  plugin_api::plugin_type type_;
  puid_t puid_;
  std::string name_;
  std::string version_;
};

typedef boost::shared_ptr<plugin_info> plugin_info_ptr;

} // namespace eiptnd

#endif // PLUGIN_INFO_HPP
