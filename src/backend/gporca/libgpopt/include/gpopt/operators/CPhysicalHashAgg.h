//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2009 Greenplum, Inc.
//
//	@filename:
//		CPhysicalHashAgg.h
//
//	@doc:
//		Hash Aggregate operator
//---------------------------------------------------------------------------
#ifndef GPOS_CPhysicalHashAgg_H
#define GPOS_CPhysicalHashAgg_H

#include "gpos/base.h"
#include "gpos/common/owner.h"

#include "gpopt/operators/CPhysicalAgg.h"

namespace gpopt
{
//---------------------------------------------------------------------------
//	@class:
//		CPhysicalHashAgg
//
//	@doc:
//		Hash-based aggregate operator
//
//---------------------------------------------------------------------------
class CPhysicalHashAgg : public CPhysicalAgg
{
private:
public:
	CPhysicalHashAgg(const CPhysicalHashAgg &) = delete;

	// ctor
	CPhysicalHashAgg(CMemoryPool *mp, gpos::owner<CColRefArray *> colref_array,
					 gpos::pointer<CColRefArray *> pdrgpcrMinimal,
					 COperator::EGbAggType egbaggtype,
					 BOOL fGeneratesDuplicates,
					 gpos::owner<CColRefArray *> pdrgpcrArgDQA,
					 BOOL fMultiStage, BOOL isAggFromSplitDQA,
					 CLogicalGbAgg::EAggStage aggStage,
					 BOOL should_enforce_distribution = true
					 // should_enforce_distribution should be set to false if
					 // 'local' and 'global' splits don't need to have different
					 // distributions. This flag is set to false if the local
					 // aggregate has been created by CXformEagerAgg.
	);

	// dtor
	~CPhysicalHashAgg() override;


	// ident accessors
	EOperatorId
	Eopid() const override
	{
		return EopPhysicalHashAgg;
	}

	// return a string for operator name
	const CHAR *
	SzId() const override
	{
		return "CPhysicalHashAgg";
	}

	//-------------------------------------------------------------------------------------
	// Required Plan Properties
	//-------------------------------------------------------------------------------------

	// compute required sort columns of the n-th child
	gpos::owner<COrderSpec *> PosRequired(
		CMemoryPool *mp, CExpressionHandle &exprhdl,
		gpos::pointer<COrderSpec *> posRequired, ULONG child_index,
		gpos::pointer<CDrvdPropArray *> pdrgpdpCtxt,
		ULONG ulOptReq) const override;

	//-------------------------------------------------------------------------------------
	// Derived Plan Properties
	//-------------------------------------------------------------------------------------

	// derive sort order
	gpos::owner<COrderSpec *> PosDerive(
		CMemoryPool *mp, CExpressionHandle &exprhdl) const override;

	//-------------------------------------------------------------------------------------
	// Enforced Properties
	//-------------------------------------------------------------------------------------

	// return order property enforcing type for this operator
	CEnfdProp::EPropEnforcingType EpetOrder(
		CExpressionHandle &exprhdl,
		gpos::pointer<const CEnfdOrder *> peo) const override;

	//-------------------------------------------------------------------------------------
	//-------------------------------------------------------------------------------------
	//-------------------------------------------------------------------------------------

	// conversion function
	static gpos::cast_func<CPhysicalHashAgg *>
	PopConvert(COperator *pop)
	{
		GPOS_ASSERT(nullptr != pop);
		GPOS_ASSERT(EopPhysicalHashAgg == pop->Eopid() ||
					EopPhysicalHashAggDeduplicate == pop->Eopid());

		return dynamic_cast<CPhysicalHashAgg *>(pop);
	}

};	// class CPhysicalHashAgg

}  // namespace gpopt


#endif	// !GPOS_CPhysicalHashAgg_H

// EOF
