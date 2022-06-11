//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2011 Greenplum, Inc.
//
//	@filename:
//		CMDProviderTest.cpp
//
//	@doc:
//		Tests the file-based metadata provider.
//---------------------------------------------------------------------------

#include "unittest/gpopt/mdcache/CMDProviderTest.h"

#include "gpos/base.h"
#include "gpos/common/owner.h"
#include "gpos/io/COstreamString.h"
#include "gpos/io/ioutils.h"
#include "gpos/memory/CAutoMemoryPool.h"
#include "gpos/task/CAutoTaskProxy.h"
#include "gpos/test/CUnittest.h"

#include "gpopt/mdcache/CAutoMDAccessor.h"
#include "gpopt/mdcache/CMDCache.h"
#include "naucrates/dxl/CDXLUtils.h"
#include "naucrates/exception.h"
#include "naucrates/md/CMDIdColStats.h"
#include "naucrates/md/CMDIdRelStats.h"
#include "naucrates/md/CMDProviderMemory.h"

#include "unittest/gpopt/CTestUtils.h"

using namespace gpos;
using namespace gpdxl;
using namespace gpopt;

const CHAR *CMDProviderTest::file_name = "../data/dxl/metadata/md.xml";

//---------------------------------------------------------------------------
//	@function:
//		CMDProviderTest::EresUnittest
//
//	@doc:
//
//
//---------------------------------------------------------------------------
GPOS_RESULT
CMDProviderTest::EresUnittest()
{
	CUnittest rgut[] = {
		GPOS_UNITTEST_FUNC(CMDProviderTest::EresUnittest_Basic),
		GPOS_UNITTEST_FUNC(CMDProviderTest::EresUnittest_Stats),
		GPOS_UNITTEST_FUNC_THROW(CMDProviderTest::EresUnittest_Negative,
								 gpdxl::ExmaMD,
								 gpdxl::ExmiMDCacheEntryNotFound),
	};

	return CUnittest::EresExecute(rgut, GPOS_ARRAY_SIZE(rgut));
}

//---------------------------------------------------------------------------
//	@function:
//		CMDProviderTest::EresUnittest_Basic
//
//	@doc:
//		Test fetching existing metadata objects from a file-based provider
//
//---------------------------------------------------------------------------
GPOS_RESULT
CMDProviderTest::EresUnittest_Basic()
{
	// create memory pool
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();

	// test lookup with a file-based provider
	gpos::Ref<CMDProviderMemory> pmdpFile =
		GPOS_NEW(mp) CMDProviderMemory(mp, file_name);
	;

	TestMDLookup(mp, pmdpFile);

	;

	// test lookup with a memory-based provider
	CHAR *dxl_string = CDXLUtils::Read(mp, file_name);

	gpos::Ref<IMDCacheObjectArray> mdcache_obj_array =
		CDXLUtils::ParseDXLToIMDObjectArray(mp, dxl_string,
											nullptr /*xsd_file_path*/);

	gpos::Ref<CMDProviderMemory> pmdpMemory =
		GPOS_NEW(mp) CMDProviderMemory(mp, mdcache_obj_array.get());
	;
	TestMDLookup(mp, pmdpMemory);

	GPOS_DELETE_ARRAY(dxl_string);
	;
	;

	return GPOS_OK;
}

//---------------------------------------------------------------------------
//	@function:
//		CMDProviderTest::TestMDLookup
//
//	@doc:
//		Test looking up objects using given MD provider
//
//---------------------------------------------------------------------------
void
CMDProviderTest::TestMDLookup(CMemoryPool *mp, IMDProvider *pmdp)
{
	CAutoMDAccessor amda(mp, pmdp, CTestUtils::m_sysidDefault,
						 CMDCache::Pcache());

	// lookup different objects
	gpos::Ref<CMDIdGPDB> pmdid1 = GPOS_NEW(mp) CMDIdGPDB(
		GPOPT_MDCACHE_TEST_OID, 1 /* major version */, 1 /* minor version */);
	gpos::Ref<CMDIdGPDB> pmdid2 = GPOS_NEW(mp) CMDIdGPDB(
		GPOPT_MDCACHE_TEST_OID, 12 /* version */, 1 /* minor version */);

	CWStringBase *pstrMDObject1 =
		pmdp->GetMDObjDXLStr(mp, amda.Pmda(), pmdid1.get());
	CWStringBase *pstrMDObject2 =
		pmdp->GetMDObjDXLStr(mp, amda.Pmda(), pmdid2.get());

	GPOS_ASSERT(nullptr != pstrMDObject1 && nullptr != pstrMDObject2);

	gpos::Ref<IMDCacheObject> pimdobj1 =
		CDXLUtils::ParseDXLToIMDIdCacheObj(mp, pstrMDObject1, nullptr);

	gpos::Ref<IMDCacheObject> pimdobj2 =
		CDXLUtils::ParseDXLToIMDIdCacheObj(mp, pstrMDObject2, nullptr);

	GPOS_ASSERT(nullptr != pimdobj1 && pmdid1->Equals(pimdobj1->MDId()));
	GPOS_ASSERT(nullptr != pimdobj2 && pmdid2->Equals(pimdobj2->MDId()));

	// cleanup
	;
	;
	GPOS_DELETE(pstrMDObject1);
	GPOS_DELETE(pstrMDObject2);
	;
	;
}

