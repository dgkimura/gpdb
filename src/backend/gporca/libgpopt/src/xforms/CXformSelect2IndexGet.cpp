//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CXformSelect2IndexGet.cpp
//
//	@doc:
//		Implementation of select over a table to an index get transformation
//---------------------------------------------------------------------------

#include "gpopt/xforms/CXformSelect2IndexGet.h"

#include "gpos/base.h"
#include "gpos/common/owner.h"

#include "gpopt/operators/CLogicalGet.h"
#include "gpopt/operators/CLogicalSelect.h"
#include "gpopt/xforms/CXformUtils.h"
#include "naucrates/md/CMDIndexGPDB.h"
#include "naucrates/md/CMDRelationGPDB.h"

using namespace gpopt;
using namespace gpmd;

//---------------------------------------------------------------------------
//	@function:
//		CXformSelect2IndexGet::CXformSelect2IndexGet
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CXformSelect2IndexGet::CXformSelect2IndexGet(CMemoryPool *mp)
	:  // pattern
	  CXformExploration(GPOS_NEW(mp) CExpression(
		  mp, GPOS_NEW(mp) CLogicalSelect(mp),
		  GPOS_NEW(mp) CExpression(
			  mp, GPOS_NEW(mp) CLogicalGet(mp)),  // relational child
		  GPOS_NEW(mp)
			  CExpression(mp, GPOS_NEW(mp) CPatternTree(mp))  // predicate tree
		  ))
{
}


//---------------------------------------------------------------------------
//	@function:
//		CXformSelect2IndexGet::Exfp
//
//	@doc:
//		Compute xform promise for a given expression handle
//
//---------------------------------------------------------------------------
CXform::EXformPromise
CXformSelect2IndexGet::Exfp(CExpressionHandle &exprhdl) const
{
	if (exprhdl.DeriveHasSubquery(1))
	{
		return CXform::ExfpNone;
	}

	return CXform::ExfpHigh;
}

//---------------------------------------------------------------------------
//	@function:
//		CXformSelect2IndexGet::Transform
//
//	@doc:
//		Actual transformation
//
//---------------------------------------------------------------------------
void
CXformSelect2IndexGet::Transform(gpos::pointer<CXformContext *> pxfctxt,
								 gpos::pointer<CXformResult *> pxfres,
								 gpos::pointer<CExpression *> pexpr) const
{
	GPOS_ASSERT(nullptr != pxfctxt);
	GPOS_ASSERT(FPromising(pxfctxt->Pmp(), this, pexpr));
	GPOS_ASSERT(FCheckPattern(pexpr));

	CMemoryPool *mp = pxfctxt->Pmp();

	// extract components
	gpos::pointer<CExpression *> pexprRelational = (*pexpr)[0];
	gpos::pointer<CExpression *> pexprScalar = (*pexpr)[1];

	// get the indexes on this relation
	gpos::pointer<CLogicalGet *> popGet =
		gpos::dyn_cast<CLogicalGet>(pexprRelational->Pop());
	const ULONG ulIndices = popGet->Ptabdesc()->IndexCount();
	if (0 == ulIndices)
	{
		return;
	}

	// array of expressions in the scalar expression
	gpos::owner<CExpressionArray *> pdrgpexpr =
		CPredicateUtils::PdrgpexprConjuncts(mp, pexprScalar);
	GPOS_ASSERT(pdrgpexpr->Size() > 0);

	// derive the scalar and relational properties to build set of required columns
	gpos::pointer<CColRefSet *> pcrsOutput = pexpr->DeriveOutputColumns();
	gpos::pointer<CColRefSet *> pcrsScalarExpr =
		pexprScalar->DeriveUsedColumns();

	gpos::owner<CColRefSet *> pcrsReqd = GPOS_NEW(mp) CColRefSet(mp);
	pcrsReqd->Include(pcrsOutput);
	pcrsReqd->Include(pcrsScalarExpr);

	// find the indexes whose included columns meet the required columns
	CMDAccessor *md_accessor = COptCtxt::PoctxtFromTLS()->Pmda();
	gpos::pointer<const IMDRelation *> pmdrel =
		md_accessor->RetrieveRel(popGet->Ptabdesc()->MDId());

	for (ULONG ul = 0; ul < ulIndices; ul++)
	{
		gpos::pointer<IMDId *> pmdidIndex = pmdrel->IndexMDidAt(ul);
		gpos::pointer<const IMDIndex *> pmdindex =
			md_accessor->RetrieveIndex(pmdidIndex);
		gpos::owner<CExpression *> pexprIndexGet =
			CXformUtils::PexprLogicalIndexGet(
				mp, md_accessor, pexprRelational, pexpr->Pop()->UlOpId(),
				pdrgpexpr, pcrsReqd, pcrsScalarExpr, nullptr /*outer_refs*/,
				pmdindex, pmdrel);
		if (nullptr != pexprIndexGet)
		{
			pxfres->Add(pexprIndexGet);
		}
	}

	pcrsReqd->Release();
	pdrgpexpr->Release();
}

// EOF
