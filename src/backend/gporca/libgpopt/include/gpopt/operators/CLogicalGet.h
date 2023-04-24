//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2009 Greenplum, Inc.
//
//	@filename:
//		CLogicalGet.h
//
//	@doc:
//		Basic table accessor
//---------------------------------------------------------------------------
#ifndef GPOPT_CLogicalGet_H
#define GPOPT_CLogicalGet_H

#include "gpos/base.h"

#include "gpopt/base/CColRefSet.h"
#include "gpopt/operators/CLogical.h"

namespace gpopt
{
// fwd declarations
class CTableDescriptor;
class CName;

//---------------------------------------------------------------------------
//	@class:
//		CLogicalGet
//
//	@doc:
//		Basic table accessor
//
//---------------------------------------------------------------------------
class CLogicalGet : public CLogical
{
private:
	// alias for table
	const CName *m_pnameAlias;

	// table descriptor
	gpos::Ref<CTableDescriptor> m_ptabdesc;

	// output columns
	gpos::Ref<CColRefArray> m_pdrgpcrOutput;

	// partition keys
	gpos::Ref<CColRef2dArray> m_pdrgpdrgpcrPart;

	// distribution columns (empty for coordinator only tables)
	gpos::Ref<CColRefSet> m_pcrsDist;

	void CreatePartCols(CMemoryPool *mp,
						const gpos::Ref<ULongPtrArray> pdrgpulPart);

	// private copy ctor
	CLogicalGet(const CLogicalGet &);

public:
	// ctors
	explicit CLogicalGet(CMemoryPool *mp);

	CLogicalGet(CMemoryPool *mp, const CName *pnameAlias,
				gpos::Ref<CTableDescriptor> ptabdesc);

	CLogicalGet(CMemoryPool *mp, const CName *pnameAlias,
				gpos::Ref<CTableDescriptor> ptabdesc,
				gpos::Ref<CColRefArray> pdrgpcrOutput);

	// dtor
	~CLogicalGet() override;

	// ident accessors
	EOperatorId
	Eopid() const override
	{
		return EopLogicalGet;
	}

	// distribution columns
	virtual const gpos::Ref<CColRefSet>
	PcrsDist() const
	{
		return m_pcrsDist;
	}

	// return a string for operator name
	const CHAR *
	SzId() const override
	{
		return "CLogicalGet";
	}

	// accessors
	gpos::Ref<CColRefArray>
	PdrgpcrOutput() const
	{
		return m_pdrgpcrOutput;
	}

	// return table's name
	const CName &
	Name() const
	{
		return *m_pnameAlias;
	}

	// return table's descriptor
	gpos::Ref<CTableDescriptor>
	Ptabdesc() const
	{
		return m_ptabdesc;
	}

	// partition columns
	gpos::Ref<CColRef2dArray>
	PdrgpdrgpcrPartColumns() const
	{
		return m_pdrgpdrgpcrPart;
	}

	// operator specific hash function
	ULONG HashValue() const override;

	// match function
	BOOL Matches(gpos::Ref<COperator> pop) const override;

	// sensitivity to order of inputs
	BOOL FInputOrderSensitive() const override;

	// return a copy of the operator with remapped columns
	gpos::Ref<COperator> PopCopyWithRemappedColumns(
		CMemoryPool *mp, gpos::Ref<UlongToColRefMap> colref_mapping,
		BOOL must_exist) override;

	//-------------------------------------------------------------------------------------
	// Derived Relational Properties
	//-------------------------------------------------------------------------------------

	// derive output columns
	gpos::Ref<CColRefSet> DeriveOutputColumns(
		CMemoryPool *mp, CExpressionHandle &exprhdl) override;

	// derive not nullable output columns
	gpos::Ref<CColRefSet> DeriveNotNullColumns(
		CMemoryPool *mp, CExpressionHandle &exprhdl) const override;

	// derive partition consumer info
	gpos::Ref<CPartInfo>
	DerivePartitionInfo(CMemoryPool *mp,
						CExpressionHandle &	 // exprhdl
	) const override
	{
		return GPOS_NEW(mp) CPartInfo(mp);
	}

	// derive constraint property
	gpos::Ref<CPropConstraint>
	DerivePropertyConstraint(CMemoryPool *mp,
							 CExpressionHandle &  // exprhdl
	) const override
	{
		return PpcDeriveConstraintFromTable(mp, m_ptabdesc, m_pdrgpcrOutput);
	}

	// derive join depth
	ULONG
	DeriveJoinDepth(CMemoryPool *,		 // mp
					CExpressionHandle &	 // exprhdl
	) const override
	{
		return 1;
	}

	// derive table descriptor
	gpos::Ref<CTableDescriptor>
	DeriveTableDescriptor(CMemoryPool *,	   // mp
						  CExpressionHandle &  // exprhdl
	) const override
	{
		return m_ptabdesc;
	}

	//-------------------------------------------------------------------------------------
	// Required Relational Properties
	//-------------------------------------------------------------------------------------

	// compute required stat columns of the n-th child
	gpos::Ref<CColRefSet>
	PcrsStat(CMemoryPool *,			 // mp,
			 CExpressionHandle &,	 // exprhdl
			 gpos::Ref<CColRefSet>,	 // pcrsInput
			 ULONG					 // child_index
	) const override
	{
		GPOS_ASSERT(!"CLogicalGet has no children");
		return nullptr;
	}

	//-------------------------------------------------------------------------------------
	// Transformations
	//-------------------------------------------------------------------------------------

	// candidate set of xforms
	gpos::Ref<CXformSet> PxfsCandidates(CMemoryPool *mp) const override;

	// derive key collections
	gpos::Ref<CKeyCollection> DeriveKeyCollection(
		CMemoryPool *mp, CExpressionHandle &exprhdl) const override;

	// derive statistics
	gpos::Ref<IStatistics> PstatsDerive(
		CMemoryPool *mp, CExpressionHandle &exprhdl,
		gpos::Ref<IStatisticsArray> stats_ctxt) const override;

	// stat promise
	EStatPromise
	Esp(CExpressionHandle &) const override
	{
		return CLogical::EspHigh;
	}

	//-------------------------------------------------------------------------------------
	//-------------------------------------------------------------------------------------
	//-------------------------------------------------------------------------------------

	// conversion function
	static gpos::Ref<CLogicalGet>
	PopConvert(gpos::Ref<COperator> pop)
	{
		GPOS_ASSERT(nullptr != pop);
		GPOS_ASSERT(EopLogicalGet == pop->Eopid() ||
					EopLogicalForeignGet == pop->Eopid());

		return gpos::dyn_cast<CLogicalGet>(pop);
	}

	// debug print
	IOstream &OsPrint(IOstream &) const override;

};	// class CLogicalGet

}  // namespace gpopt


#endif	// !GPOPT_CLogicalGet_H

// EOF
