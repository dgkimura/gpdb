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
#include "gpos/common/owner.h"

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
	gpos::owner<COrderSpec *> PosRequired(
		CMemoryPool *mp, CExpressionHandle &exprhdl,
		gpos::pointer<COrderSpec *> posInput, ULONG child_index,
		gpos::pointer<CDrvdPropArray *> pdrgpdpCtxt,
		ULONG ulOptReq) const override;

	// compute required rewindability of the n-th child
	gpos::owner<CRewindabilitySpec *> PrsRequired(
		CMemoryPool *mp, CExpressionHandle &exprhdl,
		gpos::pointer<CRewindabilitySpec *> prsRequired, ULONG child_index,
		gpos::pointer<CDrvdPropArray *> pdrgpdpCtxt,
		ULONG ulOptReq) const override;

	// compute required output columns of the n-th child
	gpos::owner<CColRefSet *> PcrsRequired(
		CMemoryPool *mp, CExpressionHandle &exprhdl,
		gpos::pointer<CColRefSet *> pcrsRequired, ULONG child_index,
		gpos::pointer<CDrvdPropArray *>,  // pdrgpdpCtxt
		ULONG							  // ulOptReq
		) override;

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

	// return true if operator is a correlated NL Join
	virtual BOOL
	FCorrelated() const
	{
		return false;
	}

	// return required inner columns -- overloaded by correlated join children
	virtual gpos::pointer<CColRefArray *>
	PdrgPcrInner() const
	{
		return nullptr;
	}

	// conversion function
	static gpos::cast_func<CPhysicalNLJoin *>
	PopConvert(COperator *pop)
	{
		GPOS_ASSERT(CUtils::FNLJoin(pop));

		return dynamic_cast<CPhysicalNLJoin *>(pop);
	}


};	// class CPhysicalNLJoin

}  // namespace gpopt

#endif	// !GPOPT_CPhysicalNLJoin_H

// EOF
