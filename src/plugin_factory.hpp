#ifndef PLUGIN_FACTORY_HPP
#define PLUGIN_FACTORY_HPP

#include <boost/application.hpp>
#include <boost/function.hpp>
#include <boost/unordered_map.hpp>

#include "plugin_api.hpp"
#include "plugin_info.hpp"
#include "log.hpp"

namespace eiptnd {

class plugin_factory
{
public:
  typedef BOOST_TYPEOF(&plugin_api::create) create_ptr_fn;
  typedef BOOST_TYPEOF(&plugin_api::destroy) destroy_ptr_fn;
  typedef unsigned short puid_t;

  plugin_factory()
    : log_(logging::keywords::channel = "plugin-factory")
  {};

  /// Load plugin from specified path to it.
  void load(const boost::filesystem::path& path_name);

  /// Load all founded plugins from specified directory.
  void load_dir(const boost::filesystem::path& path_dir);

  /// Create plugin instance.
  plugin_api_ptr create(puid_t puid);

private:
  /// Logger instance and channels.
  logging::logger log_;

  ///
  boost::unordered_map<puid_t, plugin_info_ptr> plugins_;
};

} // namespace eiptnd

#endif // PLUGIN_FACTORY_HPP
