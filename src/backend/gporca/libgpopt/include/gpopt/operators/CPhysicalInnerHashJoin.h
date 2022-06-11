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
	gpos::owner<CDistributionSpecHashed *> PdshashedCreateMatching(
		CMemoryPool *mp, CDistributionSpecHashed *pdshashed,
		ULONG ulSourceChild) const;

	// helper for deriving hash join distribution from hashed children
	gpos::owner<CDistributionSpec *> PdsDeriveFromHashedChildren(
		CMemoryPool *mp, gpos::pointer<CDistributionSpec *> pdsOuter,
		gpos::pointer<CDistributionSpec *> pdsInner) const;

	// helper for deriving hash join distribution from replicated outer child
	gpos::owner<CDistributionSpec *> PdsDeriveFromReplicatedOuter(
		CMemoryPool *mp, gpos::pointer<CDistributionSpec *> pdsOuter,
		gpos::pointer<CDistributionSpec *> pdsInner) const;

	// helper for deriving hash join distribution from hashed outer child
	gpos::owner<CDistributionSpec *> PdsDeriveFromHashedOuter(
		CMemoryPool *mp, CDistributionSpec *pdsOuter,
		gpos::pointer<CDistributionSpec *> pdsInner) const;

public:
	CPhysicalInnerHashJoin(const CPhysicalInnerHashJoin &) = delete;

	// ctor
	CPhysicalInnerHashJoin(CMemoryPool *mp,
						   gpos::owner<CExpressionArray *> pdrgpexprOuterKeys,
						   gpos::owner<CExpressionArray *> pdrgpexprInnerKeys,
						   gpos::owner<IMdIdArray *> hash_opfamilies,
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
	static gpos::cast_func<CPhysicalInnerHashJoin *>
	PopConvert(COperator *pop)
	{
		GPOS_ASSERT(EopPhysicalInnerHashJoin == pop->Eopid());

		return dynamic_cast<CPhysicalInnerHashJoin *>(pop);
	}

	// derive distribution
	gpos::owner<CDistributionSpec *> PdsDerive(
		CMemoryPool *mp, CExpressionHandle &exprhdl) const override;

	gpos::owner<CPartitionPropagationSpec *> PppsRequired(
		CMemoryPool *mp, CExpressionHandle &exprhdl,
		gpos::pointer<CPartitionPropagationSpec *> pppsRequired,
		ULONG child_index, gpos::pointer<CDrvdPropArray *> pdrgpdpCtxt,
		ULONG ulOptReq) const override;

	gpos::owner<CPartitionPropagationSpec *> PppsDerive(
		CMemoryPool *mp, CExpressionHandle &exprhdl) const override;
};	// class CPhysicalInnerHashJoin

}  // namespace gpopt

#endif	// !GPOPT_CPhysicalInnerHashJoin_H

// EOF
