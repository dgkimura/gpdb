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
	CMemoryPool *mp, gpos::owner<CConstraintArray *> pdrgpcnstr)
	: CConstraint(mp, PcrsFromConstraints(mp, pdrgpcnstr)),
	  m_pdrgpcnstr(nullptr)
{
	GPOS_ASSERT(nullptr != pdrgpcnstr);
	m_pdrgpcnstr = PdrgpcnstrFlatten(mp, std::move(pdrgpcnstr), EctDisjunction);

	m_phmcolconstr = Phmcolconstr(mp, m_pcrsUsed, m_pdrgpcnstr);
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
	m_pdrgpcnstr->Release();
	m_phmcolconstr->Release();
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
	gpos::pointer<CConstraintArray *> pdrgpcnstrCol =
		m_phmcolconstr->Find(colref);
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
gpos::owner<CConstraint *>
CConstraintDisjunction::PcnstrCopyWithRemappedColumns(
	CMemoryPool *mp, gpos::pointer<UlongToColRefMap *> colref_mapping,
	BOOL must_exist)
{
	gpos::owner<CConstraintArray *> pdrgpcnstr =
		GPOS_NEW(mp) CConstraintArray(mp);
	const ULONG length = m_pdrgpcnstr->Size();
	for (ULONG ul = 0; ul < length; ul++)
	{
		gpos::pointer<CConstraint *> pcnstr = (*m_pdrgpcnstr)[ul];
		gpos::owner<CConstraint *> pcnstrCopy =
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
gpos::owner<CConstraint *>
CConstraintDisjunction::Pcnstr(CMemoryPool *mp, const CColRef *colref)
{
	// all children referencing given column
	gpos::pointer<CConstraintArray *> pdrgpcnstrCol =
		m_phmcolconstr->Find(colref);
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

	gpos::owner<CConstraintArray *> pdrgpcnstr =
		GPOS_NEW(mp) CConstraintArray(mp);

	for (ULONG ul = 0; ul < length; ul++)
	{
		// the part of the child that references this column
		gpos::owner<CConstraint *> pcnstrCol =
			(*pdrgpcnstrCol)[ul]->Pcnstr(mp, colref);
		if (nullptr == pcnstrCol)
		{
			pcnstrCol =
				CConstraintInterval::PciUnbounded(mp, colref, true /*is_null*/);
		}
		if (pcnstrCol->IsConstraintUnbounded())
		{
			pdrgpcnstr->Release();
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
gpos::owner<CConstraint *>
CConstraintDisjunction::Pcnstr(CMemoryPool *mp,
							   gpos::pointer<CColRefSet *> pcrs)
{
	const ULONG length = m_pdrgpcnstr->Size();

	gpos::owner<CConstraintArray *> pdrgpcnstr =
		GPOS_NEW(mp) CConstraintArray(mp);

	for (ULONG ul = 0; ul < length; ul++)
	{
		gpos::pointer<CConstraint *> pcnstr = (*m_pdrgpcnstr)[ul];
		if (pcnstr->PcrsUsed()->IsDisjoint(pcrs))
		{
			// a child has none of these columns... return unbounded constraint
			pdrgpcnstr->Release();
			return CConstraintInterval::PciUnbounded(mp, pcrs,
													 true /*fIncludesNull*/);
		}

		// the part of the child that references these columns
		gpos::owner<CConstraint *> pcnstrCol = pcnstr->Pcnstr(mp, pcrs);

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
gpos::owner<CConstraint *>
CConstraintDisjunction::PcnstrRemapForColumn(CMemoryPool *mp,
											 CColRef *colref) const
{
	return PcnstrConjDisjRemapForColumn(mp, colref, m_pdrgpcnstr,
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
gpos::pointer<CExpression *>
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
				PexprScalarConjDisj(mp, m_pdrgpcnstr, false /*fConj*/);
		}
	}

	return m_pexprScalar;
}

// EOF
