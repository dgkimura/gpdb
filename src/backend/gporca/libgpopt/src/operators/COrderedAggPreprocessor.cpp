//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright 2022 VMware, Inc. or its affiliates.
//
//	@filename:
//		COrderedAggPreprocessor.cpp
//
//	@doc:
//		Preprocessing routines of ordered-set aggregate
//---------------------------------------------------------------------------

#include "gpopt/operators/COrderedAggPreprocessor.h"

#include "gpos/base.h"
#include "gpos/common/owner.h"

#include "gpopt/base/COptCtxt.h"
#include "gpopt/base/CUtils.h"
#include "gpopt/operators/CLogicalCTEAnchor.h"
#include "gpopt/operators/CLogicalCTEConsumer.h"
#include "gpopt/operators/CLogicalGbAgg.h"
#include "gpopt/operators/CLogicalInnerJoin.h"
#include "gpopt/operators/CLogicalLimit.h"
#include "gpopt/operators/CLogicalSequenceProject.h"
#include "gpopt/operators/CPredicateUtils.h"
#include "gpopt/operators/CScalarIdent.h"
#include "gpopt/operators/CScalarProjectElement.h"
#include "gpopt/operators/CScalarProjectList.h"
#include "gpopt/operators/CScalarSortGroupClause.h"
#include "gpopt/operators/CScalarValuesList.h"
#include "gpopt/operators/CScalarWindowFunc.h"
#include "gpopt/xforms/CXformUtils.h"

using namespace gpopt;


