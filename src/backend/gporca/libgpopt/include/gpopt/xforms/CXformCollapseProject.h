//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2015 VMware, Inc. or its affiliates.
//
//	@filename:
//		CXformCollapseProject.h
//
//	@doc:
//		Transform that collapses two cascaded project nodes
//---------------------------------------------------------------------------
#ifndef GPOPT_CXformCollapseProject_H
#define GPOPT_CXformCollapseProject_H

#include "gpos/base.h"
#include "gpos/common/owner.h"

#include "gpopt/xforms/CXformSubqueryUnnest.h"

namespace gpopt
{
using namespace gpos;

//---------------------------------------------------------------------------
//	@class:
//		CXformCollapseProject
//
//	@doc:
//		Transform that collapses two cascaded project nodes
//
//---------------------------------------------------------------------------
class CXformCollapseProject : public CXformExploration
{
private:
public:
	CXformCollapseProject(const CXformCollapseProject &) = delete;

	// ctor
	explicit CXformCollapseProject(CMemoryPool *mp);

	// dtor
	~CXformCollapseProject() override = default;

	// ident accessors
	EXformId
	Exfid() const override
	{
		return ExfCollapseProject;
	}

	// return a string for xform name
	const CHAR *
	SzId() const override
	{
		return "CXformCollapseProject";
	}

	// compute xform promise for a given expression handle
	EXformPromise Exfp(CExpressionHandle &exprhdl) const override;

	// actual transform
	void Transform(gpos::pointer<CXformContext *>,
				   gpos::pointer<CXformResult *>,
				   gpos::pointer<CExpression *>) const override;

};	// class CXformCollapseProject

}  // namespace gpopt

#endif	// !GPOPT_CXformCollapseProject_H

// EOF
