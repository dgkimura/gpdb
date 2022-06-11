//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CPhysicalDML.h
//
//	@doc:
//		Physical DML operator
//---------------------------------------------------------------------------
#ifndef GPOS_CPhysicalDML_H
#define GPOS_CPhysicalDML_H

#include "gpos/base.h"
#include "gpos/common/owner.h"

#include "gpopt/base/CDistributionSpec.h"
#include "gpopt/operators/CLogicalDML.h"
#include "gpopt/operators/CPhysical.h"


namespace gpopt
{
// fwd declaration
class COptimizerConfig;

//---------------------------------------------------------------------------
//	@class:
//		CPhysicalDML
//
//	@doc:
//		Physical DML operator
//
//---------------------------------------------------------------------------
class CPhysicalDML : public CPhysical
{
private:
	// dml operator
	CLogicalDML::EDMLOperator m_edmlop;

	// table descriptor
	gpos::Ref<CTableDescriptor> m_ptabdesc;

	// array of source columns
	gpos::Ref<CColRefArray> m_pdrgpcrSource;

	// set of modified columns from the target table
	gpos::Ref<CBitSet> m_pbsModified;

	// action column
	CColRef *m_pcrAction;

	// table oid column
	CColRef *m_pcrTableOid;

	// ctid column
	CColRef *m_pcrCtid;

	// segmentid column
	CColRef *m_pcrSegmentId;

	// tuple oid column
	CColRef *m_pcrTupleOid;

	// target table distribution spec
	gpos::Ref<CDistributionSpec> m_pds;

	// required order spec
	gpos::Ref<COrderSpec> m_pos;

	// required columns by local members
	gpos::Ref<CColRefSet> m_pcrsRequiredLocal;

	// needs the data to be sorted or not
	BOOL m_input_sort_req;

	// do we need to sort on insert
	BOOL FInsertSortOnRows(COptimizerConfig *optimizer_config);

	// compute required order spec
	gpos::Ref<COrderSpec> PosComputeRequired(CMemoryPool *mp,
											 CTableDescriptor *ptabdesc);

	// compute local required columns
	void ComputeRequiredLocalColumns(CMemoryPool *mp);

public:
	CPhysicalDML(const CPhysicalDML &) = delete;

	// ctor
	CPhysicalDML(CMemoryPool *mp, CLogicalDML::EDMLOperator edmlop,
				 gpos::Ref<CTableDescriptor> ptabdesc,
				 gpos::Ref<CColRefArray> pdrgpcrSource,
				 gpos::Ref<CBitSet> pbsModified, CColRef *pcrAction,
				 CColRef *pcrTableOid, CColRef *pcrCtid, CColRef *pcrSegmentId,
				 CColRef *pcrTupleOid);

	// dtor
	~CPhysicalDML() override;

	// ident accessors
	EOperatorId
	Eopid() const override
	{
		return EopPhysicalDML;
	}

	// return a string for operator name
	const CHAR *
	SzId() const override
	{
		return "CPhysicalDML";
	}

	// dml operator
	CLogicalDML::EDMLOperator
	Edmlop() const
	{
		return m_edmlop;
	}

	// table descriptor
	CTableDescriptor *
	Ptabdesc() const
	{
		return m_ptabdesc.get();
	}

	// action column
	CColRef *
	PcrAction() const
	{
		return m_pcrAction;
	}

	// table oid column
	CColRef *
	PcrTableOid() const
	{
		return m_pcrTableOid;
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

	// tuple oid column
	CColRef *
	PcrTupleOid() const
	{
		return m_pcrTupleOid;
	}

	// source columns
	virtual CColRefArray *
	PdrgpcrSource() const
	{
		return m_pdrgpcrSource.get();
	}

	// match function
	BOOL Matches(COperator *pop) const override;

	// hash function
	ULONG HashValue() const override;

	// sensitivity to order of inputs
	BOOL
	FInputOrderSensitive() const override
	{
		return false;
	}

	// needs the data to be sorted or not
	virtual BOOL
	IsInputSortReq() const
	{
		return m_input_sort_req;
	}

	//-------------------------------------------------------------------------------------
	// Required Plan Properties
	//-------------------------------------------------------------------------------------

	// compute required sort columns of the n-th child
	gpos::Ref<COrderSpec> PosRequired(CMemoryPool *mp,
									  CExpressionHandle &exprhdl,
									  COrderSpec *posRequired,
									  ULONG child_index,
									  CDrvdPropArray *pdrgpdpCtxt,
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
		CExpressionHandle &exprhdl, const CEnfdOrder *peo) const override;

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


	// distribution matching type
	CEnfdDistribution::EDistributionMatching
	Edm(CReqdPropPlan *,   // prppInput
		ULONG,			   // child_index
		CDrvdPropArray *,  //pdrgpdpCtxt
		ULONG			   // ulOptReq
		) override
	{
		if (CDistributionSpec::EdtSingleton == m_pds->Edt())
		{
			// if target table is master only, request simple satisfiability, as it will not introduce duplicates
			return CEnfdDistribution::EdmSatisfy;
		}

		// avoid duplicates by requesting exact matching of non-singleton distributions
		return CEnfdDistribution::EdmExact;
	}

	//-------------------------------------------------------------------------------------
	// Derived Plan Properties
	//-------------------------------------------------------------------------------------

	// derive distribution
	gpos::Ref<CDistributionSpec> PdsDerive(
		CMemoryPool *mp, CExpressionHandle &exprhdl) const override;

	// derive rewindability
	gpos::Ref<CRewindabilitySpec> PrsDerive(
		CMemoryPool *mp, CExpressionHandle &exprhdl) const override;

	//-------------------------------------------------------------------------------------
	// Enforced Properties
	//-------------------------------------------------------------------------------------


	// return rewindability property enforcing type for this operator
	CEnfdProp::EPropEnforcingType EpetRewindability(
		CExpressionHandle &,		// exprhdl
		const CEnfdRewindability *	// per
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
	static CPhysicalDML *
	PopConvert(COperator *pop)
	{
		GPOS_ASSERT(COperator::EopPhysicalDML == pop->Eopid());

		return dynamic_cast<CPhysicalDML *>(pop);
	}

	// debug print
	IOstream &OsPrint(IOstream &os) const override;

};	// class CPhysicalDML

}  // namespace gpopt


#endif	// !GPOS_CPhysicalDML_H

// EOF
