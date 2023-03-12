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

#include "gpopt/operators/CExpression.h"
#include "gpopt/search/CGroupExpression.h"

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
gpos::Ref<CExpression>
CConstExprEvaluatorDefault::PexprEval(gpos::Ref<CExpression> pexpr)
{
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
