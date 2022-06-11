//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2011 EMC Corp.
//
//	@filename:
//		CHashMapIterTest.cpp
//
//	@doc:
//		Test for CHashMapIter
//---------------------------------------------------------------------------

#include "unittest/gpos/common/CHashMapIterTest.h"

#include "gpos/base.h"
#include "gpos/common/CAutoRef.h"
#include "gpos/common/CHashMapIter.h"
#include "gpos/common/owner.h"
#include "gpos/memory/CAutoMemoryPool.h"
#include "gpos/test/CUnittest.h"

using namespace gpos;

//---------------------------------------------------------------------------
//	@function:
//		CHashMapIterTest::EresUnittest
//
//	@doc:
//		Unittest for basic hash map iterator
//
//---------------------------------------------------------------------------
GPOS_RESULT
CHashMapIterTest::EresUnittest()
{
	CUnittest rgut[] = {
		GPOS_UNITTEST_FUNC(CHashMapIterTest::EresUnittest_Basic),
	};

	return CUnittest::EresExecute(rgut, GPOS_ARRAY_SIZE(rgut));
}


//---------------------------------------------------------------------------
//	@function:
//		CHashMapIterTest::EresUnittest_Basic
//
//	@doc:
//		Basic iterator test
//
//---------------------------------------------------------------------------
GPOS_RESULT
CHashMapIterTest::EresUnittest_Basic()
{
	// create memory pool
	CAutoMemoryPool amp;
	CMemoryPool *mp = amp.Pmp();

	// test data
	ULONG rgul[] = {1, 2, 3, 4, 5, 6, 7, 8, 9};
	const ULONG ulCnt = GPOS_ARRAY_SIZE(rgul);

	typedef gpos::UnorderedMap<const ULONG *, ULONG *,
							   gpos::PtrHash<ULONG, HashPtr<ULONG>>,
							   gpos::PtrEqual<ULONG, gpos::Equals<ULONG>>>
		Map;

	typedef gpos::UnorderedMap<
		const ULONG *, ULONG *, gpos::PtrHash<ULONG, HashPtr<ULONG>>,
		gpos::PtrEqual<ULONG, gpos::Equals<ULONG>>>::LegacyIterator MapIter;


	// using N - 2 slots guarantees collisions
	gpos::Ref<Map> pm = GPOS_NEW(mp) Map(mp, ulCnt - 2);

#ifdef GPOS_DEBUG

	// iteration over empty map
	MapIter miEmpty(pm.get());
	GPOS_ASSERT(!miEmpty.Advance());

#endif	// GPOS_DEBUG

	typedef CDynamicPtrArray<const ULONG, CleanupNULL> ULongPtrArray;
	gpos::Ref<ULongPtrArray> pdrgpulKeys(GPOS_NEW(mp) ULongPtrArray(mp)),
		pdrgpulValues(GPOS_NEW(mp) ULongPtrArray(mp));
	// load map and iterate over it after each step
	for (ULONG ul = 0; ul < ulCnt; ++ul)
	{
		(void) pm->Insert(&rgul[ul], &rgul[ul]);
		pdrgpulKeys->Append(&rgul[ul]);
		pdrgpulValues->Append(&rgul[ul]);

		gpos::Ref<ULongPtrArray> pdrgpulIterKeys(GPOS_NEW(mp)
													 ULongPtrArray(mp)),
			pdrgpulIterValues(GPOS_NEW(mp) ULongPtrArray(mp));

		// iterate over full map
		MapIter mi(pm.get());
		while (mi.Advance())
		{
			pdrgpulIterKeys->Append(mi.Key());
			pdrgpulIterValues->Append(mi.Value());
		}

		pdrgpulIterKeys->Sort();
		pdrgpulIterValues->Sort();

		GPOS_ASSERT(pdrgpulKeys->Equals(pdrgpulIterKeys.get()));
		GPOS_ASSERT(pdrgpulValues->Equals(pdrgpulIterValues.get()));
	}

	;

	return GPOS_OK;
}


// EOF
