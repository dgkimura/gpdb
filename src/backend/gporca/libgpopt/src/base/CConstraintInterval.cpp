//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CConstraintInterval.cpp
//
//	@doc:
//		Implementation of interval constraints
//---------------------------------------------------------------------------

#include "gpopt/base/CConstraintInterval.h"

#include "gpos/base.h"
#include "gpos/common/CAutoRef.h"
#include "gpos/common/owner.h"

#include "gpopt/base/CCastUtils.h"
#include "gpopt/base/CColRefSetIter.h"
#include "gpopt/base/CConstraintDisjunction.h"
#include "gpopt/base/CDatumSortedSet.h"
#include "gpopt/base/CDefaultComparator.h"
#include "gpopt/base/COptCtxt.h"
#include "gpopt/base/CUtils.h"
#include "gpopt/operators/CPredicateUtils.h"
#include "gpopt/operators/CScalarArray.h"
#include "gpopt/operators/CScalarIdent.h"
#include "gpopt/operators/CScalarIsDistinctFrom.h"
#include "naucrates/md/IMDScalarOp.h"

using namespace gpopt;

//---------------------------------------------------------------------------
//	@function:
//		CConstraintInterval::CConstraintInterval
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CConstraintInterval::CConstraintInterval(CMemoryPool *mp, const CColRef *colref,
										 gpos::Ref<CRangeArray> pdrgprng,
										 BOOL fIncludesNull)
	: CConstraint(mp, GPOS_NEW(mp) CColRefSet(mp)),
	  m_pcr(colref),
	  m_pdrgprng(std::move(pdrgprng)),
	  m_fIncludesNull(fIncludesNull)
{
	GPOS_ASSERT(nullptr != colref);
	GPOS_ASSERT(nullptr != m_pdrgprng);
	m_pcrsUsed->Include(colref);
}

//---------------------------------------------------------------------------
//	@function:
//		CConstraintInterval::~CConstraintInterval
//
//	@doc:
//		Dtor
//
//---------------------------------------------------------------------------
CConstraintInterval::~CConstraintInterval()
{
	;
}

//---------------------------------------------------------------------------
//	@function:
//		CConstraintInterval::FContradiction
//
//	@doc:
//		Check if this constraint is a contradiction. An interval is a contradiction if
//		it has no ranges and the null flag is not set
//
//---------------------------------------------------------------------------
BOOL
CConstraintInterval::FContradiction() const
{
	return (!m_fIncludesNull && 0 == m_pdrgprng->Size());
}

//---------------------------------------------------------------------------
//	@function:
//		CConstraintInterval::IsConstraintUnbounded
//
//	@doc:
//		Check if this interval is unbounded. An interval is unbounded if
//		it has a (-inf, inf) range and the null flag is set
//
//---------------------------------------------------------------------------
BOOL
CConstraintInterval::IsConstraintUnbounded() const
{
	return (m_fIncludesNull && 1 == m_pdrgprng->Size() &&
			(*m_pdrgprng)[0]->IsConstraintUnbounded());
}

//---------------------------------------------------------------------------
//	@function:
//		CConstraintInterval::PcnstrCopyWithRemappedColumns
//
//	@doc:
//		Return a copy of the constraint with remapped columns
//
//---------------------------------------------------------------------------
gpos::Ref<CConstraint>
CConstraintInterval::PcnstrCopyWithRemappedColumns(
	CMemoryPool *mp, UlongToColRefMap *colref_mapping, BOOL must_exist)
{
	CColRef *colref = CUtils::PcrRemap(m_pcr, colref_mapping, must_exist);
	return PcnstrRemapForColumn(mp, colref);
}

//---------------------------------------------------------------------------
//	@function:
//		CConstraintInterval::PciIntervalFromScalarExpr
//
//	@doc:
//		Create interval from scalar expression
//
//		For a given expression pexpr on colref colref, return the CConstraintInterval
//		for which pexpr
//		- evaluates to true (if infer_null_as is false).
//		  This is used for WHERE predicates, which return a row only if the predicate is true.
//		- evaluates to true or null (if infer_null_as is set to true).
//		  This is used for constraints, which are satisfied if the predicate is true or null.
//
//		Let's call the function result r(pexpr) when infer_null_as is set to false,
//		and r'(pexpr) when infer_null_as is set to true. The table below shows how we
//		calculate the intervals for boolean operations AND, OR and NOT:
//
//		Range of a			Equivalent				Comment
//		Boolean expression	expression
//		------------------	---------------------	--------------------------------------------------------
//		r(x and y)			r(x) intersect r(y)		Both x and y must be true for a value of c to qualify
//		r(x or y)			r(x) union r(y)			One of x or y must be true for a value of c to qualify
//		r(not x)			complement(r’(x))		x must be false
//		r’(x and y)			r’(x) intersect r’(y)	Both x and y must not be false
//		r’(x or y)			r'(x) union r'(y)		At least one of x and y must not be false
//		r’(not x)			complement (r(x))		x must not be true
//
//---------------------------------------------------------------------------
gpos::Ref<CConstraintInterval>
CConstraintInterval::PciIntervalFromScalarExpr(CMemoryPool *mp,
											   CExpression *pexpr,
											   CColRef *colref,
											   BOOL infer_nulls_as)
{
	GPOS_ASSERT(nullptr != pexpr);
	GPOS_ASSERT(pexpr->Pop()->FScalar());

	// expression must use at most one column
	GPOS_ASSERT(1 >= pexpr->DeriveUsedColumns()->Size());
	gpos::Ref<CConstraintInterval> pci = nullptr;
	switch (pexpr->Pop()->Eopid())
	{
		case COperator::EopScalarNullTest:
			pci = PciIntervalFromScalarNullTest(mp, pexpr, colref);
			break;
		case COperator::EopScalarBoolOp:
			pci =
				PciIntervalFromScalarBoolOp(mp, pexpr, colref, infer_nulls_as);
			break;
		case COperator::EopScalarCmp:
			pci = PciIntervalFromScalarCmp(mp, pexpr, colref, infer_nulls_as);
			break;
		case COperator::EopScalarIsDistinctFrom:
			pci = PciIntervalFromScalarIDF(mp, pexpr, colref);
			break;
		case COperator::EopScalarConst:
		{
			if (CUtils::FScalarConstTrue(pexpr))
			{
				pci = CConstraintInterval::PciUnbounded(mp, colref,
														true /*fIncludesNull*/);
			}
			else
			{
				pci = GPOS_NEW(mp) CConstraintInterval(
					mp, colref, GPOS_NEW(mp) CRangeArray(mp),
					false /*fIncludesNull*/);
			}
		}
		break;
		case COperator::EopScalarArrayCmp:
			if (GPOS_FTRACE(EopttraceArrayConstraints))
			{
				pci = CConstraintInterval::PcnstrIntervalFromScalarArrayCmp(
					mp, pexpr, colref, infer_nulls_as);
			}
			break;
		default:
			pci = nullptr;
	}

	return pci;
}

