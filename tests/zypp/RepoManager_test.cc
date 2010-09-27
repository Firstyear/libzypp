
#include <iostream>
#include <fstream>
#include <list>
#include <string>

#include "zypp/base/LogTools.h"
#include "zypp/base/Exception.h"
#include "zypp/KeyRing.h"
#include "zypp/PublicKey.h"
#include "zypp/TmpPath.h"
#include "zypp/PathInfo.h"
#include "zypp/ServiceInfo.h"

#include "zypp/RepoManager.h"

#include "TestSetup.h"

#include <boost/test/auto_unit_test.hpp>


#include "KeyRingTestReceiver.h"

using boost::unit_test::test_suite;
using boost::unit_test::test_case;

using namespace std;
using namespace zypp;
using namespace zypp::filesystem;
using namespace zypp::repo;

#define DATADIR (Pathname(TESTS_SRC_DIR) + "/zypp/data/RepoManager")

#define REPODATADIR (Pathname(TESTS_SRC_DIR) + "/repo/susetags/data/addon_in_subdir")


BOOST_AUTO_TEST_CASE(refresh_addon_in_subdir)
{
    KeyRingTestReceiver keyring_callbacks;
    KeyRingTestSignalReceiver receiver;

    // disable sgnature checking
    keyring_callbacks.answerAcceptKey(KeyRingReport::KEY_TRUST_TEMPORARILY);
    keyring_callbacks.answerAcceptVerFailed(true);
    keyring_callbacks.answerAcceptUnknownKey(true);

    // make sure we can refresh an addon which is in a subpath in a media url
    TestSetup test( Arch_x86_64 );
    RepoInfo info;
    info.setBaseUrl(Url(string("dir:") + REPODATADIR.asString()));
    info.setPath("/updates");
    info.setType(RepoType::YAST2);
    info.setAlias("boooh");

    test.loadRepo(info);

    // take care we actually got the data
    Repository r( test.satpool().reposFind( "boooh" ) );
    BOOST_REQUIRE( r );
    BOOST_CHECK_EQUAL( r.solvablesSize(), 2 );
    BOOST_CHECK_EQUAL( r.info().type(), repo::RepoType::YAST2 );
    BOOST_CHECK( r.info().hasLicense() );
}

BOOST_AUTO_TEST_CASE(pluginservices_test)
{
  TmpDir tmpCachePath;
  RepoManagerOptions opts( RepoManagerOptions::makeTestSetup( tmpCachePath ) ) ;

  filesystem::mkdir( opts.knownReposPath );
  filesystem::mkdir( opts.servicePluginsPath );

  opts.servicePluginsPath = DATADIR + "/plugin-service-lib-1";
  BOOST_CHECK(PathInfo(opts.servicePluginsPath / "service").isExist());

  {
    RepoManager manager(opts);
    BOOST_REQUIRE_EQUAL(1, manager.serviceSize());
    BOOST_CHECK(manager.repoEmpty());

    ServiceInfo service(*manager.serviceBegin());
    BOOST_CHECK_EQUAL("service", service.alias());
    BOOST_CHECK_EQUAL( "file:" + DATADIR.asString() + "/plugin-service-lib-1/service", service.url().asString());

    // now refresh the service
    manager.refreshServices();
    BOOST_CHECK_EQUAL((unsigned) 2, manager.repoSize());
    //std::list<RepoInfo> infos;
    //manager.getRepositoriesInService("test",
    //  insert_iterator<std::list<RepoInfo> >(infos,infos.begin()));
    //BOOST_CHECK_EQUAL(infos.size(), 2); // 2 from new repoindex
  }
  
  // Now simulate the service changed
  opts.servicePluginsPath = DATADIR + "/plugin-service-lib-2";
  {
    RepoManager manager(opts);
    BOOST_REQUIRE_EQUAL(1, manager.serviceSize());

    ServiceInfo service(*manager.serviceBegin());
    BOOST_CHECK_EQUAL("service", service.alias());
    BOOST_CHECK_EQUAL( "file:" + DATADIR.asString() + "/plugin-service-lib-2/service", service.url().asString());
    // now refresh the service
    manager.refreshServices();
    BOOST_CHECK_EQUAL((unsigned) 1, manager.repoSize());
  }
}

