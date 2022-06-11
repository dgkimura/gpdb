//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CPhysicalDynamicTableScan.h
//
//	@doc:
//		Dynamic Table scan operator
//---------------------------------------------------------------------------
#ifndef GPOPT_CPhysicalDynamicTableScan_H
#define GPOPT_CPhysicalDynamicTableScan_H

#include "gpos/base.h"
#include "gpos/common/owner.h"

#include "gpopt/operators/CPhysicalDynamicScan.h"

namespace gpopt
{
//---------------------------------------------------------------------------
//	@class:
//		CPhysicalDynamicTableScan
//
//	@doc:
//		Dynamic Table scan operator
//
//---------------------------------------------------------------------------
class CPhysicalDynamicTableScan : public CPhysicalDynamicScan
{
private:
public:
	CPhysicalDynamicTableScan(const CPhysicalDynamicTableScan &) = delete;

	// ctors
	CPhysicalDynamicTableScan(
		CMemoryPool *mp, const CName *pnameAlias,
		gpos::owner<CTableDescriptor *> ptabdesc, ULONG ulOriginOpId,
		ULONG scan_id, CColRefArray *pdrgpcrOutput,
		gpos::owner<CColRef2dArray *> pdrgpdrgpcrParts,
		gpos::owner<IMdIdArray *> partition_mdids,
		gpos::owner<ColRefToUlongMapArray *> root_col_mapping_per_part);

	// ident accessors
	EOperatorId
	Eopid() const override
	{
		return EopPhysicalDynamicTableScan;
	}

	// return a string for operator name
	const CHAR *
	SzId() const override
	{
		return "CPhysicalDynamicTableScan";
	}

	// match function
	BOOL Matches(gpos::pointer<COperator *>) const override;

	// statistics derivation during costing
	gpos::owner<IStatistics *> PstatsDerive(
		CMemoryPool *mp, CExpressionHandle &exprhdl,
		gpos::pointer<CReqdPropPlan *> prpplan,
		gpos::pointer<IStatisticsArray *> stats_ctxt) const override;

	// conversion function
	static gpos::cast_func<CPhysicalDynamicTableScan *>
	PopConvert(COperator *pop)
	{
		GPOS_ASSERT(nullptr != pop);
		GPOS_ASSERT(EopPhysicalDynamicTableScan == pop->Eopid());

		return dynamic_cast<CPhysicalDynamicTableScan *>(pop);
	}

	gpos::owner<CPartitionPropagationSpec *> PppsDerive(
		CMemoryPool *mp, CExpressionHandle &exprhdl) const override;

};	// class CPhysicalDynamicTableScan

}  // namespace gpopt

#endif	// !GPOPT_CPhysicalDynamicTableScan_H

// EOF