//---------------------------------------------------------------------------
//	@function:
//		CConstraint::PcnstrFromScalarArrayCmp
//
//	@doc:
//		Create constraint from scalar array comparison expression. Returns
//		NULL if a constraint interval cannot be created. Has side effect of
//		removing duplicates
//
//---------------------------------------------------------------------------
gpos::Ref<CConstraintInterval>
CConstraintInterval::PcnstrIntervalFromScalarArrayCmp(CMemoryPool *mp,
													  CExpression *pexpr,
													  CColRef *colref,
													  BOOL infer_nulls_as)
{
	if (!(CPredicateUtils::FCompareIdentToConstArray(pexpr) ||
		  CPredicateUtils::FCompareCastIdentToConstArray(pexpr)))
	{
		return nullptr;
	}
#ifdef GPOS_DEBUG
	else
	{
		// verify column in expr is the same as column which was passed in
		CScalarIdent *popScId = nullptr;
		if (CUtils::FScalarIdent((*pexpr)[0]))
		{
			popScId = gpos::dyn_cast<CScalarIdent>((*pexpr)[0]->Pop());
		}
		else
		{
			GPOS_ASSERT(CScalarIdent::FCastedScId((*pexpr)[0]));
			popScId = gpos::dyn_cast<CScalarIdent>((*(*pexpr)[0])[0]->Pop());
		}
		GPOS_ASSERT(colref == (CColRef *) popScId->Pcr());
	}
#endif	// GPOS_DEBUG

	CScalarArrayCmp *popScArrayCmp =
		gpos::dyn_cast<CScalarArrayCmp>(pexpr->Pop());
	IMDType::ECmpType cmp_type = CUtils::ParseCmpType(popScArrayCmp->MdIdOp());


	CExpression *pexprArray = CUtils::PexprScalarArrayChild(pexpr);
	const ULONG ulArrayExprArity = CUtils::UlScalarArrayArity(pexprArray);
	if (0 == ulArrayExprArity)
	{
		return nullptr;
	}

	const IComparator *pcomp = COptCtxt::PoctxtFromTLS()->Pcomp();
	gpos::Ref<CDatumSortedSet> apdatumsortedset(
		GPOS_NEW(mp) CDatumSortedSet(mp, pexprArray, pcomp));
	// construct ranges representing IN or NOT IN
	gpos::Ref<CRangeArray> prgrng = GPOS_NEW(mp) CRangeArray(mp);

	switch (cmp_type)
	{
		case IMDType::EcmptEq:
		{
			// IN case, create ranges [X, X] [Y, Y] [Z, Z]
			for (ULONG ul = 0; ul < apdatumsortedset->Size(); ul++)
			{
				;
				gpos::Ref<CRange> prng = GPOS_NEW(mp)
					CRange(pcomp, IMDType::EcmptEq, (*apdatumsortedset)[ul]);
				prgrng->Append(prng);
			}
			break;
		}
		case IMDType::EcmptNEq:
		{
			// NOT IN case, create ranges: (-inf, X) (X, Y) (Y, Z) (Z, inf)
			IDatum *pprevdatum = nullptr;
			IDatum *datum = nullptr;

			for (ULONG ul = 0; ul < apdatumsortedset->Size(); ul++)
			{
				if (0 != ul)
				{
					;
				}

				datum = (*apdatumsortedset)[ul];
				;

				gpos::Ref<IMDId> mdid = datum->MDId();
				;

				gpos::Ref<CRange> prng = GPOS_NEW(mp)
					CRange(mdid, pcomp, pprevdatum, CRange::EriExcluded, datum,
						   CRange::EriExcluded);
				prgrng->Append(prng);

				pprevdatum = datum;
			}

			// add the last datum, making range (last, inf)
			IMDId *mdid = pprevdatum->MDId();
			;
			;
			gpos::Ref<CRange> prng = GPOS_NEW(mp)
				CRange(mdid, pcomp, pprevdatum, CRange::EriExcluded, nullptr,
					   CRange::EriExcluded);
			prgrng->Append(prng);
			break;
		}
		default:
		{
			// does not handle IS DISTINCT FROM
			;
			return nullptr;
		}
	}

	return GPOS_NEW(mp)
		CConstraintInterval(mp, colref, std::move(prgrng), infer_nulls_as);
}

//---------------------------------------------------------------------------
//	@function:
//		CConstraintInterval::PciIntervalFromConstraint
//
//	@doc:
//		Create interval from any general constraint that references only one column
//
//---------------------------------------------------------------------------
gpos::Ref<CConstraintInterval>
CConstraintInterval::PciIntervalFromConstraint(CMemoryPool *mp,
											   CConstraint *pcnstr,
											   CColRef *colref)
{
	if (nullptr == pcnstr)
	{
		GPOS_ASSERT(
			nullptr != colref &&
			"Must provide valid column reference to construct unbounded interval");
		return PciUnbounded(mp, colref, true /*fIncludesNull*/);
	}

	if (CConstraint::EctInterval == pcnstr->Ect())
	{
		;
		return dynamic_cast<CConstraintInterval *>(pcnstr);
	}

	CColRefSet *pcrsUsed = pcnstr->PcrsUsed();
	GPOS_ASSERT(1 == pcrsUsed->Size());

	CColRef *pcrFirst = pcrsUsed->PcrFirst();
	GPOS_ASSERT_IMP(nullptr != colref, pcrFirst == colref);

	CExpression *pexprScalar = pcnstr->PexprScalar(mp);

	return PciIntervalFromScalarExpr(mp, pexprScalar, pcrFirst);
}

