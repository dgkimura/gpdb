//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2013 VMware, Inc. or its affiliates.
//
//	@filename:
//		CLogicalForeignGet.cpp
//
//	@doc:
//		Implementation of foreign get
//---------------------------------------------------------------------------

#include "gpopt/operators/CLogicalForeignGet.h"

#include "gpos/base.h"

#include "gpopt/base/CColRefSet.h"
#include "gpopt/base/CColRefSetIter.h"
#include "gpopt/base/CColRefTable.h"
#include "gpopt/base/COptCtxt.h"
#include "gpopt/base/CUtils.h"
#include "gpopt/metadata/CName.h"
#include "gpopt/metadata/CTableDescriptor.h"


using namespace gpopt;


//---------------------------------------------------------------------------
//	@function:
//		CLogicalForeignGet::CLogicalForeignGet
//
//	@doc:
//		Ctor - for pattern
//
//---------------------------------------------------------------------------
CLogicalForeignGet::CLogicalForeignGet(CMemoryPool *mp) : CLogicalGet(mp)
{
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalForeignGet::CLogicalForeignGet
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CLogicalForeignGet::CLogicalForeignGet(CMemoryPool *mp, const CName *pnameAlias,
									   gpos::Ref<CTableDescriptor> ptabdesc)
	: CLogicalGet(mp, pnameAlias, ptabdesc)
{
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalForeignGet::CLogicalForeignGet
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CLogicalForeignGet::CLogicalForeignGet(CMemoryPool *mp, const CName *pnameAlias,
									   gpos::Ref<CTableDescriptor> ptabdesc,
									   gpos::Ref<CColRefArray> pdrgpcrOutput)
	: CLogicalGet(mp, pnameAlias, ptabdesc, pdrgpcrOutput)
{
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalForeignGet::Matches
//
//	@doc:
//		Match function on operator level
//
//---------------------------------------------------------------------------
BOOL
CLogicalForeignGet::Matches(gpos::Ref<COperator> pop) const
{
	if (pop->Eopid() != Eopid())
	{
		return false;
	}
	gpos::Ref<CLogicalForeignGet> popGet = CLogicalForeignGet::PopConvert(pop);

	return Ptabdesc() == popGet->Ptabdesc() &&
		   PdrgpcrOutput()->Equals(popGet->PdrgpcrOutput());
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalForeignGet::PopCopyWithRemappedColumns
//
//	@doc:
//		Return a copy of the operator with remapped columns
//
//---------------------------------------------------------------------------
gpos::Ref<COperator>
CLogicalForeignGet::PopCopyWithRemappedColumns(
	CMemoryPool *mp, gpos::Ref<UlongToColRefMap> colref_mapping,
	BOOL must_exist)
{
	gpos::Ref<CColRefArray> pdrgpcrOutput = nullptr;
	if (must_exist)
	{
		pdrgpcrOutput =
			CUtils::PdrgpcrRemapAndCreate(mp, PdrgpcrOutput(), colref_mapping);
	}
	else
	{
		pdrgpcrOutput = CUtils::PdrgpcrRemap(mp, PdrgpcrOutput(),
											 colref_mapping, must_exist);
	}
	CName *pnameAlias = GPOS_NEW(mp) CName(mp, Name());

	gpos::Ref<CTableDescriptor> ptabdesc = Ptabdesc();


	return GPOS_NEW(mp)
		CLogicalForeignGet(mp, pnameAlias, ptabdesc, pdrgpcrOutput);
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalForeignGet::PxfsCandidates
//
//	@doc:
//		Get candidate xforms
//
//---------------------------------------------------------------------------
gpos::Ref<CXformSet>
CLogicalForeignGet::PxfsCandidates(CMemoryPool *mp) const
{
	gpos::Ref<CXformSet> xform_set = GPOS_NEW(mp) CXformSet(mp);
	(void) xform_set->ExchangeSet(CXform::ExfForeignGet2ForeignScan);

	return xform_set;
}

// EOF
