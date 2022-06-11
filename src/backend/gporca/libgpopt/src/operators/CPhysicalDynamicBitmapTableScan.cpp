//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2014 VMware, Inc. or its affiliates.
//
//	@filename:
//		CPhysicalDynamicBitmapTableScan.cpp
//
//	@doc:
//		Dynamic bitmap table scan physical operator
//
//	@owner:
//
//
//	@test:
//
//---------------------------------------------------------------------------

#include "gpopt/operators/CPhysicalDynamicBitmapTableScan.h"

#include "gpos/common/owner.h"

#include "gpopt/base/CDistributionSpec.h"
#include "gpopt/base/CUtils.h"
#include "gpopt/metadata/CName.h"
#include "gpopt/metadata/CPartConstraint.h"
#include "gpopt/metadata/CTableDescriptor.h"
#include "gpopt/operators/CExpressionHandle.h"
#include "gpopt/operators/CPredicateUtils.h"
#include "naucrates/statistics/CFilterStatsProcessor.h"
#include "naucrates/statistics/CStatisticsUtils.h"
using namespace gpopt;
using namespace gpos;

//---------------------------------------------------------------------------
//	@function:
//		CPhysicalDynamicBitmapTableScan::CPhysicalDynamicBitmapTableScan
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CPhysicalDynamicBitmapTableScan::CPhysicalDynamicBitmapTableScan(
	CMemoryPool *mp, gpos::owner<CTableDescriptor *> ptabdesc,
	ULONG ulOriginOpId, const CName *pnameAlias, ULONG scan_id,
	gpos::owner<CColRefArray *> pdrgpcrOutput,
	gpos::owner<CColRef2dArray *> pdrgpdrgpcrParts,
	gpos::owner<IMdIdArray *> partition_mdids,
	gpos::owner<ColRefToUlongMapArray *> root_col_mapping_per_part)
	: CPhysicalDynamicScan(
		  mp, std::move(ptabdesc), ulOriginOpId, pnameAlias, scan_id,
		  std::move(pdrgpcrOutput), std::move(pdrgpdrgpcrParts),
		  std::move(partition_mdids), std::move(root_col_mapping_per_part))
{
}

//---------------------------------------------------------------------------
//	@function:
//		CPhysicalDynamicBitmapTableScan::Matches
//
//	@doc:
//		match operator
//
//---------------------------------------------------------------------------
BOOL
CPhysicalDynamicBitmapTableScan::Matches(gpos::pointer<COperator *> pop) const
{
	return CUtils::FMatchDynamicBitmapScan(this, pop);
}

//---------------------------------------------------------------------------
//	@function:
//		CPhysicalDynamicBitmapTableScan::PstatsDerive
//
//	@doc:
//		Statistics derivation during costing
//
//---------------------------------------------------------------------------
gpos::owner<IStatistics *>
CPhysicalDynamicBitmapTableScan::PstatsDerive(
	CMemoryPool *mp, CExpressionHandle &exprhdl,
	gpos::pointer<CReqdPropPlan *> prpplan,
	gpos::pointer<IStatisticsArray *> stats_ctxt) const
{
	GPOS_ASSERT(nullptr != prpplan);

	gpos::owner<IStatistics *> pstatsBaseTable =
		CStatisticsUtils::DeriveStatsForDynamicScan(
			mp, exprhdl, ScanId(), prpplan->Pepp()->PppsRequired());

	CExpression *pexprCondChild =
		exprhdl.PexprScalarRepChild(0 /*ulChidIndex*/);
	gpos::owner<CExpression *> local_expr = nullptr;
	gpos::owner<CExpression *> expr_with_outer_refs = nullptr;

	// get outer references from expression handle
	CColRefSet *outer_refs = exprhdl.DeriveOuterReferences();

	CPredicateUtils::SeparateOuterRefs(mp, pexprCondChild, outer_refs,
									   &local_expr, &expr_with_outer_refs);

	IStatistics *stats = CFilterStatsProcessor::MakeStatsFilterForScalarExpr(
		mp, exprhdl, pstatsBaseTable, local_expr, expr_with_outer_refs,
		stats_ctxt);

	pstatsBaseTable->Release();
	local_expr->Release();
	expr_with_outer_refs->Release();

	return stats;
}
// EOF
