//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CTranslatorExprToDXLUtils.h
//
//	@doc:
//		Class providing helper methods for translating from Expr to DXL
//---------------------------------------------------------------------------
#ifndef GPOPT_CTranslatorExprToDXLUtils_H
#define GPOPT_CTranslatorExprToDXLUtils_H

#include "gpos/base.h"
#include "gpos/common/owner.h"

#include "gpopt/base/CRange.h"
#include "gpopt/mdcache/CMDAccessor.h"
#include "gpopt/metadata/CTableDescriptor.h"
#include "gpopt/operators/CExpression.h"
#include "gpopt/operators/CLogicalConstTableGet.h"
#include "gpopt/translate/CTranslatorExprToDXL.h"
#include "naucrates/dxl/operators/CDXLColDescr.h"
#include "naucrates/dxl/operators/CDXLNode.h"
#include "naucrates/dxl/operators/CDXLScalarBoolExpr.h"

// fwd decl
namespace gpmd
{
class IMDRelation;
}

namespace gpdxl
{
class CDXLPhysicalProperties;
class CDXLScalarProjElem;
}  // namespace gpdxl

namespace gpopt
{
using namespace gpos;
using namespace gpmd;
using namespace gpdxl;
using namespace gpnaucrates;

//---------------------------------------------------------------------------
//	@class:
//		CTranslatorExprToDXLUtils
//
//	@doc:
//		Class providing helper methods for translating from Expr to DXL
//
//---------------------------------------------------------------------------
class CTranslatorExprToDXLUtils
{
private:
	// construct a scalar comparison of the given type between the
	// column with the given col id and the scalar expression
	static gpos::owner<CDXLNode *> PdxlnCmp(
		CMemoryPool *mp, CMDAccessor *md_accessor, ULONG ulPartLevel,
		BOOL fLowerBound, gpos::owner<CDXLNode *> pdxlnScalar,
		IMDType::ECmpType cmp_type, IMDId *pmdidTypePartKey,
		gpos::pointer<IMDId *> pmdidTypeExpr, IMDId *pmdidTypeCastExpr,
		IMDId *mdid_cast_func);


	// create a column reference
	static CColRef *PcrCreate(CMemoryPool *mp, CMDAccessor *md_accessor,
							  CColumnFactory *col_factory,
							  gpos::pointer<IMDId *> mdid, INT type_modifier,
							  const WCHAR *wszName);


	// compute a DXL datum from a point constraint
	static gpos::owner<CDXLDatum *> PdxldatumFromPointConstraint(
		CMemoryPool *mp, CMDAccessor *md_accessor, const CColRef *pcrDistrCol,
		gpos::pointer<CConstraint *> pcnstrDistrCol);

	// compute an array of DXL datum arrays from a disjunction of point constraints
	static gpos::owner<CDXLDatum2dArray *>
	PdrgpdrgpdxldatumFromDisjPointConstraint(
		CMemoryPool *mp, CMDAccessor *md_accessor, const CColRef *pcrDistrCol,
		gpos::pointer<CConstraint *> pcnstrDistrCol);

	// compute the direct dispatch info  from the constraints
	// on the distribution keys
	static gpos::owner<CDXLDirectDispatchInfo *> GetDXLDirectDispatchInfo(
		CMemoryPool *mp, CMDAccessor *md_accessor,
		gpos::pointer<CExpressionArray *> pdrgpexprHashed, CConstraint *pcnstr);

	// compute the direct dispatch info for a single distribution key from the constraints
	// on the distribution key
	static gpos::owner<CDXLDirectDispatchInfo *> PdxlddinfoSingleDistrKey(
		CMemoryPool *mp, CMDAccessor *md_accessor,
		gpos::pointer<CExpression *> pexprHashed, CConstraint *pcnstr);

	// check if the given constant value for a particular distribution column can be used
	// to identify which segment to direct dispatch to.
	static BOOL FDirectDispatchable(const CColRef *pcrDistrCol,
									gpos::pointer<const CDXLDatum *> dxl_datum);

public:
	// construct a default properties container
	static gpos::owner<CDXLPhysicalProperties *> GetProperties(CMemoryPool *mp);

	// create a scalar const value expression for the given bool value
	static gpos::owner<CDXLNode *> PdxlnBoolConst(CMemoryPool *mp,
												  CMDAccessor *md_accessor,
												  BOOL value);

	// create a scalar const value expression for the given int4 value
	static gpos::owner<CDXLNode *> PdxlnInt4Const(CMemoryPool *mp,
												  CMDAccessor *md_accessor,
												  INT val);

	// construct a filter node for a list partition predicate
	static gpos::owner<CDXLNode *> PdxlnListFilterScCmp(
		CMemoryPool *mp, CMDAccessor *md_accessor, CDXLNode *pdxlnPartKey,
		CDXLNode *pdxlnScalar, gpos::pointer<IMDId *> pmdidTypePartKey,
		gpos::pointer<IMDId *> pmdidTypeOther, IMDType::ECmpType cmp_type,
		ULONG ulPartLevel, BOOL fHasDefaultPart);

