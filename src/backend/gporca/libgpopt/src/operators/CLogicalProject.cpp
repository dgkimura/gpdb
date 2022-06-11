//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2011 EMC Corp.
//
//	@filename:
//		CLogicalProject.cpp
//
//	@doc:
//		Implementation of project operator
//---------------------------------------------------------------------------

#include "gpopt/operators/CLogicalProject.h"

#include "gpos/base.h"
#include "gpos/common/owner.h"

#include "gpopt/base/CColRefSet.h"
#include "gpopt/base/CColRefTable.h"
#include "gpopt/base/CConstraintInterval.h"
#include "gpopt/base/CDefaultComparator.h"
#include "gpopt/base/CKeyCollection.h"
#include "gpopt/base/COptCtxt.h"
#include "gpopt/operators/CExpression.h"
#include "gpopt/operators/CExpressionHandle.h"
#include "gpopt/operators/CScalarIdent.h"
#include "gpopt/operators/CScalarProjectElement.h"

using namespace gpopt;
using namespace gpnaucrates;


//---------------------------------------------------------------------------
//	@function:
//		CLogicalProject::CLogicalProject
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CLogicalProject::CLogicalProject(CMemoryPool *mp) : CLogicalUnary(mp)
{
}


//---------------------------------------------------------------------------
//	@function:
//		CLogicalProject::DeriveOutputColumns
//
//	@doc:
//		Derive output columns
//
//---------------------------------------------------------------------------
gpos::Ref<CColRefSet>
CLogicalProject::DeriveOutputColumns(CMemoryPool *mp,
									 CExpressionHandle &exprhdl)
{
	GPOS_ASSERT(2 == exprhdl.Arity());

	gpos::Ref<CColRefSet> pcrs = GPOS_NEW(mp) CColRefSet(mp);

	// the scalar child defines additional columns
	pcrs->Union(exprhdl.DeriveOutputColumns(0));
	pcrs->Union(exprhdl.DeriveDefinedColumns(1));

	return pcrs;
}


//---------------------------------------------------------------------------
//	@function:
//		CLogicalProject::PkcDeriveKeys
//
//	@doc:
//		Derive key collection
//
//---------------------------------------------------------------------------
gpos::Ref<CKeyCollection>
CLogicalProject::DeriveKeyCollection(CMemoryPool *,	 // mp
									 CExpressionHandle &exprhdl) const
{
	return PkcDeriveKeysPassThru(exprhdl, 0 /* ulChild */);
}


//---------------------------------------------------------------------------
//	@function:
//		CLogicalProject::PdrgpcrsEquivClassFromScIdent
//
//	@doc:
//		Return equivalence class from scalar ident project element
//
//---------------------------------------------------------------------------
gpos::Ref<CColRefSetArray>
CLogicalProject::PdrgpcrsEquivClassFromScIdent(CMemoryPool *mp,
											   CExpression *pexprPrEl,
											   CColRefSet *not_null_columns)
{
	GPOS_ASSERT(nullptr != pexprPrEl);

	CScalarProjectElement *popPrEl =
		gpos::dyn_cast<CScalarProjectElement>(pexprPrEl->Pop());
	CColRef *pcrPrEl = popPrEl->Pcr();
	CExpression *pexprScalar = (*pexprPrEl)[0];


	if (EopScalarIdent != pexprScalar->Pop()->Eopid())
	{
		return nullptr;
	}

	CScalarIdent *popScIdent = gpos::dyn_cast<CScalarIdent>(pexprScalar->Pop());
	const CColRef *pcrScIdent = popScIdent->Pcr();
	GPOS_ASSERT(pcrPrEl->Id() != pcrScIdent->Id());
	GPOS_ASSERT(pcrPrEl->RetrieveType()->MDId()->Equals(
		pcrScIdent->RetrieveType()->MDId()));

	if (!CUtils::FConstrainableType(pcrPrEl->RetrieveType()->MDId()))
	{
		return nullptr;
	}

	BOOL non_nullable = not_null_columns->FMember(pcrScIdent);

	// only add renamed columns to equivalent class if the column is not null-able
	// this is because equality predicates will be inferred from the equivalent class
	// during preprocessing
	if (CColRef::EcrtTable == pcrScIdent->Ecrt() && non_nullable)
	{
		// equivalence class
		gpos::Ref<CColRefSetArray> pdrgpcrs = GPOS_NEW(mp) CColRefSetArray(mp);

		gpos::Ref<CColRefSet> pcrs = GPOS_NEW(mp) CColRefSet(mp);
		pcrs->Include(pcrPrEl);
		pcrs->Include(pcrScIdent);
		pdrgpcrs->Append(std::move(pcrs));

		return pdrgpcrs;
	}

	return nullptr;
}


