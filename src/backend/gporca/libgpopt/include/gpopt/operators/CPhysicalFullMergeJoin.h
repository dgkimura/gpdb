//	Greenplum Database
//	Copyright (C) 2019 VMware, Inc. or its affiliates.

#ifndef GPOPT_CPhysicalFullMergeJoin_H
#define GPOPT_CPhysicalFullMergeJoin_H

#include "gpos/base.h"
#include "gpos/common/owner.h"

#include "gpopt/operators/CPhysicalJoin.h"

namespace gpopt
{
class CPhysicalFullMergeJoin : public CPhysicalJoin
{
private:
	gpos::owner<CExpressionArray *> m_outer_merge_clauses;

	gpos::owner<CExpressionArray *> m_inner_merge_clauses;

public:
	CPhysicalFullMergeJoin(const CPhysicalFullMergeJoin &) = delete;

	// ctor
	explicit CPhysicalFullMergeJoin(
		CMemoryPool *mp, CExpressionArray *outer_merge_clauses,
		CExpressionArray *inner_merge_clauses,
		gpos::pointer<IMdIdArray *> hash_opfamilies,
		CXform::EXformId origin_xform = CXform::ExfSentinel);

	// dtor
	~CPhysicalFullMergeJoin() override;

	// ident accessors
	EOperatorId
	Eopid() const override
	{
		return EopPhysicalFullMergeJoin;
	}

	// return a string for operator name
	const CHAR *
	SzId() const override
	{
		return "CPhysicalFullMergeJoin";
	}

	// conversion function
	static gpos::cast_func<CPhysicalFullMergeJoin *>
	PopConvert(COperator *pop)
	{
		GPOS_ASSERT(EopPhysicalFullMergeJoin == pop->Eopid());

		return dynamic_cast<CPhysicalFullMergeJoin *>(pop);
	}

	CDistributionSpec *PdsRequired(
		CMemoryPool *mp, CExpressionHandle &exprhdl,
		gpos::pointer<CDistributionSpec *> pdsRequired, ULONG child_index,
		CDrvdPropArray *pdrgpdpCtxt, ULONG ulOptReq) const override;

	gpos::owner<CEnfdDistribution *> Ped(CMemoryPool *mp,
										 CExpressionHandle &exprhdl,
										 CReqdPropPlan *prppInput,
										 ULONG child_index,
										 CDrvdPropArray *pdrgpdpCtxt,
										 ULONG ulDistrReq) override;

	COrderSpec *PosRequired(CMemoryPool *mp, CExpressionHandle &exprhdl,
							gpos::pointer<COrderSpec *> posInput,
							ULONG child_index, CDrvdPropArray *pdrgpdpCtxt,
							ULONG ulOptReq) const override;

	// compute required rewindability of the n-th child
	gpos::owner<CRewindabilitySpec *> PrsRequired(
		CMemoryPool *mp, CExpressionHandle &exprhdl,
		CRewindabilitySpec *prsRequired, ULONG child_index,
		CDrvdPropArray *pdrgpdpCtxt, ULONG ulOptReq) const override;

	// return order property enforcing type for this operator
	CEnfdProp::EPropEnforcingType EpetOrder(
		CExpressionHandle &exprhdl,
		gpos::pointer<const CEnfdOrder *> peo) const override;

	CEnfdDistribution::EDistributionMatching Edm(
		gpos::pointer<CReqdPropPlan *>,	 // prppInput
		ULONG,							 //child_index,
		CDrvdPropArray *,				 // pdrgpdpCtxt,
		ULONG							 // ulOptReq
		) override;

	gpos::owner<CDistributionSpec *> PdsDerive(
		CMemoryPool *mp, CExpressionHandle &exprhdl) const override;

};	// class CPhysicalFullMergeJoin

}  // namespace gpopt

#endif	// !GPOPT_CPhysicalFullMergeJoin_H

// EOF
