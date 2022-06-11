//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CConstraintNegation.cpp
//
//	@doc:
//		Implementation of negation constraints
//---------------------------------------------------------------------------

#include "gpopt/base/CConstraintNegation.h"

#include "gpos/base.h"
#include "gpos/common/owner.h"

#include "gpopt/base/CConstraintInterval.h"
#include "gpopt/operators/CPredicateUtils.h"

using namespace gpopt;

//---------------------------------------------------------------------------
//	@function:
//		CConstraintNegation::CConstraintNegation
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CConstraintNegation::CConstraintNegation(CMemoryPool *mp,
										 gpos::Ref<CConstraint> pcnstr)
	: CConstraint(mp, pcnstr->PcrsUsed()), m_pcnstr(std::move(pcnstr))
{
	GPOS_ASSERT(nullptr != m_pcnstr);

	;
}

//---------------------------------------------------------------------------
//	@function:
//		CConstraintNegation::~CConstraintNegation
//
//	@doc:
//		Dtor
//
//---------------------------------------------------------------------------
CConstraintNegation::~CConstraintNegation()
{
	;
}

//---------------------------------------------------------------------------
//	@function:
//		CConstraintNegation::PcnstrCopyWithRemappedColumns
//
//	@doc:
//		Return a copy of the constraint with remapped columns
//
//---------------------------------------------------------------------------
gpos::Ref<CConstraint>
CConstraintNegation::PcnstrCopyWithRemappedColumns(
	CMemoryPool *mp, UlongToColRefMap *colref_mapping, BOOL must_exist)
{
	gpos::Ref<CConstraint> pcnstr =
		m_pcnstr->PcnstrCopyWithRemappedColumns(mp, colref_mapping, must_exist);
	return GPOS_NEW(mp) CConstraintNegation(mp, std::move(pcnstr));
}

//---------------------------------------------------------------------------
//	@function:
//		CConstraintNegation::Pcnstr
//
//	@doc:
//		Return constraint on a given column
//
//---------------------------------------------------------------------------
gpos::Ref<CConstraint>
CConstraintNegation::Pcnstr(CMemoryPool *mp, const CColRef *colref)
{
	if (!m_pcrsUsed->FMember(colref) || (1 != m_pcrsUsed->Size()))
	{
		// return NULL when the constraint:
		// 1) does not contain the column requested
		// 2) constraint may include other columns as well.
		// for instance, conjunction constraint (NOT a=b) is like:
		//       NOT ({"a" (0), ranges: (-inf, inf) } AND {"b" (1), ranges: (-inf, inf) }))
		// recursing down the constraint will give NOT ({"a" (0), ranges: (-inf, inf) })
		// but that is equivalent to (NOT a) which is not the case.

		return nullptr;
	}

	return GPOS_NEW(mp) CConstraintNegation(mp, m_pcnstr->Pcnstr(mp, colref));
}

//---------------------------------------------------------------------------
//	@function:
//		CConstraintNegation::Pcnstr
//
//	@doc:
//		Return constraint on a given column set
//
//---------------------------------------------------------------------------
gpos::Ref<CConstraint>
CConstraintNegation::Pcnstr(CMemoryPool *mp, CColRefSet *pcrs)
{
	if (!m_pcrsUsed->Equals(pcrs))
	{
		return nullptr;
	}

	return GPOS_NEW(mp) CConstraintNegation(mp, m_pcnstr->Pcnstr(mp, pcrs));
}

//---------------------------------------------------------------------------
//	@function:
//		CConstraintNegation::PcnstrRemapForColumn
//
//	@doc:
//		Return a copy of the constraint for a different column
//
//---------------------------------------------------------------------------
gpos::Ref<CConstraint>
CConstraintNegation::PcnstrRemapForColumn(CMemoryPool *mp,
										  CColRef *colref) const
{
	GPOS_ASSERT(1 == m_pcrsUsed->Size());

	return GPOS_NEW(mp)
		CConstraintNegation(mp, m_pcnstr->PcnstrRemapForColumn(mp, colref));
}

//---------------------------------------------------------------------------
//	@function:
//		CConstraintNegation::PexprScalar
//
//	@doc:
//		Scalar expression
//
//---------------------------------------------------------------------------
CExpression *
CConstraintNegation::PexprScalar(CMemoryPool *mp)
{
	if (nullptr == m_pexprScalar)
	{
		EConstraintType ect = m_pcnstr->Ect();
		if (EctNegation == ect)
		{
			CConstraintNegation *pcn =
				gpos::cast<CConstraintNegation>(m_pcnstr.get());
			m_pexprScalar = pcn->PcnstrChild()->PexprScalar(mp);
			;
		}
		else if (EctInterval == ect)
		{
			CConstraintInterval *pci =
				gpos::cast<CConstraintInterval>(m_pcnstr.get());
			gpos::Ref<CConstraintInterval> pciComp = pci->PciComplement(mp);
			m_pexprScalar = pciComp->PexprScalar(mp);
			;
			;
		}
		else
		{
			gpos::Ref<CExpression> pexpr = m_pcnstr->PexprScalar(mp);
			;
			m_pexprScalar = CUtils::PexprNegate(mp, pexpr);
		}
	}

	return m_pexprScalar.get();
}

//---------------------------------------------------------------------------
//	@function:
//		CConstraintNegation::OsPrint
//
//	@doc:
//		Debug print
//
//---------------------------------------------------------------------------
IOstream &
CConstraintNegation::OsPrint(IOstream &os) const
{
	os << "(NOT " << *m_pcnstr << ")";

	return os;
}

// EOF
