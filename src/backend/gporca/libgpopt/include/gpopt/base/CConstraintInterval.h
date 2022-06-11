//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CConstraintInterval.h
//
//	@doc:
//		Representation of an interval constraint. An interval contains a number
//		of ranges + "is null" and "is not null" flags. The interval can be interpreted
//		as the ORing of the ranges and the flags that are set
//---------------------------------------------------------------------------
#ifndef GPOPT_CConstraintInterval_H
#define GPOPT_CConstraintInterval_H

#include "gpos/base.h"
#include "gpos/common/owner.h"
#include "gpos/error/CAutoTrace.h"
#include "gpos/task/CAutoTraceFlag.h"

#include "gpopt/base/CConstraint.h"
#include "gpopt/base/CRange.h"
#include "gpopt/operators/CScalarArrayCmp.h"
#include "gpopt/operators/CScalarConst.h"
#include "naucrates/dxl/xml/dxltokens.h"
#include "naucrates/traceflags/traceflags.h"

namespace gpopt
{
// range array
using CRangeArray = CDynamicPtrArray<CRange, CleanupRelease>;

using namespace gpos;
using namespace gpmd;

//---------------------------------------------------------------------------
//	@class:
//		CConstraintInterval
//
//	@doc:
//		Representation of an interval constraint
//
//		If x has a CConstraintInterval C on it, this means that x is in the
//		ranges contained in C.
//
//---------------------------------------------------------------------------
class CConstraintInterval : public CConstraint
{
private:
	// column referenced in this constraint
	const CColRef *m_pcr;

	// array of ranges
	gpos::owner<CRangeArray *> m_pdrgprng;

	// does the interval include the null value
	BOOL m_fIncludesNull;

	// adds ranges from a source array to a destination array, starting
	// at the range with the given index
	static void AddRemainingRanges(CMemoryPool *mp,
								   gpos::pointer<CRangeArray *> pdrgprngSrc,
								   ULONG ulStart,
								   gpos::pointer<CRangeArray *> pdrgprngDest);

	// append the given range to the array or extend the last element
	static void AppendOrExtend(CMemoryPool *mp,
							   gpos::pointer<CRangeArray *> pdrgprng,
							   gpos::owner<CRange *> prange);

	// difference between two ranges on the left side only -
	// any difference on the right side is reported as residual range
	static gpos::owner<CRange *> PrangeDiffWithRightResidual(
		CMemoryPool *mp, gpos::pointer<CRange *> prangeFirst,
		gpos::pointer<CRange *> prangeSecond, CRange **pprangeResidual,
		gpos::pointer<CRangeArray *> pdrgprngResidual);

	// type of this interval
	IMDId *MdidType();

	// construct scalar expression
	virtual gpos::owner<CExpression *> PexprConstructScalar(
		CMemoryPool *mp) const;

	virtual gpos::owner<CExpression *> PexprConstructArrayScalar(
		CMemoryPool *mp) const;

	// create interval from scalar comparison expression
	static gpos::owner<CConstraintInterval *> PciIntervalFromScalarCmp(
		CMemoryPool *mp, gpos::pointer<CExpression *> pexpr, CColRef *colref,
		BOOL infer_nulls_as = false);

	static gpos::owner<CConstraintInterval *> PciIntervalFromScalarIDF(
		CMemoryPool *mp, gpos::pointer<CExpression *> pexpr, CColRef *colref);

	// create interval from scalar bool operator
	static gpos::owner<CConstraintInterval *> PciIntervalFromScalarBoolOp(
		CMemoryPool *mp, gpos::pointer<CExpression *> pexpr, CColRef *colref,
		BOOL infer_nulls_as = false);

	// create interval from scalar bool AND
	static gpos::owner<CConstraintInterval *> PciIntervalFromScalarBoolAnd(
		CMemoryPool *mp, gpos::pointer<CExpression *> pexpr, CColRef *colref,
		BOOL infer_nulls_as = false);

	// create interval from scalar bool OR
	static gpos::owner<CConstraintInterval *> PciIntervalFromScalarBoolOr(
		CMemoryPool *mp, gpos::pointer<CExpression *> pexpr, CColRef *colref,
		BOOL infer_nulls_as = false);

	// create interval from scalar null test
	static gpos::owner<CConstraintInterval *> PciIntervalFromScalarNullTest(
		CMemoryPool *mp, gpos::pointer<CExpression *> pexpr, CColRef *colref);

	// creates a range like [x,x] where x is a constant
	static gpos::owner<CRangeArray *> PciRangeFromColConstCmp(
		CMemoryPool *mp, IMDType::ECmpType cmp_type,
		gpos::pointer<const CScalarConst *> popScConst);

	// create an array IN or NOT IN expression
	gpos::owner<CExpression *> PexprConstructArrayScalar(CMemoryPool *mp,
														 bool isIn) const;

public:
	CConstraintInterval(const CConstraintInterval &) = delete;

	// ctor
	CConstraintInterval(CMemoryPool *mp, const CColRef *colref,
						gpos::owner<CRangeArray *> pdrgprng, BOOL is_null);

	// dtor
	~CConstraintInterval() override;