//---------------------------------------------------------------------------
//	@function:
//		CConstraintInterval::PciIntervalFromScalarNullTest
//
//	@doc:
//		Create interval from scalar null test
//
//---------------------------------------------------------------------------
gpos::Ref<CConstraintInterval>
CConstraintInterval::PciIntervalFromScalarNullTest(CMemoryPool *mp,
												   CExpression *pexpr,
												   CColRef *colref)
{
	GPOS_ASSERT(nullptr != pexpr);
	GPOS_ASSERT(CUtils::FScalarNullTest(pexpr));

	// child of comparison operator
	CExpression *pexprChild = (*pexpr)[0];

	// TODO:  - May 28, 2012; add support for other expression forms
	// besides (ident is null)

	if (CUtils::FScalarIdent(pexprChild))
	{
#ifdef GPOS_DEBUG
		CScalarIdent *popScId = gpos::dyn_cast<CScalarIdent>(pexprChild->Pop());
		GPOS_ASSERT(colref == (CColRef *) popScId->Pcr());
#endif	// GPOS_DEBUG
		return GPOS_NEW(mp) CConstraintInterval(
			mp, colref, GPOS_NEW(mp) CRangeArray(mp), true /*fIncludesNull*/);
	}

	return nullptr;
}


//---------------------------------------------------------------------------
//	@function:
//		CConstraintInterval::PciIntervalFromScalarCmp
//
//	@doc:
//		Helper for create interval from comparison between a column and
//		a constant
//
//---------------------------------------------------------------------------
gpos::Ref<CConstraintInterval>
CConstraintInterval::PciIntervalFromColConstCmp(CMemoryPool *mp,
												CColRef *colref,
												IMDType::ECmpType cmp_type,
												CScalarConst *popScConst,
												BOOL infer_nulls_as)
{
	gpos::Ref<CConstraintInterval> pcri = nullptr;
	gpos::Ref<CRangeArray> pdrngprng =
		PciRangeFromColConstCmp(mp, cmp_type, popScConst);

	if (nullptr != pdrngprng)
	{
		// (col = const) usually implies (col IS NOT NULL) for these ops since
		// NULLs are inferred as false.  But, if asked to infer NULLS as true (e.g
		// in table constraints), include NULL in the final interval.
		pcri = GPOS_NEW(mp)
			CConstraintInterval(mp, colref, pdrngprng, infer_nulls_as);
	}
	return pcri;
}


//---------------------------------------------------------------------------
//	@function:
//		CConstraintInterval::PciIntervalFromScalarCmp
//
//	@doc:
//		Create interval from scalar comparison expression
//
//---------------------------------------------------------------------------
gpos::Ref<CConstraintInterval>
CConstraintInterval::PciIntervalFromScalarCmp(CMemoryPool *mp,
											  CExpression *pexpr,
											  CColRef *colref,
											  BOOL infer_nulls_as)
{
	GPOS_ASSERT(nullptr != pexpr);
	GPOS_ASSERT(CUtils::FScalarCmp(pexpr) || CUtils::FScalarArrayCmp(pexpr));

	// TODO:  - May 28, 2012; add support for other expression forms
	// besides (column relop const)
	if (CPredicateUtils::FCompareIdentToConst(pexpr))
	{
		// column
#ifdef GPOS_DEBUG
		CScalarIdent *popScId;
		CExpression *pexprLeft = (*pexpr)[0];
		if (CUtils::FScalarIdent((*pexpr)[0]))
		{
			popScId = gpos::dyn_cast<CScalarIdent>(pexprLeft->Pop());
		}
		else
		{
			GPOS_ASSERT(CCastUtils::FBinaryCoercibleCastedScId(pexprLeft));
			popScId = gpos::dyn_cast<CScalarIdent>((*pexprLeft)[0]->Pop());
		}
		GPOS_ASSERT(colref == (CColRef *) popScId->Pcr());
#endif	// GPOS_DEBUG

		// constant
		CExpression *pexprRight = (*pexpr)[1];
		CScalarConst *popScConst;
		if (CUtils::FScalarConst(pexprRight))
		{
			popScConst = gpos::dyn_cast<CScalarConst>(pexprRight->Pop());
		}
		else
		{
			GPOS_ASSERT(CCastUtils::FBinaryCoercibleCastedConst(pexprRight));
			popScConst = gpos::dyn_cast<CScalarConst>((*pexprRight)[0]->Pop());
		}
		CScalarCmp *popScCmp = gpos::dyn_cast<CScalarCmp>(pexpr->Pop());

		return PciIntervalFromColConstCmp(mp, colref, popScCmp->ParseCmpType(),
										  popScConst, infer_nulls_as);
	}

	return nullptr;
}


gpos::Ref<CConstraintInterval>
CConstraintInterval::PciIntervalFromScalarIDF(CMemoryPool *mp,
											  CExpression *pexpr,
											  CColRef *colref)
{
	GPOS_ASSERT(nullptr != pexpr);
	GPOS_ASSERT(CPredicateUtils::FIDF(pexpr));

	if (CPredicateUtils::FIdentIDFConst(pexpr))
	{
		// column
#ifdef GPOS_DEBUG
		CScalarIdent *popScId =
			gpos::dyn_cast<CScalarIdent>((*pexpr)[0]->Pop());
		GPOS_ASSERT(colref == (CColRef *) popScId->Pcr());
#endif	// GPOS_DEBUG

		// constant
		CScalarConst *popScConst =
			gpos::dyn_cast<CScalarConst>((*pexpr)[1]->Pop());
		// operator
		CScalarIsDistinctFrom *popScCmp =
			gpos::dyn_cast<CScalarIsDistinctFrom>(pexpr->Pop());

		GPOS_ASSERT(CScalar::EopScalarConst == popScConst->Eopid());
		GPOS_ASSERT(IMDType::EcmptIDF == popScCmp->ParseCmpType());

		IDatum *datum = popScConst->GetDatum();
		gpos::Ref<CConstraintInterval> pcri = nullptr;

		if (datum->IsNull())
		{
			// col IS DISTINCT FROM NULL
			gpos::Ref<CConstraintInterval> pcriChild = GPOS_NEW(mp)
				CConstraintInterval(mp, colref, GPOS_NEW(mp) CRangeArray(mp),
									true /*fIncludesNull*/);
			pcri = pcriChild->PciComplement(mp);
			;
		}
		else
		{
			// col IS DISTINCT FROM const
			gpos::Ref<CRangeArray> pdrgprng = PciRangeFromColConstCmp(
				mp, popScCmp->ParseCmpType(), popScConst);
			if (nullptr != pdrgprng)
			{
				pcri = GPOS_NEW(mp) CConstraintInterval(mp, colref, pdrgprng,
														true /*fIncludesNull*/);
			}
		}

		return pcri;
	}
	return nullptr;
}

