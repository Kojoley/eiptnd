#ifndef EMPTY_PTREE_HPP
#define EMPTY_PTREE_HPP

/// NOTE: https://svn.boost.org/trac/boost/ticket/9056
template<class Ptree>
inline const Ptree &empty_ptree()
{
  static Ptree pt;
  return pt;
}

#endif // EMPTY_PTREE_HPP
