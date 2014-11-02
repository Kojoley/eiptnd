#ifndef DPTREE_HPP
#define DPTREE_HPP

#include <boost/lexical_cast.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/variant.hpp>

typedef boost::variant<
    std::string
  //, int, long, unsigned int, unsigned long, long long int, unsigned long long int
  //, int, long, unsigned int, unsigned long, std::size_t, std::ptrdiff_t
  //, boost::int8_t, boost::int16_t, boost::int32_t, boost::int64_t
  //, boost::uint8_t, boost::uint16_t, boost::uint32_t, boost::uint64_t
  , boost::int_least8_t, boost::int_least16_t, boost::int_least32_t, boost::int_least64_t
  , boost::uint_least8_t, boost::uint_least16_t, boost::uint_least32_t, boost::uint_least64_t
  , float, double
  , bool
  > dptree_value;
typedef boost::property_tree::basic_ptree<std::string, dptree_value> dptree;

namespace boost {
namespace property_tree {

template <typename External>
struct dptree_translator;

template <typename External>
struct dptree_translator
{
  typedef dptree_value internal_type;
  typedef External external_type;

  boost::optional<external_type> get_value(const internal_type &v)
  { return boost::lexical_cast<external_type>(v); }

  boost::optional<internal_type> put_value(const external_type &v)
  { return internal_type(v); }
};

template <>
template <typename Ch>
struct dptree_translator<std::basic_string<Ch> >
{
  typedef dptree_value internal_type;
  typedef std::basic_string<Ch> external_type;

  struct internal_visitor: boost::static_visitor<external_type>
  {
    external_type operator()(const bool& v) const
    { return v ? external_type("true") : external_type("false"); }

    template <class T>
    external_type operator()(const T& v) const
    { return boost::lexical_cast<external_type>(v); }
  };

  boost::optional<external_type> get_value(const internal_type &v)
  { return boost::apply_visitor(internal_visitor(), v); }

  boost::optional<internal_type> put_value(const external_type &v)
  { return internal_type(v); }
};

template <typename External>
struct translator_between<dptree_value, External>
{
  typedef dptree_translator<External> type;
};

} // namespace property_tree
} // namespace boost

#endif // DPTREE_HPP
