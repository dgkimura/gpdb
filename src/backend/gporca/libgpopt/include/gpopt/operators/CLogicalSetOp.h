//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2009 Greenplum, Inc.
//
//	@filename:
//		CLogicalSetOp.h
//
//	@doc:
//		Base for set operations
//---------------------------------------------------------------------------
#ifndef GPOS_CLogicalSetOp_H
#define GPOS_CLogicalSetOp_H

#include "gpos/base.h"
#include "gpos/common/owner.h"

#include "gpopt/base/CColRefSet.h"
#include "gpopt/operators/CLogical.h"

namespace gpopt
{
//---------------------------------------------------------------------------
//	@class:
//		CLogicalSetOp
//
//	@doc:
//		Base for all set operations
//
//---------------------------------------------------------------------------
class CLogicalSetOp : public CLogical
{
protected:
	// output column array
	gpos::Ref<CColRefArray> m_pdrgpcrOutput;

	// input column array
	gpos::Ref<CColRef2dArray> m_pdrgpdrgpcrInput;

	// set representation of output columns
	gpos::Ref<CColRefSet> m_pcrsOutput;

	// set representation of input columns
	gpos::Ref<CColRefSetArray> m_pdrgpcrsInput;

	// private copy ctor
	CLogicalSetOp(const CLogicalSetOp &);

	// build set representation of input/output columns for faster set operations
	void BuildColumnSets(CMemoryPool *mp);

	// output equivalence classes
	gpos::Ref<CColRefSetArray> PdrgpcrsOutputEquivClasses(
		CMemoryPool *mp, CExpressionHandle &exprhdl, BOOL fIntersect) const;

	// equivalence classes from one input child, mapped to output columns
	gpos::Ref<CColRefSetArray> PdrgpcrsInputMapped(CMemoryPool *mp,
												   CExpressionHandle &exprhdl,
												   ULONG ulChild) const;

	// constraints for a given output column from all children
	gpos::Ref<CConstraintArray> PdrgpcnstrColumn(CMemoryPool *mp,
												 CExpressionHandle &exprhdl,
												 ULONG ulColIndex,
												 ULONG ulStart) const;

	// get constraint for a given output column from a given children
	gpos::Ref<CConstraint> PcnstrColumn(CMemoryPool *mp,
										CExpressionHandle &exprhdl,
										ULONG ulColIndex, ULONG ulChild) const;

	// derive constraint property for difference, intersect, and union
	// operators
	gpos::Ref<CPropConstraint> PpcDeriveConstraintSetop(
		CMemoryPool *mp, CExpressionHandle &exprhdl, BOOL fIntersect) const;

public:
	// ctor
	explicit CLogicalSetOp(CMemoryPool *mp);

	CLogicalSetOp(CMemoryPool *mp, gpos::Ref<CColRefArray> pdrgOutput,
				  gpos::Ref<CColRefArray> pdrgpcrLeft,
				  gpos::Ref<CColRefArray> pdrgpcrRight);

	CLogicalSetOp(CMemoryPool *mp, gpos::Ref<CColRefArray> pdrgpcrOutput,
				  gpos::Ref<CColRef2dArray> pdrgpdrgpcrInput);

	// dtor
	~CLogicalSetOp() override;

	// ident accessors
	EOperatorId Eopid() const override = 0;

	// return a string for operator name
	const CHAR *SzId() const override = 0;

	// accessor of output column array
	CColRefArray *
	PdrgpcrOutput() const
	{
		GPOS_ASSERT(nullptr != m_pdrgpcrOutput);
		return m_pdrgpcrOutput.get();
	}

	// accessor of input column array
	CColRef2dArray *
	PdrgpdrgpcrInput() const
	{
		GPOS_ASSERT(nullptr != m_pdrgpdrgpcrInput);
		return m_pdrgpdrgpcrInput.get();
	}

	// return true if we can pull projections up past this operator from its given child
	BOOL FCanPullProjectionsUp(ULONG  //child_index
	) const override
	{
		return false;
	}

	// match function
	BOOL Matches(COperator *pop) const override;

	IOstream &OsPrint(IOstream &os) const override;

	//-------------------------------------------------------------------------------------
	// Derived Relational Properties
	//-------------------------------------------------------------------------------------

	// derive output columns
	gpos::Ref<CColRefSet> DeriveOutputColumns(CMemoryPool *,
											  CExpressionHandle &) override;

	// derive key collections
	gpos::Ref<CKeyCollection> DeriveKeyCollection(
		CMemoryPool *mp, CExpressionHandle &exprhdl) const override;

	// derive partition consumer info
	gpos::Ref<CPartInfo> DerivePartitionInfo(
		CMemoryPool *mp, CExpressionHandle &exprhdl) const override;

	//-------------------------------------------------------------------------------------
	// Required Relational Properties
	//-------------------------------------------------------------------------------------

	// compute required stat columns of the n-th child
	gpos::Ref<CColRefSet> PcrsStat(CMemoryPool *,		 // mp
								   CExpressionHandle &,	 // exprhdl
								   CColRefSet *pcrsInput,
								   ULONG  // child_index
	) const override;

	// conversion function
	static CLogicalSetOp *
	PopConvert(COperator *pop)
	{
		GPOS_ASSERT(nullptr != pop);
		GPOS_ASSERT(CUtils::FLogicalSetOp(pop));

		return dynamic_cast<CLogicalSetOp *>(pop);
	}

};	// class CLogicalSetOp

}  // namespace gpopt


#endif	// !GPOS_CLogicalSetOp_H

// EOF
