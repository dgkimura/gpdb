//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2014 VMware, Inc. or its affiliates.
//
//	@filename:
//		CLogicalBitmapTableGet.h
//
//	@doc:
//		Logical operator for table access via bitmap indexes.
//
//	@owner:
//
//
//	@test:
//
//---------------------------------------------------------------------------

#ifndef GPOPT_CLogicalBitmapTableGet_H
#define GPOPT_CLogicalBitmapTableGet_H

#include "gpos/base.h"
#include "gpos/common/owner.h"

#include "gpopt/base/CColRefSet.h"
#include "gpopt/metadata/CTableDescriptor.h"
#include "gpopt/operators/CLogical.h"

namespace gpopt
{
//---------------------------------------------------------------------------
//	@class:
//		CLogicalBitmapTableGet
//
//	@doc:
//		Logical operator for table access via bitmap indexes.
//
//---------------------------------------------------------------------------
class CLogicalBitmapTableGet : public CLogical
{
private:
	// table descriptor
	gpos::owner<CTableDescriptor *> m_ptabdesc;

	// origin operator id -- gpos::ulong_max if operator was not generated via a transformation
	ULONG m_ulOriginOpId;

	// alias for table
	const CName *m_pnameTableAlias;

	// output columns
	gpos::owner<CColRefArray *> m_pdrgpcrOutput;

public:
	CLogicalBitmapTableGet(const CLogicalBitmapTableGet &) = delete;

	// ctor
	CLogicalBitmapTableGet(CMemoryPool *mp,
						   gpos::owner<CTableDescriptor *> ptabdesc,
						   ULONG ulOriginOpId, const CName *pnameTableAlias,
						   gpos::owner<CColRefArray *> pdrgpcrOutput);

	// ctor
	// only for transformations
	explicit CLogicalBitmapTableGet(CMemoryPool *mp);

	// dtor
	~CLogicalBitmapTableGet() override;

	// table descriptor
	gpos::pointer<CTableDescriptor *>
	Ptabdesc() const
	{
		return m_ptabdesc;
	}

	// table alias
	const CName *
	PnameTableAlias()
	{
		return m_pnameTableAlias;
	}

	// array of output column references
	gpos::pointer<CColRefArray *>
	PdrgpcrOutput() const
	{
		return m_pdrgpcrOutput;
	}

	// identifier
	EOperatorId
	Eopid() const override
	{
		return EopLogicalBitmapTableGet;
	}

	// return a string for operator name
	const CHAR *
	SzId() const override
	{
		return "CLogicalBitmapTableGet";
	}

	// origin operator id -- gpos::ulong_max if operator was not generated via a transformation
	ULONG
	UlOriginOpId() const
	{
		return m_ulOriginOpId;
	}

	// operator specific hash function
	ULONG HashValue() const override;

	// match function
	BOOL Matches(gpos::pointer<COperator *> pop) const override;

	// sensitivity to order of inputs
	BOOL
	FInputOrderSensitive() const override
	{
		return true;
	}

	// return a copy of the operator with remapped columns
	gpos::owner<COperator *> PopCopyWithRemappedColumns(
		CMemoryPool *mp, gpos::pointer<UlongToColRefMap *> colref_mapping,
		BOOL must_exist) override;

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
	gpos::owner<CPropConstraint *> DerivePropertyConstraint(
		CMemoryPool *mp, CExpressionHandle &exprhdl) const override;

	// derive join depth
	ULONG
	DeriveJoinDepth(CMemoryPool *,		 // mp
					CExpressionHandle &	 // exprhdl
	) const override
	{
		return 1;
	}

	// derive table descriptor
	gpos::pointer<CTableDescriptor *>
	DeriveTableDescriptor(CMemoryPool *,	   // mp
						  CExpressionHandle &  // exprhdl
	) const override
	{
		return m_ptabdesc;
	}

	// compute required stat columns of the n-th child
	gpos::owner<CColRefSet *>
	PcrsStat(CMemoryPool *mp,
			 CExpressionHandle &,		   // exprhdl
			 gpos::pointer<CColRefSet *>,  //pcrsInput
			 ULONG						   // child_index
	) const override
	{
		return GPOS_NEW(mp) CColRefSet(mp);
	}

	// candidate set of xforms
	gpos::owner<CXformSet *> PxfsCandidates(CMemoryPool *mp) const override;

	// derive statistics
	gpos::owner<IStatistics *> PstatsDerive(
		CMemoryPool *mp, CExpressionHandle &exprhdl,
		gpos::pointer<IStatisticsArray *> stats_ctxt) const override;

	// stat promise
	EStatPromise
	Esp(CExpressionHandle &) const override
	{
		return CLogical::EspHigh;
	}

	// debug print
	IOstream &OsPrint(IOstream &) const override;

	// conversion
	static gpos::cast_func<CLogicalBitmapTableGet *>
	PopConvert(COperator *pop)
	{
		GPOS_ASSERT(nullptr != pop);
		GPOS_ASSERT(EopLogicalBitmapTableGet == pop->Eopid());

		return dynamic_cast<CLogicalBitmapTableGet *>(pop);
	}

};	// class CLogicalBitmapTableGet
}  // namespace gpopt

#endif	// !GPOPT_CLogicalBitmapTableGet_H

// EOF
