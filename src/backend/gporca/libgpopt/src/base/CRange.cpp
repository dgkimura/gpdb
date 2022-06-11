//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CRange.cpp
//
//	@doc:
//		Implementation of ranges
//---------------------------------------------------------------------------

#include "gpopt/base/CRange.h"

#include "gpos/base.h"
#include "gpos/common/owner.h"
#include "gpos/error/CAutoTrace.h"

#include "gpopt/base/CUtils.h"
#include "gpopt/base/IComparator.h"
#include "gpopt/operators/CPredicateUtils.h"
#include "naucrates/md/IMDScalarOp.h"

using namespace gpopt;

FORCE_GENERATE_DBGSTR(CRange);

//---------------------------------------------------------------------------
//	@function:
//		CRange::CRange
//
//	@doc:
//		Ctor
//		Does not take ownership of 'pcomp'.
//
//---------------------------------------------------------------------------
CRange::CRange(gpos::Ref<IMDId> mdid, const IComparator *pcomp,
			   gpos::Ref<IDatum> pdatumLeft, ERangeInclusion eriLeft,
			   gpos::Ref<IDatum> pdatumRight, ERangeInclusion eriRight)
	: m_mdid(std::move(mdid)),
	  m_pcomp(pcomp),
	  m_pdatumLeft(std::move(pdatumLeft)),
	  m_eriLeft(eriLeft),
	  m_pdatumRight(std::move(pdatumRight)),
	  m_eriRight(eriRight)
{
	GPOS_ASSERT(m_mdid->IsValid());
	GPOS_ASSERT(nullptr != pcomp);
	GPOS_ASSERT(CUtils::FConstrainableType(m_mdid.get()));
	GPOS_ASSERT_IMP(
		nullptr != m_pdatumLeft && nullptr != m_pdatumRight,
		pcomp->IsLessThanOrEqual(m_pdatumLeft.get(), m_pdatumRight.get()));
}

//---------------------------------------------------------------------------
//	@function:
//		CRange::CRange
//
//	@doc:
//		Ctor
//		Does not take ownership of 'pcomp'.
//
//---------------------------------------------------------------------------
CRange::CRange(const IComparator *pcomp, IMDType::ECmpType cmp_type,
			   IDatum *datum)
	: m_mdid(nullptr),
	  m_pcomp(pcomp),
	  m_pdatumLeft(nullptr),
	  m_eriLeft(EriExcluded),
	  m_pdatumRight(nullptr),
	  m_eriRight(EriExcluded)
{
	m_mdid = datum->MDId();

	GPOS_ASSERT(m_mdid->IsValid());
	GPOS_ASSERT(nullptr != pcomp);
	GPOS_ASSERT(CUtils::FConstrainableType(m_mdid.get()));
	;

	switch (cmp_type)
	{
		case IMDType::EcmptEq:
		{
			;
			m_pdatumLeft = datum;
			m_pdatumRight = datum;
			m_eriLeft = EriIncluded;
			m_eriRight = EriIncluded;
			break;
		}

		case IMDType::EcmptL:
		{
			m_pdatumRight = datum;
			break;
		}

		case IMDType::EcmptLEq:
		{
			m_pdatumRight = datum;
			m_eriRight = EriIncluded;
			break;
		}

		case IMDType::EcmptG:
		{
			m_pdatumLeft = datum;
			break;
		}

		case IMDType::EcmptGEq:
		{
			m_pdatumLeft = datum;
			m_eriLeft = EriIncluded;
			break;
		}

		default:
			// for anything else, create a (-inf, inf) range
			break;
	}
}

//---------------------------------------------------------------------------
//	@function:
//		CRange::~CRange
//
//	@doc:
//		Dtor
//
//---------------------------------------------------------------------------
CRange::~CRange()
{
	;
	;
	;
}

//---------------------------------------------------------------------------
//	@function:
//		CRange::FDisjointLeft
//
//	@doc:
//		Is this range disjoint from the given range and to its left
//
//---------------------------------------------------------------------------
BOOL
CRange::FDisjointLeft(CRange *prange)
{
	GPOS_ASSERT(nullptr != prange);

	IDatum *pdatumLeft = prange->PdatumLeft();

	if (nullptr == m_pdatumRight || nullptr == pdatumLeft)
	{
		return false;
	}

	if (m_pcomp->IsLessThan(m_pdatumRight.get(), pdatumLeft))
	{
		return true;
	}

	if (m_pcomp->Equals(m_pdatumRight.get(), pdatumLeft))
	{
		return (EriExcluded == m_eriRight || EriExcluded == prange->EriLeft());
	}

	return false;
}

//---------------------------------------------------------------------------
//	@function:
//		CRange::Contains
//
//	@doc:
//		Does this range contain the given range
//
//---------------------------------------------------------------------------
BOOL
CRange::Contains(CRange *prange)
{
	GPOS_ASSERT(nullptr != prange);

	return FStartsWithOrBefore(prange) && FEndsWithOrAfter(prange);
}

//---------------------------------------------------------------------------
//	@function:
//		CRange::FOverlapsLeft
//
//	@doc:
//		Does this range overlap only the left end of the given range
//
//---------------------------------------------------------------------------
BOOL
CRange::FOverlapsLeft(CRange *prange)
{
	GPOS_ASSERT(nullptr != prange);

	return (FStartsBefore(prange) && !FEndsAfter(prange) &&
			!FDisjointLeft(prange));
}

//---------------------------------------------------------------------------
//	@function:
//		CRange::FOverlapsRight
//
//	@doc:
//		Does this range overlap only the right end of the given range
//
//---------------------------------------------------------------------------
BOOL
CRange::FOverlapsRight(CRange *prange)
{
	GPOS_ASSERT(nullptr != prange);

	return (FEndsAfter(prange) && !FStartsBefore(prange) &&
			!prange->FDisjointLeft(this));
}

//---------------------------------------------------------------------------
//	@function:
//		CRange::FUpperBoundEqualsLowerBound
//
//	@doc:
//		Checks if this range's upper bound value is equal to the given range's
//		lower bound value. Ignores inclusivity/exclusivity Examples:
//			(-inf, 8)(8, inf)	true
//			(-inf, 8](8, inf)	true
//			(-inf, inf)(8, inf)	false
//
//---------------------------------------------------------------------------
BOOL
CRange::FUpperBoundEqualsLowerBound(CRange *prange)
{
	GPOS_ASSERT(nullptr != prange);

	IDatum *pdatumLeft = prange->PdatumLeft();

	if (nullptr == pdatumLeft && nullptr == m_pdatumRight)
	{
		return true;
	}

	if (nullptr == pdatumLeft || nullptr == m_pdatumRight)
	{
		return false;
	}

	return m_pcomp->Equals(m_pdatumRight.get(), pdatumLeft);
}

//---------------------------------------------------------------------------
//	@function:
//		CRange::FStartsWithOrBefore
//
//	@doc:
//		Does this range start with or before the given range
//
//---------------------------------------------------------------------------
BOOL
CRange::FStartsWithOrBefore(CRange *prange)
{
	if (FStartsBefore(prange))
	{
		return true;
	}

	IDatum *pdatumLeft = prange->PdatumLeft();
	if (nullptr == pdatumLeft && nullptr == m_pdatumLeft)
	{
		return true;
	}

	if (nullptr == pdatumLeft || nullptr == m_pdatumLeft)
	{
		return false;
	}

	return (m_pcomp->Equals(m_pdatumLeft.get(), pdatumLeft) &&
			m_eriLeft == prange->EriLeft());
}

//---------------------------------------------------------------------------
//	@function:
//		CRange::FStartsBefore
//
//	@doc:
//		Does this range start before the given range starts
//
//---------------------------------------------------------------------------
BOOL
CRange::FStartsBefore(CRange *prange)
{
	GPOS_ASSERT(nullptr != prange);

	IDatum *pdatumLeft = prange->PdatumLeft();
	if (nullptr == pdatumLeft)
	{
		return (nullptr == m_pdatumLeft);
	}

	if (nullptr == m_pdatumLeft ||
		m_pcomp->IsLessThan(m_pdatumLeft.get(), pdatumLeft))
	{
		return true;
	}

	if (m_pcomp->IsGreaterThan(m_pdatumLeft.get(), pdatumLeft))
	{
		return false;
	}

	GPOS_ASSERT(m_pcomp->Equals(m_pdatumLeft.get(), pdatumLeft));

	return (EriIncluded == m_eriLeft && EriExcluded == prange->EriLeft());
}

//---------------------------------------------------------------------------
//	@function:
//		CRange::FEndsAfter
//
//	@doc:
//		Does this range end after the given range ends
//
//---------------------------------------------------------------------------
BOOL
CRange::FEndsAfter(CRange *prange)
{
	GPOS_ASSERT(nullptr != prange);

	IDatum *pdatumRight = prange->PdatumRight();
	if (nullptr == pdatumRight)
	{
		return (nullptr == m_pdatumRight);
	}

	if (nullptr == m_pdatumRight ||
		m_pcomp->IsGreaterThan(m_pdatumRight.get(), pdatumRight))
	{
		return true;
	}

	if (m_pcomp->IsLessThan(m_pdatumRight.get(), pdatumRight))
	{
		return false;
	}

	GPOS_ASSERT(m_pcomp->Equals(m_pdatumRight.get(), pdatumRight));

	return (EriIncluded == m_eriRight && EriExcluded == prange->EriRight());
}

