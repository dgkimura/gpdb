//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2011 EMC Corp.
//
//	@filename:
//		CPhysicalInnerHashJoin.h
//
//	@doc:
//		Inner hash join operator
//---------------------------------------------------------------------------
#ifndef GPOPT_CPhysicalInnerHashJoin_H
#define GPOPT_CPhysicalInnerHashJoin_H

#include "gpos/base.h"
#include "gpos/common/owner.h"

#include "gpopt/operators/CPhysicalHashJoin.h"

namespace gpopt
{
//---------------------------------------------------------------------------
//	@class:
//		CPhysicalInnerHashJoin
//
//	@doc:
//		Inner hash join operator
//
//---------------------------------------------------------------------------
class CPhysicalInnerHashJoin : public CPhysicalHashJoin
{
private:
	// helper for computing a hashed distribution matching the given distribution
	gpos::Ref<CDistributionSpecHashed> PdshashedCreateMatching(
		CMemoryPool *mp, gpos::Ref<CDistributionSpecHashed> pdshashed,
		ULONG ulSourceChild) const;

	// helper for deriving hash join distribution from hashed children
	gpos::Ref<CDistributionSpec> PdsDeriveFromHashedChildren(
		CMemoryPool *mp, gpos::Ref<CDistributionSpec> pdsOuter,
		gpos::Ref<CDistributionSpec> pdsInner) const;

	// helper for deriving hash join distribution from replicated outer child
	gpos::Ref<CDistributionSpec> PdsDeriveFromReplicatedOuter(
		CMemoryPool *mp, gpos::Ref<CDistributionSpec> pdsOuter,
		gpos::Ref<CDistributionSpec> pdsInner) const;

	// helper for deriving hash join distribution from hashed outer child
	gpos::Ref<CDistributionSpec> PdsDeriveFromHashedOuter(
		CMemoryPool *mp, gpos::Ref<CDistributionSpec> pdsOuter,
		gpos::Ref<CDistributionSpec> pdsInner) const;

public:
	CPhysicalInnerHashJoin(const CPhysicalInnerHashJoin &) = delete;

	// ctor
	CPhysicalInnerHashJoin(CMemoryPool *mp,
						   gpos::Ref<CExpressionArray> pdrgpexprOuterKeys,
						   gpos::Ref<CExpressionArray> pdrgpexprInnerKeys,
						   gpos::Ref<IMdIdArray> hash_opfamilies,
						   CXform::EXformId origin_xform = CXform::ExfSentinel);

	// dtor
	~CPhysicalInnerHashJoin() override;

	// ident accessors

	EOperatorId
	Eopid() const override
	{
		return EopPhysicalInnerHashJoin;
	}

	// return a string for operator name
	const CHAR *
	SzId() const override
	{
		return "CPhysicalInnerHashJoin";
	}

	// conversion function
	static gpos::Ref<CPhysicalInnerHashJoin>
	PopConvert(gpos::Ref<COperator> pop)
	{
		GPOS_ASSERT(EopPhysicalInnerHashJoin == pop->Eopid());

		return gpos::dyn_cast<CPhysicalInnerHashJoin>(pop);
	}

	// derive distribution
	gpos::Ref<CDistributionSpec> PdsDerive(
		CMemoryPool *mp, CExpressionHandle &exprhdl) const override;

	gpos::Ref<CPartitionPropagationSpec> PppsRequired(
		CMemoryPool *mp, CExpressionHandle &exprhdl,
		CPartitionPropagationSpec *pppsRequired, ULONG child_index,
		CDrvdPropArray *pdrgpdpCtxt, ULONG ulOptReq) const override;

	gpos::Ref<CPartitionPropagationSpec> PppsDerive(
		CMemoryPool *mp, CExpressionHandle &exprhdl) const override;
};	// class CPhysicalInnerHashJoin

}  // namespace gpopt

#endif	// !GPOPT_CPhysicalInnerHashJoin_H

// EOF
