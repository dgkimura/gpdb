//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CPhysicalNLJoin.h
//
//	@doc:
//		Base nested-loops join operator
//---------------------------------------------------------------------------
#ifndef GPOPT_CPhysicalNLJoin_H
#define GPOPT_CPhysicalNLJoin_H

#include "gpos/base.h"

#include "gpopt/base/CUtils.h"
#include "gpopt/operators/CPhysicalJoin.h"

namespace gpopt
{
//---------------------------------------------------------------------------
//	@class:
//		CPhysicalNLJoin
//
//	@doc:
//		Inner nested-loops join operator
//
//---------------------------------------------------------------------------
class CPhysicalNLJoin : public CPhysicalJoin
{
private:
public:
	CPhysicalNLJoin(const CPhysicalNLJoin &) = delete;

	// ctor
	explicit CPhysicalNLJoin(CMemoryPool *mp);

	// dtor
	~CPhysicalNLJoin() override;

	//-------------------------------------------------------------------------------------
	// Required Plan Properties
	//-------------------------------------------------------------------------------------

	// compute required sort order of the n-th child
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

	// compute required output columns of the n-th child
	gpos::Ref<CColRefSet> PcrsRequired(CMemoryPool *mp,
									   CExpressionHandle &exprhdl,
									   gpos::Ref<CColRefSet> pcrsRequired,
									   ULONG child_index,
									   CDrvdPropArray *,  // pdrgpdpCtxt
									   ULONG			  // ulOptReq
									   ) override;

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

	// return true if operator is a correlated NL Join
	virtual BOOL
	FCorrelated() const
	{
		return false;
	}

	// return required inner columns -- overloaded by correlated join children
	virtual CColRefArray *
	PdrgPcrInner() const
	{
		return nullptr;
	}

	// conversion function
	static gpos::Ref<CPhysicalNLJoin>
	PopConvert(gpos::Ref<COperator> pop)
	{
		GPOS_ASSERT(CUtils::FNLJoin(pop));

		return dynamic_cast<gpos::Ref<CPhysicalNLJoin> >(pop);
	}


};	// class CPhysicalNLJoin

}  // namespace gpopt

#endif	// !GPOPT_CPhysicalNLJoin_H

// EOF
