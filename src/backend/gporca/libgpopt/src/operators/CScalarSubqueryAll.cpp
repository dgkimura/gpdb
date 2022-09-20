//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CScalarSubqueryAll.cpp
//
//	@doc:
//		Implementation of scalar subquery ALL operator
//---------------------------------------------------------------------------

#include "gpopt/operators/CScalarSubqueryAll.h"

#include "gpos/base.h"
#include "gpos/common/owner.h"

#include "gpopt/base/CUtils.h"

using namespace gpopt;

//---------------------------------------------------------------------------
//	@function:
//		CScalarSubqueryAll::CScalarSubqueryAll
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CScalarSubqueryAll::CScalarSubqueryAll(CMemoryPool *mp,
									   gpos::Ref<IMDId> scalar_op_mdid,
									   const CWStringConst *pstrScalarOp,
									   const CColRef *colref)
	: CScalarSubqueryQuantified(mp, std::move(scalar_op_mdid), pstrScalarOp,
								colref)
{
}

//---------------------------------------------------------------------------
//	@function:
//		CScalarSubqueryAll::PopCopyWithRemappedColumns
//
//	@doc:
//		Return a copy of the operator with remapped columns
//
//---------------------------------------------------------------------------
gpos::Ref<COperator>
CScalarSubqueryAll::PopCopyWithRemappedColumns(CMemoryPool *mp,
											   UlongToColRefMap *colref_mapping,
											   BOOL must_exist)
{
	CColRef *colref = CUtils::PcrRemap(Pcr(), colref_mapping, must_exist);

	gpos::Ref<IMDId> scalar_op_mdid = MdIdOp();
	;

	CWStringConst *pstrScalarOp =
		GPOS_NEW(mp) CWStringConst(mp, PstrOp()->GetBuffer());

	return GPOS_NEW(mp)
		CScalarSubqueryAll(mp, std::move(scalar_op_mdid), pstrScalarOp, colref);
}

// EOF