//---------------------------------------------------------------------------
//	@function:
//		CConstraintInterval::PciIntervalFromScalarBoolOp
//
//	@doc:
//		Create interval from scalar boolean: AND, OR, NOT
//
//---------------------------------------------------------------------------
gpos::Ref<CConstraintInterval>
CConstraintInterval::PciIntervalFromScalarBoolOp(CMemoryPool *mp,
												 CExpression *pexpr,
												 CColRef *colref,
												 BOOL infer_nulls_as)
{
	GPOS_ASSERT(nullptr != pexpr);
	GPOS_ASSERT(CUtils::FScalarBoolOp(pexpr));

	CScalarBoolOp *popScBool = gpos::dyn_cast<CScalarBoolOp>(pexpr->Pop());
	CScalarBoolOp::EBoolOperator eboolop = popScBool->Eboolop();

	switch (eboolop)
	{
		case CScalarBoolOp::EboolopAnd:
			return PciIntervalFromScalarBoolAnd(mp, pexpr, colref,
												infer_nulls_as);

		case CScalarBoolOp::EboolopOr:
			return PciIntervalFromScalarBoolOr(mp, pexpr, colref,
											   infer_nulls_as);

		case CScalarBoolOp::EboolopNot:
		{
			gpos::Ref<CConstraintInterval> pciChild = PciIntervalFromScalarExpr(
				mp, (*pexpr)[0], colref, !infer_nulls_as);
			if (nullptr == pciChild)
			{
				return nullptr;
			}

			gpos::Ref<CConstraintInterval> pciNot = pciChild->PciComplement(mp);
			;
			return pciNot;
		}
		default:
			return nullptr;
	}
}

//---------------------------------------------------------------------------
//	@function:
//		CConstraintInterval::PciIntervalFromScalarBoolOr
//
//	@doc:
//		Create interval from scalar boolean OR
//
//---------------------------------------------------------------------------
gpos::Ref<CConstraintInterval>
CConstraintInterval::PciIntervalFromScalarBoolOr(CMemoryPool *mp,
												 CExpression *pexpr,
												 CColRef *colref,
												 BOOL infer_nulls_as)
{
	GPOS_ASSERT(nullptr != pexpr);
	GPOS_ASSERT(CUtils::FScalarBoolOp(pexpr));
	GPOS_ASSERT(CScalarBoolOp::EboolopOr ==
				gpos::dyn_cast<CScalarBoolOp>(pexpr->Pop())->Eboolop());

	const ULONG arity = pexpr->Arity();
	GPOS_ASSERT(0 < arity);

	gpos::Ref<CConstraintIntervalArray> child_constraints =
		GPOS_NEW(mp) CConstraintIntervalArray(mp);
	for (ULONG ul = 0; ul < arity; ul++)
	{
		gpos::Ref<CConstraintInterval> pciChild =
			PciIntervalFromScalarExpr(mp, (*pexpr)[ul], colref, infer_nulls_as);

		if (nullptr == pciChild)
		{
			;
			return nullptr;
		}

		child_constraints->Append(pciChild);
	}

	gpos::Ref<CConstraintIntervalArray> constraints;

	// PciUnion each interval in pairs. Given intervals I1,I2.., I5, perform the unions as follows:
	// iteration 1: I1 U I2, I3 U I4, I5
	// iteration 2: I12 U I34, I5
	// iteration 3: I1234 U I5
	while (child_constraints->Size() > 1)
	{
		constraints = GPOS_NEW(mp) CConstraintIntervalArray(mp);

		ULONG length = child_constraints->Size();
		ULONG ul;

		for (ul = 0; ul < length - 1; ul += 2)
		{
			CConstraintInterval *pci1 = (*child_constraints)[ul].get();
			CConstraintInterval *pci2 = (*child_constraints)[ul + 1].get();

			gpos::Ref<CConstraintInterval> pciOr = pci1->PciUnion(mp, pci2);
			constraints->Append(pciOr);
		}

		if (ul < length)
		{
			// append the odd one at the end
			gpos::Ref<CConstraintInterval> pciChild = (*child_constraints)[ul];
			;
			constraints->Append(pciChild);
		};
		child_constraints = constraints;
	}
	GPOS_ASSERT(child_constraints->Size() == 1);
	gpos::Ref<CConstraintInterval> dest = (*child_constraints)[0];
	;
	;

	return dest;
}

//---------------------------------------------------------------------------
//	@function:
//		CConstraintInterval::PciIntervalFromScalarBoolAnd
//
//	@doc:
//		Create interval from scalar boolean AND
//
//---------------------------------------------------------------------------
gpos::Ref<CConstraintInterval>
CConstraintInterval::PciIntervalFromScalarBoolAnd(CMemoryPool *mp,
												  CExpression *pexpr,
												  CColRef *colref,
												  BOOL infer_nulls_as)
{
	GPOS_ASSERT(nullptr != pexpr);
	GPOS_ASSERT(CUtils::FScalarBoolOp(pexpr));
	GPOS_ASSERT(CScalarBoolOp::EboolopAnd ==
				gpos::dyn_cast<CScalarBoolOp>(pexpr->Pop())->Eboolop());

	const ULONG arity = pexpr->Arity();
	GPOS_ASSERT(0 < arity);

	gpos::Ref<CConstraintInterval> pci =
		PciIntervalFromScalarExpr(mp, (*pexpr)[0], colref, infer_nulls_as);
	for (ULONG ul = 1; ul < arity; ul++)
	{
		gpos::Ref<CConstraintInterval> pciChild =
			PciIntervalFromScalarExpr(mp, (*pexpr)[ul], colref, infer_nulls_as);
		// here is where we will return a NULL child from not being able to create a
		// CConstraint interval from the ScalarExpr
		if (nullptr != pciChild && nullptr != pci)
		{
			gpos::Ref<CConstraintInterval> pciAnd =
				pci->PciIntersect(mp, pciChild.get());
			;
			;
			pci = pciAnd;
		}
		else if (nullptr != pciChild)
		{
			pci = pciChild;
		}
	}

	return pci;
}