//---------------------------------------------------------------------------
//	@function:
//		COrderedAggPreprocessor::SplitPrjList
//
//	@doc:
//		Iterate over project elements and split them elements between
//		Ordered Aggs list, and Other Aggs list
//
//---------------------------------------------------------------------------
void
COrderedAggPreprocessor::SplitPrjList(
	CMemoryPool *mp, gpos::pointer<CExpression *> pexprInputAggPrj,
	gpos::owner<CExpressionArray *> *
		ppdrgpexprOrderedAggsPrEl,	// output: list of project elements with Ordered Aggs
	gpos::owner<CExpressionArray *> *
		ppdrgpexprOtherPrEl	 // output: list of project elements with Other aggregate functions
)
{
	GPOS_ASSERT(nullptr != pexprInputAggPrj);
	GPOS_ASSERT(nullptr != ppdrgpexprOrderedAggsPrEl);
	GPOS_ASSERT(nullptr != ppdrgpexprOtherPrEl);

	gpos::pointer<CExpression *> pexprPrjList = (*pexprInputAggPrj)[1];

	gpos::owner<CExpressionArray *> pdrgpexprOrderedAggsPrEl =
		GPOS_NEW(mp) CExpressionArray(mp);

	gpos::owner<CExpressionArray *> pdrgpexprOtherPrEl =
		GPOS_NEW(mp) CExpressionArray(mp);
	// create CTE using SeqPrj child expression
	gpos::owner<CExpression *> pexprRemappedAggConsumer = nullptr;
	gpos::owner<CExpression *> pexprAggConsumer = nullptr;
	CreateCTE(mp, pexprInputAggPrj, &pexprRemappedAggConsumer,
			  &pexprAggConsumer);

	// iterate over project list and split project elements between
	// Ordered Aggs list, and Other aggs list
	const ULONG arity = pexprPrjList->Arity();
	gpos::owner<CColRefArray *> colref_array = nullptr;
	gpos::owner<CExpressionArray *> pdrgpexprSortClauseArray = nullptr;

	for (ULONG ul = 0; ul < arity; ul++)
	{
		gpos::pointer<CExpression *> pexprPrjEl = (*pexprPrjList)[ul];
		CExpression *pexprAggFunc = (*pexprPrjEl)[0];
		gpos::pointer<CScalarProjectElement *> popScPrjElem =
			gpos::dyn_cast<CScalarProjectElement>(pexprPrjEl->Pop());
		CColRef *pcrPrjElem = popScPrjElem->Pcr();

		if (CUtils::FHasOrderedAggToSplit(pexprAggFunc))
		{
			gpos::pointer<CExpression *> pexprSortCol =
				(*(*pexprAggFunc)[EAggfuncChildIndices::EaggfuncIndexArgs])[0];
			CExpression *pexprSortGroupClause =
				(*(*pexprAggFunc)[EAggfuncChildIndices::EaggfuncIndexOrder])[0];
			gpos::pointer<CScalarSortGroupClause *> curr_sort_clause =
				gpos::dyn_cast<CScalarSortGroupClause>(
					pexprSortGroupClause->Pop());

			const CColRef *colref =
				CCastUtils::PcrExtractFromScIdOrCastScId(pexprSortCol);
			BOOL skip = false;
			if (nullptr == pdrgpexprSortClauseArray)
			{
				colref_array = GPOS_NEW(mp) CColRefArray(mp);
				colref_array->Append(const_cast<CColRef *>(colref));
				pdrgpexprSortClauseArray = GPOS_NEW(mp) CExpressionArray(mp);
				pexprSortGroupClause->AddRef();
				pdrgpexprSortClauseArray->Append(pexprSortGroupClause);
			}
			else
			{
				for (ULONG uidx = 0; uidx < pdrgpexprSortClauseArray->Size();
					 uidx++)
				{
					// For multiple ordered-set aggs on the same ORDERING column and ORDERING SPEC(ASC/DESC), we optimize
					// to add the new aggregate as a ProjectiElement to the ProjectList of an existing JOIN, instead of
					// creating a new JOIN for doing the same SORT.
					// Eg. SELECT percentile_cont(0.25) WITHIN GROUP(ORDER BY a), percentile_cont(0.5) WITHIN GROUP(ORDER BY a) from tab;
					// Currently, to check if colref's SORT ORDER Spec(Asc/Desc) is same, we match only the SortOp and NullFirst
					// as that is what is used for creating the OrderSpec
					// TO-DO: (enhancement) Instead of this loop, we can probably use a map (colref->SortGroupClause)
					gpos::pointer<CScalarSortGroupClause *>
						visited_sort_clause =
							gpos::dyn_cast<CScalarSortGroupClause>(
								(*pdrgpexprSortClauseArray)[uidx]->Pop());
					if (colref == (*colref_array)[uidx] &&
						curr_sort_clause->SortOp() ==
							visited_sort_clause->SortOp() &&
						curr_sort_clause->NullsFirst() ==
							visited_sort_clause->NullsFirst())
					{
						// Create the new ordered agg function and append it as a ProjElement to the existing ProjectList
						// created for and already split ordered agg on the same column with same ORDERING SPEC
						// Existing ProjList:
						// +--CScalarProjectList
						// +--CScalarProjectElement \"percentile_cont\" (12)
						//    +--CScalarAggFunc
						//       |--CScalarValuesList
						//       |  |--CScalarIdent \"a\" (1)
						//       |  |--CScalarConst (0.2500)
						//       |  +--CScalarIdent \"ColRef_0041\" (41)
						//       ...
						gpos::pointer<CExpression *> pCurrExprProjList =
							(*(*pdrgpexprOrderedAggsPrEl)[uidx])[1];
						gpos::pointer<CExpression *> pCurrExprAggFunc =
							(*(*pCurrExprProjList)[0])[0];
						const CColRef *total_count_colref =
							CCastUtils::PcrExtractFromScIdOrCastScId((*(
								*pCurrExprAggFunc)[EAggfuncChildIndices::
													   EaggfuncIndexArgs])[2]);

						gpos::owner<CExpression *> pexprNewAggFunc =
							PexprFinalAgg(
								mp, pexprAggFunc, pcrPrjElem,
								const_cast<CColRef *>(total_count_colref));
						gpos::owner<CExpression *> pexprAddPrjElem =
							CUtils::PexprScalarProjectElement(mp, pcrPrjElem,
															  pexprNewAggFunc);
						// Updated ProjList:
						// +--CScalarProjectList
						//    |--CScalarProjectElement \"percentile_cont\" (12)
						//    |  +--CScalarAggFunc
						//    |     |--CScalarValuesList
						//    |     |  |--CScalarIdent \"a\" (1)
						//    |     |  |--CScalarConst (0.2500)
						//    |     |  +--CScalarIdent \"ColRef_0041\" (41)
						//    |     ...
						//    +--CScalarProjectElement \"percentile_cont\" (13)
						//       +--CScalarAggFunc
						//          |--CScalarValuesList
						//          |  |--CScalarIdent \"a\" (1)
						//          |  |--CScalarConst (0.500)
						//          |  +--CScalarIdent \"ColRef_0041\" (41)
						//          ...
						gpos::pointer<CExpressionArray *> pCurrProjList =
							pCurrExprProjList->PdrgPexpr();
						pCurrProjList->Append(pexprAddPrjElem);
						skip = true;
						break;
					}
				}
				if (!skip)
				{
					colref_array->Append(const_cast<CColRef *>(colref));
					pexprSortGroupClause->AddRef();
					pdrgpexprSortClauseArray->Append(pexprSortGroupClause);
				}
			}
			if (skip)
				continue;
			if (0 < ul)
			{
				pexprRemappedAggConsumer->AddRef();
				pexprAggConsumer->AddRef();
			}
			// CREATE total count aggregate
			CColumnFactory *col_factory = COptCtxt::PoctxtFromTLS()->Pcf();
			CMDAccessor *md_accessor = COptCtxt::PoctxtFromTLS()->Pmda();
			gpos::owner<CExpression *> pexprAgg = CUtils::PexprAgg(
				mp, md_accessor, IMDType::EaggCount, colref, false);
			CColRef *total_count_colref = col_factory->PcrCreate(
				md_accessor->RetrieveType(
					gpos::dyn_cast<CScalarAggFunc>(pexprAgg->Pop())
						->MdidType()),
				gpos::dyn_cast<CScalarAggFunc>(pexprAgg->Pop())
					->TypeModifier());
			gpos::owner<CExpression *> pexprNewPrjElem =
				CUtils::PexprScalarProjectElement(mp, total_count_colref,
												  pexprAgg);
			gpos::owner<CExpression *> pexprCountAggPrjList =
				GPOS_NEW(mp) CExpression(
					mp, GPOS_NEW(mp) CScalarProjectList(mp), pexprNewPrjElem);
			(*pexprInputAggPrj)[0]->AddRef();
			gpos::owner<CExpression *> pexprTotalCountAgg =
				CUtils::PexprLogicalGbAggGlobal(
					mp, GPOS_NEW(mp) CColRefArray(mp), (*pexprInputAggPrj)[0],
					pexprCountAggPrjList);

			// to match requested columns upstream, we have to re-use the same computed
			// columns that define the aggregates, we avoid recreating new columns during
			// expression copy by passing must_exist as false
			gpos::owner<CColRefArray *> pdrgpcrChildOutput =
				pexprAggConsumer->DeriveOutputColumns()->Pdrgpcr(mp);
			gpos::pointer<CColRefArray *> pdrgpcrConsumerOutput =
				gpos::dyn_cast<CLogicalCTEConsumer>(
					pexprRemappedAggConsumer->Pop())
					->Pdrgpcr();
			gpos::owner<UlongToColRefMap *> colref_mapping =
				CUtils::PhmulcrMapping(mp, pdrgpcrChildOutput,
									   pdrgpcrConsumerOutput);
			gpos::owner<CExpression *> pexprTotalCountAggRemapped =
				pexprTotalCountAgg->PexprCopyWithRemappedColumns(
					mp, colref_mapping, false /*must_exist*/);
			colref_mapping->Release();
			pdrgpcrChildOutput->Release();
			pexprTotalCountAgg->Release();

			// finalize total count agg expression by replacing its child with CTE consumer
			pexprTotalCountAggRemapped->Pop()->AddRef();
			(*pexprTotalCountAggRemapped)[1]->AddRef();
			gpos::owner<CExpression *> pexprFinalGbAgg =
				GPOS_NEW(mp) CExpression(mp, pexprTotalCountAggRemapped->Pop(),
										 pexprRemappedAggConsumer,
										 (*pexprTotalCountAggRemapped)[1]);
			pexprTotalCountAggRemapped->Release();


			gpos::owner<CExpression *> pexprJoinCondition =
				CUtils::PexprScalarConstBool(mp, true /*value*/);

			// create a join between expanded total count and CTE Consumer expressions
			gpos::owner<CExpression *> pexprJoin =
				CUtils::PexprLogicalJoin<CLogicalInnerJoin>(
					mp, pexprAggConsumer, pexprFinalGbAgg, pexprJoinCondition);

			// create ordered spec for the Join to merge on the colref passed in as part
			// of the ordered agg's WITHIN(ORDER BY) clause
			gpos::owner<COrderSpec *> pos = GPOS_NEW(mp) COrderSpec(mp);
			gpos::owner<IMDId *> mdid_curr_sortop =
				GPOS_NEW(mp) CMDIdGPDB(curr_sort_clause->SortOp());
			IMDType::ECmpType curr_cmp_type =
				CUtils::ParseCmpType(mdid_curr_sortop);
			// Instead of using the SortOp directly, we use the colref's operator for the comparison type
			// to avoid incorret results for cast related columns
			IMDId *mdid_pos =
				colref->RetrieveType()->GetMdidForCmpType(curr_cmp_type);
			mdid_curr_sortop->Release();
			COrderSpec::ENullTreatment ent = COrderSpec::EntLast;
			if (curr_sort_clause->NullsFirst())
			{
				ent = COrderSpec::EntFirst;
			}
			mdid_pos->AddRef();
			pos->Append(mdid_pos, colref, ent);
			gpos::owner<CLogicalLimit *> popLimit = GPOS_NEW(mp)
				CLogicalLimit(mp, pos, true /* fGlobal */, true /* fHasCount */,
							  false /*fTopLimitUnderDML*/);
			gpos::owner<CExpression *> pexprLimitOffset =
				CUtils::PexprScalarConstInt8(mp, 0);
			gpos::owner<CExpression *> pexprLimitCount =
				CUtils::PexprScalarConstInt8(mp, 1, true);

			gpos::owner<CExpression *> pexprJoinLimit =
				GPOS_NEW(mp) CExpression(mp, popLimit, pexprJoin,
										 pexprLimitOffset, pexprLimitCount);

			// Create the final gp_percentile agg on top
			gpos::owner<CExpression *> pexprNewAggFunc =
				PexprFinalAgg(mp, pexprAggFunc, pcrPrjElem, total_count_colref);
			gpos::owner<CExpression *> pexprExistingPrjElem =
				CUtils::PexprScalarProjectElement(mp, pcrPrjElem,
												  pexprNewAggFunc);
			gpos::owner<CExpression *> pexprNewAggPrjList = GPOS_NEW(mp)
				CExpression(mp, GPOS_NEW(mp) CScalarProjectList(mp),
							pexprExistingPrjElem);
			gpos::owner<CColRefArray *> colref_array =
				GPOS_NEW(mp) CColRefArray(mp);
			gpos::owner<CExpression *> pexprNewAgg =
				CUtils::PexprLogicalGbAggGlobal(
					mp, colref_array, pexprJoinLimit, pexprNewAggPrjList);
			pdrgpexprOrderedAggsPrEl->Append(pexprNewAgg);
		}
		else
		{
			pexprAggFunc->AddRef();
			gpos::owner<CExpression *> pexprNewPrjElem =
				CUtils::PexprScalarProjectElement(mp, pcrPrjElem, pexprAggFunc);
			pdrgpexprOtherPrEl->Append(pexprNewPrjElem);
		}
	}
	CRefCount::SafeRelease(std::move(colref_array));
	CRefCount::SafeRelease(std::move(pdrgpexprSortClauseArray));
	*ppdrgpexprOrderedAggsPrEl = pdrgpexprOrderedAggsPrEl;
	*ppdrgpexprOtherPrEl = pdrgpexprOtherPrEl;
}


