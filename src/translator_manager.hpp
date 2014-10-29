#ifndef TRANSLATOR_MANAGER_HPP
#define TRANSLATOR_MANAGER_HPP

#include "plugin_api.hpp"

#include "plugin_info.hpp"
#include "log.hpp"

#include <boost/container/flat_map.hpp>
#include <boost/range/iterator_range_core.hpp>

namespace eiptnd {

class plugin_factory;

class translator_manager
  : private boost::noncopyable
{
public:
  typedef boost::container::flat_multimap<unsigned short, puid_t> port_mapping_t;
  typedef boost::iterator_range<port_mapping_t::const_iterator> iterator_range;

  translator_manager();

  void add(plugin_info_ptr info);

  plugin_translator_ptr create(puid_t uid);

  bool map_port(unsigned short port_num, const puid_t uid);

  iterator_range list_port();

  iterator_range list_port(const unsigned short port_num);

  void load_settings(const boost::property_tree::ptree& settings);

private:
  /// Logger instance and attributes.
  logging::logger log_;

  /// Map uid with loaded translator.
  boost::container::flat_map<puid_t, plugin_info_ptr> loaded_translators_;

  /// Map port to translator uid.
  port_mapping_t port_mapping_;

};

} // namespace eiptnd

#endif // TRANSLATOR_MANAGER_HPP