BOOST_AUTO_TEST_CASE(repomanager_test)
{
  TmpDir tmpCachePath;
  RepoManagerOptions opts( RepoManagerOptions::makeTestSetup( tmpCachePath ) ) ;

  filesystem::mkdir( opts.knownReposPath );
  filesystem::mkdir( opts.knownServicesPath );
  BOOST_CHECK_EQUAL( filesystem::copy_dir_content( DATADIR + "/repos.d", opts.knownReposPath ), 0 );

  RepoManager manager(opts);

  list<RepoInfo> repos;
  repos.insert(repos.end(), manager.repoBegin(), manager.repoEnd());
  BOOST_CHECK_EQUAL(repos.size(), (unsigned) 4);

  // now add a .repo file with 2 repositories in it
  Url url;
  url.setPathName((DATADIR + "/proprietary.repo").asString());
  url.setScheme("file");

  manager.addRepositories(url);

  // check it was not overwriten the proprietary.repo file
  BOOST_CHECK( PathInfo(opts.knownReposPath + "/proprietary.repo_1").isExist() );

  // now there should be 6 repos
  repos.clear();
  repos.insert(repos.end(), manager.repoBegin(), manager.repoEnd());
  BOOST_CHECK_EQUAL(repos.size(), (unsigned) 6);

  RepoInfo office_dup;
  office_dup.setAlias("office");
  BOOST_CHECK_THROW(manager.addRepository(office_dup), RepoAlreadyExistsException);

  // delete the office repo inside the propietary_1.repo
  RepoInfo office;
  office.setAlias("office");
  manager.removeRepository(office);
  // now there should be 5 repos
  repos.clear();
  repos.insert(repos.end(), manager.repoBegin(), manager.repoEnd());
  BOOST_CHECK_EQUAL(repos.size(), (unsigned) 5);
  // the file still contained one repo, so it should still exists
  BOOST_CHECK( PathInfo(opts.knownReposPath + "/proprietary.repo_1").isExist() );

  // now delete the macromedia one
  RepoInfo macromedia;
  macromedia.setAlias("macromedia");
  manager.removeRepository(macromedia);
  BOOST_CHECK_EQUAL(manager.repoSize(), (unsigned) 4);
  // the file should not exist anymore
  BOOST_CHECK( ! PathInfo(opts.knownReposPath + "/proprietary.repo_1").isExist() );

  //test service

  Url urlS;
  urlS.setPathName(DATADIR.asString());
  urlS.setScheme("dir");

  ServiceInfo service("test", urlS);
  service.setEnabled(true);

  manager.addService(service);
  manager.refreshServices();
  BOOST_CHECK_EQUAL(manager.repoSize(), (unsigned) 7); // +3 from repoindex

  //simulate change of repoindex.xml
  urlS.setPathName((DATADIR+"second").asString());
  urlS.setScheme("dir");
  service.setUrl(urlS);
  service.setEnabled(true);

  manager.modifyService(service.alias(), service);
  manager.refreshServices();
  BOOST_CHECK_EQUAL(manager.repoSize(), (unsigned) 6); // -1 from new repoindex

  std::list<RepoInfo> infos;
  manager.getRepositoriesInService("test",
    insert_iterator<std::list<RepoInfo> >(infos,infos.begin()));
  BOOST_CHECK_EQUAL(infos.size(), 2); // 2 from new repoindex


  // let test cache creation

  RepoInfo repo;
  repo.setAlias("foo");
  Url repourl("dir:" + string(TESTS_SRC_DIR) + "/repo/yum/data/10.2-updates-subset");
  //Url repourl("dir:/mounts/dist/install/stable-x86/suse");
  //BOOST_CHECK_MESSAGE(0, repourl.asString());
  repo.setBaseUrl(repourl);

  KeyRingTestReceiver keyring_callbacks;
  KeyRingTestSignalReceiver receiver;

  // disable sgnature checking
  keyring_callbacks.answerAcceptKey(KeyRingReport::KEY_TRUST_TEMPORARILY);
  keyring_callbacks.answerAcceptVerFailed(true);
  keyring_callbacks.answerAcceptUnknownKey(true);

  // We have no metadata and cache yet
  BOOST_CHECK_MESSAGE( !manager.isCached(repo), "Repo should not yet be cached" );

  // This should download metadata and build the cache
  manager.buildCache(repo);

  // Now we have metadata and cache
  BOOST_CHECK_MESSAGE( manager.isCached(repo), "Repo should be cached now" );

  // Metadata are up to date
  RepoManager::RefreshCheckStatus ref_stat = manager.checkIfToRefreshMetadata(repo, *repo.baseUrlsBegin());
  SEC << endl << ref_stat << endl;
  BOOST_CHECK_MESSAGE( ref_stat== RepoManager::REPO_UP_TO_DATE || ref_stat == RepoManager::REPO_CHECK_DELAYED, "Metadata should be up to date" );

   // the solv file should exists now
  Pathname base = (opts.repoCachePath / "solv" / repo.alias());
  Pathname solvfile = base / "solv";
  Pathname cookiefile = base / "cookie";
  BOOST_CHECK_MESSAGE( PathInfo(solvfile).isExist(), "Solv file is created after caching: " + solvfile.asString());
  BOOST_CHECK_MESSAGE( PathInfo(cookiefile).isExist(), "Cookie file is created after caching: " + cookiefile.asString());

  MIL << "Repo already in cache, clean cache"<< endl;
  manager.cleanCache(repo);

  BOOST_CHECK_MESSAGE( !manager.isCached(repo),
                       "Repo cache was just deleted, should not be cached now" );

  // now cache should build normally
  manager.buildCache(repo);

  manager.loadFromCache(repo);

  if ( manager.isCached(repo ) )
  {
    MIL << "Repo already in cache, clean cache"<< endl;
    manager.cleanCache(repo);
  }
  MIL << "Parsing repository metadata..." << endl;
  manager.buildCache(repo);
}

BOOST_AUTO_TEST_CASE(repo_seting_test)
{
  RepoInfo repo;
  repo.setAlias("foo");
  repo.addBaseUrl(string("http://test.org"));
  BOOST_CHECK_MESSAGE( repo.keepPackages(), "http scheme is not cached");
  repo.setBaseUrl(string("ftp://test.org"));
  BOOST_CHECK_MESSAGE( repo.keepPackages(), "ftp scheme is not cached");
  repo.setBaseUrl(string("smb://test.org"));
  BOOST_CHECK_MESSAGE( !repo.keepPackages(), "smb scheme is cached");
  repo.setBaseUrl(string("file:///test.txt"));
  BOOST_CHECK_MESSAGE( !repo.keepPackages(), "file scheme is cached");
  repo.setBaseUrl(string("http://test.org"));
  BOOST_CHECK_MESSAGE( repo.keepPackages(), "cache is depend on second url.");
}

//! \todo test this
//BOOST_AUTO_TEST_CASE(repo_dont_overwrite_external_settings_test)
//{
//}
