#ifndef PLUGIN_FACTORY_HPP
#define PLUGIN_FACTORY_HPP

#include "plugin_api.hpp"
#include "plugin_info.hpp"
#include "log.hpp"

#include <boost/application.hpp>
#include <boost/function.hpp>
#include <boost/container/flat_map.hpp>

namespace eiptnd {

class plugin_factory
{
public:
  typedef std::string puid_t;
  typedef boost::container::flat_multimap<unsigned short, puid_t> plugin_ports_t;

  plugin_factory()
    : log_(boost::log::keywords::channel = "plugin-factory")
  {};

  /// Load plugin from specified path to it.
  void load(const boost::filesystem::path& path_name);

  /// Load all founded plugins from specified directory.
  void load_dir(const boost::filesystem::path& path_dir);

  /// Create plugin instance.
  plugin_interface_ptr create(puid_t puid);

  bool bind_translator_to_port(unsigned short port_num, const puid_t uid);

  boost::iterator_range<plugin_ports_t::const_iterator>
  tanslators_on_port(const unsigned short port_num);

private:
  /// Logger instance and channels.
  logging::logger log_;

  ///
  boost::container::flat_map<puid_t, plugin_info_ptr> plugins_;
  plugin_ports_t plugin_ports_;
};

} // namespace eiptnd

#endif // PLUGIN_FACTORY_HPP
