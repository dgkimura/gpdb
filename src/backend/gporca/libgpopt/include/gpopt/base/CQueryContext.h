//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2011 EMC Corp.
//
//	@filename:
//		CQueryContext.h
//
//	@doc:
//		A container for query-specific input objects to the optimizer
//---------------------------------------------------------------------------
#ifndef GPOPT_CQueryContext_H
#define GPOPT_CQueryContext_H

#include "gpos/base.h"
#include "gpos/common/DbgPrintMixin.h"
#include "gpos/common/owner.h"

#include "gpopt/base/CReqdPropPlan.h"
#include "gpopt/base/CReqdPropRelational.h"
#include "gpopt/operators/CExpressionPreprocessor.h"
#include "gpopt/search/CGroupExpression.h"


namespace gpopt
{
using namespace gpos;

//---------------------------------------------------------------------------
//	@class:
//		CQueryContext
//
//	@doc:
//		Query specific information that optimizer receives as input
//		representing the requirements that need to be satisfied by the final
//		plan. This includes:
//		- Input logical expression
//		- Required columns
//		- Required plan (physical) properties at the top level of the query.
//		  This will include sort order, rewindability etc requested by the entire
//		  query.
//
//		The function CQueryContext::PqcGenerate() is the main routine that
//		generates a query context object for a given logical expression and
//		required output columns. See there for more details of how
//		CQueryContext is constructed.
//
//		NB: One instance of CQueryContext is created per query. It is then used
//		to initialize the CEngine.
//
//
//---------------------------------------------------------------------------
class CQueryContext : public DbgPrintMixin<CQueryContext>
{
private:
	// required plan properties in optimizer's produced plan
	gpos::owner<CReqdPropPlan *> m_prpp;

	// required array of output columns
	gpos::owner<CColRefArray *> m_pdrgpcr;

	// required system columns, collected from of output columns
	gpos::owner<CColRefArray *> m_pdrgpcrSystemCols;

	// array of output column names
	gpos::owner<CMDNameArray *> m_pdrgpmdname;

	// logical expression tree to be optimized
	gpos::owner<CExpression *> m_pexpr;

	// should statistics derivation take place
	BOOL m_fDeriveStats;

	// collect system columns from output columns
	void SetSystemCols(CMemoryPool *mp);

	// return top level operator in the given expression
	static COperator *PopTop(gpos::pointer<CExpression *> pexpr);

public:
	CQueryContext(const CQueryContext &) = delete;

	// ctor
	CQueryContext(CMemoryPool *mp, gpos::pointer<CExpression *> pexpr,
				  gpos::owner<CReqdPropPlan *> prpp,
				  gpos::owner<CColRefArray *> colref_array,
				  gpos::owner<CMDNameArray *> pdrgpmdname, BOOL fDeriveStats);

	// dtor
	virtual ~CQueryContext();

	BOOL
	FDeriveStats() const
	{
		return m_fDeriveStats;
	}

	// expression accessor
	gpos::pointer<CExpression *>
	Pexpr() const
	{
		return m_pexpr;
	}

	// required plan properties accessor
	gpos::pointer<CReqdPropPlan *>
	Prpp() const
	{
		return m_prpp;
	}

	// return the array of output column references
	gpos::pointer<CColRefArray *>
	PdrgPcr() const
	{
		return m_pdrgpcr;
	}

	// system columns
	gpos::pointer<CColRefArray *>
	PdrgpcrSystemCols() const
	{
		return m_pdrgpcrSystemCols;
	}

	// return the array of output column names
	gpos::pointer<CMDNameArray *>
	Pdrgpmdname() const
	{
		return m_pdrgpmdname;
	}

	// generate the query context for the given expression and array of output column ref ids
	static CQueryContext *PqcGenerate(
		CMemoryPool *mp,  // memory pool
		gpos::pointer<CExpression *>
			pexpr,	// expression representing the query
		gpos::pointer<ULongPtrArray *>
			pdrgpulQueryOutputColRefId,	 // array of output column reference id
		CMDNameArray *pdrgpmdname,		 // array of output column names
		BOOL fDeriveStats);

#ifdef GPOS_DEBUG
	// debug print
	IOstream &OsPrint(IOstream &) const;
#endif	// GPOS_DEBUG

	// walk the expression and add the mapping between computed column
	// and their corresponding used column(s)
	static void MapComputedToUsedCols(CColumnFactory *col_factory,
									  gpos::pointer<CExpression *> pexpr);

};	// class CQueryContext
}  // namespace gpopt


#endif	// !GPOPT_CQueryContext_H

// EOF