//---------------------------------------------------------------------------
//	@function:
//		CLogicalProject::ExtractConstraintFromScConst
//
//	@doc:
//		Extract constraint from scalar constant project element
//
//---------------------------------------------------------------------------
void
CLogicalProject::ExtractConstraintFromScConst(
	CMemoryPool *mp, CExpression *pexprPrEl,
	CConstraintArray *pdrgpcnstr,  // array of range constraints
	CColRefSetArray *pdrgpcrs	   // array of equivalence class
)
{
	GPOS_ASSERT(nullptr != pexprPrEl);
	GPOS_ASSERT(nullptr != pdrgpcnstr);
	GPOS_ASSERT(nullptr != pdrgpcrs);

	CScalarProjectElement *popPrEl =
		gpos::dyn_cast<CScalarProjectElement>(pexprPrEl->Pop());
	CColRef *colref = popPrEl->Pcr();
	CExpression *pexprScalar = (*pexprPrEl)[0];

	IMDId *mdid_type = colref->RetrieveType()->MDId();

	if (EopScalarConst != pexprScalar->Pop()->Eopid() ||
		!CUtils::FConstrainableType(mdid_type))
	{
		return;
	}

	CScalarConst *popConst = gpos::dyn_cast<CScalarConst>(pexprScalar->Pop());
	IDatum *datum = popConst->GetDatum();

	gpos::Ref<CRangeArray> pdrgprng = GPOS_NEW(mp) CRangeArray(mp);
	BOOL is_null = datum->IsNull();
	if (!is_null)
	{
		;
		pdrgprng->Append(GPOS_NEW(mp) CRange(COptCtxt::PoctxtFromTLS()->Pcomp(),
											 IMDType::EcmptEq, datum));
	}

	pdrgpcnstr->Append(GPOS_NEW(mp) CConstraintInterval(
		mp, colref, std::move(pdrgprng), is_null));

	gpos::Ref<CColRefSet> pcrs = GPOS_NEW(mp) CColRefSet(mp);
	pcrs->Include(colref);
	pdrgpcrs->Append(std::move(pcrs));
}


//---------------------------------------------------------------------------
//	@function:
//		CLogicalProject::DerivePropertyConstraint
//
//	@doc:
//		Derive constraint property
//
//---------------------------------------------------------------------------
gpos::Ref<CPropConstraint>
CLogicalProject::DerivePropertyConstraint(CMemoryPool *mp,
										  CExpressionHandle &exprhdl) const
{
	CExpression *pexprPrL = exprhdl.PexprScalarExactChild(1);

	if (nullptr == pexprPrL)
	{
		return PpcDeriveConstraintPassThru(exprhdl, 0 /*ulChild*/);
	}

	gpos::Ref<CConstraintArray> pdrgpcnstr = GPOS_NEW(mp) CConstraintArray(mp);
	gpos::Ref<CColRefSetArray> pdrgpcrs = GPOS_NEW(mp) CColRefSetArray(mp);

	const ULONG ulProjElems = pexprPrL->Arity();
	for (ULONG ul = 0; ul < ulProjElems; ul++)
	{
		CExpression *pexprPrEl = (*pexprPrL)[ul];
		CExpression *pexprProjected = (*pexprPrEl)[0];

		if (EopScalarConst == pexprProjected->Pop()->Eopid())
		{
			ExtractConstraintFromScConst(mp, pexprPrEl, pdrgpcnstr.get(),
										 pdrgpcrs.get());
		}
		else
		{
			CColRefSet *not_null_columns =
				exprhdl.DeriveNotNullColumns(0 /*ulChild*/);
			gpos::Ref<CColRefSetArray> pdrgpcrsChild =
				PdrgpcrsEquivClassFromScIdent(mp, pexprPrEl, not_null_columns);

			if (nullptr != pdrgpcrsChild)
			{
				// merge with the equivalence classes we have so far
				gpos::Ref<CColRefSetArray> pdrgpcrsMerged =
					CUtils::PdrgpcrsMergeEquivClasses(mp, pdrgpcrs,
													  pdrgpcrsChild.get());

				// clean up
				;
				;

				pdrgpcrs = pdrgpcrsMerged;
			}
		}
	}

	if (0 == pdrgpcnstr->Size() && 0 == pdrgpcrs->Size())
	{
		// no constants or equivalence classes found, so just return the same constraint property of the child
		;
		;
		return PpcDeriveConstraintPassThru(exprhdl, 0 /*ulChild*/);
	}

	CPropConstraint *ppcChild =
		exprhdl.DerivePropertyConstraint(0 /* ulChild */);

	// equivalence classes coming from child
	CColRefSetArray *pdrgpcrsChild = ppcChild->PdrgpcrsEquivClasses();
	if (nullptr != pdrgpcrsChild)
	{
		// merge with the equivalence classes we have so far
		gpos::Ref<CColRefSetArray> pdrgpcrsMerged =
			CUtils::PdrgpcrsMergeEquivClasses(mp, pdrgpcrs, pdrgpcrsChild);

		// clean up
		;
		pdrgpcrs = pdrgpcrsMerged;
	}

	// constraint coming from child
	CConstraint *pcnstr = ppcChild->Pcnstr();
	if (nullptr != pcnstr)
	{
		;
		pdrgpcnstr->Append(pcnstr);
	}

	gpos::Ref<CConstraint> pcnstrNew =
		CConstraint::PcnstrConjunction(mp, std::move(pdrgpcnstr));

	return GPOS_NEW(mp)
		CPropConstraint(mp, std::move(pdrgpcrs), std::move(pcnstrNew));
}


