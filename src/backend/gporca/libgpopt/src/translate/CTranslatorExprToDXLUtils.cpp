//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CTranslatorExprToDXLUtils.cpp
//
//	@doc:
//		Implementation of the helper methods used during Expr to DXL translation
//
//---------------------------------------------------------------------------

#include "gpopt/translate/CTranslatorExprToDXLUtils.h"

#include "gpos/common/owner.h"

#include "gpopt/base/CConstraint.h"
#include "gpopt/base/CConstraintConjunction.h"
#include "gpopt/base/CConstraintDisjunction.h"
#include "gpopt/base/CConstraintInterval.h"
#include "gpopt/base/CConstraintNegation.h"
#include "gpopt/base/COptCtxt.h"
#include "gpopt/exception.h"
#include "gpopt/mdcache/CMDAccessorUtils.h"
#include "gpopt/operators/CPhysicalAgg.h"
#include "gpopt/operators/CPredicateUtils.h"
#include "gpopt/operators/CScalarCast.h"
#include "gpopt/operators/CScalarIdent.h"
#include "naucrates/dxl/operators/CDXLDatumBool.h"
#include "naucrates/dxl/operators/CDXLDatumInt4.h"
#include "naucrates/dxl/operators/CDXLDatumOid.h"
#include "naucrates/dxl/operators/CDXLDirectDispatchInfo.h"
#include "naucrates/dxl/operators/CDXLPhysicalHashJoin.h"
#include "naucrates/dxl/operators/CDXLPhysicalPartitionSelector.h"
#include "naucrates/dxl/operators/CDXLPhysicalResult.h"
#include "naucrates/dxl/operators/CDXLPhysicalValuesScan.h"
#include "naucrates/dxl/operators/CDXLScalarArrayCoerceExpr.h"
#include "naucrates/dxl/operators/CDXLScalarArrayComp.h"
#include "naucrates/dxl/operators/CDXLScalarCast.h"
#include "naucrates/dxl/operators/CDXLScalarIdent.h"
#include "naucrates/dxl/operators/CDXLScalarPartBound.h"
#include "naucrates/dxl/operators/CDXLScalarPartBoundInclusion.h"
#include "naucrates/dxl/operators/CDXLScalarPartBoundOpen.h"
#include "naucrates/dxl/operators/CDXLScalarPartDefault.h"
#include "naucrates/dxl/operators/CDXLScalarPartListValues.h"
#include "naucrates/dxl/operators/CDXLScalarPartOid.h"
#include "naucrates/dxl/operators/CDXLScalarProjElem.h"
#include "naucrates/dxl/operators/CDXLScalarProjList.h"
#include "naucrates/dxl/operators/CDXLScalarValuesList.h"
#include "naucrates/exception.h"
#include "naucrates/md/IMDCast.h"
#include "naucrates/md/IMDScalarOp.h"
#include "naucrates/md/IMDTypeBool.h"
#include "naucrates/md/IMDTypeInt4.h"
#include "naucrates/md/IMDTypeOid.h"
#include "naucrates/statistics/IStatistics.h"

