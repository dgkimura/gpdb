//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CConstraintDisjunction.cpp
//
//	@doc:
//		Implementation of disjunction constraints
//---------------------------------------------------------------------------

#include "gpopt/base/CConstraintDisjunction.h"

#include "gpos/base.h"
#include "gpos/common/owner.h"

#include "gpopt/base/CConstraintInterval.h"
#include "gpopt/base/CUtils.h"
#include "gpopt/operators/CPredicateUtils.h"

using namespace gpopt;

//---------------------------------------------------------------------------
//	@function:
//		CConstraintDisjunction::CConstraintDisjunction
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CConstraintDisjunction::CConstraintDisjunction(
	CMemoryPool *mp, gpos::Ref<CConstraintArray> pdrgpcnstr)
	: CConstraint(mp, PcrsFromConstraints(mp, pdrgpcnstr.get())),
	  m_pdrgpcnstr(nullptr)
{
	GPOS_ASSERT(nullptr != pdrgpcnstr);
	m_pdrgpcnstr = PdrgpcnstrFlatten(mp, std::move(pdrgpcnstr), EctDisjunction);

	m_phmcolconstr = Phmcolconstr(mp, m_pcrsUsed.get(), m_pdrgpcnstr.get());
}

//---------------------------------------------------------------------------
//	@function:
//		CConstraintDisjunction::~CConstraintDisjunction
//
//	@doc:
//		Dtor
//
//---------------------------------------------------------------------------
CConstraintDisjunction::~CConstraintDisjunction()
{
	;
	;
}

//---------------------------------------------------------------------------
//	@function:
//		CConstraintDisjunction::FContradiction
//
//	@doc:
//		Check if this constraint is a contradiction
//
//---------------------------------------------------------------------------
BOOL
CConstraintDisjunction::FContradiction() const
{
	const ULONG length = m_pdrgpcnstr->Size();

	BOOL fContradiction = true;
	for (ULONG ul = 0; fContradiction && ul < length; ul++)
	{
		fContradiction = (*m_pdrgpcnstr)[ul]->FContradiction();
	}

	return fContradiction;
}

//---------------------------------------------------------------------------
//	@function:
//		CConstraintDisjunction::FConstraint
//
//	@doc:
//		Check if there is a constraint on the given column
//
//---------------------------------------------------------------------------
BOOL
CConstraintDisjunction::FConstraint(const CColRef *colref) const
{
	CConstraintArray *pdrgpcnstrCol = m_phmcolconstr->Find(colref);
	return (nullptr != pdrgpcnstrCol &&
			m_pdrgpcnstr->Size() == pdrgpcnstrCol->Size());
}

//---------------------------------------------------------------------------
//	@function:
//		CConstraintDisjunction::PcnstrCopyWithRemappedColumns
//
//	@doc:
//		Return a copy of the constraint with remapped columns
//
//---------------------------------------------------------------------------
gpos::Ref<CConstraint>
CConstraintDisjunction::PcnstrCopyWithRemappedColumns(
	CMemoryPool *mp, UlongToColRefMap *colref_mapping, BOOL must_exist)
{
	gpos::Ref<CConstraintArray> pdrgpcnstr = GPOS_NEW(mp) CConstraintArray(mp);
	const ULONG length = m_pdrgpcnstr->Size();
	for (ULONG ul = 0; ul < length; ul++)
	{
		CConstraint *pcnstr = (*m_pdrgpcnstr)[ul].get();
		gpos::Ref<CConstraint> pcnstrCopy =
			pcnstr->PcnstrCopyWithRemappedColumns(mp, colref_mapping,
												  must_exist);
		pdrgpcnstr->Append(pcnstrCopy);
	}
	return GPOS_NEW(mp) CConstraintDisjunction(mp, std::move(pdrgpcnstr));
}

