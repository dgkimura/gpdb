//	Greenplum Database
//	Copyright (C) 2019 VMware, Inc. or its affiliates.

#ifndef GPOPT_CPhysicalFullMergeJoin_H
#define GPOPT_CPhysicalFullMergeJoin_H

#include "gpos/base.h"

#include "gpopt/operators/CPhysicalJoin.h"

namespace gpopt
{
class CPhysicalFullMergeJoin : public CPhysicalJoin
{
private:
	CExpressionArray *m_outer_merge_clauses;

	CExpressionArray *m_inner_merge_clauses;

public:
	CPhysicalFullMergeJoin(const CPhysicalFullMergeJoin &) = delete;

	// ctor
	explicit CPhysicalFullMergeJoin(
		CMemoryPool *mp, CExpressionArray *outer_merge_clauses,
		CExpressionArray *inner_merge_clauses, IMdIdArray *hash_opfamilies,
		BOOL is_null_aware = true,
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
	static gpos::Ref<CPhysicalFullMergeJoin>
	PopConvert(gpos::Ref<COperator> pop)
	{
		GPOS_ASSERT(EopPhysicalFullMergeJoin == pop->Eopid());

		return dynamic_cast<gpos::Ref<CPhysicalFullMergeJoin> >(pop);
	}

	gpos::Ref<CDistributionSpec> PdsRequired(
		CMemoryPool *mp, CExpressionHandle &exprhdl,
		gpos::Ref<CDistributionSpec> pdsRequired, ULONG child_index,
		CDrvdPropArray *pdrgpdpCtxt, ULONG ulOptReq) const override;

	gpos::Ref<CEnfdDistribution> Ped(CMemoryPool *mp,
									 CExpressionHandle &exprhdl,
									 gpos::Ref<CReqdPropPlan> prppInput,
									 ULONG child_index,
									 CDrvdPropArray *pdrgpdpCtxt,
									 ULONG ulDistrReq) override;

	gpos::Ref<COrderSpec> PosRequired(CMemoryPool *mp,
									  CExpressionHandle &exprhdl,
									  gpos::Ref<COrderSpec> posInput,
									  ULONG child_index,
									  CDrvdPropArray *pdrgpdpCtxt,
									  ULONG ulOptReq) const override;

	// compute required rewindability of the n-th child
	gpos::Ref<CRewindabilitySpec> PrsRequired(
		CMemoryPool *mp, CExpressionHandle &exprhdl,
		gpos::Ref<CRewindabilitySpec> prsRequired, ULONG child_index,
		CDrvdPropArray *pdrgpdpCtxt, ULONG ulOptReq) const override;

	// return order property enforcing type for this operator
	CEnfdProp::EPropEnforcingType EpetOrder(
		CExpressionHandle &exprhdl,
		const gpos::Ref<CEnfdOrder> peo) const override;

	CEnfdDistribution::EDistributionMatching Edm(
		gpos::Ref<CReqdPropPlan>,  // prppInput
		ULONG,					   //child_index,
		CDrvdPropArray *,		   // pdrgpdpCtxt,
		ULONG					   // ulOptReq
		) override;

	gpos::Ref<CDistributionSpec> PdsDerive(
		CMemoryPool *mp, CExpressionHandle &exprhdl) const override;

};	// class CPhysicalFullMergeJoin

}  // namespace gpopt

#endif	// !GPOPT_CPhysicalFullMergeJoin_H

// EOF
