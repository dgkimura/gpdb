//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2018 Pivotal Software Inc.
//
//	Base class for transforming Join to Index Apply
//---------------------------------------------------------------------------
#ifndef GPOPT_CXformJoin2IndexApplyGeneric_H
#define GPOPT_CXformJoin2IndexApplyGeneric_H

#include "gpos/base.h"
#include "gpopt/operators/CLogicalApply.h"
#include "gpopt/operators/CLogicalDynamicGet.h"
#include "gpopt/operators/CLogicalGet.h"
#include "gpopt/operators/CLogicalJoin.h"
#include "gpopt/operators/CPatternLeaf.h"
#include "gpopt/operators/CPatternNode.h"
#include "gpopt/xforms/CXformJoin2IndexApply.h"

namespace gpopt
{
	using namespace gpos;

	class CXformJoin2IndexApplyGeneric : public CXformJoin2IndexApply
	{
	private:

		// private copy ctor
		CXformJoin2IndexApplyGeneric(const CXformJoin2IndexApplyGeneric &);

		// Can transform left outer join to left outer index apply?
		// For hash distributed table, we can do outer index apply only
		// when the inner columns used in the join condition contains
		// the inner distribution key set. Master only table is ok to
		// transform to outer index apply, but random table is not.
		// Because if the inner is random distributed, there is no way
		// to redistribute outer child to match inner on the join keys.
		BOOL
		FCanLeftOuterIndexApply
		(
		 CMemoryPool *mp,
		 CExpression *pexprInner,
		 CExpression *pexprScalar,
		 CTableDescriptor *ptabDesc,
		 const CColRefSet *pcrsDist
		 ) const
		{
			GPOS_ASSERT(m_fOuterJoin);
			IMDRelation::Ereldistrpolicy ereldist = ptabDesc->GetRelDistribution();

			if (ereldist == IMDRelation::EreldistrRandom)
				return false;
			else if (ereldist == IMDRelation::EreldistrMasterOnly)
				return true;

			// now consider hash distributed table
			CColRefSet *pcrsInnerOutput = pexprInner->DeriveOutputColumns();
			CColRefSet *pcrsScalarExpr = pexprScalar->DeriveUsedColumns();
			CColRefSet *pcrsInnerRefs = GPOS_NEW(mp) CColRefSet(mp, *pcrsScalarExpr);
			pcrsInnerRefs->Intersection(pcrsInnerOutput);

			// Distribution key set of inner GET must be subset of inner columns used in
			// the left outer join condition, but doesn't need to be equal.
			BOOL fCanOuterIndexApply = pcrsInnerRefs->ContainsAll(pcrsDist);
			pcrsInnerRefs->Release();
			if (fCanOuterIndexApply)
			{
				CColRefSet *pcrsEquivPredInner = GPOS_NEW(mp) CColRefSet(mp);
				// extract array of join predicates from join condition expression
				CExpressionArray *pdrgpexpr = CPredicateUtils::PdrgpexprConjuncts(mp, pexprScalar);
				for (ULONG ul = 0; ul < pdrgpexpr->Size(); ul++)
				{
					CExpression *pexprPred = (*pdrgpexpr)[ul];
					CColRefSet *pcrsPred = pexprPred->DeriveUsedColumns();

					// if it doesn't have equi-join predicate on the distribution key,
					// we can't transform to left outer index apply, because only
					// redistribute motion is allowed for outer child of join with
					// hash distributed inner child.
					// consider R LOJ S (both distribute by a and have index on a)
					// with the predicate S.a = R.a and S.a > R.b, left outer index
					// apply is still applicable.
					if (!pcrsPred->IsDisjoint(pcrsDist) &&
						CPredicateUtils::IsEqualityOp(pexprPred))
					{
						pcrsEquivPredInner->Include(pcrsPred);
					}
				}
				fCanOuterIndexApply = pcrsEquivPredInner->ContainsAll(pcrsDist);
				pcrsEquivPredInner->Release();
				pdrgpexpr->Release();
			}

			return fCanOuterIndexApply;
		}

	public:

