//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CPhysicalSplit.h
//
//	@doc:
//		Physical split operator
//---------------------------------------------------------------------------
#ifndef GPOS_CPhysicalSplit_H
#define GPOS_CPhysicalSplit_H

#include "gpos/base.h"
#include "gpos/common/owner.h"

#include "gpopt/base/CDistributionSpec.h"
#include "gpopt/operators/CPhysical.h"

namespace gpopt
{
//---------------------------------------------------------------------------
//	@class:
//		CPhysicalSplit
//
//	@doc:
//		Physical split operator
//
//---------------------------------------------------------------------------
class CPhysicalSplit : public CPhysical
{
private:
	// deletion columns
	gpos::owner<CColRefArray *> m_pdrgpcrDelete;

	// insertion columns
	gpos::owner<CColRefArray *> m_pdrgpcrInsert;

	// ctid column
	CColRef *m_pcrCtid;

	// segmentid column
	CColRef *m_pcrSegmentId;

	// action column
	CColRef *m_pcrAction;

	// tuple oid column
	CColRef *m_pcrTupleOid;

	// required columns by local members
	gpos::owner<CColRefSet *> m_pcrsRequiredLocal;

public:
	CPhysicalSplit(const CPhysicalSplit &) = delete;

	// ctor
	CPhysicalSplit(CMemoryPool *mp, gpos::owner<CColRefArray *> pdrgpcrDelete,
				   gpos::owner<CColRefArray *> pdrgpcrInsert, CColRef *pcrCtid,
				   CColRef *pcrSegmentId, CColRef *pcrAction,
				   CColRef *pcrTupleOid);

	// dtor
	~CPhysicalSplit() override;

	// ident accessors
	EOperatorId
	Eopid() const override
	{
		return EopPhysicalSplit;
	}

	// return a string for operator name
	const CHAR *
	SzId() const override
	{
		return "CPhysicalSplit";
	}

	// action column
	CColRef *
	PcrAction() const
	{
		return m_pcrAction;
	}

	// ctid column
	CColRef *
	PcrCtid() const
	{
		return m_pcrCtid;
	}

	// segmentid column
	CColRef *
	PcrSegmentId() const
	{
		return m_pcrSegmentId;
	}

	// deletion columns
	gpos::pointer<CColRefArray *>
	PdrgpcrDelete() const
	{
		return m_pdrgpcrDelete;
	}

	// insertion columns
	gpos::pointer<CColRefArray *>
	PdrgpcrInsert() const
	{
		return m_pdrgpcrInsert;
	}

	// tuple oid column
	CColRef *
	PcrTupleOid() const
	{
		return m_pcrTupleOid;
	}

	// match function
	BOOL Matches(gpos::pointer<COperator *> pop) const override;

	// hash function
	ULONG HashValue() const override;

	// sensitivity to order of inputs
	BOOL
	FInputOrderSensitive() const override
	{
		return false;
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

	// derive distribution
	gpos::owner<CDistributionSpec *> PdsDerive(
		CMemoryPool *mp, CExpressionHandle &exprhdl) const override;

	// derive rewindability
	gpos::owner<CRewindabilitySpec *> PrsDerive(
		CMemoryPool *mp, CExpressionHandle &exprhdl) const override;


	//-------------------------------------------------------------------------------------
	// Enforced Properties
	//-------------------------------------------------------------------------------------

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
	static gpos::cast_func<CPhysicalSplit *>
	PopConvert(COperator *pop)
	{
		GPOS_ASSERT(COperator::EopPhysicalSplit == pop->Eopid());

		return dynamic_cast<CPhysicalSplit *>(pop);
	}

	// debug print
	IOstream &OsPrint(IOstream &os) const override;

};	// class CPhysicalSplit
}  // namespace gpopt

#endif	// !GPOS_CPhysicalSplit_H

// EOF