//---------------------------------------------------------------------------
//	@function:
//		CConstraintInterval::PexprScalar
//
//	@doc:
//		Return scalar expression
//
//---------------------------------------------------------------------------
CExpression *
CConstraintInterval::PexprScalar(CMemoryPool *mp)
{
	if (nullptr == m_pexprScalar)
	{
		m_pexprScalar = PexprConstructScalar(mp);
	}

	return m_pexprScalar.get();
}

//---------------------------------------------------------------------------
//	@function:
//		CConstraintInterval::PexprConstructScalar
//
//	@doc:
//		Construct scalar expression
//
//---------------------------------------------------------------------------
gpos::Ref<CExpression>
CConstraintInterval::PexprConstructScalar(CMemoryPool *mp) const
{
	if (FContradiction())
	{
		return CUtils::PexprScalarConstBool(mp, false /*fval*/,
											false /*is_null*/);
	}

	if (GPOS_FTRACE(EopttraceArrayConstraints))
	{
		// try creating an array IN/NOT IN expression
		gpos::Ref<CExpression> pexpr = PexprConstructArrayScalar(mp);
		if (pexpr != nullptr)
		{
			return pexpr;
		}
	}

	// otherwise, we generate a disjunction of ranges
	return PexprConstructDisjunctionScalar(mp);
}

//---------------------------------------------------------------------------
//	@function:
//		CConstraintInterval::PexprConstructDisjunctionScalar
//
//	@doc:
//		Returns a disjunction of several equality or inequality expressions
//		describing this interval. Or, returns a singular expression if the
//		interval can be represented as such.
//		For example an interval containing ranges like
//			[1,1],(7,inf)
//		converts to an expression like
//			x = 1 OR x > 7
//		but an interval containing the range
//			(-inf, inf)
//		converts to a scalar true
//
//---------------------------------------------------------------------------
gpos::Ref<CExpression>
CConstraintInterval::PexprConstructDisjunctionScalar(CMemoryPool *mp) const
{
	gpos::Ref<CExpressionArray> pdrgpexpr = GPOS_NEW(mp) CExpressionArray(mp);

	const ULONG length = m_pdrgprng->Size();
	for (ULONG ul = 0; ul < length; ul++)
	{
		CRange *prange = (*m_pdrgprng)[ul].get();
		gpos::Ref<CExpression> pexprChild = prange->PexprScalar(mp, m_pcr);
		pdrgpexpr->Append(pexprChild);
	}

	if (1 == pdrgpexpr->Size() &&
		CUtils::FScalarConstTrue((*pdrgpexpr)[0].get()))
	{
		// so far, interval covers all the not null values
		;

		if (m_fIncludesNull)
		{
			return CUtils::PexprScalarConstBool(mp, true /*fval*/,
												false /*is_null*/);
		}

		return CUtils::PexprIsNotNull(mp, CUtils::PexprScalarIdent(mp, m_pcr));
	}

	if (m_fIncludesNull)
	{
		gpos::Ref<CExpression> pexprIsNull =
			CUtils::PexprIsNull(mp, CUtils::PexprScalarIdent(mp, m_pcr));
		pdrgpexpr->Append(pexprIsNull);
	}

	return CPredicateUtils::PexprDisjunction(mp, std::move(pdrgpexpr));
}

//---------------------------------------------------------------------------
//	@function:
//		CConstraintInterval::FConvertsToIn
//
//	@doc:
//		Looks for a specific pattern within the array of ranges to determine
//		if this interval can be converted into an array IN statement. The
//		pattern is like [[n,n], [m,m]] is an IN
//
//---------------------------------------------------------------------------
bool
CConstraintInterval::FConvertsToIn() const
{
	if (1 >= m_pdrgprng->Size())
	{
		return false;
	}

	bool isIN = true;
	const ULONG length = m_pdrgprng->Size();
	for (ULONG ul = 0; ul < length && isIN; ul++)
	{
		isIN &= (*m_pdrgprng)[ul]->FPoint();
	}
	return isIN;
}

//---------------------------------------------------------------------------
//	@function:
//		CConstraintInterval::FConvertsToNotIn
//
//	@doc:
//		Looks for a specific pattern within the array of ranges to determine
//		if this interval can be converted into an array NOT IN statement. The
//		pattern is like [(-inf, m), (m, n), (n, inf)]
//
//---------------------------------------------------------------------------
bool
CConstraintInterval::FConvertsToNotIn() const
{
	if (1 >= m_pdrgprng->Size())
	{
		return false;
	}

	// for this to be a NOT IN, its edges must be unbounded
	if ((*m_pdrgprng)[0]->PdatumLeft() != nullptr ||
		(*m_pdrgprng)[m_pdrgprng->Size() - 1]->PdatumRight() != nullptr)
	{
		return false;
	}

	// check that each range is exclusive and that the inner values are equal
	bool isNotIn = true;
	CRange *pLeftRng = (*m_pdrgprng)[0];
	CRange *pRightRng = nullptr;
	const ULONG length = m_pdrgprng->Size();
	for (ULONG ul = 1; ul < length && isNotIn; ul++)
	{
		pRightRng = (*m_pdrgprng)[ul];
		isNotIn &= pLeftRng->EriRight() == CRange::EriExcluded;
		isNotIn &= pRightRng->EriLeft() == CRange::EriExcluded;
		isNotIn &= pLeftRng->FUpperBoundEqualsLowerBound(pRightRng);

		pLeftRng = pRightRng;
	}

	return isNotIn;
}

