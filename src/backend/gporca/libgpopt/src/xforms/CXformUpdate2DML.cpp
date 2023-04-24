//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CXformUpdate2DML.cpp
//
//	@doc:
//		Implementation of transform
//---------------------------------------------------------------------------

#include "gpopt/xforms/CXformUpdate2DML.h"

#include "gpos/base.h"

#include "gpopt/metadata/CTableDescriptor.h"
#include "gpopt/operators/CLogicalPartitionSelector.h"
#include "gpopt/operators/CLogicalSplit.h"
#include "gpopt/operators/CLogicalUpdate.h"
#include "gpopt/operators/CPatternLeaf.h"
#include "gpopt/operators/CScalarDMLAction.h"
#include "gpopt/operators/CScalarProjectElement.h"
#include "gpopt/operators/CScalarProjectList.h"
#include "gpopt/optimizer/COptimizerConfig.h"
#include "gpopt/xforms/CXformUtils.h"
#include "naucrates/md/IMDTypeInt4.h"

using namespace gpopt;


//---------------------------------------------------------------------------
//	@function:
//		CXformUpdate2DML::CXformUpdate2DML
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CXformUpdate2DML::CXformUpdate2DML(CMemoryPool *mp)
	: CXformExploration(
		  // pattern
		  GPOS_NEW(mp) CExpression(
			  mp, GPOS_NEW(mp) CLogicalUpdate(mp),
			  GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CPatternLeaf(mp))))
{
}

//---------------------------------------------------------------------------
//	@function:
//		CXformUpdate2DML::Exfp
//
//	@doc:
//		Compute promise of xform
//
//---------------------------------------------------------------------------
CXform::EXformPromise
CXformUpdate2DML::Exfp(CExpressionHandle &	// exprhdl
) const
{
	return CXform::ExfpHigh;
}

//---------------------------------------------------------------------------
//	@function:
//		CXformUpdate2DML::Transform
//
//	@doc:
//		Actual transformation
//
//---------------------------------------------------------------------------
void
CXformUpdate2DML::Transform(gpos::Ref<CXformContext> pxfctxt,
							gpos::Ref<CXformResult> pxfres,
							gpos::Ref<CExpression> pexpr) const
{
	GPOS_ASSERT(nullptr != pxfctxt);
	GPOS_ASSERT(FPromising(pxfctxt->Pmp(), this, pexpr));
	GPOS_ASSERT(FCheckPattern(pexpr));

	gpos::Ref<CLogicalUpdate> popUpdate =
		CLogicalUpdate::PopConvert(pexpr->Pop());
	CMemoryPool *mp = pxfctxt->Pmp();

	// extract components for alternative

	gpos::Ref<CTableDescriptor> ptabdesc = popUpdate->Ptabdesc();
	CColRefArray *pdrgpcrDelete = popUpdate->PdrgpcrDelete();
	CColRefArray *pdrgpcrInsert = popUpdate->PdrgpcrInsert();
	gpos::Ref<CColRef> pcrCtid = popUpdate->PcrCtid();
	gpos::Ref<CColRef> pcrSegmentId = popUpdate->PcrSegmentId();
	BOOL fSplit = popUpdate->FSplit();

	// child of update operator
	gpos::Ref<CExpression> pexprChild = (*pexpr)[0];
	pexprChild->AddRef();
	// generate the action column and split operator
	gpos::Ref<COptCtxt> poctxt = COptCtxt::PoctxtFromTLS();
	CMDAccessor *md_accessor = poctxt->Pmda();
	CColumnFactory *col_factory = poctxt->Pcf();

	const gpos::Ref<IMDType> pmdtype = md_accessor->PtMDType<IMDTypeInt4>();
	gpos::Ref<CColRef> pcrAction =
		col_factory->PcrCreate(pmdtype, default_type_modifier);

	gpos::Ref<CExpression> pexprSplit = nullptr;
	if (fSplit)
	{
		pdrgpcrDelete->AddRef();
		pdrgpcrInsert->AddRef();
		gpos::Ref<CExpression> pexprProjElem = GPOS_NEW(mp) CExpression(
			mp, GPOS_NEW(mp) CScalarProjectElement(mp, pcrAction),
			GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CScalarDMLAction(mp)));

		gpos::Ref<CExpression> pexprProjList = GPOS_NEW(mp)
			CExpression(mp, GPOS_NEW(mp) CScalarProjectList(mp), pexprProjElem);
		pexprSplit = GPOS_NEW(mp) CExpression(
			mp,
			GPOS_NEW(mp) CLogicalSplit(mp, pdrgpcrDelete, pdrgpcrInsert,
									   pcrCtid, pcrSegmentId, pcrAction),
			pexprChild, pexprProjList);
	}
	else
	{
		pexprSplit = pexprChild;
	}


	// add assert checking that no NULL values are inserted for nullable columns or no check constraints are violated
	gpos::Ref<COptimizerConfig> optimizer_config =
		COptCtxt::PoctxtFromTLS()->GetOptimizerConfig();
	gpos::Ref<CExpression> pexprProject;
	if (optimizer_config->GetHint()->FEnforceConstraintsOnDML())
	{
		pexprProject = CXformUtils::PexprAssertConstraints(
			mp, pexprSplit, ptabdesc, pdrgpcrInsert);
	}
	else
	{
		pexprProject = pexprSplit;
	}

	const ULONG num_cols = pdrgpcrInsert->Size();

	gpos::Ref<CExpression> pexprDML = nullptr;
	// create logical DML
	ptabdesc->AddRef();
	if (fSplit)
	{
		gpos::Ref<CBitSet> pbsModified =
			GPOS_NEW(mp) CBitSet(mp, ptabdesc->ColumnCount());
		for (ULONG ul = 0; ul < num_cols; ul++)
		{
			gpos::Ref<CColRef> pcrInsert = (*pdrgpcrInsert)[ul];
			gpos::Ref<CColRef> pcrDelete = (*pdrgpcrDelete)[ul];
			if (pcrInsert != pcrDelete)
			{
				// delete columns refer to the original tuple's descriptor, if it's different
				// from the corresponding insert column, then we're modifying the column
				// at that position
				pbsModified->ExchangeSet(ul);
			}
		}
		pdrgpcrDelete->AddRef();
		pexprDML = GPOS_NEW(mp) CExpression(
			mp,
			GPOS_NEW(mp) CLogicalDML(mp, CLogicalDML::EdmlUpdate, ptabdesc,
									 pdrgpcrDelete, pbsModified, pcrAction,
									 pcrCtid, pcrSegmentId, fSplit),
			pexprProject);
	}
	else
	{
		pdrgpcrInsert->AddRef();
		pexprDML = GPOS_NEW(mp) CExpression(
			mp,
			GPOS_NEW(mp) CLogicalDML(mp, CLogicalDML::EdmlUpdate, ptabdesc,
									 pdrgpcrInsert, GPOS_NEW(mp) CBitSet(mp),
									 pcrAction, pcrCtid, pcrSegmentId, fSplit),
			pexprProject);
	}

	// TODO:  - Oct 30, 2012; detect and handle AFTER triggers on update

	pxfres->Add(pexprDML);
}

// EOF
