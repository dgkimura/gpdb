//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2014 Pivotal, Inc.
//
//	@filename:
//		CScalarValuesList.h
//
//	@doc:
//		Class for scalar arrayref index list
//---------------------------------------------------------------------------
#ifndef GPOPT_CScalarValuesList_H
#define GPOPT_CScalarValuesList_H

#include "gpos/base.h"

#include "gpopt/operators/CScalar.h"
#include "naucrates/md/IMDId.h"

namespace gpopt
{
using namespace gpos;
using namespace gpmd;

//---------------------------------------------------------------------------
//	@class:
//		CScalarValuesList
//
//	@doc:
//		Scalar arrayref index list
//
//---------------------------------------------------------------------------
class CScalarValuesList : public CScalar
{
private:
	// private copy ctor
	CScalarValuesList(const CScalarValuesList &);

public:
	// ctor
	CScalarValuesList(CMemoryPool *mp);

	// ident accessors
	virtual EOperatorId
	Eopid() const
	{
		return EopScalarValuesList;
	}

	// operator name
	virtual const CHAR *
	SzId() const
	{
		return "CScalarValuesList";
	}

	// match function
	virtual BOOL Matches(COperator *pop) const;

	// sensitivity to order of inputs
	virtual BOOL
	FInputOrderSensitive() const
	{
		return true;
	}

	// return a copy of the operator with remapped columns
	virtual COperator *
	PopCopyWithRemappedColumns(CMemoryPool *,		//mp,
							   UlongToColRefMap *,	//colref_mapping,
							   BOOL					//must_exist
	)
	{
		return PopCopyDefault();
	}

	// type of expression's result
	virtual IMDId *
	MdidType() const
	{
		return NULL;
	}

	// conversion function
	static CScalarValuesList *
	PopConvert(COperator *pop)
	{
		GPOS_ASSERT(NULL != pop);
		GPOS_ASSERT(EopScalarArrayRefIndexList == pop->Eopid());

		return dynamic_cast<CScalarValuesList *>(pop);
	}

};	// class CScalarValuesList
}  // namespace gpopt

#endif	// !GPOPT_CScalarValuesList_H

// EOF
