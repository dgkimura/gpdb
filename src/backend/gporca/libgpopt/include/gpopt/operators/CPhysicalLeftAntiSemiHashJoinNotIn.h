//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2013 EMC Corp.
//
//	@filename:
//		CPhysicalLeftAntiSemiHashJoinNotIn.h
//
//	@doc:
//		Left anti semi hash join operator with NotIn semantics
//---------------------------------------------------------------------------
#ifndef GPOPT_CPhysicalLeftAntiSemiHashJoinNotIn_H
#define GPOPT_CPhysicalLeftAntiSemiHashJoinNotIn_H

#include "gpos/base.h"
#include "gpos/common/owner.h"

#include "gpopt/operators/CPhysicalLeftAntiSemiHashJoin.h"

namespace gpopt
{
//---------------------------------------------------------------------------
//	@class:
//		CPhysicalLeftAntiSemiHashJoinNotIn
//
//	@doc:
//		Left anti semi hash join operator with NotIn semantics
//
//---------------------------------------------------------------------------
class CPhysicalLeftAntiSemiHashJoinNotIn : public CPhysicalLeftAntiSemiHashJoin
{
private:
public:
	CPhysicalLeftAntiSemiHashJoinNotIn(
		const CPhysicalLeftAntiSemiHashJoinNotIn &) = delete;

	// ctor
	CPhysicalLeftAntiSemiHashJoinNotIn(
		CMemoryPool *mp, gpos::Ref<CExpressionArray> pdrgpexprOuterKeys,
		gpos::Ref<CExpressionArray> pdrgpexprInnerKeys,
		gpos::Ref<IMdIdArray> hash_opfamilies,
		CXform::EXformId origin_xform = CXform::ExfSentinel);

	// ident accessors
	EOperatorId
	Eopid() const override
	{
		return EopPhysicalLeftAntiSemiHashJoinNotIn;
	}

	// return a string for operator name
	const CHAR *
	SzId() const override
	{
		return "CPhysicalLeftAntiSemiHashJoinNotIn";
	}

	//-------------------------------------------------------------------------------------
	// Required Plan Properties
	//-------------------------------------------------------------------------------------

	// compute required distribution of the n-th child
	gpos::Ref<CDistributionSpec> PdsRequired(CMemoryPool *mp,
											 CExpressionHandle &exprhdl,
											 CDistributionSpec *pdsRequired,
											 ULONG child_index,
											 CDrvdPropArray *pdrgpdpCtxt,
											 ULONG ulOptReq) const override;

	gpos::Ref<CEnfdDistribution> Ped(CMemoryPool *mp,
									 CExpressionHandle &exprhdl,
									 gpos::Ref<CReqdPropPlan> prppInput,
									 ULONG child_index,
									 CDrvdPropArray *pdrgpdpCtxt,
									 ULONG ulDistrReq) override;

	//-------------------------------------------------------------------------------------
	//-------------------------------------------------------------------------------------
	//-------------------------------------------------------------------------------------

	// conversion function
	static CPhysicalLeftAntiSemiHashJoinNotIn *
	PopConvert(gpos::Ref<COperator> pop)
	{
		GPOS_ASSERT(EopPhysicalLeftAntiSemiHashJoinNotIn == pop->Eopid());

		return dynamic_cast<CPhysicalLeftAntiSemiHashJoinNotIn *>(pop);
	}

};	// class CPhysicalLeftAntiSemiHashJoinNotIn

}  // namespace gpopt

#endif	// !GPOPT_CPhysicalLeftAntiSemiHashJoinNotIn_H

// EOF
