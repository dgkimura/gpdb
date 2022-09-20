//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2014 VMware, Inc. or its affiliates.
//
//	@filename:
//		CConstExprEvaluatorDefault.cpp
//
//	@doc:
//		Dummy implementation of the constant expression evaluator
//
//	@owner:
//
//
//	@test:
//
//---------------------------------------------------------------------------

#include "gpopt/eval/CConstExprEvaluatorDefault.h"

#include "gpos/common/owner.h"

#include "gpopt/operators/CExpression.h"

using namespace gpopt;


//---------------------------------------------------------------------------
//	@function:
//		CConstExprEvaluatorDefault::~CConstExprEvaluatorDefault
//
//	@doc:
//		Dtor
//
//---------------------------------------------------------------------------
CConstExprEvaluatorDefault::~CConstExprEvaluatorDefault() = default;

//---------------------------------------------------------------------------
//	@function:
//		CConstExprEvaluatorDefault::PexprEval
//
//	@doc:
//		Returns the given expression after having increased its ref count
//
//---------------------------------------------------------------------------
gpos::owner<CExpression *>
CConstExprEvaluatorDefault::PexprEval(gpos::pointer<CExpression *> pexpr)
{
	pexpr->AddRef();
	return pexpr;
}

//---------------------------------------------------------------------------
//	@function:
//		CConstExprEvaluatorDefault::FCanEvalFunctions
//
//	@doc:
//		Returns false, since this evaluator cannot call any functions
//
//---------------------------------------------------------------------------
BOOL
CConstExprEvaluatorDefault::FCanEvalExpressions()
{
	return false;
}

// EOF