//---------------------------------------------------------------------------
//	@function:
//		CConstraintInterval::PexprConstructArrayScalar
//
//	@doc:
//		Constructs an array expression from the ranges stored in this interval.
//		It is a mistake to call this method without first detecting if the
//		stored ranges can be converted to an IN or NOT in statement. The param
//		'fIn' refers to the statement being an IN statement, and if set to false,
// 		it is considered a NOT IN statement
//
//---------------------------------------------------------------------------
gpos::Ref<CExpression>
CConstraintInterval::PexprConstructArrayScalar(CMemoryPool *mp, bool fIn) const
{
	GPOS_ASSERT(FConvertsToIn() || FConvertsToNotIn());

	ULONG ulRngs = m_pdrgprng->Size();
	IMDType::ECmpType ecmptype = IMDType::EcmptEq;
	CScalarArrayCmp::EArrCmpType earraycmptype = CScalarArrayCmp::EarrcmpAny;

	if (!fIn)
	{
		ecmptype = IMDType::EcmptNEq;
		earraycmptype = CScalarArrayCmp::EarrcmpAll;

		// if NOT IN, we skip the last range, as the right datum will be null
		ulRngs -= 1;
	}

	// loop through all of the constants in the ranges, creating an array of CScalarConst Expressions
	gpos::Ref<CExpressionArray> prngexpr = GPOS_NEW(mp) CExpressionArray(mp);

	// this method assumes IN or NOT IN which means that the ranges stored will look like either
	// [x,x], ... ,[y,y] or the NOT IN case (-inf, x),(x,y), ... ,(z,inf).
	for (ULONG ul = 0; ul < ulRngs; ul++)
	{
		gpos::Ref<IDatum> datum = (*m_pdrgprng)[ul]->PdatumRight();
		;
		gpos::Ref<CScalarConst> popScConst =
			GPOS_NEW(mp) CScalarConst(mp, datum);
		gpos::Ref<CExpression> pexpr = GPOS_NEW(mp) CExpression(mp, popScConst);
		prngexpr->Append(pexpr);
	}

	gpos::Ref<CExpression> pexpr = CUtils::PexprScalarArrayCmp(
		mp, earraycmptype, ecmptype, prngexpr, m_pcr);

	if (m_fIncludesNull)
	{
		gpos::Ref<CExpression> pexprIsNull =
			CUtils::PexprIsNull(mp, CUtils::PexprScalarIdent(mp, m_pcr));
		gpos::Ref<CExpression> pexprDisjuction =
			CPredicateUtils::PexprDisjunction(mp, pexpr.get(),
											  pexprIsNull.get());
		;
		;
		pexpr = pexprDisjuction;
	}

	return pexpr;
}

//---------------------------------------------------------------------------
//	@function:
//		CConstraintInterval::PexprConstructScalar
//
//	@doc:
//		Constructs an array expression if the interval can be converted into
//		an array expression. Returns null if an array scalar cannot be
//		constructed
//
//---------------------------------------------------------------------------
gpos::Ref<CExpression>
CConstraintInterval::PexprConstructArrayScalar(CMemoryPool *mp) const
{
	if (1 >= m_pdrgprng->Size())
	{
		return nullptr;
	}

	if (FConvertsToIn())
	{
		return PexprConstructArrayScalar(mp, true);
	}
	else if (FConvertsToNotIn())
	{
		return PexprConstructArrayScalar(mp, false);
	}
	else
	{
		// Does not convert to either IN or NOT IN
		return nullptr;
	}
}

//---------------------------------------------------------------------------
//	@function:
//		CConstraintInterval::Pcnstr
//
//	@doc:
//		Return constraint on a given column
//
//---------------------------------------------------------------------------
gpos::Ref<CConstraint>
CConstraintInterval::Pcnstr(CMemoryPool *,	//mp,
							const CColRef *colref)
{
	if (m_pcr == colref)
	{
		;
		return this;
	}

	return nullptr;
}

//---------------------------------------------------------------------------
//	@function:
//		CConstraintInterval::Pcnstr
//
//	@doc:
//		Return constraint on a given column set
//
//---------------------------------------------------------------------------
gpos::Ref<CConstraint>
CConstraintInterval::Pcnstr(CMemoryPool *,	//mp,
							CColRefSet *pcrs)
{
	if (pcrs->FMember(m_pcr))
	{
		;
		return this;
	}

	return nullptr;
}

//---------------------------------------------------------------------------
//	@function:
//		CConstraintInterval::PcnstrRemapForColumn
//
//	@doc:
//		Return a copy of the constraint for a different column
//
//---------------------------------------------------------------------------
gpos::Ref<CConstraint>
CConstraintInterval::PcnstrRemapForColumn(CMemoryPool *mp,
										  CColRef *colref) const
{
	GPOS_ASSERT(nullptr != colref);
	;
	return GPOS_NEW(mp)
		CConstraintInterval(mp, colref, m_pdrgprng, m_fIncludesNull);
}

//---------------------------------------------------------------------------
//	@function:
//		CConstraintInterval::PciIntersect
//
//	@doc:
//		Intersection with another interval
//
//---------------------------------------------------------------------------
gpos::Ref<CConstraintInterval>
CConstraintInterval::PciIntersect(CMemoryPool *mp, CConstraintInterval *pci)
{
	GPOS_ASSERT(nullptr != pci);
	GPOS_ASSERT(m_pcr == pci->Pcr());

	CRangeArray *pdrgprngOther = pci->Pdrgprng();

	gpos::Ref<CRangeArray> pdrgprngNew = GPOS_NEW(mp) CRangeArray(mp);

	ULONG ulFst = 0;
	ULONG ulSnd = 0;
	const ULONG ulNumRangesFst = m_pdrgprng->Size();
	const ULONG ulNumRangesSnd = pdrgprngOther->Size();
	while (ulFst < ulNumRangesFst && ulSnd < ulNumRangesSnd)
	{
		CRange *prangeThis = (*m_pdrgprng)[ulFst].get();
		CRange *prangeOther = (*pdrgprngOther)[ulSnd].get();

		gpos::Ref<CRange> prangeNew = nullptr;
		if (prangeOther->FEndsAfter(prangeThis))
		{
			prangeNew = prangeThis->PrngIntersect(mp, prangeOther);
			ulFst++;
		}
		else
		{
			prangeNew = prangeOther->PrngIntersect(mp, prangeThis);
			ulSnd++;
		}

		if (nullptr != prangeNew)
		{
			pdrgprngNew->Append(prangeNew);
		}
	}

	return GPOS_NEW(mp)
		CConstraintInterval(mp, m_pcr, std::move(pdrgprngNew),
							m_fIncludesNull && pci->FIncludesNull());
}

