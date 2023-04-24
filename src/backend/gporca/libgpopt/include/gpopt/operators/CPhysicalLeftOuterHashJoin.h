//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CPhysicalLeftOuterHashJoin.h
//
//	@doc:
//		Left outer hash join operator
//---------------------------------------------------------------------------
#ifndef GPOPT_CPhysicalLeftOuterHashJoin_H
#define GPOPT_CPhysicalLeftOuterHashJoin_H

#include "gpos/base.h"

#include "gpopt/operators/CPhysicalHashJoin.h"

namespace gpopt
{
//---------------------------------------------------------------------------
//	@class:
//		CPhysicalLeftOuterHashJoin
//
//	@doc:
//		Left outer hash join operator
//
//---------------------------------------------------------------------------
class CPhysicalLeftOuterHashJoin : public CPhysicalHashJoin
{
private:
	// helper for deriving hash join distribution from hashed children
	gpos::Ref<CDistributionSpec> PdsDeriveFromHashedChildren(
		CMemoryPool *mp, gpos::Ref<CDistributionSpec> pdsOuter,
		gpos::Ref<CDistributionSpec> pdsInner) const;

public:
	CPhysicalLeftOuterHashJoin(const CPhysicalLeftOuterHashJoin &) = delete;

	// ctor
	CPhysicalLeftOuterHashJoin(
		CMemoryPool *mp, gpos::Ref<CExpressionArray> pdrgpexprOuterKeys,
		gpos::Ref<CExpressionArray> pdrgpexprInnerKeys,
		gpos::Ref<IMdIdArray> hash_opfamilies, BOOL is_null_aware = true,
		CXform::EXformId origin_xform = CXform::ExfSentinel);

	// dtor
	~CPhysicalLeftOuterHashJoin() override;

	// ident accessors
	EOperatorId
	Eopid() const override
	{
		return EopPhysicalLeftOuterHashJoin;
	}

	// return a string for operator name
	const CHAR *
	SzId() const override
	{
		return "CPhysicalLeftOuterHashJoin";
	}

	// derive distribution
	gpos::Ref<CDistributionSpec> PdsDerive(
		CMemoryPool *mp, CExpressionHandle &exprhdl) const override;

	// conversion function
	static gpos::Ref<CPhysicalLeftOuterHashJoin>
	PopConvert(gpos::Ref<COperator> pop)
	{
		GPOS_ASSERT(nullptr != pop);
		GPOS_ASSERT(EopPhysicalLeftOuterHashJoin == pop->Eopid());

		return gpos::dyn_cast<CPhysicalLeftOuterHashJoin>(pop);
	}


};	// class CPhysicalLeftOuterHashJoin

}  // namespace gpopt

#endif	// !GPOPT_CPhysicalLeftOuterHashJoin_H

// EOF