//---------------------------------------------------------------------------
//	@function:
//		CConstraintDisjunction::Pcnstr
//
//	@doc:
//		Return constraint on a given column
//
//---------------------------------------------------------------------------
gpos::Ref<CConstraint>
CConstraintDisjunction::Pcnstr(CMemoryPool *mp, const CColRef *colref)
{
	// all children referencing given column
	CConstraintArray *pdrgpcnstrCol = m_phmcolconstr->Find(colref);
	if (nullptr == pdrgpcnstrCol)
	{
		return nullptr;
	}

	// if not all children have this col, return unbounded constraint
	const ULONG length = pdrgpcnstrCol->Size();
	if (length != m_pdrgpcnstr->Size())
	{
		return CConstraintInterval::PciUnbounded(mp, colref,
												 true /*fIncludesNull*/);
	}

	gpos::Ref<CConstraintArray> pdrgpcnstr = GPOS_NEW(mp) CConstraintArray(mp);

	for (ULONG ul = 0; ul < length; ul++)
	{
		// the part of the child that references this column
		gpos::Ref<CConstraint> pcnstrCol =
			(*pdrgpcnstrCol)[ul]->Pcnstr(mp, colref);
		if (nullptr == pcnstrCol)
		{
			pcnstrCol =
				CConstraintInterval::PciUnbounded(mp, colref, true /*is_null*/);
		}
		if (pcnstrCol->IsConstraintUnbounded())
		{
			;
			return pcnstrCol;
		}
		pdrgpcnstr->Append(pcnstrCol);
	}

	return CConstraint::PcnstrDisjunction(mp, std::move(pdrgpcnstr));
}

//---------------------------------------------------------------------------
//	@function:
//		CConstraintDisjunction::Pcnstr
//
//	@doc:
//		Return constraint on a given column set
//
//---------------------------------------------------------------------------
gpos::Ref<CConstraint>
CConstraintDisjunction::Pcnstr(CMemoryPool *mp, CColRefSet *pcrs)
{
	const ULONG length = m_pdrgpcnstr->Size();

	gpos::Ref<CConstraintArray> pdrgpcnstr = GPOS_NEW(mp) CConstraintArray(mp);

	for (ULONG ul = 0; ul < length; ul++)
	{
		CConstraint *pcnstr = (*m_pdrgpcnstr)[ul].get();
		if (pcnstr->PcrsUsed()->IsDisjoint(pcrs))
		{
			// a child has none of these columns... return unbounded constraint
			;
			return CConstraintInterval::PciUnbounded(mp, pcrs,
													 true /*fIncludesNull*/);
		}

		// the part of the child that references these columns
		gpos::Ref<CConstraint> pcnstrCol = pcnstr->Pcnstr(mp, pcrs);

		if (nullptr == pcnstrCol)
		{
			pcnstrCol = CConstraintInterval::PciUnbounded(
				mp, pcrs, true /*fIncludesNull*/);
		}
		GPOS_ASSERT(nullptr != pcnstrCol);
		pdrgpcnstr->Append(pcnstrCol);
	}

	return CConstraint::PcnstrDisjunction(mp, std::move(pdrgpcnstr));
}

//---------------------------------------------------------------------------
//	@function:
//		CConstraintDisjunction::PcnstrRemapForColumn
//
//	@doc:
//		Return a copy of the constraint for a different column
//
//---------------------------------------------------------------------------
gpos::Ref<CConstraint>
CConstraintDisjunction::PcnstrRemapForColumn(CMemoryPool *mp,
											 CColRef *colref) const
{
	return PcnstrConjDisjRemapForColumn(mp, colref, m_pdrgpcnstr.get(),
										false /*fConj*/);
}

//---------------------------------------------------------------------------
//	@function:
//		CConstraintDisjunction::PexprScalar
//
//	@doc:
//		Scalar expression
//
//---------------------------------------------------------------------------
CExpression *
CConstraintDisjunction::PexprScalar(CMemoryPool *mp)
{
	if (nullptr == m_pexprScalar)
	{
		if (FContradiction())
		{
			m_pexprScalar = CUtils::PexprScalarConstBool(mp, false /*fval*/,
														 false /*is_null*/);
		}
		else
		{
			m_pexprScalar =
				PexprScalarConjDisj(mp, m_pdrgpcnstr.get(), false /*fConj*/);
		}
	}

	return m_pexprScalar.get();
}

// EOF