//---------------------------------------------------------------------------
//	@function:
//		CConstraintInterval::PciUnion
//
//	@doc:
//		Union with another interval
//
//---------------------------------------------------------------------------
gpos::Ref<CConstraintInterval>
CConstraintInterval::PciUnion(CMemoryPool *mp, CConstraintInterval *pci)
{
	GPOS_ASSERT(nullptr != pci);
	GPOS_ASSERT(m_pcr == pci->Pcr());

	CRangeArray *pdrgprngOther = pci->Pdrgprng();

	gpos::Ref<CRangeArray> pdrgprngNew = GPOS_NEW(mp) CRangeArray(mp);

	ULONG ulFst = 0;
	ULONG ulSnd = 0;
	const ULONG ulNumRangesFst = m_pdrgprng->Size();
	const ULONG ulNumRangesSnd = pdrgprngOther->Size();
	while (ulFst < ulNumRangesFst && ulSnd < ulNumRangesSnd)
	{
		CRange *prangeThis = (*m_pdrgprng)[ulFst].get();
		CRange *prangeOther = (*pdrgprngOther)[ulSnd].get();

		gpos::Ref<CRange> prangeNew = nullptr;
		if (prangeOther->FEndsAfter(prangeThis))
		{
			prangeNew = prangeThis->PrngDifferenceLeft(mp, prangeOther);
			ulFst++;
		}
		else
		{
			prangeNew = prangeOther->PrngDifferenceLeft(mp, prangeThis);
			ulSnd++;
		}

		AppendOrExtend(mp, pdrgprngNew.get(), prangeNew);
	}

	AddRemainingRanges(mp, m_pdrgprng.get(), ulFst, pdrgprngNew.get());
	AddRemainingRanges(mp, pdrgprngOther, ulSnd, pdrgprngNew.get());

	return GPOS_NEW(mp)
		CConstraintInterval(mp, m_pcr, std::move(pdrgprngNew),
							m_fIncludesNull || pci->FIncludesNull());
}

//---------------------------------------------------------------------------
//	@function:
//		CConstraintInterval::PciDifference
//
//	@doc:
//		Difference between this interval and another interval
//
//---------------------------------------------------------------------------
gpos::Ref<CConstraintInterval>
CConstraintInterval::PciDifference(CMemoryPool *mp, CConstraintInterval *pci)
{
	GPOS_ASSERT(nullptr != pci);
	GPOS_ASSERT(m_pcr == pci->Pcr());

	CRangeArray *pdrgprngOther = pci->Pdrgprng();

	gpos::Ref<CRangeArray> pdrgprngNew = GPOS_NEW(mp) CRangeArray(mp);

	ULONG ulFst = 0;
	ULONG ulSnd = 0;
	gpos::Ref<CRangeArray> pdrgprngResidual = GPOS_NEW(mp) CRangeArray(mp);
	CRange *prangeResidual = nullptr;
	const ULONG ulNumRangesFst = m_pdrgprng->Size();
	const ULONG ulNumRangesSnd = pdrgprngOther->Size();
	while (ulFst < ulNumRangesFst && ulSnd < ulNumRangesSnd)
	{
		// if there is a residual range from previous iteration then use it
		CRange *prangeThis =
			(nullptr == prangeResidual ? (*m_pdrgprng)[ulFst].get()
									   : prangeResidual);
		CRange *prangeOther = (*pdrgprngOther)[ulSnd].get();

		gpos::Ref<CRange> prangeNew = nullptr;
		prangeResidual = nullptr;

		if (prangeOther->FEndsWithOrAfter(prangeThis))
		{
			prangeNew = prangeThis->PrngDifferenceLeft(mp, prangeOther);
			ulFst++;
		}
		else
		{
			prangeNew = PrangeDiffWithRightResidual(mp, prangeThis, prangeOther,
													&prangeResidual,
													pdrgprngResidual.get());
			ulSnd++;
		}

		AppendOrExtend(mp, pdrgprngNew.get(), prangeNew);
	}

	if (nullptr != prangeResidual)
	{
		ulFst++;
		;
	}

	AppendOrExtend(mp, pdrgprngNew.get(), prangeResidual);
	;
	AddRemainingRanges(mp, m_pdrgprng.get(), ulFst, pdrgprngNew.get());

	return GPOS_NEW(mp)
		CConstraintInterval(mp, m_pcr, std::move(pdrgprngNew),
							m_fIncludesNull && !pci->FIncludesNull());
}

//---------------------------------------------------------------------------
//	@function:
//		CConstraintInterval::FContainsInterval
//
//	@doc:
//		Does the current interval contain the given interval?
//
//---------------------------------------------------------------------------
BOOL
CConstraintInterval::FContainsInterval(CMemoryPool *mp,
									   CConstraintInterval *pci)
{
	GPOS_ASSERT(nullptr != pci);
	GPOS_ASSERT(m_pcr == pci->Pcr());

	if (IsConstraintUnbounded())
	{
		return true;
	}

	if (nullptr == pci || pci->IsConstraintUnbounded() ||
		(!FIncludesNull() && pci->FIncludesNull()))
	{
		return false;
	}

	gpos::Ref<CConstraintInterval> pciDiff = pci->PciDifference(mp, this);

	// if the difference is empty, then this interval contains the given one
	BOOL fContains = pciDiff->FContradiction();
	;

	return fContains;
}

//---------------------------------------------------------------------------
//	@function:
//		CConstraintInterval::PciUnbounded
//
//	@doc:
//		Create an unbounded interval
//
//---------------------------------------------------------------------------
gpos::Ref<CConstraintInterval>
CConstraintInterval::PciUnbounded(CMemoryPool *mp, const CColRef *colref,
								  BOOL fIncludesNull)
{
	IMDId *mdid = colref->RetrieveType()->MDId();
	if (!CUtils::FConstrainableType(mdid))
	{
		return nullptr;
	}

	;
	gpos::Ref<CRange> prange = GPOS_NEW(mp) CRange(
		mdid, COptCtxt::PoctxtFromTLS()->Pcomp(), nullptr /*ppointLeft*/,
		CRange::EriExcluded, nullptr /*ppointRight*/, CRange::EriExcluded);

	gpos::Ref<CRangeArray> pdrgprng = GPOS_NEW(mp) CRangeArray(mp);
	pdrgprng->Append(std::move(prange));

	return GPOS_NEW(mp)
		CConstraintInterval(mp, colref, std::move(pdrgprng), fIncludesNull);
}

//---------------------------------------------------------------------------
//	@function:
//		CConstraintInterval::PciUnbounded
//
//	@doc:
//		Create an unbounded interval on any column from the given set
//
//---------------------------------------------------------------------------
gpos::Ref<CConstraintInterval>
CConstraintInterval::PciUnbounded(CMemoryPool *mp, const CColRefSet *pcrs,
								  BOOL fIncludesNull)
{
	// find the first constrainable column
	CColRefSetIter crsi(*pcrs);
	while (crsi.Advance())
	{
		CColRef *colref = crsi.Pcr();
		gpos::Ref<CConstraintInterval> pci =
			PciUnbounded(mp, colref, fIncludesNull);
		if (nullptr != pci)
		{
			return pci;
		}
	}

	return nullptr;
}

