//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CConstraintConjunction.cpp
//
//	@doc:
//		Implementation of conjunction constraints
//---------------------------------------------------------------------------

#include "gpopt/base/CConstraintConjunction.h"

#include "gpos/base.h"
#include "gpos/common/owner.h"

#include "gpopt/base/CConstraintInterval.h"
#include "gpopt/base/CUtils.h"
#include "gpopt/operators/CPredicateUtils.h"

using namespace gpopt;

//---------------------------------------------------------------------------
//	@function:
//		CConstraintConjunction::CConstraintConjunction
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CConstraintConjunction::CConstraintConjunction(
	CMemoryPool *mp, gpos::owner<CConstraintArray *> pdrgpcnstr)
	: CConstraint(mp, PcrsFromConstraints(mp, pdrgpcnstr)),
	  m_pdrgpcnstr(nullptr)
{
	GPOS_ASSERT(nullptr != pdrgpcnstr);
	m_pdrgpcnstr = PdrgpcnstrFlatten(mp, std::move(pdrgpcnstr), EctConjunction);

	m_phmcolconstr = Phmcolconstr(mp, m_pcrsUsed, m_pdrgpcnstr);
}

//---------------------------------------------------------------------------
//	@function:
//		CConstraintConjunction::~CConstraintConjunction
//
//	@doc:
//		Dtor
//
//---------------------------------------------------------------------------
CConstraintConjunction::~CConstraintConjunction()
{
	m_pdrgpcnstr->Release();
	m_phmcolconstr->Release();
}

//---------------------------------------------------------------------------
//	@function:
//		CConstraintConjunction::FContradiction
//
//	@doc:
//		Check if this constraint is a contradiction
//
//---------------------------------------------------------------------------
BOOL
CConstraintConjunction::FContradiction() const
{
	const ULONG length = m_pdrgpcnstr->Size();

	BOOL fContradiction = false;
	for (ULONG ul = 0; !fContradiction && ul < length; ul++)
	{
		fContradiction = (*m_pdrgpcnstr)[ul]->FContradiction();
	}

	return fContradiction;
}

//---------------------------------------------------------------------------
//	@function:
//		CConstraintConjunction::FConstraint
//
//	@doc:
//		Check if there is a constraint on the given column
//
//---------------------------------------------------------------------------
BOOL
CConstraintConjunction::FConstraint(const CColRef *colref) const
{
	gpos::pointer<CConstraintArray *> pdrgpcnstrCol =
		m_phmcolconstr->Find(colref);
	return (nullptr != pdrgpcnstrCol && 0 < pdrgpcnstrCol->Size());
}

//---------------------------------------------------------------------------
//	@function:
//		CConstraintConjunction::PcnstrCopyWithRemappedColumns
//
//	@doc:
//		Return a copy of the constraint with remapped columns. If must_exist is
//		set to true, then every column reference in this constraint must be in
//		the hashmap in order to be replace. Otherwise, some columns may not be
//		in the mapping, and hence will not be replaced
//
//---------------------------------------------------------------------------
gpos::owner<CConstraint *>
CConstraintConjunction::PcnstrCopyWithRemappedColumns(
	CMemoryPool *mp, UlongToColRefMap *colref_mapping, BOOL must_exist)
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
	return GPOS_NEW(mp) CConstraintConjunction(mp, std::move(pdrgpcnstr));
}

//---------------------------------------------------------------------------
//	@function:
//		CConstraintConjunction::Pcnstr
//
//	@doc:
//		Return constraint on a given column
//
//---------------------------------------------------------------------------
gpos::owner<CConstraint *>
CConstraintConjunction::Pcnstr(CMemoryPool *mp, const CColRef *colref)
{
	// all children referencing given column
	gpos::pointer<CConstraintArray *> pdrgpcnstrCol =
		m_phmcolconstr->Find(colref);
	if (nullptr == pdrgpcnstrCol)
	{
		return nullptr;
	}

	gpos::owner<CConstraintArray *> pdrgpcnstr =
		GPOS_NEW(mp) CConstraintArray(mp);

	const ULONG length = pdrgpcnstrCol->Size();
	for (ULONG ul = 0; ul < length; ul++)
	{
		// the part of the child that references this column
		gpos::owner<CConstraint *> pcnstrCol =
			(*pdrgpcnstrCol)[ul]->Pcnstr(mp, colref);
		if (nullptr == pcnstrCol || pcnstrCol->IsConstraintUnbounded())
		{
			CRefCount::SafeRelease(pcnstrCol);
			continue;
		}
		pdrgpcnstr->Append(pcnstrCol);
	}

	return CConstraint::PcnstrConjunction(mp, std::move(pdrgpcnstr));
}

//---------------------------------------------------------------------------
//	@function:
//		CConstraintConjunction::Pcnstr
//
//	@doc:
//		Return constraint on a given column set
//
//---------------------------------------------------------------------------
gpos::owner<CConstraint *>
CConstraintConjunction::Pcnstr(CMemoryPool *mp,
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
			continue;
		}

		// the part of the child that references these columns
		gpos::owner<CConstraint *> pcnstrCol = pcnstr->Pcnstr(mp, pcrs);
		if (nullptr != pcnstrCol)
		{
			pdrgpcnstr->Append(pcnstrCol);
		}
	}

	return CConstraint::PcnstrConjunction(mp, std::move(pdrgpcnstr));
}

//---------------------------------------------------------------------------
//	@function:
//		CConstraintConjunction::PcnstrRemapForColumn
//
//	@doc:
//		Return a copy of the constraint for a different column
//
//---------------------------------------------------------------------------
gpos::owner<CConstraint *>
CConstraintConjunction::PcnstrRemapForColumn(CMemoryPool *mp,
											 CColRef *colref) const
{
	return PcnstrConjDisjRemapForColumn(mp, colref, m_pdrgpcnstr,
										true /*fConj*/);
}

//---------------------------------------------------------------------------
//	@function:
//		CConstraintConjunction::PexprScalar
//
//	@doc:
//		Scalar expression
//
//---------------------------------------------------------------------------
gpos::pointer<CExpression *>
CConstraintConjunction::PexprScalar(CMemoryPool *mp)
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
				PexprScalarConjDisj(mp, m_pdrgpcnstr, true /*fConj*/);
		}
	}

	return m_pexprScalar;
}

// EOF