//---------------------------------------------------------------------------
//	@function:
//		COrderedAggPreprocessor::SplitOrderedAggsPrj
//
//	@doc:
//		Split InputAgg expression into:
//		- A GbAgg expression containing ordered aggs split to gp_percentile agg, and
//		- A GbAgg expression containing all remaining non-ordered agg functions
//
//---------------------------------------------------------------------------
void
COrderedAggPreprocessor::SplitOrderedAggsPrj(
	CMemoryPool *mp, gpos::pointer<CExpression *> pexprInputAggPrj,
	gpos::owner<CExpression *> *
		ppexprOrderedGbAgg,	 // output: GbAgg expression containing ordered aggs split to normal agg
	gpos::owner<CExpression *> *
		ppexprRemainingAgg	// output: GbAgg expression containing all remaining agg functions
)
{
	GPOS_ASSERT(nullptr != pexprInputAggPrj);
	GPOS_ASSERT(nullptr != ppexprOrderedGbAgg);
	GPOS_ASSERT(nullptr != ppexprRemainingAgg);

	// split project elements between ordered Aggs list, and Other aggs list
	gpos::owner<CExpressionArray *> ppdrgpexprOrderedAggsPrEl = nullptr;
	gpos::owner<CExpressionArray *> pdrgpexprOtherPrEl = nullptr;
	SplitPrjList(mp, pexprInputAggPrj, &ppdrgpexprOrderedAggsPrEl,
				 &pdrgpexprOtherPrEl);

	CExpression *pexprInputAggPrjChild = (*pexprInputAggPrj)[0];

	gpos::owner<CExpression *> pexpr = (*ppdrgpexprOrderedAggsPrEl)[0];
	pexpr->AddRef();
	// in case of multiple ordered aggs, we need to expand the GbAgg expression
	// into a join expression where leaves carry split ordered aggs
	for (ULONG ul = 1; ul < ppdrgpexprOrderedAggsPrEl->Size(); ul++)
	{
		CExpression *pexprWindowConsumer = (*ppdrgpexprOrderedAggsPrEl)[ul];
		gpos::owner<CExpression *> pexprJoinCondition =
			CUtils::PexprScalarConstBool(mp, true /*value*/);

		// create a join between expanded DQAs and Window expressions
		pexprWindowConsumer->AddRef();
		gpos::owner<CExpression *> pexprJoin =
			CUtils::PexprLogicalJoin<CLogicalInnerJoin>(
				mp, pexpr, pexprWindowConsumer, pexprJoinCondition);
		pexpr = pexprJoin;
	}

	*ppexprOrderedGbAgg = pexpr;
	if (0 == pdrgpexprOtherPrEl->Size())
	{
		// no remaining aggregate functions after excluding ordered aggs,
		// reuse the original InputAggPrj child in this case
		pdrgpexprOtherPrEl->Release();
		ppdrgpexprOrderedAggsPrEl->Release();
		*ppexprRemainingAgg = pexprInputAggPrjChild;

		return;
	}

	// create a new GbAgg expression for remaining aggregate functions
	gpos::owner<CColRefArray *> pdrgpcrGrpCols = nullptr;
	pdrgpcrGrpCols = GPOS_NEW(mp) CColRefArray(mp);
	pexprInputAggPrjChild->AddRef();
	*ppexprRemainingAgg = GPOS_NEW(mp) CExpression(
		mp,
		GPOS_NEW(mp) CLogicalGbAgg(mp, std::move(pdrgpcrGrpCols),
								   COperator::EgbaggtypeGlobal),
		pexprInputAggPrjChild,
		GPOS_NEW(mp) CExpression(mp, GPOS_NEW(mp) CScalarProjectList(mp),
								 std::move(pdrgpexprOtherPrEl)));
	ppdrgpexprOrderedAggsPrEl->Release();
}


