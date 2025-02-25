/*---------------------------------------------------------------------\
|                          ____ _   __ __ ___                          |
|                         |__  / \ / / . \ . \                         |
|                           / / \ V /|  _/  _/                         |
|                          / /__ | | | | | |                           |
|                         /_____||_| |_| |_|                           |
|                                                                      |
\---------------------------------------------------------------------*/
/** \file	zypp/base/LogTools.h
 *
*/
#ifndef ZYPP_BASE_LOGTOOLS_H
#define ZYPP_BASE_LOGTOOLS_H

#include <iostream>
#include <string>
#include <vector>
#include <list>
#include <set>
#include <map>

#include <zypp-core/base/Hash.h>
#include <zypp-core/base/Logger.h>
#include <zypp-core/base/String.h>
#include <zypp-core/base/Iterator.h>
#include <zypp-core/Globals.h>

#ifdef __GNUG__
#include <cstdlib>
#include <memory>
#include <cxxabi.h>
#endif

///////////////////////////////////////////////////////////////////
namespace zypp
{ /////////////////////////////////////////////////////////////////

  using std::endl;

  /// \brief Helper to produce not-NL-terminated multi line output.
  /// Used as leading separator it prints a separating NL by omitting
  /// output upon it's first invocation.
  /// A custom separator char can be passed to the ctor.
  /// \code
  ///   Container foo { 1,2,3 };
  ///   MLSep sep;
  ///   for ( auto && el : foo )
  ///     cout << sep << el;
  ///   # "1\n2\n3"
  /// \endcode
  struct MLSep
  {
    MLSep() {}
    MLSep( char sep_r ) : _sep { sep_r } {}
    bool _first = true;
    char _sep = '\n';
  };
  inline std::ostream & operator<<( std::ostream & str, MLSep & obj )
  { if ( obj._first ) obj._first = false; else str << obj._sep; return str; }

  /** Print range defined by iterators (multiline style).
   * \code
   * intro [ pfx ITEM [ { sep ITEM }+ ] sfx ] extro
   * \endcode
   *
   * The defaults print the range enclosed in \c {}, one item per
   * line indented by 2 spaces.
   * \code
   * {
   *   item1
   *   item2
   * }
   * {} // on empty range
   * \endcode
   *
   * A comma separated list enclosed in \c () would be:
   * \code
   * dumpRange( stream, begin, end, "(", "", ", ", "", ")" );
   * // or shorter:
   * dumpRangeLine( stream, begin, end );
   * \endcode
   *
   * \note Some special handling is required for printing std::maps.
   * Therefore iomaipulators \ref dumpMap, \ref dumpKeys and \ref dumpValues
   * are provided.
   * \code
   * std::map<string,int> m;
   * m["a"]=1;
   * m["b"]=2;
   * m["c"]=3;
   *
   * dumpRange( DBG, dumpMap(m).begin(), dumpMap(m).end() ) << endl;
   * // {
   * //   [a] = 1
   * //   [b] = 2
   * //   [c] = 3
   * // }
   * dumpRange( DBG, dumpKeys(m).begin(), dumpKeys(m).end() ) << endl;
   * // {
   * //   a
   * //   b
   * //   c
   * // }
   * dumpRange( DBG, dumpValues(m).begin(), dumpValues(m).end() ) << endl;
   * // {
   * //   1
   * //   2
   * //   3
   * // }
   * dumpRangeLine( DBG, dumpMap(m).begin(), dumpMap(m).end() ) << endl;
   * // ([a] = 1, [b] = 2, [c] = 3)
   * dumpRangeLine( DBG, dumpKeys(m).begin(), dumpKeys(m).end() ) << endl;
   * // (a, b, c)
   * dumpRangeLine( DBG, dumpValues(m).begin(), dumpValues(m).end() ) << endl;
   * // (1, 2, 3)
   * \endcode
  */
  template<class TIterator>
    std::ostream & dumpRange( std::ostream & str,
                              TIterator begin, TIterator end,
                              const std::string & intro = "{",
                              const std::string & pfx   = "\n  ",
                              const std::string & sep   = "\n  ",
                              const std::string & sfx   = "\n",
                              const std::string & extro = "}" )
    {
      str << intro;
      if ( begin != end )
        {
          str << pfx << *begin;
          for (  ++begin; begin != end; ++begin )
            str << sep << *begin;
          str << sfx;
        }
      return str << extro;
    }