	// construct a DXL node for the part key portion of the list partition filter
	static gpos::owner<CDXLNode *> PdxlnListFilterPartKey(
		CMemoryPool *mp, CMDAccessor *md_accessor,
		gpos::pointer<CExpression *> pexprPartKey, IMDId *pmdidTypePartKey,
		ULONG ulPartLevel);

	// construct a filter node for a range predicate
	static gpos::owner<CDXLNode *> PdxlnRangeFilterScCmp(
		CMemoryPool *mp, CMDAccessor *md_accessor, CDXLNode *pdxlnScalar,
		IMDId *pmdidTypePartKey, gpos::pointer<IMDId *> pmdidTypeOther,
		IMDId *pmdidTypeCastExpr, IMDId *mdid_cast_func,
		IMDType::ECmpType cmp_type, ULONG ulPartLevel);

	// construct a range filter for an equality comparison
	static gpos::owner<CDXLNode *> PdxlnRangeFilterEqCmp(
		CMemoryPool *mp, CMDAccessor *md_accessor, CDXLNode *pdxlnScalar,
		IMDId *pmdidTypePartKey, gpos::pointer<IMDId *> pmdidTypeOther,
		IMDId *pmdidTypeCastExpr, IMDId *mdid_cast_func, ULONG ulPartLevel);

	// construct a predicate for the lower or upper bound of a partition
	static gpos::owner<CDXLNode *> PdxlnRangeFilterPartBound(
		CMemoryPool *mp, CMDAccessor *md_accessor, CDXLNode *pdxlnScalar,
		IMDId *pmdidTypePartKey, gpos::pointer<IMDId *> pmdidTypeOther,
		IMDId *pmdidTypeCastExpr, IMDId *mdid_cast_func, ULONG ulPartLevel,
		ULONG fLowerBound, IMDType::ECmpType cmp_type);

	// construct predicates to cover the cases of default partition and
	// open-ended partitions if necessary
	static gpos::owner<CDXLNode *> PdxlnRangeFilterDefaultAndOpenEnded(
		CMemoryPool *mp, ULONG ulPartLevel, BOOL fLTComparison,
		BOOL fGTComparison, BOOL fEQComparison, BOOL fDefaultPart);


	// check if the DXL Node is a scalar const TRUE
	static BOOL FScalarConstTrue(CMDAccessor *md_accessor,
								 gpos::pointer<CDXLNode *> dxlnode);

	// check if the DXL Node is a scalar const false
	static BOOL FScalarConstFalse(CMDAccessor *md_accessor,
								  gpos::pointer<CDXLNode *> dxlnode);

	// check whether a project list has the same columns in the given array
	// and in the same order
	static BOOL FProjectListMatch(gpos::pointer<CDXLNode *> pdxlnPrL,
								  gpos::pointer<CColRefArray *> colref_array);

	// create a project list by creating references to the columns of the given
	// project list of the child node
	static gpos::owner<CDXLNode *> PdxlnProjListFromChildProjList(
		CMemoryPool *mp, CColumnFactory *col_factory,
		gpos::pointer<ColRefToDXLNodeMap *> phmcrdxln,
		gpos::pointer<const CDXLNode *> pdxlnProjListChild);

	// construct the project list of a partition selector
	static gpos::owner<CDXLNode *> PdxlnPrLPartitionSelector(
		CMemoryPool *mp, CMDAccessor *md_accessor, CColumnFactory *col_factory,
		gpos::pointer<ColRefToDXLNodeMap *> phmcrdxln, BOOL fUseChildProjList,
		gpos::pointer<CDXLNode *> pdxlnPrLChild, CColRef *pcrOid,
		ULONG ulPartLevels, BOOL fGeneratePartOid);

	// create a DXL project elem node from as a scalar identifier for the
	// child project element node
	static gpos::owner<CDXLNode *> PdxlnProjElem(
		CMemoryPool *mp, CColumnFactory *col_factory,
		gpos::pointer<ColRefToDXLNodeMap *> phmcrdxln,
		gpos::pointer<const CDXLNode *> pdxlnProjElemChild);

	// create a scalar identifier node for the given column reference
	static gpos::owner<CDXLNode *> PdxlnIdent(
		CMemoryPool *mp, gpos::pointer<ColRefToDXLNodeMap *> phmcrdxlnSubplans,
		gpos::pointer<ColRefToDXLNodeMap *> phmcrdxlnIndexLookup,
		gpos::pointer<ColRefToUlongMap *> phmcrulPartColId,
		const CColRef *colref);

	// replace subplan entry in the given map with a dxl column reference
	static void ReplaceSubplan(
		CMemoryPool *mp, gpos::pointer<ColRefToDXLNodeMap *> phmcrdxlnSubplans,
		const CColRef *colref, gpos::pointer<CDXLScalarProjElem *> pdxlopPrEl);

	// create a project elem from a given col ref
	static gpos::owner<CDXLNode *> PdxlnProjElem(
		CMemoryPool *mp, gpos::pointer<ColRefToDXLNodeMap *> phmcrdxlnSubplans,
		const CColRef *colref);