//---------------------------------------------------------------------------
//	@function:
//		CMDProviderTest::EresUnittest_Stats
//
//	@doc:
//		Test fetching existing stats objects from a file-based provider
//
//---------------------------------------------------------------------------
GPOS_RESULT
CMDProviderTest::EresUnittest_Stats()
{
	// create memory pool
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();

	gpos::Ref<CMDProviderMemory> pmdpFile =
		GPOS_NEW(mp) CMDProviderMemory(mp, file_name);

	{
		;
		CAutoMDAccessor amda(mp, pmdpFile, CTestUtils::m_sysidDefault,
							 CMDCache::Pcache());

		// lookup different objects
		gpos::Ref<CMDIdRelStats> rel_stats_mdid = GPOS_NEW(mp)
			CMDIdRelStats(GPOS_NEW(mp) CMDIdGPDB(GPOPT_MDCACHE_TEST_OID, 1, 1));

		CWStringBase *pstrRelStats =
			pmdpFile->GetMDObjDXLStr(mp, amda.Pmda(), rel_stats_mdid.get());
		GPOS_ASSERT(nullptr != pstrRelStats);
		gpos::Ref<IMDCacheObject> pmdobjRelStats =
			CDXLUtils::ParseDXLToIMDIdCacheObj(mp, pstrRelStats, nullptr);
		GPOS_ASSERT(nullptr != pmdobjRelStats);

		gpos::Ref<CMDIdColStats> mdid_col_stats = GPOS_NEW(mp)
			CMDIdColStats(GPOS_NEW(mp) CMDIdGPDB(GPOPT_MDCACHE_TEST_OID, 1, 1),
						  1 /* attno */);

		CWStringBase *pstrColStats =
			pmdpFile->GetMDObjDXLStr(mp, amda.Pmda(), mdid_col_stats.get());
		GPOS_ASSERT(nullptr != pstrColStats);
		gpos::Ref<IMDCacheObject> pmdobjColStats =
			CDXLUtils::ParseDXLToIMDIdCacheObj(mp, pstrColStats, nullptr);
		GPOS_ASSERT(nullptr != pmdobjColStats);

		// cleanup
		;
		;
		GPOS_DELETE(pstrRelStats);
		GPOS_DELETE(pstrColStats);
		;
		;
	}

	;

	return GPOS_OK;
}

//---------------------------------------------------------------------------
//	@function:
//		CMDProviderTest::EresUnittest_Negative
//
//	@doc:
//		Test fetching non-exiting metadata objects from a file-based provider
//
//---------------------------------------------------------------------------
GPOS_RESULT
CMDProviderTest::EresUnittest_Negative()
{
	CAutoMemoryPool amp(CAutoMemoryPool::ElcNone);
	CMemoryPool *mp = amp.Pmp();

	gpos::Ref<CMDProviderMemory> pmdpFile =
		GPOS_NEW(mp) CMDProviderMemory(mp, file_name);
	;

	// we need to use an auto pointer for the cache here to ensure
	// deleting memory of cached objects when we throw
	CAutoP<CMDAccessor::MDCache> apcache;
	apcache = CCacheFactory::CreateCache<gpos::Ref<gpopt::IMDCacheObject>,
										 gpopt::CMDKey *>(
		true,  // fUnique
		0 /* unlimited cache quota */, CMDKey::UlHashMDKey,
		CMDKey::FEqualMDKey);

	CMDAccessor::MDCache *pcache = apcache.Value();

	{
		CAutoMDAccessor amda(mp, pmdpFile, CTestUtils::m_sysidDefault, pcache);

		// lookup a non-existing objects
		gpos::Ref<CMDIdGPDB> mdid = GPOS_NEW(mp)
			CMDIdGPDB(GPOPT_MDCACHE_TEST_OID, 15 /* major version */,
					  1 /* minor version */);

		// call should result in an exception
		(void) pmdpFile->GetMDObjDXLStr(mp, amda.Pmda(), mdid.get());
	}

	return GPOS_FAILED;
}

// EOF