//---------------------------------------------------------------------------
//	@function:
//		COrderedAggPreprocessor::CreateCTE
//
//	@doc:
//		Create a CTE with two consumers using the child expression of
//		Sequence Project
//
//---------------------------------------------------------------------------
void
COrderedAggPreprocessor::CreateCTE(
	CMemoryPool *mp, gpos::pointer<CExpression *> pexprInputAggPrj,
	gpos::owner<CExpression *> *ppexprFirstConsumer,
	gpos::owner<CExpression *> *ppexprSecondConsumer)
{
	GPOS_ASSERT(nullptr != pexprInputAggPrj);
	GPOS_ASSERT(COperator::EopLogicalGbAgg == pexprInputAggPrj->Pop()->Eopid());
	GPOS_ASSERT(nullptr != ppexprFirstConsumer);
	GPOS_ASSERT(nullptr != ppexprSecondConsumer);

	gpos::pointer<CExpression *> pexprChild = (*pexprInputAggPrj)[0];
	gpos::pointer<CColRefSet *> pcrsChildOutput =
		pexprChild->DeriveOutputColumns();
	gpos::owner<CColRefArray *> pdrgpcrChildOutput =
		pcrsChildOutput->Pdrgpcr(mp);

	// create a CTE producer based on SeqPrj child expression
	gpos::pointer<CCTEInfo *> pcteinfo = COptCtxt::PoctxtFromTLS()->Pcteinfo();
	const ULONG ulCTEId = pcteinfo->next_id();
	gpos::pointer<CExpression *> pexprCTEProd =
		CXformUtils::PexprAddCTEProducer(mp, ulCTEId, pdrgpcrChildOutput,
										 pexprChild);
	gpos::owner<CColRefArray *> pdrgpcrProducerOutput =
		pexprCTEProd->DeriveOutputColumns()->Pdrgpcr(mp);

	//	 first consumer creates new output columns to be used later as input to GbAgg expression
	*ppexprFirstConsumer = GPOS_NEW(mp) CExpression(
		mp, GPOS_NEW(mp) CLogicalCTEConsumer(
				mp, ulCTEId, CUtils::PdrgpcrCopy(mp, pdrgpcrProducerOutput)));
	pcteinfo->IncrementConsumers(ulCTEId);
	pdrgpcrProducerOutput->Release();

	// second consumer reuses the same output columns of SeqPrj child to be able to provide any requested columns upstream
	*ppexprSecondConsumer = GPOS_NEW(mp)
		CExpression(mp, GPOS_NEW(mp) CLogicalCTEConsumer(
							mp, ulCTEId, std::move(pdrgpcrChildOutput)));
	pcteinfo->IncrementConsumers(ulCTEId);
}


