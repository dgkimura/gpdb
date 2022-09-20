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
	gpos::owner<CIndexDescriptor *> m_pindexdesc;

	// table descriptor
	gpos::owner<CTableDescriptor *> m_ptabdesc;

	// origin operator id -- gpos::ulong_max if operator was not generated via a transformation
	ULONG m_ulOriginOpId;

	// alias for table
	const CName *m_pnameAlias;

	// output columns
	gpos::owner<CColRefArray *> m_pdrgpcrOutput;

	// set representation of output columns
	gpos::owner<CColRefSet *> m_pcrsOutput;

	// order spec
	gpos::owner<COrderSpec *> m_pos;

	// distribution columns (empty for master only tables)
	gpos::owner<CColRefSet *> m_pcrsDist;

public:
	CLogicalIndexGet(const CLogicalIndexGet &) = delete;

	// ctors
	explicit CLogicalIndexGet(CMemoryPool *mp);

	CLogicalIndexGet(CMemoryPool *mp, gpos::pointer<const IMDIndex *> pmdindex,
					 gpos::owner<CTableDescriptor *> ptabdesc,
					 ULONG ulOriginOpId, const CName *pnameAlias,
					 gpos::owner<CColRefArray *> pdrgpcrOutput);

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
	virtual gpos::pointer<const CColRefSet *>
	PcrsDist() const
	{
		return m_pcrsDist;
	}

	// array of output columns
	gpos::pointer<CColRefArray *>
	PdrgpcrOutput() const
	{
		return m_pdrgpcrOutput;
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
	gpos::pointer<CIndexDescriptor *>
	Pindexdesc() const
	{
		return m_pindexdesc;
	}

	// table descriptor
	gpos::pointer<CTableDescriptor *>
	Ptabdesc() const
	{
		return m_ptabdesc;
	}

	// order spec
	gpos::pointer<COrderSpec *>
	Pos() const
	{
		return m_pos;
	}

	// operator specific hash function
	ULONG HashValue() const override;

	// match function
	BOOL Matches(gpos::pointer<COperator *> pop) const override;

	// sensitivity to order of inputs
	BOOL FInputOrderSensitive() const override;

	// return a copy of the operator with remapped columns
	gpos::owner<COperator *> PopCopyWithRemappedColumns(
		CMemoryPool *mp, gpos::pointer<UlongToColRefMap *> colref_mapping,
		BOOL must_exist) override;

	//-------------------------------------------------------------------------------------
	// Derived Relational Properties
	//-------------------------------------------------------------------------------------

	// derive output columns
	gpos::owner<CColRefSet *> DeriveOutputColumns(
		CMemoryPool *mp, CExpressionHandle &exprhdl) override;

	// derive outer references
	gpos::owner<CColRefSet *> DeriveOuterReferences(
		CMemoryPool *mp, CExpressionHandle &exprhdl) override;

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
							 CExpressionHandle &  // exprhdl
	) const override
	{
		return PpcDeriveConstraintFromTable(mp, m_ptabdesc, m_pdrgpcrOutput);
	}

	// derive key collections
	gpos::owner<CKeyCollection *> DeriveKeyCollection(
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
	gpos::owner<CColRefSet *>
	PcrsStat(CMemoryPool *mp,
			 CExpressionHandle &,		   // exprhdl
			 gpos::pointer<CColRefSet *>,  //pcrsInput
			 ULONG						   // child_index
	) const override
	{
		// TODO:  March 26 2012; statistics derivation for indexes
		return GPOS_NEW(mp) CColRefSet(mp);
	}

	// derive statistics
	gpos::owner<IStatistics *> PstatsDerive(
		CMemoryPool *mp, CExpressionHandle &exprhdl,
		gpos::pointer<IStatisticsArray *> stats_ctxt) const override;

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

	//-------------------------------------------------------------------------------------
	// conversion function
	//-------------------------------------------------------------------------------------

	static gpos::cast_func<CLogicalIndexGet *>
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