//---------------------------------------------------------------------------
//	@function:
//		CConstraintInterval::MdidType
//
//	@doc:
//		Type of this interval
//
//---------------------------------------------------------------------------
IMDId *
CConstraintInterval::MdidType()
{
	// if there is at least one range, return range type
	if (0 < m_pdrgprng->Size())
	{
		CRange *prange = (*m_pdrgprng)[0].get();
		return prange->MDId();
	}

	// otherwise return type of column ref
	return m_pcr->RetrieveType()->MDId();
}

//---------------------------------------------------------------------------
//	@function:
//		CConstraintInterval::PciComplement
//
//	@doc:
//		Complement of this interval
//
//---------------------------------------------------------------------------
gpos::Ref<CConstraintInterval>
CConstraintInterval::PciComplement(CMemoryPool *mp)
{
	// create an unbounded interval
	gpos::Ref<CConstraintInterval> pciUniversal =
		PciUnbounded(mp, m_pcr, true /*fIncludesNull*/);

	gpos::Ref<CConstraintInterval> pciComp =
		pciUniversal->PciDifference(mp, this);
	;

	return pciComp;
}

//---------------------------------------------------------------------------
//	@function:
//		CConstraintInterval::PrangeDiffWithRightResidual
//
//	@doc:
//		Difference between two ranges on the left side only -
//		Any difference on the right side is reported as residual range
//
//		this    |----------------------|
//		prange         |-----------|
//		result  |------|
//		residual                   |---|
//---------------------------------------------------------------------------
gpos::Ref<CRange>
CConstraintInterval::PrangeDiffWithRightResidual(CMemoryPool *mp,
												 CRange *prangeFirst,
												 CRange *prangeSecond,
												 CRange **pprangeResidual,
												 CRangeArray *pdrgprngResidual)
{
	if (prangeSecond->FDisjointLeft(prangeFirst))
	{
		return nullptr;
	}

	gpos::Ref<CRange> prangeRet = nullptr;

	if (prangeFirst->Contains(prangeSecond))
	{
		prangeRet = prangeFirst->PrngDifferenceLeft(mp, prangeSecond);
	}

	// the part of prangeFirst that goes beyond prangeSecond
	*pprangeResidual = prangeFirst->PrngDifferenceRight(mp, prangeSecond);
	// add it to array so we can release it later on
	if (nullptr != *pprangeResidual)
	{
		pdrgprngResidual->Append(*pprangeResidual);
	}

	return prangeRet;
}

//---------------------------------------------------------------------------
//	@function:
//		CConstraintInterval::AddRemainingRanges
//
//	@doc:
//		Add ranges from a source array to a destination array, starting at the
//		range with the given index
//
//---------------------------------------------------------------------------
void
CConstraintInterval::AddRemainingRanges(CMemoryPool *mp,
										CRangeArray *pdrgprngSrc, ULONG ulStart,
										CRangeArray *pdrgprngDest)
{
	const ULONG length = pdrgprngSrc->Size();
	for (ULONG ul = ulStart; ul < length; ul++)
	{
		gpos::Ref<CRange> prange = (*pdrgprngSrc)[ul];
		;
		AppendOrExtend(mp, pdrgprngDest, prange);
	}
}

//---------------------------------------------------------------------------
//	@function:
//		CConstraintInterval::AppendOrExtend
//
//	@doc:
//		Append the given range to the array or extend the last range in that
//		array
//
//---------------------------------------------------------------------------
void
CConstraintInterval::AppendOrExtend(CMemoryPool *mp, CRangeArray *pdrgprng,
									gpos::Ref<CRange> prange)
{
	if (nullptr == prange)
	{
		return;
	}

	GPOS_ASSERT(nullptr != pdrgprng);

	const ULONG length = pdrgprng->Size();
	if (0 == length)
	{
		pdrgprng->Append(std::move(prange));
		return;
	}

	CRange *prangeLast = (*pdrgprng)[length - 1].get();
	gpos::Ref<CRange> prangeNew = prangeLast->PrngExtend(mp, prange.get());
	if (nullptr == prangeNew)
	{
		pdrgprng->Append(std::move(prange));
	}
	else
	{
		pdrgprng->Replace(length - 1, std::move(prangeNew));
		;
	}
}

//---------------------------------------------------------------------------
//	@function:
//		CConstraintInterval::OsPrint
//
//	@doc:
//		Debug print interval
//
//---------------------------------------------------------------------------
IOstream &
CConstraintInterval::OsPrint(IOstream &os) const
{
	os << "{";
	m_pcr->OsPrint(os);
	const ULONG length = m_pdrgprng->Size();
	os << ", ranges: ";
	for (ULONG ul = 0; ul < length; ul++)
	{
		CRange *prange = (*m_pdrgprng)[ul].get();
		os << *prange << " ";
	}

	if (m_fIncludesNull)
	{
		os << "[NULL] ";
	}

	os << "}";

	return os;
}

//---------------------------------------------------------------------------
//	@function:
//		CRange::PciRangeFromColConstCmp
//
//	@doc:
//		Creates an array of 1 or 2 ranges which represent the comparison to
//		a scalar.
//
//---------------------------------------------------------------------------
gpos::Ref<CRangeArray>
CConstraintInterval::PciRangeFromColConstCmp(CMemoryPool *mp,
											 IMDType::ECmpType cmp_type,
											 const CScalarConst *popsccnst)
{
	GPOS_ASSERT(CScalar::EopScalarConst == popsccnst->Eopid());

	// comparison operator
	if (IMDType::EcmptOther == cmp_type)
	{
		return nullptr;
	}

	IDatum *datum = popsccnst->GetDatum();
	gpos::Ref<CRangeArray> pdrgprng = GPOS_NEW(mp) CRangeArray(mp);

	const IComparator *pcomp = COptCtxt::PoctxtFromTLS()->Pcomp();
	if (IMDType::EcmptNEq == cmp_type || IMDType::EcmptIDF == cmp_type)
	{
		// need an interval with 2 ranges
		;
		pdrgprng->Append(GPOS_NEW(mp) CRange(pcomp, IMDType::EcmptL, datum));
		;
		pdrgprng->Append(GPOS_NEW(mp) CRange(pcomp, IMDType::EcmptG, datum));
	}
	else
	{
		;
		pdrgprng->Append(GPOS_NEW(mp) CRange(pcomp, cmp_type, datum));
	}

	return pdrgprng;
}

// EOF