  /** Print range defined by iterators (single line style).
   * \see dumpRange
   */
  template<class TIterator>
    std::ostream & dumpRangeLine( std::ostream & str,
                                  TIterator begin, TIterator end )
    { return dumpRange( str, begin, end, "(", "", ", ", "", ")" ); }
  /** \overload for container */
  template<class TContainer>
    std::ostream & dumpRangeLine( std::ostream & str, const TContainer & cont )
    { return dumpRangeLine( str, cont.begin(), cont.end() ); }


  ///////////////////////////////////////////////////////////////////
  namespace iomanip
  {
    ///////////////////////////////////////////////////////////////////
    /// \class RangeLine<TIterator>
    /// \brief Iomanip helper printing dumpRangeLine style
    ///////////////////////////////////////////////////////////////////
    template<class TIterator>
    struct RangeLine
    {
      RangeLine( TIterator begin, TIterator end )
      : _begin( begin )
      , _end( end )
      {}
      TIterator _begin;
      TIterator _end;
    };

    /** \relates RangeLine<TIterator> */
    template<class TIterator>
    std::ostream & operator<<( std::ostream & str, const RangeLine<TIterator> & obj )
    { return dumpRangeLine( str, obj._begin, obj._end ); }

  } // namespce iomanip
  ///////////////////////////////////////////////////////////////////

  /** Iomanip printing dumpRangeLine style
   * \code
   *   std::vector<int> c( { 1, 1, 2, 3, 5, 8 } );
   *   std::cout << rangeLine(c) << std::endl;
   *   -> (1, 1, 2, 3, 5, 8)
   * \endcode
   */
  template<class TIterator>
  iomanip::RangeLine<TIterator> rangeLine( TIterator begin, TIterator end )
  { return iomanip::RangeLine<TIterator>( begin, end ); }
  /** \overload for container */
  template<class TContainer>
  auto rangeLine( const TContainer & cont ) -> decltype( rangeLine( cont.begin(), cont.end() ) )
  { return rangeLine( cont.begin(), cont.end() ); }

  template<class Tp>
    std::ostream & operator<<( std::ostream & str, const std::vector<Tp> & obj )
    { return dumpRange( str, obj.begin(), obj.end() ); }

  template<class Tp, class TCmp, class TAlloc>
    std::ostream & operator<<( std::ostream & str, const std::set<Tp,TCmp,TAlloc> & obj )
    { return dumpRange( str, obj.begin(), obj.end() ); }

  template<class Tp>
    std::ostream & operator<<( std::ostream & str, const std::unordered_set<Tp> & obj )
    { return dumpRange( str, obj.begin(), obj.end() ); }

  template<class Tp>
    std::ostream & operator<<( std::ostream & str, const std::multiset<Tp> & obj )
    { return dumpRange( str, obj.begin(), obj.end() ); }

  template<class Tp>
    std::ostream & operator<<( std::ostream & str, const std::list<Tp> & obj )
    { return dumpRange( str, obj.begin(), obj.end() ); }

  template<class Tp>
    std::ostream & operator<<( std::ostream & str, const Iterable<Tp> & obj )
    { return dumpRange( str, obj.begin(), obj.end() ); }

  ///////////////////////////////////////////////////////////////////
  namespace _logtoolsdetail
  { /////////////////////////////////////////////////////////////////

    ///////////////////////////////////////////////////////////////////
    // mapEntry
    ///////////////////////////////////////////////////////////////////

    /** std::pair wrapper for std::map output.
     * Just because we want a special output format for std::pair
     * used in a std::map. The mapped std::pair is printed as
     * <tt>[key] = value</tt>.
    */
    template<class TPair>
      class MapEntry
      {
      public:
        MapEntry( const TPair & pair_r )
        : _pair( &pair_r )
        {}

        const TPair & pair() const
        { return *_pair; }

      private:
        const TPair *const _pair;
      };

    /** \relates MapEntry Stream output. */
    template<class TPair>
      std::ostream & operator<<( std::ostream & str, const MapEntry<TPair> & obj )
      {
        return str << '[' << obj.pair().first << "] = " << obj.pair().second;
      }

    /** \relates MapEntry Convenience function to create MapEntry from std::pair. */
    template<class TPair>
      MapEntry<TPair> mapEntry( const TPair & pair_r )
      { return MapEntry<TPair>( pair_r ); }

    ///////////////////////////////////////////////////////////////////
    // dumpMap
    ///////////////////////////////////////////////////////////////////

