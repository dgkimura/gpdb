//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2023 VMware Inc.
//
//	@filename:
//		CLogicalDynamicForeignGet.cpp
//
//	@doc:
//		Implementation of foreign get
//---------------------------------------------------------------------------

#include "gpopt/operators/CLogicalDynamicForeignGet.h"

#include "gpos/base.h"

#include "gpopt/base/CColRefSet.h"
#include "gpopt/base/CColRefSetIter.h"
#include "gpopt/base/CColRefTable.h"
#include "gpopt/base/COptCtxt.h"
#include "gpopt/base/CUtils.h"
#include "gpopt/metadata/CName.h"
#include "gpopt/metadata/CTableDescriptor.h"
#include "naucrates/statistics/CStatistics.h"

using namespace gpopt;


//---------------------------------------------------------------------------
//	@function:
//		CLogicalDynamicForeignGet::CLogicalDynamicForeignGet
//
//	@doc:
//		Ctor - for pattern
//
//---------------------------------------------------------------------------
CLogicalDynamicForeignGet::CLogicalDynamicForeignGet(CMemoryPool *mp)
	: CLogicalDynamicGetBase(mp)
{
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalDynamicForeignGet::CLogicalDynamicForeignGet
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CLogicalDynamicForeignGet::CLogicalDynamicForeignGet(
	CMemoryPool *mp, const CName *pnameAlias,
	gpos::Ref<CTableDescriptor> ptabdesc, ULONG ulPartIndex,
	gpos::Ref<CColRefArray> pdrgpcrOutput,
	gpos::Ref<CColRef2dArray> pdrgpdrgpcrPart,
	gpos::Ref<IMdIdArray> partition_mdids, OID foreign_server_oid,
	BOOL is_master_only)


	: CLogicalDynamicGetBase(mp, pnameAlias, ptabdesc, ulPartIndex,
							 pdrgpcrOutput, pdrgpdrgpcrPart, partition_mdids),
	  m_foreign_server_oid(foreign_server_oid),
	  m_is_master_only(is_master_only)
{
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalDynamicForeignGet::Matches
//
//	@doc:
//		Match function on operator level
//
//---------------------------------------------------------------------------
BOOL
CLogicalDynamicForeignGet::Matches(gpos::Ref<COperator> pop) const
{
	if (pop->Eopid() != Eopid())
	{
		return false;
	}
	gpos::Ref<CLogicalDynamicForeignGet> popGet =
		CLogicalDynamicForeignGet::PopConvert(pop);

	return Ptabdesc() == popGet->Ptabdesc() &&
		   PdrgpcrOutput()->Equals(popGet->PdrgpcrOutput()) &&
		   GetForeignServerOid() == popGet->GetForeignServerOid();
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalDynamicForeignGet::PopCopyWithRemappedColumns
//
//	@doc:
//		Return a copy of the operator with remapped columns
//
//---------------------------------------------------------------------------
gpos::Ref<COperator>
CLogicalDynamicForeignGet::PopCopyWithRemappedColumns(
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



	return GPOS_NEW(mp) CLogicalDynamicForeignGet(
		mp, pnameAlias, m_ptabdesc, m_scan_id, pdrgpcrOutput, pdrgpdrgpcrPart,
		m_partition_mdids, m_foreign_server_oid, m_is_master_only);
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalDynamicForeignGet::PxfsCandidates
//
//	@doc:
//		Get candidate xforms
//
//---------------------------------------------------------------------------
gpos::Ref<CXformSet>
CLogicalDynamicForeignGet::PxfsCandidates(CMemoryPool *mp) const
{
	gpos::Ref<CXformSet> xform_set = GPOS_NEW(mp) CXformSet(mp);
	(void) xform_set->ExchangeSet(
		CXform::ExfDynamicForeignGet2DynamicForeignScan);

	return xform_set;
}

gpos::Ref<IStatistics>
CLogicalDynamicForeignGet::PstatsDerive(CMemoryPool *mp,
										CExpressionHandle &exprhdl,
										gpos::Ref<IStatisticsArray>	 // not used
) const
{
	// requesting stats on distribution columns to estimate data skew
	gpos::Ref<IStatistics> pstatsTable =
		PstatsBaseTable(mp, exprhdl, m_ptabdesc, m_pcrsDist);

	gpos::Ref<CColRefSet> pcrs = GPOS_NEW(mp) CColRefSet(mp, m_pdrgpcrOutput);
	CUpperBoundNDVs *upper_bound_NDVs =
		GPOS_NEW(mp) CUpperBoundNDVs(pcrs, pstatsTable->Rows());
	CStatistics::CastStats(pstatsTable)->AddCardUpperBound(upper_bound_NDVs);

	return pstatsTable;
}

// EOF
