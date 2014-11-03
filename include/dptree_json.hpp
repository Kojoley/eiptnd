#ifndef DPTREE_JSON_HPP
#define DPTREE_JSON_HPP

#include <boost/property_tree/json_parser.hpp>
#include <boost/variant.hpp>

namespace boost {
namespace property_tree {
namespace json_parser {

template <class Ptree>
class out_json_value
  : public boost::static_visitor<>
{
  typedef typename Ptree::data_type data_type;
  typedef typename Ptree::key_type::value_type Ch;
  typedef std::basic_string<Ch> Str;
  typedef std::basic_ostream<Ch> stream_type;

public:
  out_json_value(stream_type& stream)
    : stream_(stream)
  {}

  void operator()(const Str& v) const {
    stream_ << Ch('"') << create_escapes(v) << Ch('"');
  }

  template <class T>
  void operator()(const T& v) const {
    typename translator_between<data_type, Str>::type tr;
    Str data = *tr.get_value(v);
    stream_ << data;
  }

private:
  stream_type& stream_;
};

template<>
void write_json_helper(std::basic_ostream<dptree::key_type::value_type> &stream,
                       const dptree &pt,
                       int indent, bool pretty)
{
    typedef dptree::key_type::value_type Ch;
    typedef typename std::basic_string<Ch> Str;

    // Value or object or array
    if (indent > 0 && pt.empty())
    {
        // Write value
        boost::apply_visitor(out_json_value<dptree>(stream), pt.data());

    }
    else if (indent > 0 && pt.count(Str()) == pt.size())
    {
        // Write array
        stream << Ch('[');
        if (pretty) stream << Ch('\n');
        dptree::const_iterator it = pt.begin();
        for (; it != pt.end(); ++it)
        {
            if (pretty) stream << Str(4 * (indent + 1), Ch(' '));
            write_json_helper(stream, it->second, indent + 1, pretty);
            if (boost::next(it) != pt.end())
                stream << Ch(',');
            if (pretty) stream << Ch('\n');
        }
        if (pretty) stream << Str(4 * indent, Ch(' '));
        stream << Ch(']');

    }
    else
    {
        // Write object
        stream << Ch('{');
        if (pretty) stream << Ch('\n');
        dptree::const_iterator it = pt.begin();
        for (; it != pt.end(); ++it)
        {
            if (pretty) stream << Str(4 * (indent + 1), Ch(' '));
            stream << Ch('"') << create_escapes(it->first) << Ch('"') << Ch(':');
            if (pretty) stream << Ch(' ');
            write_json_helper(stream, it->second, indent + 1, pretty);
            if (boost::next(it) != pt.end())
                stream << Ch(',');
            if (pretty) stream << Ch('\n');
        }
        if (pretty) stream << Str(4 * indent, Ch(' '));
        stream << Ch('}');
    }

}

} // namespace json_parser
} // namespace property_tree
} // namespace boost

#endif // DPTREE_JSON_HPP
