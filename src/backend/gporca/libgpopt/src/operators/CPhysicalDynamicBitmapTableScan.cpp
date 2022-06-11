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
	CMemoryPool *mp, gpos::Ref<CTableDescriptor> ptabdesc, ULONG ulOriginOpId,
	const CName *pnameAlias, ULONG scan_id,
	gpos::Ref<CColRefArray> pdrgpcrOutput,
	gpos::Ref<CColRef2dArray> pdrgpdrgpcrParts,
	gpos::Ref<IMdIdArray> partition_mdids,
	gpos::Ref<ColRefToUlongMapArray> root_col_mapping_per_part)
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
CPhysicalDynamicBitmapTableScan::Matches(COperator *pop) const
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
gpos::Ref<IStatistics>
CPhysicalDynamicBitmapTableScan::PstatsDerive(
	CMemoryPool *mp, CExpressionHandle &exprhdl, CReqdPropPlan *prpplan,
	IStatisticsArray *stats_ctxt) const
{
	GPOS_ASSERT(nullptr != prpplan);

	gpos::Ref<IStatistics> pstatsBaseTable =
		CStatisticsUtils::DeriveStatsForDynamicScan(
			mp, exprhdl, ScanId(), prpplan->Pepp()->PppsRequired());

	CExpression *pexprCondChild =
		exprhdl.PexprScalarRepChild(0 /*ulChidIndex*/);
	gpos::Ref<CExpression> local_expr = nullptr;
	gpos::Ref<CExpression> expr_with_outer_refs = nullptr;

	// get outer references from expression handle
	CColRefSet *outer_refs = exprhdl.DeriveOuterReferences();

	CPredicateUtils::SeparateOuterRefs(mp, pexprCondChild, outer_refs,
									   &local_expr, &expr_with_outer_refs);

	IStatistics *stats = CFilterStatsProcessor::MakeStatsFilterForScalarExpr(
		mp, exprhdl, pstatsBaseTable.get(), local_expr.get(),
		expr_with_outer_refs, stats_ctxt);

	;
	;
	;

	return stats;
}
// EOF
