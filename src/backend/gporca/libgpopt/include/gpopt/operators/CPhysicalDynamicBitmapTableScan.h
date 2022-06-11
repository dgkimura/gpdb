//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2014 VMware, Inc. or its affiliates.
//
//	@filename:
//		CPhysicalDynamicBitmapTableScan.h
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

#ifndef GPOPT_CPhysicalDynamicBitmapTableScan_H
#define GPOPT_CPhysicalDynamicBitmapTableScan_H

#include "gpos/base.h"
#include "gpos/common/owner.h"

#include "gpopt/operators/CPhysicalDynamicScan.h"

namespace gpopt
{
// fwd declarations
class CTableDescriptor;
class CName;
class CPartConstraint;

//---------------------------------------------------------------------------
//	@class:
//		CPhysicalDynamicBitmapTableScan
//
//	@doc:
//		Dynamic bitmap table scan physical operator
//
//---------------------------------------------------------------------------
class CPhysicalDynamicBitmapTableScan : public CPhysicalDynamicScan
{
private:
public:
	CPhysicalDynamicBitmapTableScan(const CPhysicalDynamicBitmapTableScan &) =
		delete;

	// ctor
	CPhysicalDynamicBitmapTableScan(
		CMemoryPool *mp, gpos::owner<CTableDescriptor *> ptabdesc,
		ULONG ulOriginOpId, const CName *pnameAlias, ULONG scan_id,
		gpos::owner<CColRefArray *> pdrgpcrOutput,
		gpos::owner<CColRef2dArray *> pdrgpdrgpcrParts,
		gpos::owner<IMdIdArray *> partition_mdids,
		gpos::owner<ColRefToUlongMapArray *> root_col_mapping_per_part);

	// ident accessors
	EOperatorId
	Eopid() const override
	{
		return EopPhysicalDynamicBitmapTableScan;
	}

	// return a string for operator name
	const CHAR *
	SzId() const override
	{
		return "CPhysicalDynamicBitmapTableScan";
	}

	// match function
	BOOL Matches(gpos::pointer<COperator *>) const override;

	// statistics derivation during costing
	gpos::owner<IStatistics *> PstatsDerive(
		CMemoryPool *mp, CExpressionHandle &exprhdl,
		gpos::pointer<CReqdPropPlan *> prpplan,
		gpos::pointer<IStatisticsArray *> stats_ctxt) const override;

	// conversion function
	static gpos::cast_func<CPhysicalDynamicBitmapTableScan *>
	PopConvert(COperator *pop)
	{
		GPOS_ASSERT(nullptr != pop);
		GPOS_ASSERT(EopPhysicalDynamicBitmapTableScan == pop->Eopid());

		return dynamic_cast<CPhysicalDynamicBitmapTableScan *>(pop);
	}
};
}  // namespace gpopt

#endif	// !GPOPT_CPhysicalDynamicBitmapTableScan_H

// EOF
