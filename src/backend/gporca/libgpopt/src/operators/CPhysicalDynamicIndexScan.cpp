//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CPhysicalDynamicIndexScan.cpp
//
//	@doc:
//		Implementation of dynamic index scan operator
//---------------------------------------------------------------------------

#include "gpopt/operators/CPhysicalDynamicIndexScan.h"

#include "gpos/base.h"
#include "gpos/common/owner.h"
#include "gpos/error/CAutoTrace.h"

#include "gpopt/base/COptCtxt.h"
#include "gpopt/base/CUtils.h"
#include "gpopt/cost/ICostModel.h"
#include "gpopt/metadata/CPartConstraint.h"
#include "gpopt/operators/CExpressionHandle.h"
#include "gpopt/operators/CPredicateUtils.h"
#include "naucrates/statistics/CFilterStatsProcessor.h"
#include "naucrates/statistics/CStatisticsUtils.h"

using namespace gpopt;


//---------------------------------------------------------------------------
//	@function:
//		CPhysicalDynamicIndexScan::CPhysicalDynamicIndexScan
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CPhysicalDynamicIndexScan::CPhysicalDynamicIndexScan(
	CMemoryPool *mp, gpos::Ref<CIndexDescriptor> pindexdesc,
	gpos::Ref<CTableDescriptor> ptabdesc, ULONG ulOriginOpId,
	const CName *pnameAlias, gpos::Ref<CColRefArray> pdrgpcrOutput,
	ULONG scan_id, gpos::Ref<CColRef2dArray> pdrgpdrgpcrPart,
	gpos::Ref<COrderSpec> pos, gpos::Ref<IMdIdArray> partition_mdids,
	gpos::Ref<ColRefToUlongMapArray> root_col_mapping_per_part)
	: CPhysicalDynamicScan(
		  mp, std::move(ptabdesc), ulOriginOpId, pnameAlias, scan_id,
		  std::move(pdrgpcrOutput), std::move(pdrgpdrgpcrPart),
		  std::move(partition_mdids), std::move(root_col_mapping_per_part)),
	  m_pindexdesc(std::move(pindexdesc)),
	  m_pos(std::move(pos))
{
	GPOS_ASSERT(nullptr != m_pindexdesc);
	GPOS_ASSERT(nullptr != m_pos);
}


//---------------------------------------------------------------------------
//	@function:
//		CPhysicalDynamicIndexScan::~CPhysicalDynamicIndexScan
//
//	@doc:
//		Dtor
//
//---------------------------------------------------------------------------
CPhysicalDynamicIndexScan::~CPhysicalDynamicIndexScan()
{
	;
	;
}

//---------------------------------------------------------------------------
//	@function:
//		CPhysicalDynamicIndexScan::EpetOrder
//
//	@doc:
//		Return the enforcing type for order property based on this operator
//
//---------------------------------------------------------------------------
CEnfdProp::EPropEnforcingType
CPhysicalDynamicIndexScan::EpetOrder(CExpressionHandle &,  // exprhdl
									 const CEnfdOrder *peo) const
{
	GPOS_ASSERT(nullptr != peo);
	GPOS_ASSERT(!peo->PosRequired()->IsEmpty());

	if (peo->FCompatible(m_pos.get()))
	{
		// required order is already established by the index
		return CEnfdProp::EpetUnnecessary;
	}

	return CEnfdProp::EpetRequired;
}

//---------------------------------------------------------------------------
//	@function:
//		CPhysicalDynamicIndexScan::HashValue
//
//	@doc:
//		Combine pointers for table descriptor, index descriptor and Eop
//
//---------------------------------------------------------------------------
ULONG
CPhysicalDynamicIndexScan::HashValue() const
{
	ULONG scan_id = ScanId();
	return gpos::CombineHashes(
		COperator::HashValue(),
		gpos::CombineHashes(gpos::HashValue(&scan_id),
							m_pindexdesc->MDId()->HashValue()));
}

//---------------------------------------------------------------------------
//	@function:
//		CPhysicalDynamicIndexScan::Matches
//
//	@doc:
//		match operator
//
//---------------------------------------------------------------------------
BOOL
CPhysicalDynamicIndexScan::Matches(COperator *pop) const
{
	return CUtils::FMatchDynamicIndex(this, pop);
}

//---------------------------------------------------------------------------
//	@function:
//		CPhysicalDynamicIndexScan::OsPrint
//
//	@doc:
//		debug print
//
//---------------------------------------------------------------------------
IOstream &
CPhysicalDynamicIndexScan::OsPrint(IOstream &os) const
{
	if (m_fPattern)
	{
		return COperator::OsPrint(os);
	}

	os << SzId() << " ";
	// index name
	os << "  Index: (";
	m_pindexdesc->Name().OsPrint(os);
	// table name
	os << ")";
	os << ", Table: (";
	Ptabdesc()->Name().OsPrint(os);
	os << ")";
	os << ", Columns: [";
	CUtils::OsPrintDrgPcr(os, PdrgpcrOutput());
	os << "] Scan Id: " << ScanId();


	return os;
}

//---------------------------------------------------------------------------
//	@function:
//		CPhysicalDynamicIndexScan::PstatsDerive
//
//	@doc:
//		Statistics derivation during costing
//
//---------------------------------------------------------------------------
gpos::Ref<IStatistics>
CPhysicalDynamicIndexScan::PstatsDerive(CMemoryPool *mp,
										CExpressionHandle &exprhdl,
										CReqdPropPlan *prpplan,
										IStatisticsArray *stats_ctxt) const
{
	GPOS_ASSERT(nullptr != prpplan);

	gpos::Ref<IStatistics> pstatsBaseTable =
		CStatisticsUtils::DeriveStatsForDynamicScan(
			mp, exprhdl, ScanId(), prpplan->Pepp()->PppsRequired());

	// create a conjunction of index condition and additional filters
	CExpression *pexprScalar = exprhdl.PexprScalarRepChild(0 /*ulChidIndex*/);
	gpos::Ref<CExpression> local_expr = nullptr;
	gpos::Ref<CExpression> expr_with_outer_refs = nullptr;

	// get outer references from expression handle
	CColRefSet *outer_refs = exprhdl.DeriveOuterReferences();

	CPredicateUtils::SeparateOuterRefs(mp, pexprScalar, outer_refs, &local_expr,
									   &expr_with_outer_refs);

	IStatistics *stats = CFilterStatsProcessor::MakeStatsFilterForScalarExpr(
		mp, exprhdl, pstatsBaseTable.get(), local_expr.get(),
		expr_with_outer_refs, stats_ctxt);

	;
	;
	;

	return stats;
}

// EOF
