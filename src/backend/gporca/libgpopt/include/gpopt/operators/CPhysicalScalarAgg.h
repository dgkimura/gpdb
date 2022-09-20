//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2011 EMC Corp.
//
//	@filename:
//		CPhysicalScalarAgg.h
//
//	@doc:
//		Scalar Aggregate operator
//---------------------------------------------------------------------------
#ifndef GPOS_CPhysicalScalarAgg_H
#define GPOS_CPhysicalScalarAgg_H

#include "gpos/base.h"
#include "gpos/common/owner.h"

#include "gpopt/operators/CPhysicalAgg.h"

namespace gpopt
{
//---------------------------------------------------------------------------
//	@class:
//		CPhysicalScalarAgg
//
//	@doc:
//		scalar aggregate operator
//
//---------------------------------------------------------------------------
class CPhysicalScalarAgg : public CPhysicalAgg
{
private:
public:
	CPhysicalScalarAgg(const CPhysicalScalarAgg &) = delete;

	// ctor
	CPhysicalScalarAgg(
		CMemoryPool *mp, gpos::owner<CColRefArray *> colref_array,
		gpos::pointer<CColRefArray *>
			pdrgpcrMinimal,	 // minimal grouping columns based on FD's
		COperator::EGbAggType egbaggtype, BOOL fGeneratesDuplicates,
		gpos::owner<CColRefArray *> pdrgpcrArgDQA, BOOL fMultiStage,
		BOOL isAggFromSplitDQA, CLogicalGbAgg::EAggStage aggStage,
		BOOL should_enforce_distribution);

	// dtor
	~CPhysicalScalarAgg() override;


	// ident accessors
	EOperatorId
	Eopid() const override
	{
		return EopPhysicalScalarAgg;
	}

	// return a string for operator name
	const CHAR *
	SzId() const override
	{
		return "CPhysicalScalarAgg";
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
	static gpos::cast_func<CPhysicalScalarAgg *>
	PopConvert(COperator *pop)
	{
		GPOS_ASSERT(nullptr != pop);
		GPOS_ASSERT(EopPhysicalScalarAgg == pop->Eopid());

		return dynamic_cast<CPhysicalScalarAgg *>(pop);
	}

};	// class CPhysicalScalarAgg

}  // namespace gpopt


#endif	// !GPOS_CPhysicalScalarAgg_H

// EOF