		// ctor
		explicit
		CXformJoin2IndexApplyGeneric(CMemoryPool *mp)
		:
		// pattern
		CXformJoin2IndexApply
		(
		 GPOS_NEW(mp) CExpression
		 (
		  mp,
		  GPOS_NEW(mp) CPatternNode(mp, CPatternNode::EmtMatchInnerOrLeftOuterJoin),
		  GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CPatternLeaf(mp)), // outer child
		  GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CPatternTree(mp)), // inner child
		  GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CPatternTree(mp))  // predicate tree operator,
		 )
		)
		{}

		// dtor
		virtual
		~CXformJoin2IndexApplyGeneric()
		{}

		// actual transform
		virtual
		void Transform(CXformContext *pxfctxt, CXformResult *pxfres, CExpression *pexpr) const
		{
			GPOS_ASSERT(NULL != pxfctxt);
			GPOS_ASSERT(FPromising(pxfctxt->Pmp(), this, pexpr));
			GPOS_ASSERT(FCheckPattern(pexpr));

			CMemoryPool *mp = pxfctxt->Pmp();

			// extract components
			CExpression *pexprOuter = (*pexpr)[0];
			CExpression *pexprInner = (*pexpr)[1];
			CExpression *pexprScalar = (*pexpr)[2];

			CExpression *pexprCurrInnerChild = pexprInner;
			CExpression *pexprAllPredicates = pexprScalar;
			CExpression *selectThatIsParentOfGet = NULL;
			CExpression *pexprGet = NULL;

			// walk down the right child tree, accepting some unary operators
			// like project and GbAgg and select, until we find a get
			while (NULL == pexprGet)
			{
				switch (pexprCurrInnerChild->Pop()->Eopid())
				{
					case COperator::EopLogicalSelect:
						selectThatIsParentOfGet = pexprCurrInnerChild;
						break;

					case COperator::EopLogicalProject:
					case COperator::EopLogicalGbAgg:
						// we tolerate these operators in the tree and will just copy them into
						// the result of the transform
						pexprGet = pexprCurrInnerChild;
						break;

					case COperator::EopLogicalGet:
					case COperator::EopLogicalDynamicGet:
						pexprGet = pexprCurrInnerChild;
						break;

					default:
						// in all other cases, the expression does not conform to our
						// expectations and we won't generate an alternative
						return;
				}
				pexprCurrInnerChild = (*pexprCurrInnerChild)[0];
			}

			if (NULL != selectThatIsParentOfGet)
			{
				pexprAllPredicates = CPredicateUtils::PexprConjunction(mp, pexprAllPredicates, (*selectThatIsParentOfGet)[1]);
			}
			else
			{
				pexprAllPredicates->AddRef();
			}

			CTableDescriptor *ptabdescInner = NULL;
			const CColRefSet *distributionCols = NULL;
			CLogicalDynamicGet *popDynamicGet = NULL;
			if (COperator::EopLogicalDynamicGet == pexprGet->Pop()->Eopid())
			{
				popDynamicGet = CLogicalDynamicGet::PopConvert(pexprGet->Pop());
				ptabdescInner = popDynamicGet->Ptabdesc();
				distributionCols = popDynamicGet->PcrsDist();
			}
			else
			{
				CLogicalGet *popGet = CLogicalGet::PopConvert(pexprGet->Pop());
				ptabdescInner = popGet->Ptabdesc();
				distributionCols = popGet->PcrsDist();
			}

			if (m_fOuterJoin && !FCanLeftOuterIndexApply(mp, pexprGet, pexprScalar, ptabdescInner, distributionCols))
			{
				// It is a left outer join, but we can't do outer index apply,
				// stop transforming and return immediately.
				CRefCount::SafeRelease(pexprAllPredicates);
				return;
			}

			// for now, try only homogeneous indexes
			CreateHomogeneousIndexApplyAlternatives
				(
				 mp,
				 pexpr->Pop()->UlOpId(),
				 pexprOuter,
				 pexprGet,
				 pexprAllPredicates,
				 ptabdescInner,
				 popDynamicGet,
				 pxfres,
				 IMDIndex::EmdindBtree  // FIXME: also do this for bitmap
				 );
			CRefCount::SafeRelease(pexprAllPredicates);
		}

		// Return true if xform should be applied only once.
		// For now return false. We may need to revisit this if we find that
		// there are multiple bindings and we miss interesting bindings because
		// we extract only one of them.
		virtual
		BOOL IsApplyOnce()
		{
			return false;
		}

	}; // class CXformJoin2IndexApplyBase

}

#endif // !GPOPT_CXformJoin2IndexApplyGeneric_H

// EOF

