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
using CRangeArray = gpos::Vector<gpos::Ref<CRange>>;

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
	gpos::Ref<CRangeArray> m_pdrgprng;

	// does the interval include the null value
	BOOL m_fIncludesNull;

	// adds ranges from a source array to a destination array, starting
	// at the range with the given index
	static void AddRemainingRanges(CMemoryPool *mp, CRangeArray *pdrgprngSrc,
								   ULONG ulStart, CRangeArray *pdrgprngDest);

	// append the given range to the array or extend the last element
	static void AppendOrExtend(CMemoryPool *mp, CRangeArray *pdrgprng,
							   gpos::Ref<CRange> prange);

	// difference between two ranges on the left side only -
	// any difference on the right side is reported as residual range
	static gpos::Ref<CRange> PrangeDiffWithRightResidual(
		CMemoryPool *mp, gpos::Ref<CRange> prangeFirst,
		gpos::Ref<CRange> prangeSecond, gpos::Ref<CRange> *pprangeResidual,
		CRangeArray *pdrgprngResidual);

	// type of this interval
	gpos::Ref<IMDId> MdidType();

	// construct scalar expression
	virtual gpos::Ref<CExpression> PexprConstructScalar(CMemoryPool *mp) const;

	virtual gpos::Ref<CExpression> PexprConstructArrayScalar(
		CMemoryPool *mp) const;

	// create interval from scalar comparison expression
	static gpos::Ref<CConstraintInterval> PciIntervalFromScalarCmp(
		CMemoryPool *mp, gpos::Ref<CExpression> pexpr, CColRef *colref,
		BOOL infer_nulls_as = false);

	static gpos::Ref<CConstraintInterval> PciIntervalFromScalarIDF(
		CMemoryPool *mp, gpos::Ref<CExpression> pexpr, CColRef *colref);

	// create interval from scalar bool operator
	static gpos::Ref<CConstraintInterval> PciIntervalFromScalarBoolOp(
		CMemoryPool *mp, gpos::Ref<CExpression> pexpr, CColRef *colref,
		BOOL infer_nulls_as = false);

	// create interval from scalar bool AND
	static gpos::Ref<CConstraintInterval> PciIntervalFromScalarBoolAnd(
		CMemoryPool *mp, gpos::Ref<CExpression> pexpr, CColRef *colref,
		BOOL infer_nulls_as = false);

	// create interval from scalar bool OR
	static gpos::Ref<CConstraintInterval> PciIntervalFromScalarBoolOr(
		CMemoryPool *mp, gpos::Ref<CExpression> pexpr, CColRef *colref,
		BOOL infer_nulls_as = false);

	// create interval from scalar null test
	static gpos::Ref<CConstraintInterval> PciIntervalFromScalarNullTest(
		CMemoryPool *mp, gpos::Ref<CExpression> pexpr, CColRef *colref);

	// creates a range like [x,x] where x is a constant
	static gpos::Ref<CRangeArray> PciRangeFromColConstCmp(
		CMemoryPool *mp, IMDType::ECmpType cmp_type,
		const gpos::Ref<CScalarConst> popScConst);

	// create an array IN or NOT IN expression
	gpos::Ref<CExpression> PexprConstructArrayScalar(CMemoryPool *mp,
													 bool isIn) const;

public:
	CConstraintInterval(const CConstraintInterval &) = delete;

	// ctor
	CConstraintInterval(CMemoryPool *mp, const CColRef *colref,
						gpos::Ref<CRangeArray> pdrgprng, BOOL is_null);

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
	CRangeArray *
	Pdrgprng() const
	{
		return m_pdrgprng.get();
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
	gpos::Ref<CConstraint> PcnstrCopyWithRemappedColumns(
		CMemoryPool *mp, gpos::Ref<UlongToColRefMap> colref_mapping,
		BOOL must_exist) override;

	// interval intersection
	gpos::Ref<CConstraintInterval> PciIntersect(
		CMemoryPool *mp, gpos::Ref<CConstraintInterval> pci);

	// interval union
	gpos::Ref<CConstraintInterval> PciUnion(CMemoryPool *mp,
											gpos::Ref<CConstraintInterval> pci);

	// interval difference
	gpos::Ref<CConstraintInterval> PciDifference(
		CMemoryPool *mp, gpos::Ref<CConstraintInterval> pci);

	// interval complement
	gpos::Ref<CConstraintInterval> PciComplement(CMemoryPool *mp);

	// does the current interval contain the given interval?
	BOOL FContainsInterval(CMemoryPool *mp, gpos::Ref<CConstraintInterval> pci);

	// scalar expression
	gpos::Ref<CExpression> PexprScalar(CMemoryPool *mp) override;

	// scalar expression  which will be a disjunction
	gpos::Ref<CExpression> PexprConstructDisjunctionScalar(
		CMemoryPool *mp) const;

	// return constraint on a given column
	gpos::Ref<CConstraint> Pcnstr(CMemoryPool *mp,
								  const CColRef *colref) override;

	// return constraint on a given column set
	gpos::Ref<CConstraint> Pcnstr(CMemoryPool *mp,
								  gpos::Ref<CColRefSet> pcrs) override;

	// return a clone of the constraint for a different column
	gpos::Ref<CConstraint> PcnstrRemapForColumn(CMemoryPool *mp,
												CColRef *colref) const override;

	// converts to an array in expression
	bool FConvertsToNotIn() const;

	// converts to an array not in expression
	bool FConvertsToIn() const;

	// print
	IOstream &OsPrint(IOstream &os) const override;

	// create unbounded interval
	static gpos::Ref<CConstraintInterval> PciUnbounded(CMemoryPool *mp,
													   const CColRef *colref,
													   BOOL fIncludesNull);

	// create an unbounded interval on any column from the given set
	static gpos::Ref<CConstraintInterval> PciUnbounded(
		CMemoryPool *mp, const gpos::Ref<CColRefSet> pcrs, BOOL fIncludesNull);

	// helper for create interval from comparison between a column and a constant
	static gpos::Ref<CConstraintInterval> PciIntervalFromColConstCmp(
		CMemoryPool *mp, CColRef *colref, IMDType::ECmpType cmp_type,
		gpos::Ref<CScalarConst> popScConst, BOOL infer_nulls_as = false);

	// create interval from scalar expression
	static gpos::Ref<CConstraintInterval> PciIntervalFromScalarExpr(
		CMemoryPool *mp, gpos::Ref<CExpression> pexpr, CColRef *colref,
		BOOL infer_nulls_as = false);

	// create interval from any general constraint that references
	// only one column
	static gpos::Ref<CConstraintInterval> PciIntervalFromConstraint(
		CMemoryPool *mp, gpos::Ref<CConstraint> pcnstr,
		CColRef *colref = nullptr);

	// generate a ConstraintInterval from the given expression
	static gpos::Ref<CConstraintInterval> PcnstrIntervalFromScalarArrayCmp(
		CMemoryPool *mp, gpos::Ref<CExpression> pexpr, CColRef *colref,
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
operator<<(IOstream &os, const gpos::Ref<CConstraintInterval> interval)
{
	return interval->OsPrint(os);
}

using CConstraintIntervalArray = gpos::Vector<gpos::Ref<CConstraintInterval>>;
}  // namespace gpopt

#endif	// !GPOPT_CConstraintInterval_H

// EOF