    /** std::map wrapper for stream output.
     * Uses a transform_iterator to wrap the std::pair into MapEntry.
     *
     */
    template<class TMap>
      class DumpMap
      {
      public:
        typedef TMap                        MapType;
        typedef typename TMap::value_type   PairType;
        typedef MapEntry<PairType>          MapEntryType;

        struct Transformer
        {
          MapEntryType operator()( const PairType & pair_r ) const
          { return mapEntry( pair_r ); }
        };

        typedef transform_iterator<Transformer, typename MapType::const_iterator>
                MapEntry_const_iterator;

      public:
        DumpMap( const TMap & map_r )
        : _map( &map_r )
        {}

        const TMap & map() const
        { return *_map; }

        MapEntry_const_iterator begin() const
        { return make_transform_iterator( map().begin(), Transformer() ); }

        MapEntry_const_iterator end() const
        { return make_transform_iterator( map().end(), Transformer() );}

      private:
        const TMap *const _map;
      };

    /** \relates DumpMap Stream output. */
    template<class TMap>
      std::ostream & operator<<( std::ostream & str, const DumpMap<TMap> & obj )
      { return dumpRange( str, obj.begin(), obj.end() ); }

    /** \relates DumpMap Convenience function to create DumpMap from std::map. */
    template<class TMap>
      DumpMap<TMap> dumpMap( const TMap & map_r )
      { return DumpMap<TMap>( map_r ); }

    ///////////////////////////////////////////////////////////////////
    // dumpKeys
    ///////////////////////////////////////////////////////////////////

    /** std::map wrapper for stream output of keys.
     * Uses MapKVIterator iterate and write the key values.
     * \code
     * std::map<...> mymap;
     * std::cout << dumpKeys(mymap) << std::endl;
     * \endcode
     */
    template<class TMap>
      class DumpKeys
      {
      public:
        typedef typename MapKVIteratorTraits<TMap>::Key_const_iterator MapKey_const_iterator;

      public:
        DumpKeys( const TMap & map_r )
        : _map( &map_r )
        {}

        const TMap & map() const
        { return *_map; }

        MapKey_const_iterator begin() const
        { return make_map_key_begin( map() ); }

        MapKey_const_iterator end() const
        { return make_map_key_end( map() ); }

      private:
        const TMap *const _map;
      };

    /** \relates DumpKeys Stream output. */
    template<class TMap>
      std::ostream & operator<<( std::ostream & str, const DumpKeys<TMap> & obj )
      { return dumpRange( str, obj.begin(), obj.end() ); }

    /** \relates DumpKeys Convenience function to create DumpKeys from std::map. */
    template<class TMap>
      DumpKeys<TMap> dumpKeys( const TMap & map_r )
      { return DumpKeys<TMap>( map_r ); }

    ///////////////////////////////////////////////////////////////////
    // dumpValues
    ///////////////////////////////////////////////////////////////////

    /** std::map wrapper for stream output of values.
     * Uses MapKVIterator iterate and write the values.
     * \code
     * std::map<...> mymap;
     * std::cout << dumpValues(mymap) << std::endl;
     * \endcode
     */
    template<class TMap>
      class DumpValues
      {
      public:
        typedef typename MapKVIteratorTraits<TMap>::Value_const_iterator MapValue_const_iterator;

      public:
        DumpValues( const TMap & map_r )
        : _map( &map_r )
        {}

        const TMap & map() const
        { return *_map; }

        MapValue_const_iterator begin() const
        { return make_map_value_begin( map() ); }

        MapValue_const_iterator end() const
        { return make_map_value_end( map() ); }

      private:
        const TMap *const _map;
      };

    /** \relates DumpValues Stream output. */
    template<class TMap>
      std::ostream & operator<<( std::ostream & str, const DumpValues<TMap> & obj )
      { return dumpRange( str, obj.begin(), obj.end() ); }

    /** \relates DumpValues Convenience function to create DumpValues from std::map. */
    template<class TMap>
      DumpValues<TMap> dumpValues( const TMap & map_r )
      { return DumpValues<TMap>( map_r ); }

    /////////////////////////////////////////////////////////////////
  } // namespace _logtoolsdetail
  ///////////////////////////////////////////////////////////////////

  // iomanipulator
  using _logtoolsdetail::mapEntry;   // std::pair as '[key] = value'
  using _logtoolsdetail::dumpMap;    // dumpRange '[key] = value'
  using _logtoolsdetail::dumpKeys;   // dumpRange keys
  using _logtoolsdetail::dumpValues; // dumpRange values

