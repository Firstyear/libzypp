/*---------------------------------------------------------------------\
|                          ____ _   __ __ ___                          |
|                         |__  / \ / / . \ . \                         |
|                           / / \ V /|  _/  _/                         |
|                          / /__ | | | | | |                           |
|                         /_____||_| |_| |_|                           |
|                                                                      |
\---------------------------------------------------------------------*/
/** \file zypp/parser/yum/FilelistsFileReader.cc
 * Implementation of filelists.xml.gz file reader.
 */
#include "zypp/base/Logger.h"
#include "zypp/parser/xml/Reader.h"
#include "zypp/base/UserRequestException.h"

#include "zypp/parser/yum/FilelistsFileReader.h"

#undef ZYPP_BASE_LOGGER_LOGGROUP
#define ZYPP_BASE_LOGGER_LOGGROUP "parser::yum"

using namespace std;
using namespace zypp::xml;

namespace zypp
{
  namespace parser
  {
    namespace yum
    {


  ///////////////////////////////////////////////////////////////////////
  //
  //  CLASS NAME : FilelistsFileReader::Impl
  //
  class FilelistsFileReader::Impl : private base::NonCopyable
  {
  public:
    Impl(
      const Pathname & filelists_file,
      const ProcessPackage & callback,
      const ProgressData::ReceiverFnc & progress = ProgressData::ReceiverFnc());

  public:

    /**
     * Callback provided to the XML parser.
     */
    bool consumeNode(xml::Reader & reader_r);

    /**
     * Creates a new \ref data::Resolvable_Ptr, swaps its contents with
     * \ref _resolvable and returns it. Used to hand-out the data object to its consumer
     * (a \ref ProcessPackage function) after it has been read.
     */
    data::Resolvable_Ptr handoutResolvable();

  private:

    /**
     * Pointer to the \ref zypp::data::Resolvable object for storing the NVRA
     * data.
     */
    zypp::data::Resolvable_Ptr _resolvable;

    /**
     * Changelog of \ref _resolvable.
     */
    data::Filenames _filenames;

    /**
     * Callback for processing package metadata. Passed in through constructor.
     */
    ProcessPackage _callback;

    /**
     * Progress reporting object.
     */
    ProgressData _ticks;
  };
  ///////////////////////////////////////////////////////////////////////

  FilelistsFileReader::Impl::Impl(
      const Pathname & filelists_file,
      const ProcessPackage & callback,
      const ProgressData::ReceiverFnc & progress)
    :
      _callback(callback)
  {
    _ticks.sendTo(progress);
    _ticks.name("filelist.xml.gz");

    Reader reader(filelists_file);
    MIL << "Reading " << filelists_file << endl;
    reader.foreachNode(bind(&FilelistsFileReader::Impl::consumeNode, this, _1));
  }

  // --------------------------------------------------------------------------

  /*
   * xpath and multiplicity of processed nodes are included in the code
   * for convenience:
   *
   * // xpath: <xpath> (?|*|+)
   *
   * if multiplicity is ommited, then the node has multiplicity 'one'.
   */

  // --------------------------------------------------------------------------


  bool FilelistsFileReader::Impl::consumeNode(Reader & reader_r)
  {
    if (reader_r->nodeType() == XML_READER_TYPE_ELEMENT)
    {
      // xpath: /filelists
      if (reader_r->name() == "filelists")
      {
        unsigned total_packages;
        zypp::str::strtonum(reader_r->getAttribute("packages").asString(), total_packages);
        _ticks.range(total_packages);
        _ticks.toMin();
        return true;
      }

      // xpath: /filelists/package (+)
      if (reader_r->name() == "package")
      {
        _resolvable = new data::Resolvable;
        _filenames.clear();

        _resolvable->name = reader_r->getAttribute("name").asString();
        _resolvable->arch = Arch(reader_r->getAttribute("arch").asString());

        return true;
      }

      // xpath: /filelists/package/version
      if (reader_r->name() == "version")
      {
        _resolvable->edition = Edition(reader_r->getAttribute("ver").asString(),
                                    reader_r->getAttribute("rel").asString(),
                                    reader_r->getAttribute("epoch").asString());
        return true;
      }

      // xpath: /filelists/package/file (*)
      if (reader_r->name() == "file")
      {
        // ignoring type dir/ghost  reader_r->getAttribute("type").asString();
        _filenames.push_back(reader_r.nodeText().asString());
        return true;
      }
    }

    else if (reader_r->nodeType() == XML_READER_TYPE_END_ELEMENT)
    {
      // xpath: /filelists/package
      if (reader_r->name() == "package")
      {
        if (_callback && !_filenames.empty())
          _callback(handoutResolvable(), _filenames);

        if (!_ticks.incr())
          ZYPP_THROW(AbortRequestException());

        return true;
      }

      // xpath: /filelists
      if (reader_r->name() == "filelists")
      {
        _ticks.toMax();
        return true;
      }
    }

    return true;
  }

  // --------------------------------------------------------------------------

  data::Resolvable_Ptr FilelistsFileReader::Impl::handoutResolvable()
  {
    data::Resolvable_Ptr ret;
    ret.swap(_resolvable);
    return ret;
  }


  ///////////////////////////////////////////////////////////////////
  //
  //  CLASS NAME : FilelistsFileReader
  //
  ///////////////////////////////////////////////////////////////////

  FilelistsFileReader::FilelistsFileReader(
      const Pathname & filelists_file,
      const ProcessPackage & callback,
      const ProgressData::ReceiverFnc & progress)
    :
      _pimpl(new Impl(filelists_file, callback, progress))
  {}

  FilelistsFileReader::~FilelistsFileReader()
  {}


    } // ns yum
  } // ns parser
} // ns zypp

// vim: set ts=2 sts=2 sw=2 et ai:
