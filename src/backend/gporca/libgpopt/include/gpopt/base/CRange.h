//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CRange.h
//
//	@doc:
//		Representation of a range of values
//---------------------------------------------------------------------------
#ifndef GPOPT_CRange_H
#define GPOPT_CRange_H

#include "gpos/base.h"
#include "gpos/common/CRefCount.h"
#include "gpos/common/owner.h"
#include "gpos/types.h"

#include "gpopt/base/CColRef.h"
#include "gpopt/operators/CExpression.h"
#include "naucrates/md/IMDType.h"

namespace gpnaucrates
{
// fwd declarations
class IDatum;
}  // namespace gpnaucrates

namespace gpopt
{
using namespace gpos;
using namespace gpmd;
using namespace gpnaucrates;

//fwd declarations
class CExpression;
class CRange;
class IComparator;

//---------------------------------------------------------------------------
//	@class:
//		CRange
//
//	@doc:
//		Representation of a range of values
//
//---------------------------------------------------------------------------
class CRange : public CRefCount<CRange>, public DbgPrintMixin<CRange>
{
public:
	enum ERangeInclusion
	{
		EriIncluded,
		EriExcluded,
		EriSentinel
	};

private:
	// range type
	gpos::Ref<IMDId> m_mdid;

	// datum comparator
	const IComparator *m_pcomp;

	// left end point, NULL if infinite
	gpos::Ref<IDatum> m_pdatumLeft;

	// inclusion option for left end
	ERangeInclusion m_eriLeft;

	// right end point, NULL if infinite
	gpos::Ref<IDatum> m_pdatumRight;

	// inclusion option for right end
	ERangeInclusion m_eriRight;

	// construct an equality predicate if possible
	gpos::Ref<CExpression> PexprEquality(CMemoryPool *mp,
										 const CColRef *colref);

	// construct a scalar comparison expression from one of the ends
	static gpos::Ref<CExpression> PexprScalarCompEnd(
		CMemoryPool *mp, gpos::Ref<IDatum> datum, ERangeInclusion eri,
		IMDType::ECmpType ecmptIncl, IMDType::ECmpType ecmptExcl,
		const CColRef *colref);

	// inverse of the inclusion option
	static ERangeInclusion
	EriInverseInclusion(ERangeInclusion eri)
	{
		if (EriIncluded == eri)
		{
			return EriExcluded;
		}
		return EriIncluded;
	}

	// print a bound
	static IOstream &OsPrintBound(IOstream &os, gpos::Ref<IDatum> datum,
								  const CHAR *szInfinity);

public:
	CRange(const CRange &) = delete;

	// ctor
	CRange(gpos::Ref<IMDId> mdid, const IComparator *pcomp,
		   gpos::Ref<IDatum> pdatumLeft, ERangeInclusion eriLeft,
		   gpos::Ref<IDatum> pdatumRight, ERangeInclusion eriRight);

	// ctor
	CRange(const IComparator *pcomp, IMDType::ECmpType cmp_type,
		   gpos::Ref<IDatum> datum);

	// dtor
	~CRange() override;

	// range type
	gpos::Ref<IMDId>
	MDId() const
	{
		return m_mdid;
	}

	// range beginning
	gpos::Ref<IDatum>
	PdatumLeft() const
	{
		return m_pdatumLeft.get();
	}

	// range end
	gpos::Ref<IDatum>
	PdatumRight() const
	{
		return m_pdatumRight.get();
	}

	// left end inclusion
	ERangeInclusion
	EriLeft() const
	{
		return m_eriLeft;
	}

	// right end inclusion
	ERangeInclusion
	EriRight() const
	{
		return m_eriRight;
	}

	// is this range disjoint from the given range and to its left
	BOOL FDisjointLeft(gpos::Ref<CRange> prange);

	// does this range contain the given range
	BOOL Contains(gpos::Ref<CRange> prange);

	// does this range overlap only the left end of the given range
	BOOL FOverlapsLeft(gpos::Ref<CRange> prange);

	// does this range overlap only the right end of the given range
	BOOL FOverlapsRight(gpos::Ref<CRange> prange);

	// does this range's upper bound equal the given range's lower bound
	BOOL FUpperBoundEqualsLowerBound(gpos::Ref<CRange> prange);

	// does this range start before the given range starts
	BOOL FStartsBefore(gpos::Ref<CRange> prange);

	// does this range start with or before the given range
	BOOL FStartsWithOrBefore(gpos::Ref<CRange> prange);

	// does this range end after the given range ends
	BOOL FEndsAfter(gpos::Ref<CRange> prange);

	// does this range end with or after the given range
	BOOL FEndsWithOrAfter(gpos::Ref<CRange> prange);

	// check if range represents a point
	BOOL FPoint() const;

	// intersection with another range
	gpos::Ref<CRange> PrngIntersect(CMemoryPool *mp, gpos::Ref<CRange> prange);

	// difference between this range and a given range on the left side only
	gpos::Ref<CRange> PrngDifferenceLeft(CMemoryPool *mp,
										 gpos::Ref<CRange> prange);

	// difference between this range and a given range on the right side only
	gpos::Ref<CRange> PrngDifferenceRight(CMemoryPool *mp,
										  gpos::Ref<CRange> prange);

	// return the extension of this range with the given range. The given
	// range must start right after this range, otherwise NULL is returned
	gpos::Ref<CRange> PrngExtend(CMemoryPool *mp, gpos::Ref<CRange> prange);

	// construct scalar expression
	gpos::Ref<CExpression> PexprScalar(CMemoryPool *mp, const CColRef *colref);

	// is this interval unbounded
	BOOL
	IsConstraintUnbounded() const
	{
		return (nullptr == m_pdatumLeft && nullptr == m_pdatumRight);
	}

	// print
	IOstream &OsPrint(IOstream &os) const;

};	// class CRange

// shorthand for printing
inline IOstream &
operator<<(IOstream &os, CRange &range)
{
	return range.OsPrint(os);
}
}  // namespace gpopt

#endif	// !GPOPT_CRange_H

// EOF
