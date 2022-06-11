//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CLogicalTVF.h
//
//	@doc:
//		Table-valued function
//---------------------------------------------------------------------------
#ifndef GPOPT_CLogicalTVF_H
#define GPOPT_CLogicalTVF_H

#include "gpos/base.h"
#include "gpos/common/owner.h"

#include "gpopt/base/CColRefSet.h"
#include "gpopt/operators/CLogical.h"

namespace gpopt
{
//---------------------------------------------------------------------------
//	@class:
//		CLogicalTVF
//
//	@doc:
//		Table-valued function
//
//---------------------------------------------------------------------------
class CLogicalTVF : public CLogical
{
private:
	// function mdid
	gpos::Ref<IMDId> m_func_mdid;

	// return type
	gpos::Ref<IMDId> m_return_type_mdid;

	// function name
	CWStringConst *m_pstr;

	// array of column descriptors: the schema of the function result
	gpos::Ref<CColumnDescriptorArray> m_pdrgpcoldesc;

	// output columns
	gpos::Ref<CColRefArray> m_pdrgpcrOutput;

	// function stability
	IMDFunction::EFuncStbl m_efs;

	// does this function return a set of rows
	BOOL m_returns_set;

public:
	CLogicalTVF(const CLogicalTVF &) = delete;

	// ctors
	explicit CLogicalTVF(CMemoryPool *mp);

	CLogicalTVF(CMemoryPool *mp, gpos::Ref<IMDId> mdid_func,
				gpos::Ref<IMDId> mdid_return_type, CWStringConst *str,
				gpos::Ref<CColumnDescriptorArray> pdrgpcoldesc);

	CLogicalTVF(CMemoryPool *mp, gpos::Ref<IMDId> mdid_func,
				gpos::Ref<IMDId> mdid_return_type, CWStringConst *str,
				gpos::Ref<CColumnDescriptorArray> pdrgpcoldesc,
				gpos::Ref<CColRefArray> pdrgpcrOutput);

	// dtor
	~CLogicalTVF() override;

	// ident accessors
	EOperatorId
	Eopid() const override
	{
		return EopLogicalTVF;
	}

	// return a string for operator name
	const CHAR *
	SzId() const override
	{
		return "CLogicalTVF";
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
	CColRefArray *
	PdrgpcrOutput() const
	{
		return m_pdrgpcrOutput.get();
	}

	// sensitivity to order of inputs
	BOOL FInputOrderSensitive() const override;

	// operator specific hash function
	ULONG HashValue() const override;

	// match function
	BOOL Matches(COperator *pop) const override;

	// return a copy of the operator with remapped columns
	gpos::Ref<COperator> PopCopyWithRemappedColumns(
		CMemoryPool *mp, UlongToColRefMap *colref_mapping,
		BOOL must_exist) override;

	//-------------------------------------------------------------------------------------
	// Derived Relational Properties
	//-------------------------------------------------------------------------------------

	// derive output columns
	gpos::Ref<CColRefSet> DeriveOutputColumns(CMemoryPool *,
											  CExpressionHandle &) override;

	// derive partition consumer info
	gpos::Ref<CPartInfo>
	DerivePartitionInfo(CMemoryPool *mp,
						CExpressionHandle &	 //exprhdl
	) const override
	{
		return GPOS_NEW(mp) CPartInfo(mp);
	}

	// derive constraint property
	gpos::Ref<CPropConstraint>
	DerivePropertyConstraint(CMemoryPool *mp,
							 CExpressionHandle &  //exprhdl
	) const override
	{
		return GPOS_NEW(mp) CPropConstraint(
			mp, GPOS_NEW(mp) CColRefSetArray(mp), nullptr /*pcnstr*/);
	}

	// derive function properties
	gpos::Ref<CFunctionProp> DeriveFunctionProperties(
		CMemoryPool *mp, CExpressionHandle &exprhdl) const override;

	// derive max card
	CMaxCard DeriveMaxCard(CMemoryPool *mp,
						   CExpressionHandle &exprhdl) const override;

	//-------------------------------------------------------------------------------------
	// Required Relational Properties
	//-------------------------------------------------------------------------------------

	// compute required stat columns of the n-th child
	gpos::Ref<CColRefSet>
	PcrsStat(CMemoryPool *,		   // mp
			 CExpressionHandle &,  // exprhdl
			 CColRefSet *,		   // pcrsInput
			 ULONG				   // child_index
	) const override
	{
		return nullptr;
	}

	//-------------------------------------------------------------------------------------
	// Transformations
	//-------------------------------------------------------------------------------------

	// candidate set of xforms
	gpos::Ref<CXformSet> PxfsCandidates(CMemoryPool *mp) const override;

	// stat promise
	EStatPromise
	Esp(CExpressionHandle &) const override
	{
		return CLogical::EspLow;
	}

	// derive statistics
	gpos::Ref<IStatistics> PstatsDerive(
		CMemoryPool *mp, CExpressionHandle &exprhdl,
		IStatisticsArray *stats_ctxt) const override;

	//-------------------------------------------------------------------------------------
	//-------------------------------------------------------------------------------------
	//-------------------------------------------------------------------------------------

	// conversion function
	static CLogicalTVF *
	PopConvert(COperator *pop)
	{
		GPOS_ASSERT(nullptr != pop);
		GPOS_ASSERT(EopLogicalTVF == pop->Eopid());

		return dynamic_cast<CLogicalTVF *>(pop);
	}


	// debug print
	IOstream &OsPrint(IOstream &) const override;

};	// class CLogicalTVF

}  // namespace gpopt


#endif	// !GPOPT_CLogicalTVF_H

// EOF
