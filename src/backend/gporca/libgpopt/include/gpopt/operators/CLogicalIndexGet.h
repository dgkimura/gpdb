//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CLogicalIndexGet.h
//
//	@doc:
//		Basic index accessor
//---------------------------------------------------------------------------
#ifndef GPOPT_CLogicalIndexGet_H
#define GPOPT_CLogicalIndexGet_H

#include "gpos/base.h"
#include "gpos/common/owner.h"

#include "gpopt/base/CColRefSet.h"
#include "gpopt/base/COrderSpec.h"
#include "gpopt/metadata/CIndexDescriptor.h"
#include "gpopt/operators/CLogical.h"


namespace gpopt
{
// fwd declarations
class CName;

//---------------------------------------------------------------------------
//	@class:
//		CLogicalIndexGet
//
//	@doc:
//		Basic index accessor
//
//---------------------------------------------------------------------------
class CLogicalIndexGet : public CLogical
{
private:
	// index descriptor
	gpos::Ref<CIndexDescriptor> m_pindexdesc;

	// table descriptor
	gpos::Ref<CTableDescriptor> m_ptabdesc;

	// origin operator id -- gpos::ulong_max if operator was not generated via a transformation
	ULONG m_ulOriginOpId;

	// alias for table
	const CName *m_pnameAlias;

	// output columns
	gpos::Ref<CColRefArray> m_pdrgpcrOutput;

	// set representation of output columns
	gpos::Ref<CColRefSet> m_pcrsOutput;

	// order spec
	gpos::Ref<COrderSpec> m_pos;

	// distribution columns (empty for master only tables)
	gpos::Ref<CColRefSet> m_pcrsDist;

public:
	CLogicalIndexGet(const CLogicalIndexGet &) = delete;

	// ctors
	explicit CLogicalIndexGet(CMemoryPool *mp);

	CLogicalIndexGet(CMemoryPool *mp, const IMDIndex *pmdindex,
					 gpos::Ref<CTableDescriptor> ptabdesc, ULONG ulOriginOpId,
					 const CName *pnameAlias,
					 gpos::Ref<CColRefArray> pdrgpcrOutput);

	// dtor
	~CLogicalIndexGet() override;

	// ident accessors
	EOperatorId
	Eopid() const override
	{
		return EopLogicalIndexGet;
	}

	// return a string for operator name
	const CHAR *
	SzId() const override
	{
		return "CLogicalIndexGet";
	}

	// distribution columns
	virtual const CColRefSet *
	PcrsDist() const
	{
		return m_pcrsDist.get();
	}

	// array of output columns
	CColRefArray *
	PdrgpcrOutput() const
	{
		return m_pdrgpcrOutput.get();
	}

	// origin operator id -- gpos::ulong_max if operator was not generated via a transformation
	ULONG
	UlOriginOpId() const
	{
		return m_ulOriginOpId;
	}

	// index name
	const CName &
	Name() const
	{
		return m_pindexdesc->Name();
	}

	// table alias name
	const CName &
	NameAlias() const
	{
		return *m_pnameAlias;
	}

	// index descriptor
	CIndexDescriptor *
	Pindexdesc() const
	{
		return m_pindexdesc.get();
	}

	// table descriptor
	CTableDescriptor *
	Ptabdesc() const
	{
		return m_ptabdesc.get();
	}

	// order spec
	COrderSpec *
	Pos() const
	{
		return m_pos.get();
	}

	// operator specific hash function
	ULONG HashValue() const override;

	// match function
	BOOL Matches(COperator *pop) const override;

	// sensitivity to order of inputs
	BOOL FInputOrderSensitive() const override;

	// return a copy of the operator with remapped columns
	gpos::Ref<COperator> PopCopyWithRemappedColumns(
		CMemoryPool *mp, UlongToColRefMap *colref_mapping,
		BOOL must_exist) override;

	//-------------------------------------------------------------------------------------
	// Derived Relational Properties
	//-------------------------------------------------------------------------------------

	// derive output columns
	gpos::Ref<CColRefSet> DeriveOutputColumns(
		CMemoryPool *mp, CExpressionHandle &exprhdl) override;

	// derive outer references
	gpos::Ref<CColRefSet> DeriveOuterReferences(
		CMemoryPool *mp, CExpressionHandle &exprhdl) override;

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
							 CExpressionHandle &  // exprhdl
	) const override
	{
		return PpcDeriveConstraintFromTable(mp, m_ptabdesc.get(),
											m_pdrgpcrOutput.get());
	}

	// derive key collections
	gpos::Ref<CKeyCollection> DeriveKeyCollection(
		CMemoryPool *mp, CExpressionHandle &exprhdl) const override;

	// derive join depth
	ULONG
	DeriveJoinDepth(CMemoryPool *,		 // mp
					CExpressionHandle &	 // exprhdl
	) const override
	{
		return 1;
	}

	//-------------------------------------------------------------------------------------
	// Required Relational Properties
	//-------------------------------------------------------------------------------------

	// compute required stat columns of the n-th child
	gpos::Ref<CColRefSet>
	PcrsStat(CMemoryPool *mp,
			 CExpressionHandle &,  // exprhdl
			 CColRefSet *,		   //pcrsInput
			 ULONG				   // child_index
	) const override
	{
		// TODO:  March 26 2012; statistics derivation for indexes
		return GPOS_NEW(mp) CColRefSet(mp);
	}

	// derive statistics
	gpos::Ref<IStatistics> PstatsDerive(
		CMemoryPool *mp, CExpressionHandle &exprhdl,
		IStatisticsArray *stats_ctxt) const override;

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

	//-------------------------------------------------------------------------------------
	// conversion function
	//-------------------------------------------------------------------------------------

	static CLogicalIndexGet *
	PopConvert(COperator *pop)
	{
		GPOS_ASSERT(nullptr != pop);
		GPOS_ASSERT(EopLogicalIndexGet == pop->Eopid());

		return dynamic_cast<CLogicalIndexGet *>(pop);
	}


	// debug print
	IOstream &OsPrint(IOstream &) const override;

};	// class CLogicalIndexGet

}  // namespace gpopt

#endif	// !GPOPT_CLogicalIndexGet_H

// EOF