//---------------------------------------------------------------------------
//    @function:
//        COrderedAggPreprocessor::PexprFinalAgg
//
//    @doc:
//        Return expression for final agg function
//        Input:
//          expression for percentile_cont(direct_arg) WITHIN GROUP(ORDER BY col)
//          total_count
//        Returns:
//          expression for gp_percentile_cont(col, direct_arg, total_count, peer_count)
//
//---------------------------------------------------------------------------
gpos::owner<CExpression *>
COrderedAggPreprocessor::PexprFinalAgg(
	CMemoryPool *mp, gpos::pointer<CExpression *> pexprAggFunc,
	CColRef *arg_col_ref, CColRef *total_count_colref)
{
	gpos::pointer<CScalarAggFunc *> popScAggFunc =
		gpos::dyn_cast<CScalarAggFunc>(pexprAggFunc->Pop());
	gpos::owner<CExpressionArray *> pdrgpexpr =
		GPOS_NEW(mp) CExpressionArray(mp);
	gpos::owner<CMDIdGPDB *> mdid =
		CUtils::GetPercentileAggMDId(mp, pexprAggFunc);
	gpos::owner<ULongPtrArray *> argtypes = GPOS_NEW(mp) ULongPtrArray(mp);

	gpos::owner<CScalarValuesList *> popScalarValuesList =
		GPOS_NEW(mp) CScalarValuesList(mp);
	gpos::owner<CExpressionArray *> pdrgpexprChildren =
		GPOS_NEW(mp) CExpressionArray(mp);

	gpos::owner<CExpression *> pexprAggFuncArg =
		(*(*pexprAggFunc)[EAggfuncChildIndices::EaggfuncIndexArgs])[0];
	pexprAggFuncArg->AddRef();
	pdrgpexprChildren->Append(pexprAggFuncArg);
	argtypes->Append(GPOS_NEW(mp) ULONG(
		gpos::dyn_cast<CMDIdGPDB>(
			gpos::dyn_cast<CScalar>(pexprAggFuncArg->Pop())->MdidType())
			->Oid()));

	gpos::owner<CExpression *> pexprAggFuncDirectArg =
		(*(*pexprAggFunc)[EAggfuncChildIndices::EaggfuncIndexDirectArgs])[0];
	pexprAggFuncDirectArg->AddRef();
	pdrgpexprChildren->Append(pexprAggFuncDirectArg);
	argtypes->Append(GPOS_NEW(mp) ULONG(
		gpos::dyn_cast<CMDIdGPDB>(
			gpos::dyn_cast<CScalar>(pexprAggFuncDirectArg->Pop())->MdidType())
			->Oid()));
	pdrgpexprChildren->Append(CUtils::PexprScalarIdent(mp, total_count_colref));
	argtypes->Append(GPOS_NEW(mp) ULONG(
		gpos::dyn_cast<CMDIdGPDB>(total_count_colref->RetrieveType()->MDId())
			->Oid()));
	// Currently passing dummy peer_count as 1 always. Will pass along the
	// calculated peer_count col_ref once we dedup data for handling skew
	gpos::owner<CExpression *> pexprDummyPeerCount =
		CUtils::PexprScalarConstInt8(mp, 1 /*val*/);
	pdrgpexprChildren->Append(pexprDummyPeerCount);
	argtypes->Append(GPOS_NEW(mp) ULONG(
		gpos::dyn_cast<CMDIdGPDB>(
			gpos::dyn_cast<CScalar>(pexprDummyPeerCount->Pop())->MdidType())
			->Oid()));

	pdrgpexpr->Append(
		GPOS_NEW(mp) CExpression(mp, popScalarValuesList, pdrgpexprChildren));

	pdrgpexpr->Append(GPOS_NEW(mp)
						  CExpression(mp, GPOS_NEW(mp) CScalarValuesList(mp),
									  GPOS_NEW(mp) CExpressionArray(mp)));

	pdrgpexpr->Append(GPOS_NEW(mp)
						  CExpression(mp, GPOS_NEW(mp) CScalarValuesList(mp),
									  GPOS_NEW(mp) CExpressionArray(mp)));

	pdrgpexpr->Append(GPOS_NEW(mp)
						  CExpression(mp, GPOS_NEW(mp) CScalarValuesList(mp),
									  GPOS_NEW(mp) CExpressionArray(mp)));

	gpos::owner<IMDId *> ret_type = nullptr;
	if (popScAggFunc->FHasAmbiguousReturnType())
	{
		popScAggFunc->MdidType()->AddRef();
		ret_type = popScAggFunc->MdidType();
	}
	gpos::owner<CScalarAggFunc *> popNewAggFunc = CUtils::PopAggFunc(
		mp, std::move(mdid), arg_col_ref->Name().Pstr(), false /*is_distinct*/,
		EaggfuncstageGlobal /*eaggfuncstage*/, false /*fSplit*/,
		std::move(ret_type), EaggfunckindNormal, std::move(argtypes));

	return GPOS_NEW(mp)
		CExpression(mp, std::move(popNewAggFunc), std::move(pdrgpexpr));
}