//---------------------------------------------------------------------------
//	@function:
//		CLogicalProject::DeriveMaxCard
//
//	@doc:
//		Derive max card
//
//---------------------------------------------------------------------------
CMaxCard
CLogicalProject::DeriveMaxCard(CMemoryPool *,  // mp
							   CExpressionHandle &exprhdl) const
{
	if (exprhdl.DeriveHasNonScalarFunction(1))
	{
		// unbounded by default
		return CMaxCard();
	}

	// pass on max card of first child
	return exprhdl.DeriveMaxCard(0);
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalProject::PxfsCandidates
//
//	@doc:
//		Get candidate xforms
//
//---------------------------------------------------------------------------
gpos::Ref<CXformSet>
CLogicalProject::PxfsCandidates(CMemoryPool *mp) const
{
	gpos::Ref<CXformSet> xform_set = GPOS_NEW(mp) CXformSet(mp);

	(void) xform_set->ExchangeSet(CXform::ExfSimplifyProjectWithSubquery);
	(void) xform_set->ExchangeSet(CXform::ExfProject2Apply);
	(void) xform_set->ExchangeSet(CXform::ExfProject2ComputeScalar);
	(void) xform_set->ExchangeSet(CXform::ExfCollapseProject);

	return xform_set;
}

//---------------------------------------------------------------------------
//	@function:
//		CLogicalProject::PstatsDerive
//
//	@doc:
//		Derive statistics
//
//---------------------------------------------------------------------------
gpos::Ref<IStatistics>
CLogicalProject::PstatsDerive(CMemoryPool *mp, CExpressionHandle &exprhdl,
							  IStatisticsArray *  // stats_ctxt
) const
{
	gpos::Ref<UlongToIDatumMap> phmuldatum = GPOS_NEW(mp) UlongToIDatumMap(mp);

	// extract scalar constant expression that can be used for
	// statistics calculation
	CExpression *pexprPrList = exprhdl.PexprScalarRepChild(1 /*child_index*/);
	const ULONG arity = pexprPrList->Arity();
	for (ULONG ul = 0; ul < arity; ul++)
	{
		CExpression *pexprPrElem = (*pexprPrList)[ul];
		GPOS_ASSERT(1 == pexprPrElem->Arity());
		CColRef *colref =
			gpos::dyn_cast<CScalarProjectElement>(pexprPrElem->Pop())->Pcr();

		CExpression *pexprScalar = (*pexprPrElem)[0];
		COperator *pop = pexprScalar->Pop();
		if (COperator::EopScalarConst == pop->Eopid())
		{
			IDatum *datum = gpos::dyn_cast<CScalarConst>(pop)->GetDatum();
			if (datum->StatsMappable())
			{
				;
				BOOL fInserted GPOS_ASSERTS_ONLY =
					phmuldatum->Insert(GPOS_NEW(mp) ULONG(colref->Id()), datum);
				GPOS_ASSERT(fInserted);
			}
		}
	}

	gpos::Ref<IStatistics> stats =
		PstatsDeriveProject(mp, exprhdl, phmuldatum.get());

	// clean up
	;

	return stats;
}


// EOF
