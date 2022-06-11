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
	gpos::owner<IMDId *> m_func_mdid;

	// return type
	gpos::owner<IMDId *> m_return_type_mdid;

	// function name
	CWStringConst *m_pstr;

	// array of column descriptors: the schema of the function result
	gpos::owner<CColumnDescriptorArray *> m_pdrgpcoldesc;

	// output columns
	gpos::owner<CColRefArray *> m_pdrgpcrOutput;

	// function stability
	IMDFunction::EFuncStbl m_efs;

	// does this function return a set of rows
	BOOL m_returns_set;

public:
	CLogicalTVF(const CLogicalTVF &) = delete;

	// ctors
	explicit CLogicalTVF(CMemoryPool *mp);

	CLogicalTVF(CMemoryPool *mp, gpos::owner<IMDId *> mdid_func,
				gpos::owner<IMDId *> mdid_return_type, CWStringConst *str,
				gpos::owner<CColumnDescriptorArray *> pdrgpcoldesc);

	CLogicalTVF(CMemoryPool *mp, gpos::owner<IMDId *> mdid_func,
				gpos::owner<IMDId *> mdid_return_type, CWStringConst *str,
				gpos::owner<CColumnDescriptorArray *> pdrgpcoldesc,
				gpos::owner<CColRefArray *> pdrgpcrOutput);

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
	gpos::pointer<IMDId *>
	FuncMdId() const
	{
		return m_func_mdid;
	}

	// return type
	gpos::pointer<IMDId *>
	ReturnTypeMdId() const
	{
		return m_return_type_mdid;
	}

	// function name
	const CWStringConst *
	Pstr() const
	{
		return m_pstr;
	}

	// col descr accessor
	gpos::pointer<CColumnDescriptorArray *>
	Pdrgpcoldesc() const
	{
		return m_pdrgpcoldesc;
	}

	// accessors
	gpos::pointer<CColRefArray *>
	PdrgpcrOutput() const
	{
		return m_pdrgpcrOutput;
	}

	// sensitivity to order of inputs
	BOOL FInputOrderSensitive() const override;

	// operator specific hash function
	ULONG HashValue() const override;

	// match function
	BOOL Matches(gpos::pointer<COperator *> pop) const override;

	// return a copy of the operator with remapped columns
	gpos::owner<COperator *> PopCopyWithRemappedColumns(
		CMemoryPool *mp, gpos::pointer<UlongToColRefMap *> colref_mapping,
		BOOL must_exist) override;

	//-------------------------------------------------------------------------------------
	// Derived Relational Properties
	//-------------------------------------------------------------------------------------

	// derive output columns
	gpos::owner<CColRefSet *> DeriveOutputColumns(CMemoryPool *,
												  CExpressionHandle &) override;

	// derive partition consumer info
	gpos::owner<CPartInfo *>
	DerivePartitionInfo(CMemoryPool *mp,
						CExpressionHandle &	 //exprhdl
	) const override
	{
		return GPOS_NEW(mp) CPartInfo(mp);
	}

	// derive constraint property
	gpos::owner<CPropConstraint *>
	DerivePropertyConstraint(CMemoryPool *mp,
							 CExpressionHandle &  //exprhdl
	) const override
	{
		return GPOS_NEW(mp) CPropConstraint(
			mp, GPOS_NEW(mp) CColRefSetArray(mp), nullptr /*pcnstr*/);
	}

	// derive function properties
	gpos::owner<CFunctionProp *> DeriveFunctionProperties(
		CMemoryPool *mp, CExpressionHandle &exprhdl) const override;

	// derive max card
	CMaxCard DeriveMaxCard(CMemoryPool *mp,
						   CExpressionHandle &exprhdl) const override;

	//-------------------------------------------------------------------------------------
	// Required Relational Properties
	//-------------------------------------------------------------------------------------

	// compute required stat columns of the n-th child
	gpos::owner<CColRefSet *>
	PcrsStat(CMemoryPool *,				   // mp
			 CExpressionHandle &,		   // exprhdl
			 gpos::pointer<CColRefSet *>,  // pcrsInput
			 ULONG						   // child_index
	) const override
	{
		return nullptr;
	}

	//-------------------------------------------------------------------------------------
	// Transformations
	//-------------------------------------------------------------------------------------

	// candidate set of xforms
	gpos::owner<CXformSet *> PxfsCandidates(CMemoryPool *mp) const override;

	// stat promise
	EStatPromise
	Esp(CExpressionHandle &) const override
	{
		return CLogical::EspLow;
	}

	// derive statistics
	gpos::owner<IStatistics *> PstatsDerive(
		CMemoryPool *mp, CExpressionHandle &exprhdl,
		gpos::pointer<IStatisticsArray *> stats_ctxt) const override;

	//-------------------------------------------------------------------------------------
	//-------------------------------------------------------------------------------------
	//-------------------------------------------------------------------------------------

	// conversion function
	static gpos::cast_func<CLogicalTVF *>
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