	// constraint type accessor
	EConstraintType
	Ect() const override
	{
		return CConstraint::EctInterval;
	}

	// column referenced in constraint
	const CColRef *
	Pcr() const
	{
		return m_pcr;
	}

	// all ranges in interval
	gpos::pointer<CRangeArray *>
	Pdrgprng() const
	{
		return m_pdrgprng;
	}

	// does the interval include the null value
	BOOL
	FIncludesNull() const
	{
		return m_fIncludesNull;
	}

	// is this constraint a contradiction
	BOOL FContradiction() const override;

	// is this interval unbounded
	BOOL IsConstraintUnbounded() const override;

	// check if there is a constraint on the given column
	BOOL
	FConstraint(const CColRef *colref) const override
	{
		return m_pcr == colref;
	}

	// check if constraint is on the gp_segment_id column
	BOOL
	FConstraintOnSegmentId() const override
	{
		return m_pcr->IsSystemCol() &&
			   m_pcr->Name().Equals(
				   CDXLTokens::GetDXLTokenStr(EdxltokenGpSegmentIdColName));
	}

	// return a copy of the constraint with remapped columns
	gpos::owner<CConstraint *> PcnstrCopyWithRemappedColumns(
		CMemoryPool *mp, gpos::pointer<UlongToColRefMap *> colref_mapping,
		BOOL must_exist) override;

	// interval intersection
	gpos::owner<CConstraintInterval *> PciIntersect(
		CMemoryPool *mp, gpos::pointer<CConstraintInterval *> pci);

	// interval union
	gpos::owner<CConstraintInterval *> PciUnion(
		CMemoryPool *mp, gpos::pointer<CConstraintInterval *> pci);

	// interval difference
	gpos::owner<CConstraintInterval *> PciDifference(
		CMemoryPool *mp, gpos::pointer<CConstraintInterval *> pci);

	// interval complement
	gpos::owner<CConstraintInterval *> PciComplement(CMemoryPool *mp);

	// does the current interval contain the given interval?
	BOOL FContainsInterval(CMemoryPool *mp,
						   gpos::pointer<CConstraintInterval *> pci);

	// scalar expression
	gpos::pointer<CExpression *> PexprScalar(CMemoryPool *mp) override;

	// scalar expression  which will be a disjunction
	gpos::owner<CExpression *> PexprConstructDisjunctionScalar(
		CMemoryPool *mp) const;

	// return constraint on a given column
	gpos::owner<CConstraint *> Pcnstr(CMemoryPool *mp,
									  const CColRef *colref) override;

	// return constraint on a given column set
	gpos::owner<CConstraint *> Pcnstr(
		CMemoryPool *mp, gpos::pointer<CColRefSet *> pcrs) override;

	// return a clone of the constraint for a different column
	gpos::owner<CConstraint *> PcnstrRemapForColumn(
		CMemoryPool *mp, CColRef *colref) const override;

	// converts to an array in expression
	bool FConvertsToNotIn() const;

	// converts to an array not in expression
	bool FConvertsToIn() const;

	// print
	IOstream &OsPrint(IOstream &os) const override;

	// create unbounded interval
	static gpos::owner<CConstraintInterval *> PciUnbounded(
		CMemoryPool *mp, const CColRef *colref, BOOL fIncludesNull);

	// create an unbounded interval on any column from the given set
	static gpos::owner<CConstraintInterval *> PciUnbounded(
		CMemoryPool *mp, gpos::pointer<const CColRefSet *> pcrs,
		BOOL fIncludesNull);

	// helper for create interval from comparison between a column and a constant
	static gpos::owner<CConstraintInterval *> PciIntervalFromColConstCmp(
		CMemoryPool *mp, CColRef *colref, IMDType::ECmpType cmp_type,
		gpos::pointer<CScalarConst *> popScConst, BOOL infer_nulls_as = false);

	// create interval from scalar expression
	static gpos::owner<CConstraintInterval *> PciIntervalFromScalarExpr(
		CMemoryPool *mp, gpos::pointer<CExpression *> pexpr, CColRef *colref,
		BOOL infer_nulls_as = false);

	// create interval from any general constraint that references
	// only one column
	static gpos::owner<CConstraintInterval *> PciIntervalFromConstraint(
		CMemoryPool *mp, gpos::pointer<CConstraint *> pcnstr,
		CColRef *colref = nullptr);

	// generate a ConstraintInterval from the given expression
	static gpos::owner<CConstraintInterval *> PcnstrIntervalFromScalarArrayCmp(
		CMemoryPool *mp, gpos::pointer<CExpression *> pexpr, CColRef *colref,
		BOOL infer_nulls_as = false);

};	// class CConstraintInterval

// shorthand for printing, reference
inline IOstream &
operator<<(IOstream &os, const CConstraintInterval &interval)
{
	return interval.OsPrint(os);
}

// shorthand for printing, pointer
inline IOstream &
operator<<(IOstream &os, gpos::pointer<const CConstraintInterval *> interval)
{
	return interval->OsPrint(os);
}

using CConstraintIntervalArray =
	CDynamicPtrArray<CConstraintInterval, CleanupRelease>;
}  // namespace gpopt

#endif	// !GPOPT_CConstraintInterval_H

// EOF
