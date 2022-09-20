//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CConstraintConjunction.h
//
//	@doc:
//		Representation of a conjunction constraint. A conjunction is a number
//		of ANDed constraints
//---------------------------------------------------------------------------
#ifndef GPOPT_CConstraintConjunction_H
#define GPOPT_CConstraintConjunction_H

#include "gpos/base.h"
#include "gpos/common/owner.h"

#include "gpopt/base/CConstraint.h"
#include "gpopt/base/CRange.h"

namespace gpopt
{
using namespace gpos;
using namespace gpmd;

//---------------------------------------------------------------------------
//	@class:
//		CConstraintConjunction
//
//	@doc:
//		Representation of a conjunction constraint
//
//---------------------------------------------------------------------------
class CConstraintConjunction : public CConstraint
{
private:
	// array of constraints
	gpos::Ref<CConstraintArray> m_pdrgpcnstr;

	// mapping colref -> array of child constraints
	gpos::Ref<ColRefToConstraintArrayMap> m_phmcolconstr;

public:
	CConstraintConjunction(const CConstraintConjunction &) = delete;

	// ctor
	CConstraintConjunction(CMemoryPool *mp,
						   gpos::Ref<CConstraintArray> pdrgpcnstr);

	// dtor
	~CConstraintConjunction() override;

	// constraint type accessor
	EConstraintType
	Ect() const override
	{
		return CConstraint::EctConjunction;
	}

	// all constraints in conjunction
	CConstraintArray *
	Pdrgpcnstr() const
	{
		return m_pdrgpcnstr.get();
	}

	// is this constraint a contradiction
	BOOL FContradiction() const override;

	// scalar expression
	CExpression *PexprScalar(CMemoryPool *mp) override;

	// check if there is a constraint on the given column
	BOOL FConstraint(const CColRef *colref) const override;

	// return a copy of the constraint with remapped columns
	gpos::Ref<CConstraint> PcnstrCopyWithRemappedColumns(
		CMemoryPool *mp, UlongToColRefMap *colref_mapping,
		BOOL must_exist) override;

	// return constraint on a given column
	gpos::Ref<CConstraint> Pcnstr(CMemoryPool *mp,
								  const CColRef *colref) override;

	// return constraint on a given column set
	gpos::Ref<CConstraint> Pcnstr(CMemoryPool *mp, CColRefSet *pcrs) override;

	// return a clone of the constraint for a different column
	gpos::Ref<CConstraint> PcnstrRemapForColumn(CMemoryPool *mp,
												CColRef *colref) const override;

	// print
	IOstream &
	OsPrint(IOstream &os) const override
	{
		return PrintConjunctionDisjunction(os, m_pdrgpcnstr.get());
	}

};	// class CConstraintConjunction
}  // namespace gpopt

#endif	// !GPOPT_CConstraintConjunction_H

// EOF
