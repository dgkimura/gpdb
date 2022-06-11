//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2020 VMware, Inc.
//
//	@filename:
//		CXformIndexGet2IndexOnlyScan.cpp
//
//	@doc:
//		Implementation of transform
//---------------------------------------------------------------------------

#include "gpopt/xforms/CXformIndexGet2IndexOnlyScan.h"

#include <cwchar>

#include "gpos/base.h"
#include "gpos/common/owner.h"

#include "gpopt/base/COptCtxt.h"
#include "gpopt/metadata/CIndexDescriptor.h"
#include "gpopt/metadata/CTableDescriptor.h"
#include "gpopt/operators/CExpressionHandle.h"
#include "gpopt/operators/CLogicalIndexGet.h"
#include "gpopt/operators/CPatternLeaf.h"
#include "gpopt/operators/CPhysicalIndexOnlyScan.h"
#include "gpopt/xforms/CXformUtils.h"
#include "naucrates/md/CMDIndexGPDB.h"

using namespace gpopt;


//---------------------------------------------------------------------------
//	@function:
//		CXformIndexGet2IndexOnlyScan::CXformIndexGet2IndexOnlyScan
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CXformIndexGet2IndexOnlyScan::CXformIndexGet2IndexOnlyScan(CMemoryPool *mp)
	:  // pattern
	  CXformImplementation(GPOS_NEW(mp) CExpression(
		  mp, GPOS_NEW(mp) CLogicalIndexGet(mp),
		  GPOS_NEW(mp) CExpression(
			  mp, GPOS_NEW(mp) CPatternLeaf(mp))  // index lookup predicate
		  ))
{
}

CXform::EXformPromise
CXformIndexGet2IndexOnlyScan::Exfp(CExpressionHandle &exprhdl) const
{
	CLogicalIndexGet *popGet = gpos::dyn_cast<CLogicalIndexGet>(exprhdl.Pop());

	CTableDescriptor *ptabdesc = popGet->Ptabdesc();
	CIndexDescriptor *pindexdesc = popGet->Pindexdesc();
	BOOL possible_ao_table = ptabdesc->IsAORowOrColTable() ||
							 ptabdesc->RetrieveRelStorageType() ==
								 IMDRelation::ErelstorageMixedPartitioned;

	if ((pindexdesc->IndexType() == IMDIndex::EmdindBtree &&
		 possible_ao_table) ||
		!pindexdesc->SupportsIndexOnlyScan())
	{
		// we don't support btree index scans on AO tables
		// FIXME: relax btree requirement. GiST and SP-GiST indexes can support some operator classes, but Gin cannot
		return CXform::ExfpNone;
	}

	if (exprhdl.DeriveHasSubquery(0))
	{
		return CXform::ExfpNone;
	}

	return CXform::ExfpHigh;
}

//---------------------------------------------------------------------------
//	@function:
//		CXformIndexGet2IndexOnlyScan::Transform
//
//	@doc:
//		Actual transformation
//
//---------------------------------------------------------------------------
void
CXformIndexGet2IndexOnlyScan::Transform(CXformContext *pxfctxt,
										CXformResult *pxfres,
										CExpression *pexpr) const
{
	GPOS_ASSERT(nullptr != pxfctxt);
	GPOS_ASSERT(FPromising(pxfctxt->Pmp(), this, pexpr));
	GPOS_ASSERT(FCheckPattern(pexpr));

	CLogicalIndexGet *pop = gpos::dyn_cast<CLogicalIndexGet>(pexpr->Pop());
	CMemoryPool *mp = pxfctxt->Pmp();
	CIndexDescriptor *pindexdesc = pop->Pindexdesc();
	CTableDescriptor *ptabdesc = pop->Ptabdesc();

	// extract components
	CExpression *pexprIndexCond = (*pexpr)[0];
	if (pexprIndexCond->DeriveHasSubquery())
	{
		return;
	}

	CMDAccessor *md_accessor = COptCtxt::PoctxtFromTLS()->Pmda();
	const IMDRelation *pmdrel = md_accessor->RetrieveRel(ptabdesc->MDId());
	const IMDIndex *pmdindex = md_accessor->RetrieveIndex(pindexdesc->MDId());

	gpos::Ref<CColRefArray> pdrgpcrOutput = pop->PdrgpcrOutput();
	GPOS_ASSERT(nullptr != pdrgpcrOutput);
	;

	gpos::Ref<CColRefSet> matched_cols =
		CXformUtils::PcrsIndexKeys(mp, pdrgpcrOutput.get(), pmdindex, pmdrel);
	gpos::Ref<CColRefSet> output_cols = GPOS_NEW(mp) CColRefSet(mp);

	// An index only scan is allowed iff each used output column reference also
	// exists as a column in the index.
	for (ULONG i = 0; i < pdrgpcrOutput->Size(); i++)
	{
		CColRef *col = (*pdrgpcrOutput)[i];

		// In most cases we want to treat system columns unconditionally as
		// used. This is because certain transforms like those for DML or
		// CXformPushGbBelowJoin use unique keys in the derived properties,
		// even if they are not referenced in the query. Those keys are system
		// columns gp_segment_id and ctid. We also treat distribution columns
		// as used, since they appear in the CDistributionSpecHashed of
		// physical properties and therefore might be used in the plan.
		if (col->GetUsage(true /*check_system_cols*/,
						  true /*check_distribution_col*/) == CColRef::EUsed)
		{
			output_cols->Include(col);
		}
	}

	if (!matched_cols->ContainsAll(output_cols.get()))
	{
		;
		;
		;
		return;
	}

	;
	;

	;
	;

	COrderSpec *pos = pop->Pos();
	GPOS_ASSERT(nullptr != pos);
	;



	// addref all children
	;

	gpos::Ref<CExpression> pexprAlt = GPOS_NEW(mp)
		CExpression(mp,
					GPOS_NEW(mp) CPhysicalIndexOnlyScan(
						mp, pindexdesc, ptabdesc, pexpr->Pop()->UlOpId(),
						GPOS_NEW(mp) CName(mp, pop->NameAlias()),
						std::move(pdrgpcrOutput), pos),
					pexprIndexCond);
	pxfres->Add(std::move(pexprAlt));
}


// EOF