	// construct an array of NULL datums for a given array of columns
	static gpos::owner<IDatumArray *> PdrgpdatumNulls(
		CMemoryPool *mp, gpos::pointer<CColRefArray *> colref_array);

	// map an array of columns to a new array of columns
	static gpos::owner<CColRefArray *> PdrgpcrMapColumns(
		CMemoryPool *mp, gpos::pointer<CColRefArray *> pdrgpcrInput,
		gpos::pointer<ColRefToUlongMap *> phmcrul,
		gpos::pointer<CColRefArray *> pdrgpcrMapDest);

	// combine two boolean expressions using the given boolean operator
	static gpos::owner<CDXLNode *> PdxlnCombineBoolean(
		CMemoryPool *mp, gpos::owner<CDXLNode *> first_child_dxlnode,
		gpos::owner<CDXLNode *> second_child_dxlnode,
		EdxlBoolExprType boolexptype);

	// create a DXL result node
	static gpos::owner<CDXLNode *> PdxlnResult(
		CMemoryPool *mp, gpos::owner<CDXLPhysicalProperties *> dxl_properties,
		gpos::owner<CDXLNode *> pdxlnPrL,
		gpos::owner<CDXLNode *> filter_dxlnode,
		gpos::owner<CDXLNode *> one_time_filter,
		gpos::owner<CDXLNode *> child_dxlnode);

	// create a DXL ValuesScan node
	static gpos::owner<CDXLNode *> PdxlnValuesScan(
		CMemoryPool *mp, gpos::owner<CDXLPhysicalProperties *> dxl_properties,
		CDXLNode *pdxlnPrL, gpos::pointer<IDatum2dArray *> pdrgpdrgdatum);

	// build hashmap based on a column array, where the key is the column
	// and the value is the index of that column in the array
	static gpos::owner<ColRefToUlongMap *> PhmcrulColIndex(
		CMemoryPool *mp, gpos::pointer<CColRefArray *> colref_array);

	// set statistics of the operator
	static void SetStats(CMemoryPool *mp, CMDAccessor *md_accessor,
						 gpos::pointer<CDXLNode *> dxlnode,
						 gpos::pointer<const IStatistics *> stats, BOOL fRoot);

	// set direct dispatch info of the operator
	static void SetDirectDispatchInfo(
		CMemoryPool *mp, CMDAccessor *md_accessor,
		gpos::pointer<CDXLNode *> dxlnode, gpos::pointer<CExpression *> pexpr,
		gpos::pointer<CDistributionSpecArray *> pdrgpdsBaseTables);

	// is the aggregate a local hash aggregate that is safe to stream
	static BOOL FLocalHashAggStreamSafe(gpos::pointer<CExpression *> pexprAgg);

	// if operator is a scalar cast or func allowed for Partition selection, extract type and function
	static void ExtractCastFuncMdids(gpos::pointer<COperator *> pop,
									 gpos::pointer<IMDId *> *ppmdidType,
									 gpos::pointer<IMDId *> *ppmdidCastFunc);

	// produce DXL representation of a datum
	static gpos::owner<CDXLDatum *>
	GetDatumVal(CMemoryPool *mp, CMDAccessor *md_accessor,
				gpos::pointer<IDatum *> datum)
	{
		gpos::pointer<IMDId *> mdid = datum->MDId();
		return md_accessor->RetrieveType(mdid)->GetDatumVal(mp, datum);
	}

	// return a copy the dxl node's physical properties
	static gpos::owner<CDXLPhysicalProperties *> PdxlpropCopy(
		CMemoryPool *mp, gpos::pointer<CDXLNode *> dxlnode);

	// check if given dxl operator exists in the given list
	static BOOL FDXLOpExists(gpos::pointer<const CDXLOperator *> pop,
							 const gpdxl::Edxlopid *peopid, ULONG ulOps);

	// check if given dxl node has any operator in the given list
	static BOOL FHasDXLOp(gpos::pointer<const CDXLNode *> dxlnode,
						  const gpdxl::Edxlopid *peopid, ULONG ulOps);

	// check if the project lists contains subplans with broadcast motion
	static BOOL FProjListContainsSubplanWithBroadCast(
		gpos::pointer<CDXLNode *> pdxlnPrLNew);

	// check if the dxl node imposes a motion hazard
	static BOOL FMotionHazard(CMemoryPool *mp,
							  gpos::pointer<CDXLNode *> dxlnode,
							  const gpdxl::Edxlopid *peopid, ULONG ulOps,
							  gpos::pointer<CBitSet *> pbsPrjCols);

	// check if the dxl operator does not impose a motion hazard
	static BOOL FMotionHazardSafeOp(gpos::pointer<CDXLNode *> dxlnode);

	// extract the column ids of the ident from project list
	static void ExtractIdentColIds(gpos::pointer<CDXLNode *> dxlnode,
								   CBitSet *pbs);

	// is this Filter node direct dispatchable?
	static BOOL FDirectDispatchableFilter(
		gpos::pointer<CExpression *> pexprFilter);
};
}  // namespace gpopt

#endif	// !GPOPT_CTranslatorExprToDXLUtils_H

// EOF
