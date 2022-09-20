//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CPhysicalTVF.h
//
//	@doc:
//		Physical Table-valued function
//---------------------------------------------------------------------------
#ifndef GPOPT_CPhysicalTVF_H
#define GPOPT_CPhysicalTVF_H

#include "gpos/base.h"
#include "gpos/common/owner.h"

#include "gpopt/metadata/CTableDescriptor.h"
#include "gpopt/operators/CPhysical.h"

namespace gpopt
{
//---------------------------------------------------------------------------
//	@class:
//		CPhysicalTVF
//
//	@doc:
//		Physical Table-valued function
//
//---------------------------------------------------------------------------
class CPhysicalTVF : public CPhysical
{
private:
	// function mdid
	gpos::Ref<IMDId> m_func_mdid;

	// return type
	gpos::Ref<IMDId> m_return_type_mdid;

	// function name
	CWStringConst *m_pstr;

	// MD cache info
	const IMDFunction *m_pmdfunc;

	// array of column descriptors: the schema of the function result
	gpos::Ref<CColumnDescriptorArray> m_pdrgpcoldesc;

	// output columns
	gpos::Ref<CColRefSet> m_pcrsOutput;

public:
	CPhysicalTVF(const CPhysicalTVF &) = delete;

	// ctor
	CPhysicalTVF(CMemoryPool *mp, gpos::Ref<IMDId> mdid_func,
				 gpos::Ref<IMDId> mdid_return_type, CWStringConst *str,
				 gpos::Ref<CColumnDescriptorArray> pdrgpcoldesc,
				 gpos::Ref<CColRefSet> pcrsOutput);

	// dtor
	~CPhysicalTVF() override;

	// ident accessors
	EOperatorId
	Eopid() const override
	{
		return EopPhysicalTVF;
	}

	// return a string for operator name
	const CHAR *
	SzId() const override
	{
		return "CPhysicalTVF";
	}

	// function mdid
	IMDId *
	FuncMdId() const
	{
		return m_func_mdid.get();
	}

	// return type
	IMDId *
	ReturnTypeMdId() const
	{
		return m_return_type_mdid.get();
	}

	// function name
	const CWStringConst *
	Pstr() const
	{
		return m_pstr;
	}

	// col descr accessor
	CColumnDescriptorArray *
	Pdrgpcoldesc() const
	{
		return m_pdrgpcoldesc.get();
	}

	// accessors
	CColRefSet *
	DeriveOutputColumns() const
	{
		return m_pcrsOutput.get();
	}

	// sensitivity to order of inputs
	BOOL FInputOrderSensitive() const override;

	// match function
	BOOL Matches(COperator *pop) const override;

	//-------------------------------------------------------------------------------------
	// Required Plan Properties
	//-------------------------------------------------------------------------------------

	// compute required output columns of the n-th child
	gpos::Ref<CColRefSet> PcrsRequired(CMemoryPool *mp,
									   CExpressionHandle &exprhdl,
									   CColRefSet *pcrsRequired,
									   ULONG child_index,
									   CDrvdPropArray *pdrgpdpCtxt,
									   ULONG ulOptReq) override;

	// compute required ctes of the n-th child
	gpos::Ref<CCTEReq> PcteRequired(CMemoryPool *mp, CExpressionHandle &exprhdl,
									CCTEReq *pcter, ULONG child_index,
									CDrvdPropArray *pdrgpdpCtxt,
									ULONG ulOptReq) const override;

	// compute required sort order of the n-th child
	gpos::Ref<COrderSpec> PosRequired(CMemoryPool *mp,
									  CExpressionHandle &exprhdl,
									  COrderSpec *posRequired,
									  ULONG child_index,
									  CDrvdPropArray *pdrgpdpCtxt,
									  ULONG ulOptReq) const override;

	// compute required distribution of the n-th child
	gpos::Ref<CDistributionSpec> PdsRequired(CMemoryPool *mp,
											 CExpressionHandle &exprhdl,
											 CDistributionSpec *pdsRequired,
											 ULONG child_index,
											 CDrvdPropArray *pdrgpdpCtxt,
											 ULONG ulOptReq) const override;

	// compute required rewindability of the n-th child
	gpos::Ref<CRewindabilitySpec> PrsRequired(CMemoryPool *mp,
											  CExpressionHandle &exprhdl,
											  CRewindabilitySpec *prsRequired,
											  ULONG child_index,
											  CDrvdPropArray *pdrgpdpCtxt,
											  ULONG ulOptReq) const override;

	// check if required columns are included in output columns
	BOOL FProvidesReqdCols(CExpressionHandle &exprhdl, CColRefSet *pcrsRequired,
						   ULONG ulOptReq) const override;

	//-------------------------------------------------------------------------------------
	// Derived Plan Properties
	//-------------------------------------------------------------------------------------

	// derive sort order
	gpos::Ref<COrderSpec> PosDerive(CMemoryPool *mp,
									CExpressionHandle &exprhdl) const override;

	// derive distribution
	gpos::Ref<CDistributionSpec> PdsDerive(
		CMemoryPool *mp, CExpressionHandle &exprhdl) const override;

	// derive rewindability
	gpos::Ref<CRewindabilitySpec> PrsDerive(
		CMemoryPool *mp, CExpressionHandle &exprhdl) const override;

	// derive partition propagation
	gpos::Ref<CPartitionPropagationSpec> PppsDerive(
		CMemoryPool *mp, CExpressionHandle &exprhdl) const override;

	// derive cte map
	gpos::Ref<CCTEMap> PcmDerive(CMemoryPool *mp,
								 CExpressionHandle &exprhdl) const override;

	//-------------------------------------------------------------------------------------
	// Enforced Properties
	//-------------------------------------------------------------------------------------

	// return order property enforcing type for this operator
	CEnfdProp::EPropEnforcingType EpetOrder(
		CExpressionHandle &exprhdl, const CEnfdOrder *peo) const override;


	// return rewindability property enforcing type for this operator
	CEnfdProp::EPropEnforcingType EpetRewindability(
		CExpressionHandle &exprhdl,
		const CEnfdRewindability *per) const override;

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
	static CPhysicalTVF *
	PopConvert(COperator *pop)
	{
		GPOS_ASSERT(nullptr != pop);
		GPOS_ASSERT(EopPhysicalTVF == pop->Eopid());

		return dynamic_cast<CPhysicalTVF *>(pop);
	}

};	// class CPhysicalTVF

}  // namespace gpopt

#endif	// !GPOPT_CPhysicalTVF_H

// EOF
