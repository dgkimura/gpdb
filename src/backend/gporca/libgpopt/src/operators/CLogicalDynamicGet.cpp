//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CLogicalDynamicGet.cpp
//
//	@doc:
//		Implementation of dynamic table access
//---------------------------------------------------------------------------

#include "gpopt/operators/CLogicalDynamicGet.h"

#include "gpos/base.h"

#include "gpopt/base/CColRefSet.h"
#include "gpopt/base/CColRefSetIter.h"
#include "gpopt/base/CColRefTable.h"
#include "gpopt/base/CConstraintInterval.h"
#include "gpopt/base/COptCtxt.h"
#include "gpopt/base/CUtils.h"
#include "gpopt/metadata/CName.h"
#include "gpopt/metadata/CPartConstraint.h"
#include "gpopt/metadata/CTableDescriptor.h"
#include "gpopt/operators/CExpressionHandle.h"
#include "naucrates/statistics/CFilterStatsProcessor.h"
#include "naucrates/statistics/CStatistics.h"
#include "naucrates/statistics/CStatsPredUtils.h"

using namespace gpopt;


//---------------------------------------------------------------------------
//	@function:
//		CLogicalDynamicGet::CLogicalDynamicGet
//
//	@doc:
//		ctor - for pattern
//
//---------------------------------------------------------------------------
CLogicalDynamicGet::CLogicalDynamicGet(CMemoryPool *mp)
	: CLogicalDynamicGetBase(mp)
{
}


//---------------------------------------------------------------------------
//	@function:
//		CLogicalDynamicGet::CLogicalDynamicGet
//
//	@doc:
//		ctor
//
//---------------------------------------------------------------------------
CLogicalDynamicGet::CLogicalDynamicGet(
	CMemoryPool *mp, const CName *pnameAlias,
	gpos::Ref<CTableDescriptor> ptabdesc, ULONG ulPartIndex,
	gpos::Ref<CColRefArray> pdrgpcrOutput,
	gpos::Ref<CColRef2dArray> pdrgpdrgpcrPart,
	gpos::Ref<IMdIdArray> partition_mdids,
	gpos::Ref<CConstraint> partition_cnstrs_disj, BOOL static_pruned,
	gpos::Ref<IMdIdArray> foreign_server_mdids)
	: CLogicalDynamicGetBase(mp, pnameAlias, ptabdesc, ulPartIndex,
							 pdrgpcrOutput, pdrgpdrgpcrPart, partition_mdids),
	  m_partition_cnstrs_disj(partition_cnstrs_disj),
	  m_static_pruned(static_pruned),
	  m_foreign_server_mdids(foreign_server_mdids)
{
	GPOS_ASSERT(static_pruned || (nullptr == partition_cnstrs_disj));
	GPOS_ASSERT(nullptr != foreign_server_mdids);
}


