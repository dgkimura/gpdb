//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2013 VMware, Inc. or its affiliates.
//
//	@filename:
//		CLogicalForeignGet.h
//
//	@doc:
//		Logical foreign get operator
//---------------------------------------------------------------------------
#ifndef GPOPT_CLogicalForeignGet_H
#define GPOPT_CLogicalForeignGet_H

#include "gpos/base.h"

#include "gpopt/base/CColRefSet.h"
#include "gpopt/operators/CLogicalGet.h"

namespace gpopt
{
// fwd declarations
class CTableDescriptor;
class CName;

//---------------------------------------------------------------------------
//	@class:
//		CLogicalForeignGet
//
//	@doc:
//		Logical foreign get operator
//
//---------------------------------------------------------------------------
class CLogicalForeignGet : public CLogicalGet
{
private:
public:
	CLogicalForeignGet(const CLogicalForeignGet &) = delete;

	// ctors
	explicit CLogicalForeignGet(CMemoryPool *mp);

	CLogicalForeignGet(CMemoryPool *mp, const CName *pnameAlias,
					   gpos::Ref<CTableDescriptor> ptabdesc);

	CLogicalForeignGet(CMemoryPool *mp, const CName *pnameAlias,
					   gpos::Ref<CTableDescriptor> ptabdesc,
					   gpos::Ref<CColRefArray> pdrgpcrOutput);

	// ident accessors
	EOperatorId
	Eopid() const override
	{
		return EopLogicalForeignGet;
	}

	// return a string for operator name
	const CHAR *
	SzId() const override
	{
		return "CLogicalForeignGet";
	}

	// match function
	BOOL Matches(gpos::Ref<COperator> pop) const override;

	// return a copy of the operator with remapped columns
	gpos::Ref<COperator> PopCopyWithRemappedColumns(
		CMemoryPool *mp, gpos::Ref<UlongToColRefMap> colref_mapping,
		BOOL must_exist) override;

	//-------------------------------------------------------------------------------------
	// Required Relational Properties
	//-------------------------------------------------------------------------------------

	// compute required stat columns of the n-th child
	gpos::Ref<CColRefSet>
	PcrsStat(CMemoryPool *,			 // mp,
			 CExpressionHandle &,	 // exprhdl
			 gpos::Ref<CColRefSet>,	 // pcrsInput
			 ULONG					 // child_index
	) const override
	{
		GPOS_ASSERT(!"CLogicalForeignGet has no children");
		return nullptr;
	}

	//-------------------------------------------------------------------------------------
	// Transformations
	//-------------------------------------------------------------------------------------

	// candidate set of xforms
	gpos::Ref<CXformSet> PxfsCandidates(CMemoryPool *mp) const override;

	//-------------------------------------------------------------------------------------
	//-------------------------------------------------------------------------------------
	//-------------------------------------------------------------------------------------

	// conversion function
	static gpos::Ref<CLogicalForeignGet>
	PopConvert(gpos::Ref<COperator> pop)
	{
		GPOS_ASSERT(nullptr != pop);
		GPOS_ASSERT(EopLogicalForeignGet == pop->Eopid());

		return gpos::dyn_cast<CLogicalForeignGet>(pop);
	}

};	// class CLogicalForeignGet
}  // namespace gpopt

#endif	// !GPOPT_CLogicalForeignGet_H

// EOF
