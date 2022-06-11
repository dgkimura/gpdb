//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CIndexDescriptorTest.cpp
//
//	@doc:
//		Test for CIndexDescriptor
//---------------------------------------------------------------------------
#include "unittest/gpopt/metadata/CIndexDescriptorTest.h"

#include "gpos/common/owner.h"
#include "gpos/io/COstreamString.h"
#include "gpos/string/CWStringDynamic.h"

#include "gpopt/base/CQueryContext.h"
#include "gpopt/base/CUtils.h"
#include "gpopt/eval/CConstExprEvaluatorDefault.h"
#include "gpopt/metadata/CTableDescriptor.h"
#include "naucrates/md/CMDIdGPDB.h"
#include "naucrates/md/CMDProviderMemory.h"
#include "naucrates/md/IMDIndex.h"

#include "unittest/base.h"
#include "unittest/gpopt/CTestUtils.h"


//---------------------------------------------------------------------------
//	@function:
//		CIndexDescriptorTest::EresUnittest
//
//	@doc:
//		Unittest for metadata names
//
//---------------------------------------------------------------------------
GPOS_RESULT
CIndexDescriptorTest::EresUnittest()
{
	CUnittest rgut[] = {
		GPOS_UNITTEST_FUNC(CIndexDescriptorTest::EresUnittest_Basic)};

	return CUnittest::EresExecute(rgut, GPOS_ARRAY_SIZE(rgut));
}

//---------------------------------------------------------------------------
//	@function:
//		CIndexDescriptorTest::EresUnittest_Basic
//
//	@doc:
//		Basic naming, key columns and index columns printing test
//
//---------------------------------------------------------------------------
GPOS_RESULT
CIndexDescriptorTest::EresUnittest_Basic()
{
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();

	// Setup an MD cache with a file-based provider
	gpos::Ref<CMDProviderMemory> pmdp = CTestUtils::m_pmdpf;
	;
	CMDAccessor mda(mp, CMDCache::Pcache(), CTestUtils::m_sysidDefault,
					std::move(pmdp));

	// install opt context in TLS
	CAutoOptCtxt aoc(mp, &mda, nullptr, /* pceeval */
					 CTestUtils::GetCostModel(mp));

	CWStringConst strName(GPOS_WSZ_LIT("MyTable"));
	gpos::Ref<CMDIdGPDB> mdid =
		GPOS_NEW(mp) CMDIdGPDB(GPOPT_MDCACHE_TEST_OID, 1, 1);
	gpos::Ref<CTableDescriptor> ptabdesc =
		CTestUtils::PtabdescCreate(mp, 10, std::move(mdid), CName(&strName));

	// get the index associated with the table
	const IMDRelation *pmdrel = mda.RetrieveRel(ptabdesc->MDId());
	GPOS_ASSERT(0 < pmdrel->IndexCount());

	// create an index descriptor
	IMDId *pmdidIndex = pmdrel->IndexMDidAt(0);	 // get the first index
	const IMDIndex *pmdindex = mda.RetrieveIndex(pmdidIndex);
	gpos::Ref<CIndexDescriptor> pindexdesc =
		CIndexDescriptor::Pindexdesc(mp, ptabdesc.get(), pmdindex);

#ifdef GPOS_DEBUG
	CWStringDynamic str(mp);
	COstreamString oss(&str);
	pindexdesc->OsPrint(oss);

	GPOS_TRACE(str.GetBuffer());
#endif	// GPOS_DEBUG

	// clean up
	;
	;

	return GPOS_OK;
}

// EOF
