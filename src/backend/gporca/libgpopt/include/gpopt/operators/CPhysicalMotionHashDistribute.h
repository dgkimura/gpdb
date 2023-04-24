//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2011 EMC Corp.
//
//	@filename:
//		CPhysicalMotionHashDistribute.h
//
//	@doc:
//		Physical Hash distribute motion operator
//---------------------------------------------------------------------------
#ifndef GPOPT_CPhysicalMotionHashDistribute_H
#define GPOPT_CPhysicalMotionHashDistribute_H

#include "gpos/base.h"

#include "gpopt/base/CDistributionSpecHashed.h"
#include "gpopt/base/COrderSpec.h"
#include "gpopt/operators/CPhysicalMotion.h"

namespace gpopt
{
//---------------------------------------------------------------------------
//	@class:
//		CPhysicalMotionHashDistribute
//
//	@doc:
//		Hash distribute motion operator
//
//---------------------------------------------------------------------------
class CPhysicalMotionHashDistribute : public CPhysicalMotion
{
private:
	// hash distribution spec
	gpos::Ref<CDistributionSpecHashed> m_pdsHashed;

	// required columns in distribution spec
	gpos::Ref<CColRefSet> m_pcrsRequiredLocal;

public:
	CPhysicalMotionHashDistribute(const CPhysicalMotionHashDistribute &) =
		delete;

	// ctor
	CPhysicalMotionHashDistribute(CMemoryPool *mp,
								  gpos::Ref<CDistributionSpecHashed> pdsHashed);

	// dtor
	~CPhysicalMotionHashDistribute() override;

	// ident accessors
	EOperatorId
	Eopid() const override
	{
		return EopPhysicalMotionHashDistribute;
	}

	const CHAR *
	SzId() const override
	{
		return "CPhysicalMotionHashDistribute";
	}

	// output distribution accessor
	gpos::Ref<CDistributionSpec>
	Pds() const override
	{
		return m_pdsHashed;
	}

	// is motion eliminating duplicates
	BOOL
	IsDuplicateSensitive() const
	{
		return m_pdsHashed->IsDuplicateSensitive();
	}

	// match function
	BOOL Matches(gpos::Ref<COperator>) const override;

	//-------------------------------------------------------------------------------------
	// Required Plan Properties
	//-------------------------------------------------------------------------------------

	// compute required output columns of the n-th child
	gpos::Ref<CColRefSet> PcrsRequired(CMemoryPool *mp,
									   CExpressionHandle &exprhdl,
									   gpos::Ref<CColRefSet> pcrsInput,
									   ULONG child_index,
									   CDrvdPropArray *pdrgpdpCtxt,
									   ULONG ulOptReq) override;

	// compute required sort order of the n-th child
	gpos::Ref<COrderSpec> PosRequired(CMemoryPool *mp,
									  CExpressionHandle &exprhdl,
									  gpos::Ref<COrderSpec> posInput,
									  ULONG child_index,
									  CDrvdPropArray *pdrgpdpCtxt,
									  ULONG ulOptReq) const override;

	// check if required columns are included in output columns
	BOOL FProvidesReqdCols(CExpressionHandle &exprhdl,
						   gpos::Ref<CColRefSet> pcrsRequired,
						   ULONG ulOptReq) const override;

	//-------------------------------------------------------------------------------------
	// Derived Plan Properties
	//-------------------------------------------------------------------------------------

	// derive sort order
	gpos::Ref<COrderSpec> PosDerive(CMemoryPool *mp,
									CExpressionHandle &exprhdl) const override;

	//-------------------------------------------------------------------------------------
	// Enforced Properties
	//-------------------------------------------------------------------------------------

	// return order property enforcing type for this operator
	CEnfdProp::EPropEnforcingType EpetOrder(
		CExpressionHandle &exprhdl,
		const gpos::Ref<CEnfdOrder> peo) const override;

	//-------------------------------------------------------------------------------------
	//-------------------------------------------------------------------------------------
	//-------------------------------------------------------------------------------------

	// print
	IOstream &OsPrint(IOstream &) const override;

	// conversion function
	static gpos::Ref<CPhysicalMotionHashDistribute> PopConvert(
		gpos::Ref<COperator> pop);

	gpos::Ref<CDistributionSpec> PdsRequired(
		CMemoryPool *mp, CExpressionHandle &exprhdl,
		gpos::Ref<CDistributionSpec> pdsRequired, ULONG child_index,
		CDrvdPropArray *pdrgpdpCtxt, ULONG ulOptReq) const override;

};	// class CPhysicalMotionHashDistribute

}  // namespace gpopt

#endif	// !GPOPT_CPhysicalMotionHashDistribute_H

// EOF