  template<class TKey, class Tp>
    std::ostream & operator<<( std::ostream & str, const std::map<TKey, Tp> & obj )
    { return str << dumpMap( obj ); }

  template<class TKey, class Tp>
    std::ostream & operator<<( std::ostream & str, const std::unordered_map<TKey, Tp> & obj )
    { return str << dumpMap( obj ); }

  template<class TKey, class Tp>
    std::ostream & operator<<( std::ostream & str, const std::multimap<TKey, Tp> & obj )
    { return str << dumpMap( obj ); }

  /** Print stream status bits.
   * Prints the values of a streams \c good, \c eof, \c failed and \c bad bit.
   *
   * \code
   *  [g___] - good
   *  [_eF_] - eof and fail bit set
   *  [__FB] - fail and bad bit set
   * \endcode
  */
  inline std::ostream & operator<<( std::ostream & str, const std::basic_ios<char> & obj )
  {
    std::string ret( "[" );
    ret += ( obj.good() ? 'g' : '_' );
    ret += ( obj.eof()  ? 'e' : '_' );
    ret += ( obj.fail() ? 'F' : '_' );
    ret += ( obj.bad()  ? 'B' : '_' );
    ret += "]";
    return str << ret;
  }

  ///////////////////////////////////////////////////////////////////
  // iomanipulator: str << dump(val) << ...
  // calls:         std::ostream & dumpOn( std::ostream & str, const Type & obj )
  ///////////////////////////////////////////////////////////////////

  namespace detail
  {
    template<class Tp>
    struct Dump
    {
      Dump( const Tp & obj_r ) : _obj( obj_r ) {}
      const Tp & _obj;
    };

    template<class Tp>
    std::ostream & operator<<( std::ostream & str, const Dump<Tp> & obj )
    { return dumpOn( str, obj._obj ); }
  }

  template<class Tp>
  detail::Dump<Tp> dump( const Tp & obj_r )
  { return detail::Dump<Tp>(obj_r); }

  /** hexdump data on stream
   * \code
   * hexdump 0000000333 bytes (0x0000014d):
   * 0000: 0c 00 01 49 03 00 17 41 04 af 7c 75 5e 4c 2d f7 ...I...A..|u^L-.
   * 0010: c9 c9 75 bf a8 41 37 2a d0 03 2c ff 96 d2 43 89 ..u..A7*..,...C.
   * 0020: ...
   * \endcode
   */
  inline std::ostream & hexdumpOn( std::ostream & outs, const unsigned char *ptr, size_t size )
  {
    size_t i,c;
    unsigned width = 0x10;
    outs << str::form( "hexdump %10.10ld bytes (0x%8.8lx):\n", (long)size, (long)size );

    for ( i = 0; i < size; i += width ) {
      outs << str::form( "%4.4lx: ", (long)i );
      /* show hex to the left */
      for ( c = 0; c < width; ++c ) {
        if ( i+c < size )
          outs << str::form( "%02x ", ptr[i+c] );
        else
          outs << ("   ");
      }
      /* show data on the right */
      for ( c = 0; (c < width) && (i+c < size); ++c ) {
        char x = (ptr[i+c] >= 0x20 && ptr[i+c] < 0x7f) ? ptr[i+c] : '.';
        outs << x;
      }
      outs << std::endl;
    }
    return outs;
  }
  /** \overload */
  inline std::ostream & hexdumpOn( std::ostream & outs, const char *ptr, size_t size )
  { return hexdumpOn( outs, (const unsigned char *)ptr, size ); }

  /*!
   * Write type info to stream
   * @TODO de-inline me
   */
  inline std::ostream & operator<<( std::ostream & str, const std::type_info &info )
  {
#ifdef __GNUG__
    int status = -4; // some arbitrary value to eliminate the compiler warning

    // enable c++11 by passing the flag -std=c++11 to g++
    std::unique_ptr<char, void(*)(void*)> res {
        abi::__cxa_demangle(info.name(), NULL, NULL, &status),
        std::free
    };
    return str << std::string((status==0) ? res.get() : info.name());
#else
    return str << info.name();
#endif
  }


  /////////////////////////////////////////////////////////////////
} // namespace zypp
///////////////////////////////////////////////////////////////////
#endif // ZYPP_BASE_LOGTOOLS_H