using namespace gpos;
using namespace gpmd;
using namespace gpdxl;
using namespace gpopt;
using namespace gpnaucrates;

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXLUtils::PdxlnInt4Const
//
//	@doc:
// 		Construct a scalar const value expression for the given INT value
//
//---------------------------------------------------------------------------
gpos::owner<CDXLNode *>
CTranslatorExprToDXLUtils::PdxlnInt4Const(CMemoryPool *mp,
										  CMDAccessor *md_accessor, INT val)
{
	GPOS_ASSERT(nullptr != mp);

	gpos::pointer<const IMDTypeInt4 *> pmdtypeint4 =
		md_accessor->PtMDType<IMDTypeInt4>();
	pmdtypeint4->MDId()->AddRef();

	gpos::owner<CDXLDatumInt4 *> dxl_datum = GPOS_NEW(mp)
		CDXLDatumInt4(mp, pmdtypeint4->MDId(), false /*is_null*/, val);
	gpos::owner<CDXLScalarConstValue *> pdxlConst =
		GPOS_NEW(mp) CDXLScalarConstValue(mp, std::move(dxl_datum));

	return GPOS_NEW(mp) CDXLNode(mp, std::move(pdxlConst));
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXLUtils::PdxlnBoolConst
//
//	@doc:
// 		Construct a scalar const value expression for the given BOOL value
//
//---------------------------------------------------------------------------
gpos::owner<CDXLNode *>
CTranslatorExprToDXLUtils::PdxlnBoolConst(CMemoryPool *mp,
										  CMDAccessor *md_accessor, BOOL value)
{
	GPOS_ASSERT(nullptr != mp);

	gpos::pointer<const IMDTypeBool *> pmdtype =
		md_accessor->PtMDType<IMDTypeBool>();
	pmdtype->MDId()->AddRef();

	gpos::owner<CDXLDatumBool *> dxl_datum = GPOS_NEW(mp)
		CDXLDatumBool(mp, pmdtype->MDId(), false /*is_null*/, value);
	gpos::owner<CDXLScalarConstValue *> pdxlConst =
		GPOS_NEW(mp) CDXLScalarConstValue(mp, std::move(dxl_datum));

	return GPOS_NEW(mp) CDXLNode(mp, std::move(pdxlConst));
}



// construct a DXL node for the part key portion of the list partition filter
gpos::owner<CDXLNode *>
CTranslatorExprToDXLUtils::PdxlnListFilterPartKey(
	CMemoryPool *mp, CMDAccessor *md_accessor,
	gpos::pointer<CExpression *> pexprPartKey, IMDId *pmdidTypePartKey,
	ULONG ulPartLevel)
{
	GPOS_ASSERT(nullptr != pexprPartKey);
	GPOS_ASSERT(nullptr != pmdidTypePartKey);
	GPOS_ASSERT(gpos::dyn_cast<CScalar>(pexprPartKey->Pop())
					->MdidType()
					->Equals(pmdidTypePartKey));

	gpos::owner<CDXLNode *> pdxlnPartKey = nullptr;

	if (CUtils::FScalarIdent(pexprPartKey))
	{
		// Simple Scalar Ident - create a ScalarPartListValues from the partition key
		gpos::owner<IMDId *> pmdidResultArray =
			md_accessor->RetrieveType(pmdidTypePartKey)->GetArrayTypeMdid();
		pmdidResultArray->AddRef();
		pmdidTypePartKey->AddRef();

		pdxlnPartKey = GPOS_NEW(mp) CDXLNode(
			mp, GPOS_NEW(mp) CDXLScalarPartListValues(
					mp, ulPartLevel, pmdidResultArray, pmdidTypePartKey));
	}
	else if (CScalarIdent::FCastedScId(pexprPartKey) ||
			 CScalarIdent::FAllowedFuncScId(pexprPartKey))
	{
		gpos::pointer<IMDId *> pmdidDestElem = nullptr;
		gpos::pointer<IMDId *> pmdidArrayCastFunc = nullptr;
		ExtractCastFuncMdids(pexprPartKey->Pop(), &pmdidDestElem,
							 &pmdidArrayCastFunc);
		IMDId *pmdidDestArray =
			md_accessor->RetrieveType(pmdidDestElem)->GetArrayTypeMdid();

		gpos::pointer<CScalarIdent *> pexprScalarIdent =
			gpos::dyn_cast<CScalarIdent>((*pexprPartKey)[0]->Pop());
		IMDId *pmdidSrcElem = pexprScalarIdent->MdidType();
		gpos::owner<IMDId *> pmdidSrcArray =
			md_accessor->RetrieveType(pmdidSrcElem)->GetArrayTypeMdid();
		pmdidSrcArray->AddRef();
		pmdidSrcElem->AddRef();
		gpos::owner<CDXLNode *> pdxlnPartKeyIdent = GPOS_NEW(mp)
			CDXLNode(mp, GPOS_NEW(mp) CDXLScalarPartListValues(
							 mp, ulPartLevel, pmdidSrcArray, pmdidSrcElem));

		pmdidDestArray->AddRef();
		pmdidArrayCastFunc->AddRef();
		pdxlnPartKey = GPOS_NEW(mp)
			CDXLNode(mp,
					 GPOS_NEW(mp) CDXLScalarArrayCoerceExpr(
						 mp, pmdidArrayCastFunc, pmdidDestArray,
						 default_type_modifier, true, /* is_explicit */
						 EdxlcfDontCare, -1			  /* location */
						 ),
					 pdxlnPartKeyIdent);
	}
	else
	{
		// Not supported - should be unreachable.
		CWStringDynamic *str = GPOS_NEW(mp) CWStringDynamic(mp);
		str->AppendFormat(
			GPOS_WSZ_LIT(
				"Unsupported part filter operator for list partitions : %ls"),
			pexprPartKey->Pop()->SzId());
		GPOS_THROW_EXCEPTION(gpopt::ExmaGPOPT, gpopt::ExmiUnsupportedOp,
							 str->GetBuffer());
	}

	GPOS_ASSERT(nullptr != pdxlnPartKey);

	return pdxlnPartKey;
}


// Construct a predicate node for a list partition filter
gpos::owner<CDXLNode *>
CTranslatorExprToDXLUtils::PdxlnListFilterScCmp(
	CMemoryPool *mp, CMDAccessor *md_accessor, CDXLNode *pdxlnPartKey,
	CDXLNode *pdxlnOther, gpos::pointer<IMDId *> pmdidTypePartKey,
	gpos::pointer<IMDId *> pmdidTypeOther, IMDType::ECmpType cmp_type,
	ULONG ulPartLevel, BOOL fHasDefaultPart)
{
	IMDId *pmdidScCmp = nullptr;

	pmdidScCmp = CMDAccessorUtils::GetScCmpMdIdConsiderCasts(
		md_accessor, pmdidTypeOther, pmdidTypePartKey, cmp_type);

	gpos::pointer<const IMDScalarOp *> md_scalar_op =
		md_accessor->RetrieveScOp(pmdidScCmp);
	const CWStringConst *pstrScCmp = md_scalar_op->Mdname().GetMDName();

	pmdidScCmp->AddRef();
	gpos::owner<CDXLNode *> pdxlnScCmp = GPOS_NEW(mp)
		CDXLNode(mp,
				 GPOS_NEW(mp) CDXLScalarArrayComp(
					 mp, pmdidScCmp,
					 GPOS_NEW(mp) CWStringConst(mp, pstrScCmp->GetBuffer()),
					 Edxlarraycomptypeany),
				 pdxlnOther, pdxlnPartKey);

	if (fHasDefaultPart)
	{
		gpos::owner<CDXLNode *> pdxlnDefault = GPOS_NEW(mp)
			CDXLNode(mp, GPOS_NEW(mp) CDXLScalarPartDefault(mp, ulPartLevel));
		return GPOS_NEW(mp)
			CDXLNode(mp, GPOS_NEW(mp) CDXLScalarBoolExpr(mp, Edxlor),
					 std::move(pdxlnScCmp), std::move(pdxlnDefault));
	}
	else
	{
		return pdxlnScCmp;
	}
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXLUtils::PdxlnRangeFilterScCmp
//
//	@doc:
// 		Construct a Result node for a filter min <= Scalar or max >= Scalar
//
//---------------------------------------------------------------------------
gpos::owner<CDXLNode *>
CTranslatorExprToDXLUtils::PdxlnRangeFilterScCmp(
	CMemoryPool *mp, CMDAccessor *md_accessor, CDXLNode *pdxlnScalar,
	IMDId *pmdidTypePartKey, gpos::pointer<IMDId *> pmdidTypeOther,
	IMDId *pmdidTypeCastExpr, IMDId *mdid_cast_func, IMDType::ECmpType cmp_type,
	ULONG ulPartLevel)
{
	if (IMDType::EcmptEq == cmp_type)
	{
		return PdxlnRangeFilterEqCmp(
			mp, md_accessor, pdxlnScalar, pmdidTypePartKey, pmdidTypeOther,
			pmdidTypeCastExpr, mdid_cast_func, ulPartLevel);
	}

	BOOL fLowerBound = false;
	IMDType::ECmpType ecmptScCmp = IMDType::EcmptOther;

	if (IMDType::EcmptLEq == cmp_type || IMDType::EcmptL == cmp_type)
	{
		// partkey </<= other: construct condition min < other
		fLowerBound = true;
		ecmptScCmp = IMDType::EcmptL;
	}
	else if (IMDType::EcmptGEq == cmp_type || IMDType::EcmptG == cmp_type)
	{
		// partkey >/>= other: construct condition max > other
		ecmptScCmp = IMDType::EcmptG;
	}
	else
	{
		GPOS_ASSERT(IMDType::EcmptNEq == cmp_type);
		ecmptScCmp = IMDType::EcmptNEq;
	}

	if (IMDType::EcmptLEq != cmp_type && IMDType::EcmptGEq != cmp_type)
	{
		// scalar comparison does not include equality: no need to consider part constraint boundaries
		gpos::owner<CDXLNode *> pdxlnPredicateExclusive =
			PdxlnCmp(mp, md_accessor, ulPartLevel, fLowerBound, pdxlnScalar,
					 ecmptScCmp, pmdidTypePartKey, pmdidTypeOther,
					 pmdidTypeCastExpr, mdid_cast_func);
		return pdxlnPredicateExclusive;
		// This is also correct for lossy casts. when we have predicate such as
		// float::int < 2, we dont want to select values such as 1.7 which cast to 2.
		// So, in this case, we should check lower bound::int < 2
	}

	gpos::owner<CDXLNode *> pdxlnInclusiveCmp = PdxlnCmp(
		mp, md_accessor, ulPartLevel, fLowerBound, pdxlnScalar, cmp_type,
		pmdidTypePartKey, pmdidTypeOther, pmdidTypeCastExpr, mdid_cast_func);

	if (nullptr != mdid_cast_func && mdid_cast_func->IsValid() &&
		md_accessor->RetrieveFunc(mdid_cast_func)
			->IsAllowedForPS())	 // is a lossy cast
	{
		// In case of lossy casts, we don't want to eliminate partitions with
		// exclusive ends when the predicate is on that end
		// A partition such as [1,2) should be selected for float::int = 2,
		// but shouldn't be selected for float = 2.0
		return pdxlnInclusiveCmp;
	}

	pdxlnScalar->AddRef();
	gpos::owner<CDXLNode *> pdxlnPredicateExclusive = PdxlnCmp(
		mp, md_accessor, ulPartLevel, fLowerBound, pdxlnScalar, ecmptScCmp,
		pmdidTypePartKey, pmdidTypeOther, pmdidTypeCastExpr, mdid_cast_func);

	gpos::owner<CDXLNode *> pdxlnInclusiveBoolPredicate =
		GPOS_NEW(mp) CDXLNode(mp, GPOS_NEW(mp) CDXLScalarPartBoundInclusion(
									  mp, ulPartLevel, fLowerBound));

	gpos::owner<CDXLNode *> pdxlnPredicateInclusive = GPOS_NEW(mp) CDXLNode(
		mp, GPOS_NEW(mp) CDXLScalarBoolExpr(mp, Edxland),
		std::move(pdxlnInclusiveCmp), std::move(pdxlnInclusiveBoolPredicate));

	// return the final predicate in the form "(point <= col and colIncluded) or point < col" / "(point >= col and colIncluded) or point > col"
	return GPOS_NEW(mp) CDXLNode(
		mp, GPOS_NEW(mp) CDXLScalarBoolExpr(mp, Edxlor),
		std::move(pdxlnPredicateInclusive), std::move(pdxlnPredicateExclusive));
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXLUtils::PdxlnRangeFilterEqCmp
//
//	@doc:
// 		Construct a predicate node for a filter min <= Scalar and max >= Scalar
//
//---------------------------------------------------------------------------
gpos::owner<CDXLNode *>
CTranslatorExprToDXLUtils::PdxlnRangeFilterEqCmp(
	CMemoryPool *mp, CMDAccessor *md_accessor, CDXLNode *pdxlnScalar,
	IMDId *pmdidTypePartKey, gpos::pointer<IMDId *> pmdidTypeOther,
	IMDId *pmdidTypeCastExpr, IMDId *mdid_cast_func, ULONG ulPartLevel)
{
	gpos::owner<CDXLNode *> pdxlnPredicateMin = PdxlnRangeFilterPartBound(
		mp, md_accessor, pdxlnScalar, pmdidTypePartKey, pmdidTypeOther,
		pmdidTypeCastExpr, mdid_cast_func, ulPartLevel, true /*fLowerBound*/,
		IMDType::EcmptL);
	pdxlnScalar->AddRef();
	gpos::owner<CDXLNode *> pdxlnPredicateMax = PdxlnRangeFilterPartBound(
		mp, md_accessor, pdxlnScalar, pmdidTypePartKey, pmdidTypeOther,
		pmdidTypeCastExpr, mdid_cast_func, ulPartLevel, false /*fLowerBound*/,
		IMDType::EcmptG);

	// return the conjunction of the predicate for the lower and upper bounds
	return GPOS_NEW(mp)
		CDXLNode(mp, GPOS_NEW(mp) CDXLScalarBoolExpr(mp, Edxland),
				 std::move(pdxlnPredicateMin), std::move(pdxlnPredicateMax));
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXLUtils::PdxlnRangeFilterPartBound
//
//	@doc:
// 		Construct a predicate for a partition bound of one of the two forms
//		(min <= Scalar and minincl) or min < Scalar
//		(max >= Scalar and maxinc) or Max > Scalar
//
//---------------------------------------------------------------------------
gpos::owner<CDXLNode *>
CTranslatorExprToDXLUtils::PdxlnRangeFilterPartBound(
	CMemoryPool *mp, CMDAccessor *md_accessor, CDXLNode *pdxlnScalar,
	IMDId *pmdidTypePartKey, gpos::pointer<IMDId *> pmdidTypeOther,
	IMDId *pmdidTypeCastExpr, IMDId *mdid_cast_func, ULONG ulPartLevel,
	ULONG fLowerBound, IMDType::ECmpType cmp_type)
{
	GPOS_ASSERT(IMDType::EcmptL == cmp_type || IMDType::EcmptG == cmp_type);

	IMDType::ECmpType ecmptInc = IMDType::EcmptLEq;
	if (IMDType::EcmptG == cmp_type)
	{
		ecmptInc = IMDType::EcmptGEq;
	}

	gpos::owner<CDXLNode *> pdxlnInclusiveCmp = PdxlnCmp(
		mp, md_accessor, ulPartLevel, fLowerBound, pdxlnScalar, ecmptInc,
		pmdidTypePartKey, pmdidTypeOther, pmdidTypeCastExpr, mdid_cast_func);

	if (nullptr != mdid_cast_func && mdid_cast_func->IsValid() &&
		md_accessor->RetrieveFunc(mdid_cast_func)
			->IsAllowedForPS())	 // is a lossy cast
	{
		// In case of lossy casts, we don't want to eliminate partitions with
		// exclusive ends when the predicate is on that end
		// A partition such as [1,2) should be selected for float::int = 2
		// but shouldn't be selected for float = 2.0
		return pdxlnInclusiveCmp;
	}

	pdxlnScalar->AddRef();

	gpos::owner<CDXLNode *> pdxlnPredicateExclusive = PdxlnCmp(
		mp, md_accessor, ulPartLevel, fLowerBound, pdxlnScalar, cmp_type,
		pmdidTypePartKey, pmdidTypeOther, pmdidTypeCastExpr, mdid_cast_func);

	gpos::owner<CDXLNode *> pdxlnInclusiveBoolPredicate =
		GPOS_NEW(mp) CDXLNode(mp, GPOS_NEW(mp) CDXLScalarPartBoundInclusion(
									  mp, ulPartLevel, fLowerBound));

	gpos::owner<CDXLNode *> pdxlnPredicateInclusive = GPOS_NEW(mp) CDXLNode(
		mp, GPOS_NEW(mp) CDXLScalarBoolExpr(mp, Edxland),
		std::move(pdxlnInclusiveCmp), std::move(pdxlnInclusiveBoolPredicate));

	// return the final predicate in the form "(point <= col and colIncluded) or point < col" / "(point >= col and colIncluded) or point > col"
	return GPOS_NEW(mp) CDXLNode(
		mp, GPOS_NEW(mp) CDXLScalarBoolExpr(mp, Edxlor),
		std::move(pdxlnPredicateInclusive), std::move(pdxlnPredicateExclusive));
}


//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXLUtils::PdxlnRangeFilterDefaultAndOpenEnded
//
//	@doc:
//		Construct predicates to cover the cases of default partition and
//		open-ended partitions if necessary
//
//---------------------------------------------------------------------------
gpos::owner<CDXLNode *>
CTranslatorExprToDXLUtils::PdxlnRangeFilterDefaultAndOpenEnded(
	CMemoryPool *mp, ULONG ulPartLevel, BOOL fLTComparison, BOOL fGTComparison,
	BOOL fEQComparison, BOOL fDefaultPart)
{
	gpos::owner<CDXLNode *> pdxlnResult = nullptr;
	if (fLTComparison || fEQComparison)
	{
		// add a condition to cover the cases of open-ended interval (-inf, x)
		pdxlnResult = GPOS_NEW(mp)
			CDXLNode(mp, GPOS_NEW(mp) CDXLScalarPartBoundOpen(
							 mp, ulPartLevel, true /*is_lower_bound*/));
	}

	if (fGTComparison || fEQComparison)
	{
		// add a condition to cover the cases of open-ended interval (x, inf)
		gpos::owner<CDXLNode *> pdxlnOpenMax = GPOS_NEW(mp)
			CDXLNode(mp, GPOS_NEW(mp) CDXLScalarPartBoundOpen(
							 mp, ulPartLevel, false /*is_lower_bound*/));

		// construct a boolean OR expression over the two expressions
		if (nullptr != pdxlnResult)
		{
			pdxlnResult = GPOS_NEW(mp)
				CDXLNode(mp, GPOS_NEW(mp) CDXLScalarBoolExpr(mp, Edxlor),
						 std::move(pdxlnResult), std::move(pdxlnOpenMax));
		}
		else
		{
			pdxlnResult = std::move(pdxlnOpenMax);
		}
	}

	if (fDefaultPart)
	{
		// add a condition to cover the cases of default partition
		gpos::owner<CDXLNode *> pdxlnDefault = GPOS_NEW(mp)
			CDXLNode(mp, GPOS_NEW(mp) CDXLScalarPartDefault(mp, ulPartLevel));

		if (nullptr != pdxlnResult)
		{
			pdxlnResult = GPOS_NEW(mp)
				CDXLNode(mp, GPOS_NEW(mp) CDXLScalarBoolExpr(mp, Edxlor),
						 std::move(pdxlnDefault), std::move(pdxlnResult));
		}
		else
		{
			pdxlnResult = std::move(pdxlnDefault);
		}
	}

	return pdxlnResult;
}


//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXLUtils::PdxlpropCopy
//
//	@doc:
//		Return a copy the dxl node's physical properties
//
//---------------------------------------------------------------------------
gpos::owner<CDXLPhysicalProperties *>
CTranslatorExprToDXLUtils::PdxlpropCopy(CMemoryPool *mp,
										gpos::pointer<CDXLNode *> dxlnode)
{
	GPOS_ASSERT(nullptr != dxlnode);

	GPOS_ASSERT(nullptr != dxlnode->GetProperties());
	gpos::pointer<CDXLPhysicalProperties *> dxl_properties =
		gpos::dyn_cast<CDXLPhysicalProperties>(dxlnode->GetProperties());

	CWStringDynamic *pstrStartupcost = GPOS_NEW(mp) CWStringDynamic(
		mp,
		dxl_properties->GetDXLOperatorCost()->GetStartUpCostStr()->GetBuffer());
	CWStringDynamic *pstrCost = GPOS_NEW(mp) CWStringDynamic(
		mp,
		dxl_properties->GetDXLOperatorCost()->GetTotalCostStr()->GetBuffer());
	CWStringDynamic *rows_out_str = GPOS_NEW(mp) CWStringDynamic(
		mp, dxl_properties->GetDXLOperatorCost()->GetRowsOutStr()->GetBuffer());
	CWStringDynamic *width_str = GPOS_NEW(mp) CWStringDynamic(
		mp, dxl_properties->GetDXLOperatorCost()->GetWidthStr()->GetBuffer());

	return GPOS_NEW(mp) CDXLPhysicalProperties(GPOS_NEW(mp) CDXLOperatorCost(
		pstrStartupcost, pstrCost, rows_out_str, width_str));
}


//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXLUtils::PdxlnCmp
//
//	@doc:
//		Construct a scalar comparison of the given type between the column with
//		the given col id and the scalar expression
//
//---------------------------------------------------------------------------
gpos::owner<CDXLNode *>
CTranslatorExprToDXLUtils::PdxlnCmp(CMemoryPool *mp, CMDAccessor *md_accessor,
									ULONG ulPartLevel, BOOL fLowerBound,
									gpos::owner<CDXLNode *> pdxlnScalar,
									IMDType::ECmpType cmp_type,
									IMDId *pmdidTypePartKey,
									gpos::pointer<IMDId *> pmdidTypeExpr,
									IMDId *pmdidTypeCastExpr,
									IMDId *mdid_cast_func)
{
	IMDId *pmdidScCmp = nullptr;

	if (IMDId::IsValid(pmdidTypeCastExpr))
	{
		pmdidScCmp = CMDAccessorUtils::GetScCmpMdIdConsiderCasts(
			md_accessor, pmdidTypeCastExpr, pmdidTypeExpr, cmp_type);
	}
	else
	{
		pmdidScCmp = CMDAccessorUtils::GetScCmpMdIdConsiderCasts(
			md_accessor, pmdidTypePartKey, pmdidTypeExpr, cmp_type);
	}

	gpos::pointer<const IMDScalarOp *> md_scalar_op =
		md_accessor->RetrieveScOp(pmdidScCmp);
	const CWStringConst *pstrScCmp = md_scalar_op->Mdname().GetMDName();

	pmdidScCmp->AddRef();

	gpos::owner<CDXLScalarComp *> pdxlopCmp = GPOS_NEW(mp) CDXLScalarComp(
		mp, pmdidScCmp, GPOS_NEW(mp) CWStringConst(mp, pstrScCmp->GetBuffer()));
	gpos::owner<CDXLNode *> pdxlnScCmp = GPOS_NEW(mp) CDXLNode(mp, pdxlopCmp);

	pmdidTypePartKey->AddRef();
	gpos::owner<CDXLNode *> pdxlnPartBound = GPOS_NEW(mp)
		CDXLNode(mp, GPOS_NEW(mp) CDXLScalarPartBound(
						 mp, ulPartLevel, pmdidTypePartKey, fLowerBound));

	if (IMDId::IsValid(pmdidTypeCastExpr))
	{
		GPOS_ASSERT(nullptr != mdid_cast_func);
		pmdidTypeCastExpr->AddRef();
		mdid_cast_func->AddRef();

		pdxlnPartBound = GPOS_NEW(mp) CDXLNode(
			mp,
			GPOS_NEW(mp) CDXLScalarCast(mp, pmdidTypeCastExpr, mdid_cast_func),
			pdxlnPartBound);
	}
	pdxlnScCmp->AddChild(std::move(pdxlnPartBound));
	pdxlnScCmp->AddChild(std::move(pdxlnScalar));

	return pdxlnScCmp;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXLUtils::PcrCreate
//
//	@doc:
//		Construct a column reference with the given name and type
//
//---------------------------------------------------------------------------
CColRef *
CTranslatorExprToDXLUtils::PcrCreate(CMemoryPool *mp, CMDAccessor *md_accessor,
									 CColumnFactory *col_factory,
									 gpos::pointer<IMDId *> mdid_type,
									 INT type_modifier, const WCHAR *wszName)
{
	gpos::pointer<const IMDType *> pmdtype =
		md_accessor->RetrieveType(mdid_type);

	CName *pname = GPOS_NEW(mp)
		CName(GPOS_NEW(mp) CWStringConst(wszName), true /*fOwnsMemory*/);
	CColRef *colref = col_factory->PcrCreate(pmdtype, type_modifier, *pname);
	GPOS_DELETE(pname);
	return colref;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXLUtils::GetProperties
//
//	@doc:
//		Construct a DXL physical properties container with operator costs for
//		the given expression
//
//---------------------------------------------------------------------------
gpos::owner<CDXLPhysicalProperties *>
CTranslatorExprToDXLUtils::GetProperties(CMemoryPool *mp)
{
	// TODO:  - May 10, 2012; replace the dummy implementation with a real one
	CWStringDynamic *pstrStartupcost =
		GPOS_NEW(mp) CWStringDynamic(mp, GPOS_WSZ_LIT("10"));
	CWStringDynamic *pstrTotalcost =
		GPOS_NEW(mp) CWStringDynamic(mp, GPOS_WSZ_LIT("100"));
	CWStringDynamic *rows_out_str =
		GPOS_NEW(mp) CWStringDynamic(mp, GPOS_WSZ_LIT("100"));
	CWStringDynamic *width_str =
		GPOS_NEW(mp) CWStringDynamic(mp, GPOS_WSZ_LIT("4"));

	gpos::owner<CDXLOperatorCost *> cost = GPOS_NEW(mp) CDXLOperatorCost(
		pstrStartupcost, pstrTotalcost, rows_out_str, width_str);
	return GPOS_NEW(mp) CDXLPhysicalProperties(std::move(cost));
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXLUtils::FScalarConstTrue
//
//	@doc:
//		Checks to see if the DXL Node is a scalar const TRUE
//
//---------------------------------------------------------------------------
BOOL
CTranslatorExprToDXLUtils::FScalarConstTrue(CMDAccessor *md_accessor,
											gpos::pointer<CDXLNode *> dxlnode)
{
	GPOS_ASSERT(nullptr != dxlnode);
	if (EdxlopScalarConstValue == dxlnode->GetOperator()->GetDXLOperator())
	{
		gpos::pointer<CDXLScalarConstValue *> pdxlopConst =
			gpos::dyn_cast<CDXLScalarConstValue>(dxlnode->GetOperator());

		gpos::pointer<const IMDType *> pmdtype =
			md_accessor->RetrieveType(pdxlopConst->GetDatumVal()->MDId());
		if (IMDType::EtiBool == pmdtype->GetDatumType())
		{
			gpos::pointer<CDXLDatumBool *> dxl_datum =
				gpos::dyn_cast<CDXLDatumBool>(
					const_cast<CDXLDatum *>(pdxlopConst->GetDatumVal()));

			return (!dxl_datum->IsNull() && dxl_datum->GetValue());
		}
	}

	return false;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXLUtils::FScalarConstFalse
//
//	@doc:
//		Checks to see if the DXL Node is a scalar const FALSE
//
//---------------------------------------------------------------------------
BOOL
CTranslatorExprToDXLUtils::FScalarConstFalse(CMDAccessor *md_accessor,
											 gpos::pointer<CDXLNode *> dxlnode)
{
	GPOS_ASSERT(nullptr != dxlnode);
	if (EdxlopScalarConstValue == dxlnode->GetOperator()->GetDXLOperator())
	{
		gpos::pointer<CDXLScalarConstValue *> pdxlopConst =
			gpos::dyn_cast<CDXLScalarConstValue>(dxlnode->GetOperator());

		gpos::pointer<const IMDType *> pmdtype =
			md_accessor->RetrieveType(pdxlopConst->GetDatumVal()->MDId());
		if (IMDType::EtiBool == pmdtype->GetDatumType())
		{
			gpos::pointer<CDXLDatumBool *> dxl_datum =
				gpos::dyn_cast<CDXLDatumBool>(
					const_cast<CDXLDatum *>(pdxlopConst->GetDatumVal()));
			return (!dxl_datum->IsNull() && !dxl_datum->GetValue());
		}
	}

	return false;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXLUtils::PdxlnProjListFromChildProjList
//
//	@doc:
//		Construct a project list node by creating references to the columns
//		of the given project list of the child node
//
//---------------------------------------------------------------------------
gpos::owner<CDXLNode *>
CTranslatorExprToDXLUtils::PdxlnProjListFromChildProjList(
	CMemoryPool *mp, CColumnFactory *col_factory,
	gpos::pointer<ColRefToDXLNodeMap *> phmcrdxln,
	gpos::pointer<const CDXLNode *> pdxlnProjListChild)
{
	GPOS_ASSERT(nullptr != pdxlnProjListChild);

	gpos::owner<CDXLScalarProjList *> pdxlopPrL =
		GPOS_NEW(mp) CDXLScalarProjList(mp);
	gpos::owner<CDXLNode *> proj_list_dxlnode =
		GPOS_NEW(mp) CDXLNode(mp, pdxlopPrL);

	// create a scalar identifier for each project element of the child
	const ULONG arity = pdxlnProjListChild->Arity();
	for (ULONG ul = 0; ul < arity; ul++)
	{
		gpos::pointer<CDXLNode *> pdxlnProjElemChild =
			(*pdxlnProjListChild)[ul];

		// translate proj elem
		gpos::owner<CDXLNode *> pdxlnProjElem =
			PdxlnProjElem(mp, col_factory, phmcrdxln, pdxlnProjElemChild);
		proj_list_dxlnode->AddChild(pdxlnProjElem);
	}

	return proj_list_dxlnode;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXLUtils::PdxlnPrLPartitionSelector
//
//	@doc:
//		Construct the project list of a partition selector
//
//---------------------------------------------------------------------------
gpos::owner<CDXLNode *>
CTranslatorExprToDXLUtils::PdxlnPrLPartitionSelector(
	CMemoryPool *mp, CMDAccessor *md_accessor, CColumnFactory *col_factory,
	gpos::pointer<ColRefToDXLNodeMap *> phmcrdxln, BOOL fUseChildProjList,
	gpos::pointer<CDXLNode *> pdxlnPrLChild, CColRef *pcrOid,
	ULONG ulPartLevels, BOOL fGeneratePartOid)
{
	GPOS_ASSERT_IMP(fUseChildProjList, nullptr != pdxlnPrLChild);

	gpos::owner<CDXLNode *> pdxlnPrL = nullptr;
	if (fUseChildProjList)
	{
		pdxlnPrL = PdxlnProjListFromChildProjList(mp, col_factory, phmcrdxln,
												  pdxlnPrLChild);
	}
	else
	{
		pdxlnPrL =
			GPOS_NEW(mp) CDXLNode(mp, GPOS_NEW(mp) CDXLScalarProjList(mp));
	}

	if (fGeneratePartOid)
	{
		// add to it the Oid column
		if (nullptr == pcrOid)
		{
			gpos::pointer<const IMDTypeOid *> pmdtype =
				md_accessor->PtMDType<IMDTypeOid>();
			pcrOid = col_factory->PcrCreate(pmdtype, default_type_modifier);
		}

		CMDName *mdname = GPOS_NEW(mp) CMDName(mp, pcrOid->Name().Pstr());
		gpos::owner<CDXLScalarProjElem *> pdxlopPrEl =
			GPOS_NEW(mp) CDXLScalarProjElem(mp, pcrOid->Id(), mdname);
		gpos::owner<CDXLNode *> pdxlnPrEl =
			GPOS_NEW(mp) CDXLNode(mp, pdxlopPrEl);
		gpos::owner<CDXLNode *> pdxlnPartOid = GPOS_NEW(mp)
			CDXLNode(mp, GPOS_NEW(mp) CDXLScalarPartOid(mp, ulPartLevels - 1));
		pdxlnPrEl->AddChild(pdxlnPartOid);
		pdxlnPrL->AddChild(pdxlnPrEl);
	}

	return pdxlnPrL;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXLUtils::PdxlnProjElem
//
//	@doc:
//		Create a project elem as a scalar identifier for the given child
//		project element
//
//---------------------------------------------------------------------------
gpos::owner<CDXLNode *>
CTranslatorExprToDXLUtils::PdxlnProjElem(
	CMemoryPool *mp, CColumnFactory *col_factory,
	gpos::pointer<ColRefToDXLNodeMap *> phmcrdxln,
	gpos::pointer<const CDXLNode *> pdxlnChildProjElem)
{
	GPOS_ASSERT(nullptr != pdxlnChildProjElem &&
				1 == pdxlnChildProjElem->Arity());

	gpos::pointer<CDXLScalarProjElem *> pdxlopPrElChild =
		dynamic_cast<CDXLScalarProjElem *>(pdxlnChildProjElem->GetOperator());

	// find the col ref corresponding to this element's id through column factory
	CColRef *colref = col_factory->LookupColRef(pdxlopPrElChild->Id());
	if (nullptr == colref)
	{
		GPOS_RAISE(gpdxl::ExmaDXL, gpdxl::ExmiExpr2DXLAttributeNotFound,
				   pdxlopPrElChild->Id());
	}

	gpos::owner<CDXLNode *> pdxlnProjElemResult =
		PdxlnProjElem(mp, phmcrdxln, colref);

	return pdxlnProjElemResult;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXLUtils::ReplaceSubplan
//
//	@doc:
//		 Replace subplan entry in the given map with a dxl column ref
//
//---------------------------------------------------------------------------
void
CTranslatorExprToDXLUtils::ReplaceSubplan(
	CMemoryPool *mp,
	gpos::pointer<ColRefToDXLNodeMap *>
		phmcrdxlnSubplans,	// map of col ref to subplan
	const CColRef *colref,	// key of entry in the passed map
	gpos::pointer<CDXLScalarProjElem *>
		pdxlopPrEl	// project element to use for creating DXL col ref to replace subplan
)
{
	GPOS_ASSERT(nullptr != phmcrdxlnSubplans);
	GPOS_ASSERT(nullptr != colref);
	GPOS_ASSERT(pdxlopPrEl->Id() == colref->Id());

	gpos::owner<IMDId *> mdid_type = colref->RetrieveType()->MDId();
	mdid_type->AddRef();
	CMDName *mdname =
		GPOS_NEW(mp) CMDName(mp, pdxlopPrEl->GetMdNameAlias()->GetMDName());
	gpos::owner<CDXLColRef *> dxl_colref = GPOS_NEW(mp) CDXLColRef(
		mdname, pdxlopPrEl->Id(), std::move(mdid_type), colref->TypeModifier());
	gpos::owner<CDXLScalarIdent *> pdxlnScId =
		GPOS_NEW(mp) CDXLScalarIdent(mp, std::move(dxl_colref));
	gpos::owner<CDXLNode *> dxlnode =
		GPOS_NEW(mp) CDXLNode(mp, std::move(pdxlnScId));
	BOOL fReplaced GPOS_ASSERTS_ONLY =
		phmcrdxlnSubplans->Replace(colref, std::move(dxlnode));
	GPOS_ASSERT(fReplaced);
}


//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXLUtils::PdxlnProjElem
//
//	@doc:
//		Create a project elem from a given col ref,
//
//		if the given col has a corresponding subplan entry in the given map,
//		the function returns a project element with a child subplan,
//		the function then replaces the subplan entry in the given map with the
//		projected column reference, so that all subplan references higher up in
//		the DXL tree use the projected col ref
//
//---------------------------------------------------------------------------
gpos::owner<CDXLNode *>
CTranslatorExprToDXLUtils::PdxlnProjElem(
	CMemoryPool *mp,
	gpos::pointer<ColRefToDXLNodeMap *>
		phmcrdxlnSubplans,	// map of col ref -> subplan: can be modified by this function
	const CColRef *colref)
{
	GPOS_ASSERT(nullptr != colref);

	CMDName *mdname = GPOS_NEW(mp) CMDName(mp, colref->Name().Pstr());

	gpos::owner<CDXLScalarProjElem *> pdxlopPrEl =
		GPOS_NEW(mp) CDXLScalarProjElem(mp, colref->Id(), mdname);
	gpos::owner<CDXLNode *> pdxlnPrEl = GPOS_NEW(mp) CDXLNode(mp, pdxlopPrEl);

	// create a scalar identifier for the proj element expression
	gpos::owner<CDXLNode *> pdxlnScId =
		PdxlnIdent(mp, phmcrdxlnSubplans, nullptr /*phmcrdxlnIndexLookup*/,
				   nullptr /*phmcrulPartColId*/, colref);

	if (EdxlopScalarSubPlan == pdxlnScId->GetOperator()->GetDXLOperator())
	{
		// modify map by replacing subplan entry with the projected
		// column reference so that all subplan references higher up
		// in the DXL tree use the projected col ref
		ReplaceSubplan(mp, phmcrdxlnSubplans, colref, pdxlopPrEl);
	}

	// attach scalar id expression to proj elem
	pdxlnPrEl->AddChild(std::move(pdxlnScId));

	return pdxlnPrEl;
}


//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXLUtils::PdxlnIdent
//
//	@doc:
//		 Create a scalar identifier node from a given col ref
//
//---------------------------------------------------------------------------
gpos::owner<CDXLNode *>
CTranslatorExprToDXLUtils::PdxlnIdent(
	CMemoryPool *mp, gpos::pointer<ColRefToDXLNodeMap *> phmcrdxlnSubplans,
	gpos::pointer<ColRefToDXLNodeMap *> phmcrdxlnIndexLookup,
	gpos::pointer<ColRefToUlongMap *> phmcrulPartColId, const CColRef *colref)
{
	GPOS_ASSERT(nullptr != colref);
	GPOS_ASSERT(nullptr != phmcrdxlnSubplans);

	gpos::pointer<CDXLNode *> dxlnode = phmcrdxlnSubplans->Find(colref);

	if (nullptr != dxlnode)
	{
		dxlnode->AddRef();
		return dxlnode;
	}

	// Check if partition mapping exists (which implies that it is a partitioned
	// table)
	ULONG colid = colref->Id();
	if (nullptr != phmcrulPartColId)
	{
		// if colref doesn't exist in partition mapping, then this scalar ident
		// is an outer ref, and we must look it up in the index outer-ref mapping
		ULONG *pul = phmcrulPartColId->Find(colref);
		if (nullptr == pul)
		{
			gpos::pointer<CDXLNode *> pdxlnIdent =
				phmcrdxlnIndexLookup->Find(colref);
			GPOS_ASSERT(nullptr != pdxlnIdent);
			pdxlnIdent->AddRef();
			return pdxlnIdent;
		}
		// the colref does exist in the partition mapping, it is therefore NOT
		// an outer ref, and we should create a dxl node
		GPOS_ASSERT(nullptr != pul);
		colid = *pul;
	}
	else
	{
		// scalar ident is not part of partition table, can look up in index
		// directly in index outer-ref mapping
		if (nullptr != phmcrdxlnIndexLookup)
		{
			gpos::pointer<CDXLNode *> pdxlnIdent =
				phmcrdxlnIndexLookup->Find(colref);
			if (nullptr != pdxlnIdent)
			{
				pdxlnIdent->AddRef();
				return pdxlnIdent;
			}
		}
	}

	CMDName *mdname = GPOS_NEW(mp) CMDName(mp, colref->Name().Pstr());

	gpos::owner<IMDId *> mdid = colref->RetrieveType()->MDId();
	mdid->AddRef();

	gpos::owner<CDXLColRef *> dxl_colref = GPOS_NEW(mp)
		CDXLColRef(mdname, colid, std::move(mdid), colref->TypeModifier());

	gpos::owner<CDXLScalarIdent *> dxl_op =
		GPOS_NEW(mp) CDXLScalarIdent(mp, std::move(dxl_colref));
	return GPOS_NEW(mp) CDXLNode(mp, std::move(dxl_op));
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXLUtils::PdrgpdatumNulls
//
//	@doc:
//		Create an array of NULL datums for a given array of columns
//
//---------------------------------------------------------------------------
gpos::owner<IDatumArray *>
CTranslatorExprToDXLUtils::PdrgpdatumNulls(
	CMemoryPool *mp, gpos::pointer<CColRefArray *> colref_array)
{
	gpos::owner<IDatumArray *> pdrgpdatum = GPOS_NEW(mp) IDatumArray(mp);

	const ULONG size = colref_array->Size();
	for (ULONG ul = 0; ul < size; ul++)
	{
		CColRef *colref = (*colref_array)[ul];
		gpos::pointer<const IMDType *> pmdtype = colref->RetrieveType();
		gpos::owner<IDatum *> datum = pmdtype->DatumNull();
		datum->AddRef();
		pdrgpdatum->Append(datum);
	}

	return pdrgpdatum;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXLUtils::FProjectListMatch
//
//	@doc:
//		Check whether a project list has the same columns in the given array
//		and in the same order
//
//---------------------------------------------------------------------------
BOOL
CTranslatorExprToDXLUtils::FProjectListMatch(
	gpos::pointer<CDXLNode *> pdxlnPrL,
	gpos::pointer<CColRefArray *> colref_array)
{
	GPOS_ASSERT(nullptr != pdxlnPrL);
	GPOS_ASSERT(nullptr != colref_array);
	GPOS_ASSERT(EdxlopScalarProjectList ==
				pdxlnPrL->GetOperator()->GetDXLOperator());

	const ULONG length = colref_array->Size();
	if (pdxlnPrL->Arity() != length)
	{
		return false;
	}

	for (ULONG ul = 0; ul < length; ul++)
	{
		CColRef *colref = (*colref_array)[ul];

		gpos::pointer<CDXLNode *> pdxlnPrEl = (*pdxlnPrL)[ul];
		gpos::pointer<CDXLScalarProjElem *> pdxlopPrEl =
			gpos::dyn_cast<CDXLScalarProjElem>(pdxlnPrEl->GetOperator());

		if (colref->Id() != pdxlopPrEl->Id())
		{
			return false;
		}
	}

	return true;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXLUtils::PdrgpcrMapColumns
//
//	@doc:
//		Map an array of columns to a new array of columns. The column index is
//		look up in the given hash map, then the corresponding column from
//		the destination array is used
//
//---------------------------------------------------------------------------
gpos::owner<CColRefArray *>
CTranslatorExprToDXLUtils::PdrgpcrMapColumns(
	CMemoryPool *mp, gpos::pointer<CColRefArray *> pdrgpcrInput,
	gpos::pointer<ColRefToUlongMap *> phmcrul,
	gpos::pointer<CColRefArray *> pdrgpcrMapDest)
{
	GPOS_ASSERT(nullptr != phmcrul);
	GPOS_ASSERT(nullptr != pdrgpcrMapDest);

	if (nullptr == pdrgpcrInput)
	{
		return nullptr;
	}

	gpos::owner<CColRefArray *> pdrgpcrNew = GPOS_NEW(mp) CColRefArray(mp);

	const ULONG length = pdrgpcrInput->Size();
	for (ULONG ul = 0; ul < length; ul++)
	{
		CColRef *colref = (*pdrgpcrInput)[ul];

		// get column index from hashmap
		ULONG *pul = phmcrul->Find(colref);
		GPOS_ASSERT(nullptr != pul);

		// add corresponding column from dest array
		pdrgpcrNew->Append((*pdrgpcrMapDest)[*pul]);
	}

	return pdrgpcrNew;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXLUtils::PdxlnResult
//
//	@doc:
//		Create a DXL result node using the given properties, project list,
//		filters, and relational child
//
//---------------------------------------------------------------------------
gpos::owner<CDXLNode *>
CTranslatorExprToDXLUtils::PdxlnResult(
	CMemoryPool *mp, gpos::owner<CDXLPhysicalProperties *> dxl_properties,
	gpos::owner<CDXLNode *> pdxlnPrL, gpos::owner<CDXLNode *> filter_dxlnode,
	gpos::owner<CDXLNode *> one_time_filter,
	gpos::owner<CDXLNode *> child_dxlnode)
{
	gpos::owner<CDXLPhysicalResult *> dxl_op =
		GPOS_NEW(mp) CDXLPhysicalResult(mp);
	gpos::owner<CDXLNode *> pdxlnResult = GPOS_NEW(mp) CDXLNode(mp, dxl_op);
	pdxlnResult->SetProperties(dxl_properties);

	pdxlnResult->AddChild(pdxlnPrL);
	pdxlnResult->AddChild(filter_dxlnode);
	pdxlnResult->AddChild(one_time_filter);

	if (nullptr != child_dxlnode)
	{
		pdxlnResult->AddChild(child_dxlnode);
	}

#ifdef GPOS_DEBUG
	dxl_op->AssertValid(pdxlnResult, false /* validate_children */);
#endif

	return pdxlnResult;
}

// create a DXL Value Scan node
gpos::owner<CDXLNode *>
CTranslatorExprToDXLUtils::PdxlnValuesScan(
	CMemoryPool *mp, gpos::owner<CDXLPhysicalProperties *> dxl_properties,
	CDXLNode *pdxlnPrL, gpos::pointer<IDatum2dArray *> pdrgpdrgdatum)
{
	gpos::owner<CDXLPhysicalValuesScan *> dxl_op =
		GPOS_NEW(mp) CDXLPhysicalValuesScan(mp);
	gpos::owner<CDXLNode *> pdxlnValuesScan = GPOS_NEW(mp) CDXLNode(mp, dxl_op);
	pdxlnValuesScan->SetProperties(dxl_properties);

	pdxlnValuesScan->AddChild(pdxlnPrL);

	const ULONG ulTuples = pdrgpdrgdatum->Size();

	for (ULONG ulTuplePos = 0; ulTuplePos < ulTuples; ulTuplePos++)
	{
		gpos::owner<IDatumArray *> pdrgpdatum = (*pdrgpdrgdatum)[ulTuplePos];
		pdrgpdatum->AddRef();
		const ULONG num_cols = pdrgpdatum->Size();
		gpos::owner<CDXLScalarValuesList *> values =
			GPOS_NEW(mp) CDXLScalarValuesList(mp);
		gpos::owner<CDXLNode *> value_list_dxlnode =
			GPOS_NEW(mp) CDXLNode(mp, values);

		for (ULONG ulColPos = 0; ulColPos < num_cols; ulColPos++)
		{
			gpos::pointer<IDatum *> datum = (*pdrgpdatum)[ulColPos];
			CMDAccessor *md_accessor = COptCtxt::PoctxtFromTLS()->Pmda();
			gpos::pointer<const IMDType *> pmdtype =
				md_accessor->RetrieveType(datum->MDId());

			gpos::owner<CDXLNode *> pdxlnValue =
				GPOS_NEW(mp) CDXLNode(mp, pmdtype->GetDXLOpScConst(mp, datum));
			value_list_dxlnode->AddChild(pdxlnValue);
		}
		pdrgpdatum->Release();
		pdxlnValuesScan->AddChild(value_list_dxlnode);
	}

#ifdef GPOS_DEBUG
	dxl_op->AssertValid(pdxlnValuesScan, true /* validate_children */);
#endif

	return pdxlnValuesScan;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXLUtils::PdxlnCombineBoolean
//
//	@doc:
//		Combine two boolean expressions using the given boolean operator
//
//---------------------------------------------------------------------------
gpos::owner<CDXLNode *>
CTranslatorExprToDXLUtils::PdxlnCombineBoolean(
	CMemoryPool *mp, gpos::owner<CDXLNode *> first_child_dxlnode,
	gpos::owner<CDXLNode *> second_child_dxlnode, EdxlBoolExprType boolexptype)
{
	GPOS_ASSERT(Edxlor == boolexptype || Edxland == boolexptype);

	if (nullptr == first_child_dxlnode)
	{
		return second_child_dxlnode;
	}

	if (nullptr == second_child_dxlnode)
	{
		return first_child_dxlnode;
	}

	return GPOS_NEW(mp) CDXLNode(
		mp, GPOS_NEW(mp) CDXLScalarBoolExpr(mp, boolexptype),
		std::move(first_child_dxlnode), std::move(second_child_dxlnode));
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXLUtils::PhmcrulColIndex
//
//	@doc:
//		Build a hashmap based on a column array, where the key is the column
//		and the value is the index of that column in the array
//
//---------------------------------------------------------------------------
gpos::owner<ColRefToUlongMap *>
CTranslatorExprToDXLUtils::PhmcrulColIndex(
	CMemoryPool *mp, gpos::pointer<CColRefArray *> colref_array)
{
	gpos::owner<ColRefToUlongMap *> phmcrul = GPOS_NEW(mp) ColRefToUlongMap(mp);

	const ULONG length = colref_array->Size();
	for (ULONG ul = 0; ul < length; ul++)
	{
		CColRef *colref = (*colref_array)[ul];
		ULONG *pul = GPOS_NEW(mp) ULONG(ul);

		// add to hashmap
#ifdef GPOS_DEBUG
		BOOL fRes =
#endif	// GPOS_DEBUG
			phmcrul->Insert(colref, pul);
		GPOS_ASSERT(fRes);
	}

	return phmcrul;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXLUtils::SetStats
//
//	@doc:
//		Set the statistics of the dxl operator
//
//---------------------------------------------------------------------------
void
CTranslatorExprToDXLUtils::SetStats(CMemoryPool *mp, CMDAccessor *md_accessor,
									gpos::pointer<CDXLNode *> dxlnode,
									gpos::pointer<const IStatistics *> stats,
									BOOL fRoot)
{
	if (nullptr != stats && GPOS_FTRACE(EopttraceExtractDXLStats) &&
		(GPOS_FTRACE(EopttraceExtractDXLStatsAllNodes) || fRoot))
	{
		gpos::dyn_cast<CDXLPhysicalProperties>(dxlnode->GetProperties())
			->SetStats(stats->GetDxlStatsDrvdRelation(mp, md_accessor));
	}
}


//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXLUtils::SetDirectDispatchInfo
//
//	@doc:
//		Set the direct dispatch info of the dxl operator
//
//---------------------------------------------------------------------------
void
CTranslatorExprToDXLUtils::SetDirectDispatchInfo(
	CMemoryPool *mp, CMDAccessor *md_accessor,
	gpos::pointer<CDXLNode *> dxlnode, gpos::pointer<CExpression *> pexpr,
	gpos::pointer<CDistributionSpecArray *> pdrgpdsBaseTables)
{
	GPOS_ASSERT(nullptr != dxlnode);
	GPOS_ASSERT(nullptr != pexpr);
	GPOS_ASSERT(nullptr != pdrgpdsBaseTables);

	Edxlopid edxlopid = dxlnode->GetOperator()->GetDXLOperator();
	if (EdxlopPhysicalCTAS == edxlopid || EdxlopPhysicalDML == edxlopid ||
		EdxlopPhysicalRowTrigger == edxlopid)
	{
		// direct dispatch for CTAS and DML handled elsewhere
		// TODO:  - Oct 15, 2014; unify
		return;
	}

	if (1 != pexpr->DeriveJoinDepth() || 1 != pdrgpdsBaseTables->Size())
	{
		// direct dispatch not supported for join queries
		return;
	}

	gpos::pointer<CExpressionArray *> pexprFilterArray =
		COptCtxt::PoctxtFromTLS()->GetDirectDispatchableFilters();
	ULONG size = pexprFilterArray->Size();

	if (0 == size)
	{
		return;
	}

	gpos::pointer<CDistributionSpec *> pds = (*pdrgpdsBaseTables)[0];

	// go thru all the filters and see if we have one that can
	// give us a direct dispatch.
	// e.g. in the below plan, the CPhysicalFilter is on a non distribution
	// key but CPhysicalIndexScan is on distribution key. So this plan can be
	// direct dispatched.
	//
	//		+--CPhysicalMotionGather
	//			+--CPhysicalFilter
	//			  |--CPhysicalIndexScan
	//			  |  +--CScalarCmp (=)
	//			  |     |--CScalarIdent "dist_key"
	//			  |     +--CScalarConst (5)
	//			  +--CScalarCmp (=)
	//			  |--CScalarIdent "non_dist_key"
	//			  +--CScalarConst (5)

	if (CDistributionSpec::EdtHashed == pds->Edt())
	{
		// direct dispatch only supported for scans over hash distributed tables
		for (ULONG i = 0; i < size; i++)
		{
			gpos::pointer<CExpression *> pexprFilter = (*pexprFilterArray)[i];
			gpos::pointer<CPropConstraint *> ppc =
				pexprFilter->DerivePropertyConstraint();

			if (nullptr != ppc->Pcnstr())
			{
				GPOS_ASSERT(nullptr != ppc->Pcnstr());

				gpos::pointer<CDistributionSpecHashed *> pdsHashed =
					gpos::dyn_cast<CDistributionSpecHashed>(pds);
				gpos::pointer<CExpressionArray *> pdrgpexprHashed =
					pdsHashed->Pdrgpexpr();

				gpos::owner<CDXLDirectDispatchInfo *> dxl_direct_dispatch_info =
					GetDXLDirectDispatchInfo(mp, md_accessor, pdrgpexprHashed,
											 ppc->Pcnstr());

				if (nullptr != dxl_direct_dispatch_info)
				{
					dxlnode->SetDirectDispatchInfo(
						std::move(dxl_direct_dispatch_info));
					break;
				}
			}
		}
	}
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXLUtils::GetDXLDirectDispatchInfo
//
//	@doc:
//		Compute the direct dispatch info spec. Returns NULL if this is not
//		possible
//
//---------------------------------------------------------------------------
gpos::owner<CDXLDirectDispatchInfo *>
CTranslatorExprToDXLUtils::GetDXLDirectDispatchInfo(
	CMemoryPool *mp, CMDAccessor *md_accessor,
	gpos::pointer<CExpressionArray *> pdrgpexprHashed, CConstraint *pcnstr)
{
	GPOS_ASSERT(nullptr != pdrgpexprHashed);
	GPOS_ASSERT(nullptr != pcnstr);

	const ULONG ulHashExpr = pdrgpexprHashed->Size();
	GPOS_ASSERT(0 < ulHashExpr);

	if (1 == ulHashExpr)
	{
		gpos::pointer<CExpression *> pexprHashed = (*pdrgpexprHashed)[0];
		return PdxlddinfoSingleDistrKey(mp, md_accessor, pexprHashed, pcnstr);
	}

	BOOL fSuccess = true;
	gpos::owner<CDXLDatumArray *> pdrgpdxldatum =
		GPOS_NEW(mp) CDXLDatumArray(mp);

	for (ULONG ul = 0; ul < ulHashExpr && fSuccess; ul++)
	{
		gpos::pointer<CExpression *> pexpr = (*pdrgpexprHashed)[ul];
		if (!CUtils::FScalarIdent(pexpr))
		{
			fSuccess = false;
			break;
		}

		const CColRef *pcrDistrCol =
			gpos::dyn_cast<CScalarIdent>(pexpr->Pop())->Pcr();

		gpos::owner<CConstraint *> pcnstrDistrCol =
			pcnstr->Pcnstr(mp, pcrDistrCol);

		gpos::owner<CDXLDatum *> dxl_datum = PdxldatumFromPointConstraint(
			mp, md_accessor, pcrDistrCol, pcnstrDistrCol);
		CRefCount::SafeRelease(pcnstrDistrCol);

		if (nullptr != dxl_datum && FDirectDispatchable(pcrDistrCol, dxl_datum))
		{
			pdrgpdxldatum->Append(dxl_datum);
		}
		else
		{
			CRefCount::SafeRelease(dxl_datum);

			fSuccess = false;
			break;
		}
	}

	if (!fSuccess)
	{
		pdrgpdxldatum->Release();

		return nullptr;
	}

	gpos::owner<CDXLDatum2dArray *> pdrgpdrgpdxldatum =
		GPOS_NEW(mp) CDXLDatum2dArray(mp);
	pdrgpdrgpdxldatum->Append(std::move(pdrgpdxldatum));
	return GPOS_NEW(mp)
		CDXLDirectDispatchInfo(std::move(pdrgpdrgpdxldatum), false);
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXLUtils::PdxlddinfoSingleDistrKey
//
//	@doc:
//		Compute the direct dispatch info spec for a single distribution key.
//		Returns NULL if this is not possible
//
//---------------------------------------------------------------------------
gpos::owner<CDXLDirectDispatchInfo *>
CTranslatorExprToDXLUtils::PdxlddinfoSingleDistrKey(
	CMemoryPool *mp, CMDAccessor *md_accessor,
	gpos::pointer<CExpression *> pexprHashed, CConstraint *pcnstr)
{
	GPOS_ASSERT(nullptr != pexprHashed);
	if (!CUtils::FScalarIdent(pexprHashed))
	{
		return nullptr;
	}

	const CColRef *pcrDistrCol =
		gpos::dyn_cast<CScalarIdent>(pexprHashed->Pop())->Pcr();

	BOOL useRawValues = false;
	gpos::owner<CConstraint *> pcnstrDistrCol = pcnstr->Pcnstr(mp, pcrDistrCol);
	CConstraintInterval *pcnstrInterval;
	if (pcnstrDistrCol == nullptr &&
		(pcnstrInterval = dynamic_cast<CConstraintInterval *>(pcnstr)))
	{
		if (pcnstrInterval->FConstraintOnSegmentId())
		{
			// If the constraint is on gp_segment_id then we trick ourselves into
			// considering the constraint as being on a distribution column.
			pcnstrDistrCol = pcnstr;
			pcnstrDistrCol->AddRef();
			pcrDistrCol = pcnstrInterval->Pcr();
			useRawValues = true;
		}
	}

	gpos::owner<CDXLDatum2dArray *> pdrgpdrgpdxldatum = nullptr;

	if (CPredicateUtils::FConstColumn(pcnstrDistrCol, pcrDistrCol))
	{
		gpos::owner<CDXLDatum *> dxl_datum = PdxldatumFromPointConstraint(
			mp, md_accessor, pcrDistrCol, pcnstrDistrCol);
		GPOS_ASSERT(nullptr != dxl_datum);

		if (FDirectDispatchable(pcrDistrCol, dxl_datum))
		{
			gpos::owner<CDXLDatumArray *> pdrgpdxldatum =
				GPOS_NEW(mp) CDXLDatumArray(mp);

			dxl_datum->AddRef();
			pdrgpdxldatum->Append(dxl_datum);

			pdrgpdrgpdxldatum = GPOS_NEW(mp) CDXLDatum2dArray(mp);
			pdrgpdrgpdxldatum->Append(pdrgpdxldatum);
		}

		dxl_datum->Release();
	}
	else if (CPredicateUtils::FColumnDisjunctionOfConst(pcnstrDistrCol,
														pcrDistrCol))
	{
		pdrgpdrgpdxldatum = PdrgpdrgpdxldatumFromDisjPointConstraint(
			mp, md_accessor, pcrDistrCol, pcnstrDistrCol);
	}

	CRefCount::SafeRelease(pcnstrDistrCol);

	if (nullptr == pdrgpdrgpdxldatum)
	{
		return nullptr;
	}

	return GPOS_NEW(mp)
		CDXLDirectDispatchInfo(std::move(pdrgpdrgpdxldatum), useRawValues);
}


//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXLUtils::FDirectDispatchable
//
//	@doc:
//		Check if the given constant value for a particular distribution column
// 		can be used to identify which segment to direct dispatch to.
//
//---------------------------------------------------------------------------
BOOL
CTranslatorExprToDXLUtils::FDirectDispatchable(
	const CColRef *pcrDistrCol, gpos::pointer<const CDXLDatum *> dxl_datum)
{
	GPOS_ASSERT(nullptr != pcrDistrCol);
	GPOS_ASSERT(nullptr != dxl_datum);

	gpos::pointer<IMDId *> pmdidDatum = dxl_datum->MDId();
	gpos::pointer<IMDId *> pmdidDistrCol = pcrDistrCol->RetrieveType()->MDId();

	// since all integer values are up-casted to int64, the hash value will be
	// consistent. If either the constant or the distribution column are
	// not integers, then their datatypes must be identical to ensure that
	// the hash value of the constant will point to the right segment.
	BOOL fBothInt =
		CUtils::FIntType(pmdidDistrCol) && CUtils::FIntType(pmdidDatum);

	return fBothInt || (pmdidDatum->Equals(pmdidDistrCol));
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXLUtils::PdxldatumFromPointConstraint
//
//	@doc:
//		Compute a DXL datum from a point constraint. Returns NULL if this is not
//		possible
//
//---------------------------------------------------------------------------
gpos::owner<CDXLDatum *>
CTranslatorExprToDXLUtils::PdxldatumFromPointConstraint(
	CMemoryPool *mp, CMDAccessor *md_accessor, const CColRef *pcrDistrCol,
	gpos::pointer<CConstraint *> pcnstrDistrCol)
{
	if (!CPredicateUtils::FConstColumn(pcnstrDistrCol, pcrDistrCol))
	{
		return nullptr;
	}

	GPOS_ASSERT(CConstraint::EctInterval == pcnstrDistrCol->Ect());

	gpos::pointer<CConstraintInterval *> pci =
		dynamic_cast<CConstraintInterval *>(pcnstrDistrCol);
	GPOS_ASSERT(1 >= pci->Pdrgprng()->Size());

	gpos::owner<CDXLDatum *> dxl_datum = nullptr;

	if (1 == pci->Pdrgprng()->Size())
	{
		gpos::pointer<const CRange *> prng = (*pci->Pdrgprng())[0];
		dxl_datum = CTranslatorExprToDXLUtils::GetDatumVal(mp, md_accessor,
														   prng->PdatumLeft());
	}
	else
	{
		GPOS_ASSERT(pci->FIncludesNull());
		dxl_datum = pcrDistrCol->RetrieveType()->GetDXLDatumNull(mp);
	}

	return dxl_datum;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXLUtils::PdrgpdrgpdxldatumFromDisjPointConstraint
//
//	@doc:
//		Compute an array of DXL datum arrays from a disjunction of point constraints.
//		Returns NULL if this is not possible
//
//---------------------------------------------------------------------------
gpos::owner<CDXLDatum2dArray *>
CTranslatorExprToDXLUtils::PdrgpdrgpdxldatumFromDisjPointConstraint(
	CMemoryPool *mp, CMDAccessor *md_accessor, const CColRef *pcrDistrCol,
	gpos::pointer<CConstraint *> pcnstrDistrCol)
{
	GPOS_ASSERT(nullptr != pcnstrDistrCol);
	if (CPredicateUtils::FConstColumn(pcnstrDistrCol, pcrDistrCol))
	{
		gpos::owner<CDXLDatum2dArray *> pdrgpdrgpdxldatum = nullptr;

		gpos::owner<CDXLDatum *> dxl_datum = PdxldatumFromPointConstraint(
			mp, md_accessor, pcrDistrCol, pcnstrDistrCol);

		if (FDirectDispatchable(pcrDistrCol, dxl_datum))
		{
			gpos::owner<CDXLDatumArray *> pdrgpdxldatum =
				GPOS_NEW(mp) CDXLDatumArray(mp);

			dxl_datum->AddRef();
			pdrgpdxldatum->Append(dxl_datum);

			pdrgpdrgpdxldatum = GPOS_NEW(mp) CDXLDatum2dArray(mp);
			pdrgpdrgpdxldatum->Append(pdrgpdxldatum);
		}

		// clean up
		dxl_datum->Release();

		return pdrgpdrgpdxldatum;
	}

	GPOS_ASSERT(CConstraint::EctInterval == pcnstrDistrCol->Ect());

	gpos::pointer<CConstraintInterval *> pcnstrInterval =
		dynamic_cast<CConstraintInterval *>(pcnstrDistrCol);

	gpos::pointer<CRangeArray *> pdrgprng = pcnstrInterval->Pdrgprng();

	const ULONG ulRanges = pdrgprng->Size();
	gpos::owner<CDXLDatum2dArray *> pdrgpdrgpdxdatum =
		GPOS_NEW(mp) CDXLDatum2dArray(mp);

	for (ULONG ul = 0; ul < ulRanges; ul++)
	{
		gpos::pointer<CRange *> prng = (*pdrgprng)[ul];
		GPOS_ASSERT(prng->FPoint());
		gpos::owner<CDXLDatum *> dxl_datum =
			CTranslatorExprToDXLUtils::GetDatumVal(mp, md_accessor,
												   prng->PdatumLeft());

		if (!FDirectDispatchable(pcrDistrCol, dxl_datum))
		{
			// clean up
			dxl_datum->Release();
			pdrgpdrgpdxdatum->Release();

			return nullptr;
		}

		gpos::owner<CDXLDatumArray *> pdrgpdxldatum =
			GPOS_NEW(mp) CDXLDatumArray(mp);

		pdrgpdxldatum->Append(dxl_datum);
		pdrgpdrgpdxdatum->Append(pdrgpdxldatum);
	}

	if (pcnstrInterval->FIncludesNull())
	{
		gpos::owner<CDXLDatum *> dxl_datum =
			pcrDistrCol->RetrieveType()->GetDXLDatumNull(mp);

		if (!FDirectDispatchable(pcrDistrCol, dxl_datum))
		{
			// clean up
			dxl_datum->Release();
			pdrgpdrgpdxdatum->Release();

			return nullptr;
		}

		gpos::owner<CDXLDatumArray *> pdrgpdxldatum =
			GPOS_NEW(mp) CDXLDatumArray(mp);
		pdrgpdxldatum->Append(dxl_datum);
		pdrgpdrgpdxdatum->Append(pdrgpdxldatum);
	}

	if (0 < pdrgpdrgpdxdatum->Size())
	{
		return pdrgpdrgpdxdatum;
	}

	// clean up
	pdrgpdrgpdxdatum->Release();

	return nullptr;
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXLUtils::FLocalHashAggStreamSafe
//
//	@doc:
//		Is the aggregate a local hash aggregate that is safe to stream
//
//---------------------------------------------------------------------------
BOOL
CTranslatorExprToDXLUtils::FLocalHashAggStreamSafe(
	gpos::pointer<CExpression *> pexprAgg)
{
	GPOS_ASSERT(nullptr != pexprAgg);

	COperator::EOperatorId op_id = pexprAgg->Pop()->Eopid();

	if (COperator::EopPhysicalHashAgg != op_id &&
		COperator::EopPhysicalHashAggDeduplicate != op_id)
	{
		// not a hash aggregate
		return false;
	}

	gpos::pointer<CPhysicalAgg *> popAgg =
		gpos::dyn_cast<CPhysicalAgg>(pexprAgg->Pop());

	// is a local hash aggregate and it generates duplicates (therefore safe to stream)
	return (COperator::EgbaggtypeLocal == popAgg->Egbaggtype()) &&
		   popAgg->FGeneratesDuplicates();
}

//---------------------------------------------------------------------------
//	@function:
//		CTranslatorExprToDXLUtils::ExtractCastFuncMdids
//
//	@doc:
//		If operator is a scalar cast/scalar func extract type and function
//
//---------------------------------------------------------------------------
void
CTranslatorExprToDXLUtils::ExtractCastFuncMdids(
	gpos::pointer<COperator *> pop, gpos::pointer<IMDId *> *ppmdidType,
	gpos::pointer<IMDId *> *ppmdidCastFunc)
{
	GPOS_ASSERT(nullptr != pop);
	GPOS_ASSERT(nullptr != ppmdidType);
	GPOS_ASSERT(nullptr != ppmdidCastFunc);

	if (COperator::EopScalarCast != pop->Eopid() &&
		COperator::EopScalarFunc != pop->Eopid())
	{
		// not a cast or allowed func
		return;
	}
	if (COperator::EopScalarCast == pop->Eopid())
	{
		gpos::pointer<CScalarCast *> popCast = gpos::dyn_cast<CScalarCast>(pop);
		*ppmdidType = popCast->MdidType();
		*ppmdidCastFunc = popCast->FuncMdId();
	}
	else
	{
		GPOS_ASSERT(COperator::EopScalarFunc == pop->Eopid());
		gpos::pointer<CScalarFunc *> popFunc = gpos::dyn_cast<CScalarFunc>(pop);
		*ppmdidType = popFunc->MdidType();
		*ppmdidCastFunc = popFunc->FuncMdId();
	}
}

BOOL
CTranslatorExprToDXLUtils::FDXLOpExists(gpos::pointer<const CDXLOperator *> pop,
										const gpdxl::Edxlopid *peopid,
										ULONG ulOps)
{
	GPOS_ASSERT(nullptr != pop);
	GPOS_ASSERT(nullptr != peopid);

	gpdxl::Edxlopid op_id = pop->GetDXLOperator();
	for (ULONG ul = 0; ul < ulOps; ul++)
	{
		if (op_id == peopid[ul])
		{
			return true;
		}
	}

	return false;
}

BOOL
CTranslatorExprToDXLUtils::FHasDXLOp(gpos::pointer<const CDXLNode *> dxlnode,
									 const gpdxl::Edxlopid *peopid, ULONG ulOps)
{
	GPOS_CHECK_STACK_SIZE;
	GPOS_ASSERT(nullptr != dxlnode);
	GPOS_ASSERT(nullptr != peopid);

	if (FDXLOpExists(dxlnode->GetOperator(), peopid, ulOps))
	{
		return true;
	}

	// recursively check children
	const ULONG arity = dxlnode->Arity();

	for (ULONG ul = 0; ul < arity; ul++)
	{
		if (FHasDXLOp((*dxlnode)[ul], peopid, ulOps))
		{
			return true;
		}
	}

	return false;
}

BOOL
CTranslatorExprToDXLUtils::FProjListContainsSubplanWithBroadCast(
	gpos::pointer<CDXLNode *> pdxlnPrjList)
{
	if (pdxlnPrjList->GetOperator()->GetDXLOperator() == EdxlopScalarSubPlan)
	{
		gpdxl::Edxlopid rgeopidMotion[] = {EdxlopPhysicalMotionBroadcast};
		return FHasDXLOp(pdxlnPrjList, rgeopidMotion,
						 GPOS_ARRAY_SIZE(rgeopidMotion));
	}

	const ULONG arity = pdxlnPrjList->Arity();

	for (ULONG ul = 0; ul < arity; ul++)
	{
		if (FProjListContainsSubplanWithBroadCast((*pdxlnPrjList)[ul]))
		{
			return true;
		}
	}

	return false;
}

void
CTranslatorExprToDXLUtils::ExtractIdentColIds(gpos::pointer<CDXLNode *> dxlnode,
											  CBitSet *pbs)
{
	if (dxlnode->GetOperator()->GetDXLOperator() == EdxlopScalarIdent)
	{
		gpos::pointer<const CDXLColRef *> dxl_colref =
			gpos::dyn_cast<CDXLScalarIdent>(dxlnode->GetOperator())
				->GetDXLColRef();
		pbs->ExchangeSet(dxl_colref->Id());
	}

	ULONG arity = dxlnode->Arity();
	for (ULONG ul = 0; ul < arity; ul++)
	{
		ExtractIdentColIds((*dxlnode)[ul], pbs);
	}
}

BOOL
CTranslatorExprToDXLUtils::FMotionHazard(CMemoryPool *mp,
										 gpos::pointer<CDXLNode *> dxlnode,
										 const gpdxl::Edxlopid *peopid,
										 ULONG ulOps,
										 gpos::pointer<CBitSet *> pbsPrjCols)
{
	GPOS_ASSERT(pbsPrjCols);

	// non-streaming operator/Gather motion neutralizes any motion hazard that its subtree imposes
	// hence stop recursing further
	if (FMotionHazardSafeOp(dxlnode))
		return false;

	if (FDXLOpExists(dxlnode->GetOperator(), peopid, ulOps))
	{
		// check if the current motion node projects any column from the
		// input project list.
		// If yes, then we have detected a motion hazard for the parent Result node.
		gpos::owner<CBitSet *> pbsPrjList = GPOS_NEW(mp) CBitSet(mp);
		ExtractIdentColIds((*dxlnode)[0], pbsPrjList);
		BOOL fDisJoint = pbsPrjCols->IsDisjoint(pbsPrjList);
		pbsPrjList->Release();

		return !fDisJoint;
	}

	// recursively check children
	const ULONG arity = dxlnode->Arity();

	// In ORCA, inner child of Hash Join is always exhausted first,
	// so only check the outer child for motions
	if (dxlnode->GetOperator()->GetDXLOperator() == EdxlopPhysicalHashJoin)
	{
		if (FMotionHazard(mp, (*dxlnode)[EdxlhjIndexHashLeft], peopid, ulOps,
						  pbsPrjCols))
		{
			return true;
		}
	}
	else
	{
		for (ULONG ul = 0; ul < arity; ul++)
		{
			if (FMotionHazard(mp, (*dxlnode)[ul], peopid, ulOps, pbsPrjCols))
			{
				return true;
			}
		}
	}

	return false;
}

BOOL
CTranslatorExprToDXLUtils::FMotionHazardSafeOp(
	gpos::pointer<CDXLNode *> dxlnode)
{
	BOOL fMotionHazardSafeOp = false;
	Edxlopid edxlop = dxlnode->GetOperator()->GetDXLOperator();

	switch (edxlop)
	{
		case EdxlopPhysicalSort:
		case EdxlopPhysicalMotionGather:
		case EdxlopPhysicalMaterialize:
			fMotionHazardSafeOp = true;
			break;

		case EdxlopPhysicalAgg:
		{
			gpos::pointer<CDXLPhysicalAgg *> pdxlnPhysicalAgg =
				gpos::dyn_cast<CDXLPhysicalAgg>(dxlnode->GetOperator());
			if (pdxlnPhysicalAgg->GetAggStrategy() == EdxlaggstrategyHashed)
				fMotionHazardSafeOp = true;
		}
		break;

		default:
			break;
	}

	return fMotionHazardSafeOp;
}

BOOL
CTranslatorExprToDXLUtils::FDirectDispatchableFilter(
	gpos::pointer<CExpression *> pexprFilter)
{
	GPOS_ASSERT(nullptr != pexprFilter);

	gpos::pointer<CExpression *> pexprChild = (*pexprFilter)[0];
	gpos::pointer<COperator *> pop = pexprChild->Pop();

	// find the first child or grandchild of filter which is not
	// a Project, Filter or PhysicalComputeScalar (result node)
	// if it is a scan, then this Filter is direct dispatchable
	while (COperator::EopPhysicalPartitionSelector == pop->Eopid() ||
		   COperator::EopPhysicalFilter == pop->Eopid() ||
		   COperator::EopPhysicalComputeScalar == pop->Eopid())
	{
		pexprChild = (*pexprChild)[0];
		pop = pexprChild->Pop();
	}

	return (CUtils::FPhysicalScan(pop));
}

// EOF
