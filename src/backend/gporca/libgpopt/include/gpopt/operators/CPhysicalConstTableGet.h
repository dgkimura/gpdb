//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CPhysicalConstTableGet.h
//
//	@doc:
//		Physical const table get
//---------------------------------------------------------------------------
#ifndef GPOPT_CPhysicalConstTableGet_H
#define GPOPT_CPhysicalConstTableGet_H

#include "gpos/base.h"
#include "gpos/common/owner.h"

#include "gpopt/operators/CLogicalConstTableGet.h"
#include "gpopt/operators/CPhysical.h"

namespace gpopt
{
//---------------------------------------------------------------------------
//	@class:
//		CPhysicalConstTableGet
//
//	@doc:
//		Physical const table get operator
//
//---------------------------------------------------------------------------
class CPhysicalConstTableGet : public CPhysical
{
private:
	// array of column descriptors: the schema of the const table
	gpos::owner<CColumnDescriptorArray *> m_pdrgpcoldesc;

	// array of datum arrays
	gpos::owner<IDatum2dArray *> m_pdrgpdrgpdatum;

	// output columns
	gpos::owner<CColRefArray *> m_pdrgpcrOutput;

public:
	CPhysicalConstTableGet(const CPhysicalConstTableGet &) = delete;

	// ctor
	CPhysicalConstTableGet(CMemoryPool *mp,
						   gpos::owner<CColumnDescriptorArray *> pdrgpcoldesc,
						   gpos::owner<IDatum2dArray *> pdrgpdrgpconst,
						   gpos::owner<CColRefArray *> pdrgpcrOutput);

	// dtor
	~CPhysicalConstTableGet() override;

	// ident accessors
	EOperatorId
	Eopid() const override
	{
		return EopPhysicalConstTableGet;
	}

	const CHAR *
	SzId() const override
	{
		return "CPhysicalConstTableGet";
	}

	// col descr accessor
	gpos::pointer<CColumnDescriptorArray *>
	Pdrgpcoldesc() const
	{
		return m_pdrgpcoldesc;
	}

	// const table values accessor
	gpos::pointer<IDatum2dArray *>
	Pdrgpdrgpdatum() const
	{
		return m_pdrgpdrgpdatum;
	}

	// output columns accessors
	gpos::pointer<CColRefArray *>
	PdrgpcrOutput() const
	{
		return m_pdrgpcrOutput;
	}


	// match function
	BOOL Matches(gpos::pointer<COperator *>) const override;

	// sensitivity to order of inputs
	BOOL
	FInputOrderSensitive() const override
	{
		GPOS_ASSERT(!"Unexpected function call of FInputOrderSensitive");
		return false;
	}

	//-------------------------------------------------------------------------------------
	// Required Plan Properties
	//-------------------------------------------------------------------------------------

	// compute required output columns of the n-th child
	gpos::owner<CColRefSet *> PcrsRequired(
		CMemoryPool *mp, CExpressionHandle &exprhdl,
		gpos::pointer<CColRefSet *> pcrsRequired, ULONG child_index,
		gpos::pointer<CDrvdPropArray *> pdrgpdpCtxt, ULONG ulOptReq) override;

	// compute required ctes of the n-th child
	gpos::owner<CCTEReq *> PcteRequired(
		CMemoryPool *mp, CExpressionHandle &exprhdl,
		gpos::pointer<CCTEReq *> pcter, ULONG child_index,
		gpos::pointer<CDrvdPropArray *> pdrgpdpCtxt,
		ULONG ulOptReq) const override;

	// compute required sort order of the n-th child
	gpos::owner<COrderSpec *> PosRequired(
		CMemoryPool *mp, CExpressionHandle &exprhdl,
		gpos::pointer<COrderSpec *> posRequired, ULONG child_index,
		gpos::pointer<CDrvdPropArray *> pdrgpdpCtxt,
		ULONG ulOptReq) const override;

	// compute required distribution of the n-th child
	gpos::owner<CDistributionSpec *> PdsRequired(
		CMemoryPool *mp, CExpressionHandle &exprhdl,
		gpos::pointer<CDistributionSpec *> pdsRequired, ULONG child_index,
		gpos::pointer<CDrvdPropArray *> pdrgpdpCtxt,
		ULONG ulOptReq) const override;

	// compute required rewindability of the n-th child
	gpos::owner<CRewindabilitySpec *> PrsRequired(
		CMemoryPool *mp, CExpressionHandle &exprhdl,
		gpos::pointer<CRewindabilitySpec *> prsRequired, ULONG child_index,
		gpos::pointer<CDrvdPropArray *> pdrgpdpCtxt,
		ULONG ulOptReq) const override;

	// check if required columns are included in output columns
	BOOL FProvidesReqdCols(CExpressionHandle &exprhdl,
						   gpos::pointer<CColRefSet *> pcrsRequired,
						   ULONG ulOptReq) const override;

	//-------------------------------------------------------------------------------------
	// Derived Plan Properties
	//-------------------------------------------------------------------------------------

	// derive sort order
	gpos::owner<COrderSpec *> PosDerive(
		CMemoryPool *mp, CExpressionHandle &exprhdl) const override;

	// derive distribution
	gpos::owner<CDistributionSpec *> PdsDerive(
		CMemoryPool *mp, CExpressionHandle &exprhdl) const override;

	// derive rewindability
	gpos::owner<CRewindabilitySpec *> PrsDerive(
		CMemoryPool *mp, CExpressionHandle &exprhdl) const override;

	// derive cte map
	gpos::owner<CCTEMap *> PcmDerive(CMemoryPool *mp,
									 CExpressionHandle &exprhdl) const override;

	//-------------------------------------------------------------------------------------
	// Enforced Properties
	//-------------------------------------------------------------------------------------

	// return order property enforcing type for this operator
	CEnfdProp::EPropEnforcingType EpetOrder(
		CExpressionHandle &exprhdl,
		gpos::pointer<const CEnfdOrder *> peo) const override;

	// return rewindability property enforcing type for this operator
	CEnfdProp::EPropEnforcingType EpetRewindability(
		CExpressionHandle &,					   // exprhdl
		gpos::pointer<const CEnfdRewindability *>  // per
	) const override;

	// return true if operator passes through stats obtained from children,
	// this is used when computing stats during costing
	BOOL
	FPassThruStats() const override
	{
		return false;
	}

	//-------------------------------------------------------------------------------------
	//-------------------------------------------------------------------------------------
	//-------------------------------------------------------------------------------------

	// conversion function
	static gpos::cast_func<CPhysicalConstTableGet *>
	PopConvert(COperator *pop)
	{
		GPOS_ASSERT(nullptr != pop);
		GPOS_ASSERT(EopPhysicalConstTableGet == pop->Eopid());

		return dynamic_cast<CPhysicalConstTableGet *>(pop);
	}

	// debug print
	IOstream &OsPrint(IOstream &) const override;

};	// class CPhysicalConstTableGet

}  // namespace gpopt

#endif	// !GPOPT_CPhysicalConstTableGet_H

// EOF