//---------------------------------------------------------------------------
//	@function:
//		COrderedAggPreprocessor::PexprInputAggPrj2Join
//
//	@doc:
//		Transform GbAgg project expression with ordered aggregates
//		into an inner join expression,
//			- the outer child of the join is a GbAgg expression that computes
//			gp_percentile aggs,
//			- the inner child of the join is a GbAgg expression that computes
//			remaining aggregate functions
//
//		we use a CTE to compute the input to both join children, while maintaining
//		all column references upstream by reusing the same computed columns in the
//		original input expression
//
//---------------------------------------------------------------------------
CExpression *
COrderedAggPreprocessor::PexprInputAggPrj2Join(
	CMemoryPool *mp, gpos::pointer<CExpression *> pexprInputAggPrj)
{
	GPOS_ASSERT(nullptr != pexprInputAggPrj);
	GPOS_ASSERT(COperator::EopLogicalGbAgg == pexprInputAggPrj->Pop()->Eopid());
	GPOS_ASSERT(0 < (*pexprInputAggPrj)[1]->DeriveTotalOrderedAggs());

	// split InputAgg expression into a GbAgg expression (for ordered Aggs), and
	// another GbAgg expression (for remaining aggregate functions)
	gpos::owner<CExpression *> pexprOrderedAgg = nullptr;
	gpos::owner<CExpression *> pexprOtherAgg = nullptr;
	SplitOrderedAggsPrj(mp, pexprInputAggPrj, &pexprOrderedAgg, &pexprOtherAgg);

	gpos::owner<CExpression *> pexprFinalJoin = nullptr;
	if (COperator::EopLogicalGbAgg == pexprOtherAgg->Pop()->Eopid())
	{
		gpos::owner<CExpression *> pexprJoinCondition =
			CUtils::PexprScalarConstBool(mp, true /*value*/);
		pexprFinalJoin = CUtils::PexprLogicalJoin<CLogicalInnerJoin>(
			mp, pexprOtherAgg, pexprOrderedAgg, pexprJoinCondition);
	}
	else
	{
		pexprFinalJoin = pexprOrderedAgg;
	}
	ULONG arity = pexprOrderedAgg->Arity();
	BOOL has_nlj_ontop =
		(COperator::EopLogicalInnerJoin == pexprOrderedAgg->Pop()->Eopid());
	gpos::pointer<CExpression *> pexprTopmostCTE = pexprOrderedAgg;
	while (COperator::EopLogicalGbAgg != pexprTopmostCTE->Pop()->Eopid())
	{
		pexprTopmostCTE = (*pexprTopmostCTE)[0];
	}
	ULONG ulCTEIdStart = gpos::dyn_cast<CLogicalCTEConsumer>(
							 (*(*(*(*pexprTopmostCTE)[0])[0])[1])[0]->Pop())
							 ->UlCTEId();
	ULONG ulCTEIdEnd = ulCTEIdStart;
	if (has_nlj_ontop)
		ulCTEIdEnd =
			gpos::dyn_cast<CLogicalCTEConsumer>(
				(*(*(*(*(*pexprOrderedAgg)[arity - 2])[0])[0])[1])[0]->Pop())
				->UlCTEId();
	gpos::owner<CExpression *> pexpResult = pexprFinalJoin;
	for (ULONG ul = ulCTEIdEnd; ul > ulCTEIdStart; ul--)
	{
		gpos::owner<CExpression *> ctenext = GPOS_NEW(mp)
			CExpression(mp, GPOS_NEW(mp) CLogicalCTEAnchor(mp, ul), pexpResult);
		pexpResult = ctenext;
	}
	pexpResult = GPOS_NEW(mp) CExpression(
		mp, GPOS_NEW(mp) CLogicalCTEAnchor(mp, ulCTEIdStart), pexpResult);
	return pexpResult;
}


