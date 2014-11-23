#ifndef PLUGIN_FACTORY_HPP
#define PLUGIN_FACTORY_HPP

#include "plugin/plugin_info.hpp"
#include "translator_manager.hpp"
#include "request_router.hpp"
#include "log.hpp"

#include <boost/application.hpp>
#include <boost/function.hpp>
#include <boost/container/flat_map.hpp>

namespace eiptnd {

class core;

class plugin_factory
  : private boost::noncopyable
{
public:
  plugin_factory(core& core);

  /// Load plugin from specified path to it.
  void load(const boost::filesystem::path& path_name);

  /// Load all founded plugins from specified directory.
  void load_dir(const boost::filesystem::path& path_dir);

  /// Create plugin instance.
  plugin_interface_ptr create(puid_t puid);

  void load_settings(const boost::property_tree::ptree& settings);

  translator_manager& get_tm() { return translator_manager_; }
  request_router& get_rr() { return request_router_; }

private:
  /// Logger instance and channels.
  logging::logger log_;

  ///
  core& core_;

  ///
  translator_manager translator_manager_;

  ///
  request_router request_router_;

  ///
  boost::container::flat_map<puid_t, plugin_info_ptr> plugins_;
};

} // namespace eiptnd

#endif // PLUGIN_FACTORY_HPP