//---------------------------------------------------------------------------
//	@function:
//		CRange::FEndsWithOrAfter
//
//	@doc:
//		Does this range end with or after the given range
//
//---------------------------------------------------------------------------
BOOL
CRange::FEndsWithOrAfter(CRange *prange)
{
	if (FEndsAfter(prange))
	{
		return true;
	}

	IDatum *pdatumRight = prange->PdatumRight();
	if (nullptr == pdatumRight && nullptr == m_pdatumRight)
	{
		return true;
	}

	if (nullptr == pdatumRight || nullptr == m_pdatumRight)
	{
		return false;
	}

	return (m_pcomp->Equals(m_pdatumRight.get(), pdatumRight) &&
			m_eriRight == prange->EriRight());
}

//---------------------------------------------------------------------------
//	@function:
//		CRange::FPoint
//
//	@doc:
//		Is the range a point
//
//---------------------------------------------------------------------------
BOOL
CRange::FPoint() const
{
	return (EriIncluded == m_eriLeft && EriIncluded == m_eriRight &&
			m_pcomp->Equals(m_pdatumRight.get(), m_pdatumLeft.get()));
}

//---------------------------------------------------------------------------
//	@function:
//		CRange::PexprScalar
//
//	@doc:
//		Construct scalar comparison expression using given column
//
//---------------------------------------------------------------------------
gpos::Ref<CExpression>
CRange::PexprScalar(CMemoryPool *mp, const CColRef *colref)
{
	gpos::Ref<CExpression> pexprEq = PexprEquality(mp, colref);
	if (nullptr != pexprEq)
	{
		return pexprEq;
	}

	gpos::Ref<CExpression> pexprLeft =
		PexprScalarCompEnd(mp, m_pdatumLeft, m_eriLeft, IMDType::EcmptGEq,
						   IMDType::EcmptG, colref);

	gpos::Ref<CExpression> pexprRight =
		PexprScalarCompEnd(mp, m_pdatumRight, m_eriRight, IMDType::EcmptLEq,
						   IMDType::EcmptL, colref);

	gpos::Ref<CExpressionArray> pdrgpexpr = GPOS_NEW(mp) CExpressionArray(mp);

	if (nullptr != pexprLeft)
	{
		pdrgpexpr->Append(pexprLeft);
	}

	if (nullptr != pexprRight)
	{
		pdrgpexpr->Append(pexprRight);
	}

	return CPredicateUtils::PexprConjunction(mp, std::move(pdrgpexpr));
}

//---------------------------------------------------------------------------
//	@function:
//		CRange::PexprEquality
//
//	@doc:
//		Construct an equality predicate if possible
//
//---------------------------------------------------------------------------
gpos::Ref<CExpression>
CRange::PexprEquality(CMemoryPool *mp, const CColRef *colref)
{
	if (nullptr == m_pdatumLeft || nullptr == m_pdatumRight ||
		!m_pcomp->Equals(m_pdatumLeft.get(), m_pdatumRight.get()) ||
		EriExcluded == m_eriLeft || EriExcluded == m_eriRight)
	{
		// not an equality predicate
		return nullptr;
	}

	;
	gpos::Ref<CExpression> pexprVal = GPOS_NEW(mp)
		CExpression(mp, GPOS_NEW(mp) CScalarConst(mp, m_pdatumLeft));

	return CUtils::PexprScalarCmp(mp, colref, std::move(pexprVal),
								  IMDType::EcmptEq);
}

//---------------------------------------------------------------------------
//	@function:
//		CRange::PexprScalarCompEnd
//
//	@doc:
//		Construct a scalar comparison expression from one of the ends
//
//---------------------------------------------------------------------------
gpos::Ref<CExpression>
CRange::PexprScalarCompEnd(CMemoryPool *mp, IDatum *datum, ERangeInclusion eri,
						   IMDType::ECmpType ecmptIncl,
						   IMDType::ECmpType ecmptExcl, const CColRef *colref)
{
	if (nullptr == datum)
	{
		// unbounded end
		return nullptr;
	}

	;
	gpos::Ref<CExpression> pexprVal =
		GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CScalarConst(mp, datum));

	IMDType::ECmpType cmp_type;
	if (EriIncluded == eri)
	{
		cmp_type = ecmptIncl;
	}
	else
	{
		cmp_type = ecmptExcl;
	}

	return CUtils::PexprScalarCmp(mp, colref, std::move(pexprVal), cmp_type);
}