//---------------------------------------------------------------------------
//	@function:
//		COrderedAggPreprocessor::PexprPreprocess
//
//	@doc:
//		Main driver
//
//---------------------------------------------------------------------------
gpos::owner<CExpression *>
COrderedAggPreprocessor::PexprPreprocess(CMemoryPool *mp,
										 gpos::pointer<CExpression *> pexpr)
{
	// protect against stack overflow during recursion
	GPOS_CHECK_STACK_SIZE;
	GPOS_ASSERT(nullptr != mp);
	GPOS_ASSERT(nullptr != pexpr);

	COperator *pop = pexpr->Pop();
	if (COperator::EopLogicalGbAgg == pop->Eopid() &&
		0 == gpos::dyn_cast<CLogicalGbAgg>(pop)->Pdrgpcr()->Size() &&
		0 < (*pexpr)[1]->DeriveTotalOrderedAggs())
	{
		gpos::owner<CExpression *> pexprJoin = PexprInputAggPrj2Join(mp, pexpr);

		// recursively process the resulting expression
		gpos::owner<CExpression *> pexprResult = PexprPreprocess(mp, pexprJoin);
		pexprJoin->Release();

		return pexprResult;
	}

	// recursively process child expressions
	const ULONG arity = pexpr->Arity();
	gpos::owner<CExpressionArray *> pdrgpexprChildren =
		GPOS_NEW(mp) CExpressionArray(mp);
	for (ULONG ul = 0; ul < arity; ul++)
	{
		gpos::owner<CExpression *> pexprChild =
			PexprPreprocess(mp, (*pexpr)[ul]);
		pdrgpexprChildren->Append(pexprChild);
	}

	pop->AddRef();
	return GPOS_NEW(mp) CExpression(mp, pop, std::move(pdrgpexprChildren));
}

// EOF
