//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CTranslatorDXLToExprUtils.h
//
//	@doc:
//		Class providing helper methods for translating from DXL to Expr
//---------------------------------------------------------------------------
#ifndef GPOPT_CTranslatorDXLToExprUtils_H
#define GPOPT_CTranslatorDXLToExprUtils_H

#include "gpos/base.h"
#include "gpos/common/owner.h"

#include "gpopt/mdcache/CMDAccessor.h"
#include "gpopt/metadata/CTableDescriptor.h"
#include "gpopt/operators/CExpression.h"
#include "gpopt/operators/CScalarBoolOp.h"
#include "gpopt/operators/CScalarConst.h"
#include "naucrates/dxl/operators/CDXLColDescr.h"
#include "naucrates/dxl/operators/CDXLNode.h"
#include "naucrates/dxl/operators/CDXLScalarBoolExpr.h"

namespace gpmd
{
class IMDRelation;
}

namespace gpopt
{
using namespace gpos;
using namespace gpmd;
using namespace gpdxl;

//---------------------------------------------------------------------------
//	@class:
//		CTranslatorDXLToExprUtils
//
//	@doc:
//		Class providing helper methods for translating from DXL to Expr
//
//---------------------------------------------------------------------------
class CTranslatorDXLToExprUtils
{
public:
	// create a cast expression from INT to INT8
	static gpos::owner<CExpression *> PexprConstInt8(CMemoryPool *mp,
													 CMDAccessor *md_accessor,
													 CSystemId sysid,
													 LINT liValue);

	// create a scalar const operator from a DXL scalar const operator
	static gpos::owner<CScalarConst *> PopConst(
		CMemoryPool *mp, CMDAccessor *md_accessor,
		gpos::pointer<const CDXLScalarConstValue *> dxl_op);

	// create a datum from a DXL scalar const operator
	static gpos::owner<IDatum *> GetDatum(
		CMDAccessor *md_accessor,
		gpos::pointer<const CDXLScalarConstValue *> dxl_op);

	// create a datum array from a dxl datum array
	static gpos::owner<IDatumArray *> Pdrgpdatum(
		CMemoryPool *mp, CMDAccessor *md_accessor,
		gpos::pointer<const CDXLDatumArray *> pdrgpdatum);

	// update table descriptor's key sets info from the MD cache object
	static void AddKeySets(CMemoryPool *mp,
						   gpos::pointer<CTableDescriptor *> ptabdesc,
						   gpos::pointer<const IMDRelation *> pmdrel,
						   gpos::pointer<UlongToUlongMap *> phmululColMapping);

	// check if a dxl node is a boolean expression of the given type
	static BOOL FScalarBool(gpos::pointer<const CDXLNode *> dxlnode,
							EdxlBoolExprType edxlboolexprtype);

	// returns the equivalent bool expression type in the optimizer for
	// a given DXL bool expression type
	static CScalarBoolOp::EBoolOperator EBoolOperator(
		EdxlBoolExprType edxlbooltype);

	// construct a dynamic array of col refs corresponding to the given col ids
	static gpos::owner<CColRefArray *> Pdrgpcr(
		CMemoryPool *mp, gpos::pointer<UlongToColRefMap *> colref_mapping,
		gpos::pointer<const ULongPtrArray *> colids);

	// is the given expression is a scalar function that casts
	static BOOL FCastFunc(CMDAccessor *md_accessor,
						  gpos::pointer<const CDXLNode *> dxlnode,
						  gpos::pointer<IMDId *> pmdidInput);
};
}  // namespace gpopt

#endif	// !GPOPT_CTranslatorDXLToExprUtils_H

// EOF