//---------------------------------------------------------------------------
//	@function:
//		CRange::PrngIntersect
//
//	@doc:
//		Intersection with another range
//
//---------------------------------------------------------------------------
gpos::Ref<CRange>
CRange::PrngIntersect(CMemoryPool *mp, CRange *prange)
{
	if (Contains(prange))
	{
		;
		return prange;
	}

	if (prange->Contains(this))
	{
		;
		return this;
	}

	if (FOverlapsLeft(prange))
	{
		;

		gpos::Ref<IDatum> pdatumLeft = prange->PdatumLeft();
		;
		;

		return GPOS_NEW(mp)
			CRange(m_mdid, m_pcomp, std::move(pdatumLeft), prange->EriLeft(),
				   m_pdatumRight, m_eriRight);
	}

	if (FOverlapsRight(prange))
	{
		;

		gpos::Ref<IDatum> pdatumRight = prange->PdatumRight();
		;
		;

		return GPOS_NEW(mp) CRange(m_mdid, m_pcomp, m_pdatumLeft, m_eriLeft,
								   std::move(pdatumRight), prange->EriRight());
	}

	return nullptr;
}

//---------------------------------------------------------------------------
//	@function:
//		CRange::PrngDifferenceLeft
//
//	@doc:
//		Difference between this range and a given range on the left side only
//
//		this    |----------------------|
//		prange         |-----------|
//		result  |------|
//---------------------------------------------------------------------------
gpos::Ref<CRange>
CRange::PrngDifferenceLeft(CMemoryPool *mp, CRange *prange)
{
	if (FDisjointLeft(prange))
	{
		;
		return this;
	}

	if (nullptr != prange->PdatumLeft() && FStartsBefore(prange))
	{
		;

		if (nullptr != m_pdatumLeft)
		{
			;
		}

		gpos::Ref<IDatum> pdatumRight = prange->PdatumLeft();
		;

		return GPOS_NEW(mp) CRange(m_mdid, m_pcomp, m_pdatumLeft, m_eriLeft,
								   std::move(pdatumRight),
								   EriInverseInclusion(prange->EriLeft()));
	}

	return nullptr;
}

//---------------------------------------------------------------------------
//	@function:
//		CRange::PrngDifferenceRight
//
//	@doc:
//		Difference between this range and a given range on the right side only
//
//		this    |----------------------|
//		prange      |-----------|
//		result                  |------|
//---------------------------------------------------------------------------
gpos::Ref<CRange>
CRange::PrngDifferenceRight(CMemoryPool *mp, CRange *prange)
{
	if (prange->FDisjointLeft(this))
	{
		;
		return this;
	}

	if (nullptr != prange->PdatumRight() && FEndsAfter(prange))
	{
		;

		if (nullptr != m_pdatumRight)
		{
			;
		}

		gpos::Ref<IDatum> pdatumRight = prange->PdatumRight();
		;

		return GPOS_NEW(mp) CRange(m_mdid, m_pcomp, std::move(pdatumRight),
								   EriInverseInclusion(prange->EriRight()),
								   m_pdatumRight, m_eriRight);
	}

	return nullptr;
}

//---------------------------------------------------------------------------
//	@function:
//		CRange::PrngExtend
//
//	@doc:
//		Return the extension of this range with the given range. The given range
//		must start right after this range, otherwise NULL is returned
//
//---------------------------------------------------------------------------
gpos::Ref<CRange>
CRange::PrngExtend(CMemoryPool *mp, CRange *prange)
{
	if ((EriIncluded == prange->EriLeft() || EriIncluded == m_eriRight) &&
		(m_pcomp->Equals(prange->PdatumLeft(), m_pdatumRight.get())))
	{
		// ranges are contiguous so combine them into one
		;

		if (nullptr != m_pdatumLeft)
		{
			;
		}

		IDatum *pdatumRight = prange->PdatumRight();
		if (nullptr != pdatumRight)
		{
			;
		}

		return GPOS_NEW(mp) CRange(m_mdid, m_pcomp, m_pdatumLeft, m_eriLeft,
								   pdatumRight, prange->EriRight());
	}

	return nullptr;
}

//---------------------------------------------------------------------------
//	@function:
//		CRange::OsPrint
//
//	@doc:
//		debug print
//
//---------------------------------------------------------------------------
IOstream &
CRange::OsPrint(IOstream &os) const
{
	if (EriIncluded == m_eriLeft)
	{
		os << "[";
	}
	else
	{
		os << "(";
	}

	OsPrintBound(os, m_pdatumLeft.get(), "-inf");
	os << ", ";
	OsPrintBound(os, m_pdatumRight.get(), "inf");

	if (EriIncluded == m_eriRight)
	{
		os << "]";
	}
	else
	{
		os << ")";
	}

	return os;
}

//---------------------------------------------------------------------------
//	@function:
//		CRange::OsPrintPoint
//
//	@doc:
//		debug print a point
//
//---------------------------------------------------------------------------
IOstream &
CRange::OsPrintBound(IOstream &os, IDatum *datum, const CHAR *szInfinity)
{
	if (nullptr == datum)
	{
		os << szInfinity;
	}
	else
	{
		datum->OsPrint(os);
	}

	return os;
}

// EOF