//---------------------------------------------------------------------------
//	@function:
//		CLogicalDynamicGet::CLogicalDynamicGet
//
//	@doc:
//		ctor
//
//---------------------------------------------------------------------------
CLogicalDynamicGet::CLogicalDynamicGet(
	CMemoryPool *mp, const CName *pnameAlias,
	gpos::Ref<CTableDescriptor> ptabdesc, ULONG ulPartIndex,
	gpos::Ref<IMdIdArray> partition_mdids,
	gpos::Ref<IMdIdArray> foreign_server_mdids)
	: CLogicalDynamicGetBase(mp, pnameAlias, ptabdesc, ulPartIndex,
							 partition_mdids),
	  m_foreign_server_mdids(foreign_server_mdids)
{
	GPOS_ASSERT(nullptr != foreign_server_mdids);
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalDynamicGet::~CLogicalDynamicGet
//
//	@doc:
//		dtor
//
//---------------------------------------------------------------------------
CLogicalDynamicGet::~CLogicalDynamicGet()
{
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalDynamicGet::HashValue
//
//	@doc:
//		Operator specific hash function
//
//---------------------------------------------------------------------------
ULONG
CLogicalDynamicGet::HashValue() const
{
	ULONG ulHash = gpos::CombineHashes(COperator::HashValue(),
									   m_ptabdesc->MDId()->HashValue());
	ulHash =
		gpos::CombineHashes(ulHash, CUtils::UlHashColArray(m_pdrgpcrOutput));

	return ulHash;
}


//---------------------------------------------------------------------------
//	@function:
//		CLogicalDynamicGet::Matches
//
//	@doc:
//		Match function on operator level
//
//---------------------------------------------------------------------------
BOOL
CLogicalDynamicGet::Matches(gpos::Ref<COperator> pop) const
{
	return CUtils::FMatchDynamicScan(
		gpos::dyn_cast<CLogicalDynamicGet>(SharedFromThis()), pop);
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalDynamicGet::PopCopyWithRemappedColumns
//
//	@doc:
//		Return a copy of the operator with remapped columns
//
//---------------------------------------------------------------------------
gpos::Ref<COperator>
CLogicalDynamicGet::PopCopyWithRemappedColumns(
	CMemoryPool *mp, gpos::Ref<UlongToColRefMap> colref_mapping,
	BOOL must_exist)
{
	gpos::Ref<CColRefArray> pdrgpcrOutput = nullptr;
	if (must_exist)
	{
		pdrgpcrOutput =
			CUtils::PdrgpcrRemapAndCreate(mp, m_pdrgpcrOutput, colref_mapping);
	}
	else
	{
		pdrgpcrOutput = CUtils::PdrgpcrRemap(mp, m_pdrgpcrOutput,
											 colref_mapping, must_exist);
	}
	gpos::Ref<CColRef2dArray> pdrgpdrgpcrPart =
		PdrgpdrgpcrCreatePartCols(mp, pdrgpcrOutput, m_ptabdesc->PdrgpulPart());
	CName *pnameAlias = GPOS_NEW(mp) CName(mp, *m_pnameAlias);



	gpos::Ref<CConstraint> partition_cnstrs_disj = nullptr;

	if (m_partition_cnstrs_disj)
	{
		partition_cnstrs_disj =
			m_partition_cnstrs_disj->PcnstrCopyWithRemappedColumns(
				mp, colref_mapping, must_exist);
	}

	if (m_foreign_server_mdids)
	{
	}

	return GPOS_NEW(mp) CLogicalDynamicGet(
		mp, pnameAlias, m_ptabdesc, m_scan_id, pdrgpcrOutput, pdrgpdrgpcrPart,
		m_partition_mdids, partition_cnstrs_disj, m_static_pruned,
		m_foreign_server_mdids);
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalDynamicGet::FInputOrderSensitive
//
//	@doc:
//		Not called for leaf operators
//
//---------------------------------------------------------------------------
BOOL
CLogicalDynamicGet::FInputOrderSensitive() const
{
	GPOS_ASSERT(!"Unexpected function call of FInputOrderSensitive");
	return false;
}

CMaxCard
CLogicalDynamicGet::DeriveMaxCard(CMemoryPool *mp,
								  CExpressionHandle &exprhdl) const
{
	if (nullptr == GetPartitionMdids() || GetPartitionMdids()->Size() == 0)
	{
		return CMaxCard(0);
	}

	return CLogical::DeriveMaxCard(mp, exprhdl);
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalDynamicGet::PxfsCandidates
//
//	@doc:
//		Get candidate xforms
//
//---------------------------------------------------------------------------
gpos::Ref<CXformSet>
CLogicalDynamicGet::PxfsCandidates(CMemoryPool *mp) const
{
	gpos::Ref<CXformSet> xform_set = GPOS_NEW(mp) CXformSet(mp);
	(void) xform_set->ExchangeSet(CXform::ExfDynamicGet2DynamicTableScan);
	(void) xform_set->ExchangeSet(
		CXform::ExfExpandDynamicGetWithForeignPartitions);
	return xform_set;
}



//---------------------------------------------------------------------------
//	@function:
//		CLogicalDynamicGet::OsPrint
//
//	@doc:
//		debug print
//
//---------------------------------------------------------------------------
IOstream &
CLogicalDynamicGet::OsPrint(IOstream &os) const
{
	if (m_fPattern)
	{
		return COperator::OsPrint(os);
	}
	else
	{
		os << SzId() << " ";
		// alias of table as referenced in the query
		m_pnameAlias->OsPrint(os);

		// actual name of table in catalog and columns
		os << " (";
		m_ptabdesc->Name().OsPrint(os);
		os << "), ";
		os << "Columns: [";
		CUtils::OsPrintDrgPcr(os, m_pdrgpcrOutput);
		os << "] Scan Id: " << m_scan_id;
		os << " Parts to scan: " << m_partition_mdids->Size();
	}

	return os;
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalDynamicGet::PstatsDerive
//
//	@doc:
//		Load up statistics from metadata
//
//---------------------------------------------------------------------------
gpos::Ref<IStatistics>
CLogicalDynamicGet::PstatsDerive(CMemoryPool *mp, CExpressionHandle &exprhdl,
								 gpos::Ref<IStatisticsArray>  // not used
) const
{
	gpos::Ref<CExpression> expr = nullptr;
	if (m_partition_cnstrs_disj)
	{
		expr = m_partition_cnstrs_disj->PexprScalar(mp);
	}
	gpos::Ref<IStatistics> stats = PstatsDeriveFilter(mp, exprhdl, expr);

	gpos::Ref<CColRefSet> pcrs = GPOS_NEW(mp) CColRefSet(mp, m_pdrgpcrOutput);
	CUpperBoundNDVs *upper_bound_NDVs =
		GPOS_NEW(mp) CUpperBoundNDVs(pcrs, stats->Rows());
	CStatistics::CastStats(stats)->AddCardUpperBound(upper_bound_NDVs);

	return stats;
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalDynamicGet::PstatsDeriveFilter
//
//	@doc:
//		Derive stats from base table using filters on partition and/or index columns
//
//---------------------------------------------------------------------------
gpos::Ref<IStatistics>
CLogicalDynamicGet::PstatsDeriveFilter(CMemoryPool *mp,
									   CExpressionHandle &exprhdl,
									   gpos::Ref<CExpression> pexprFilter) const
{
	GPOS_ASSERT(COperator::EopLogicalDynamicGet == exprhdl.Pop()->Eopid());
	gpos::Ref<CLogicalDynamicGet> dyn_get =
		CLogicalDynamicGet::PopConvert(exprhdl.Pop());

	gpos::Ref<CColRefSet> pcrsStat = GPOS_NEW(mp) CColRefSet(mp);

	if (nullptr != pexprFilter)
	{
		pcrsStat->Include(pexprFilter->DeriveUsedColumns());
	}

	// requesting statistics on distribution columns to estimate data skew
	if (nullptr != m_pcrsDist)
	{
		pcrsStat->Include(m_pcrsDist);
	}

	gpos::Ref<CStatistics> pstatsFullTable = gpos::dyn_cast<CStatistics>(
		PstatsBaseTable(mp, exprhdl, m_ptabdesc, pcrsStat));



	if (nullptr == pexprFilter || pexprFilter->DeriveHasSubquery())
	{
		return pstatsFullTable;
	}

	gpos::Ref<CExpression> pexprFilterNew;
	if (dyn_get->FStaticPruned())
	{
		// Static pruned Dynamic Table Scan uses the CExpression of
		// selected partitions' combined constraints as the filter to
		// derive stats

		gpos::Ref<CConstraint> cnstrDisj =
			dyn_get->GetPartitionConstraintsDisj();
		if (cnstrDisj)
		{
			pexprFilterNew = cnstrDisj->PexprScalar(mp);
		}
		else
		{
			// Default partition is the only child partition
			GPOS_ASSERT(dyn_get->GetPartitionMdids()->Size() == 1);
			return pstatsFullTable;
		}
	}
	else
	{
		// Dynamic partition elimination uses the partition predicate
		// from the original query as the filter to derive stats
		// FIXME: why don't we also use the disjunctive constraints for DPE?
		pexprFilterNew = pexprFilter;
	}

	gpos::Ref<CStatsPred> pred_stats = CStatsPredUtils::ExtractPredStats(
		mp, pexprFilterNew, nullptr /*outer_refs*/
	);


	gpos::Ref<IStatistics> result_stats =
		CFilterStatsProcessor::MakeStatsFilter(mp, pstatsFullTable, pred_stats,
											   true /* do_cap_NDVs */);



	return result_stats;
}

// returns whether table contains foreign partitions
BOOL
CLogicalDynamicGet::ContainsForeignParts() const
{
	for (ULONG ul = 0; ul < m_foreign_server_mdids->Size(); ++ul)
	{
		gpos::Ref<IMDId> foreign_server_mdid = (*m_foreign_server_mdids)[ul];
		if (foreign_server_mdid->IsValid())
		{
			return true;
		}
	}
	return false;
}
// EOF
