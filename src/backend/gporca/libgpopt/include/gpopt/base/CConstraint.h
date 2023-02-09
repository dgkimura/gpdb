//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CConstraint.h
//
//	@doc:
//		Base class for representing constraints
//---------------------------------------------------------------------------
#ifndef GPOPT_CConstraint_H
#define GPOPT_CConstraint_H

#include "gpos/base.h"
#include "gpos/common/CDynamicPtrArray.h"
#include "gpos/common/CHashMap.h"
#include "gpos/common/CRefCount.h"
#include "gpos/common/owner.h"
#include "gpos/types.h"

#include "gpopt/base/CColRef.h"
#include "gpopt/base/CColRefSet.h"
#include "gpopt/operators/CExpression_Fwd.h"

namespace gpopt
{
using namespace gpos;

// fwd declaration
class CConstraint;

// constraint array
using CConstraintArray = gpos::Vector<gpos::Ref<CConstraint>>;

// hash map mapping CColRef -> CConstraintArray
using ColRefToConstraintArrayMap =
	gpos::UnorderedMap<const CColRef *, gpos::Ref<CConstraintArray>,
					   gpos::PtrHash<CColRef, CColRef::HashValue>,
					   gpos::PtrEqual<CColRef, CColRef::Equals>>;

// mapping CConstraint -> BOOL to cache previous containment queries,
// we use pointer equality here for fast map lookup -- since we do shallow comparison, we do not take ownership
// of pointer values
using ConstraintContainmentMap = gpos::UnorderedMap<
	const gpos::Ref<CConstraint>, BOOL *,
	gpos::PtrHash<CConstraint, gpos::HashPtr<CConstraint>>,
	gpos::PtrEqual<CConstraint, gpos::EqualPtr<CConstraint>>>;

// hash map mapping ULONG -> CConstraint
using UlongToConstraintMap =
	CHashMap<ULONG, CConstraint, gpos::HashValue<ULONG>, gpos::Equals<ULONG>,
			 CleanupDelete<ULONG>, CleanupRelease<CConstraint>>;

//---------------------------------------------------------------------------
//	@class:
//		CConstraint
//
//	@doc:
//		Base class for representing constraints
//
//---------------------------------------------------------------------------
class CConstraint : public CRefCount, public DbgPrintMixin<CConstraint>
{
public:
	enum EConstraintType
	{
		EctInterval,	 // a single interval on a single columns
		EctConjunction,	 // a set of ANDed constraints
		EctDisjunction,	 // a set of ORed constraints
		EctNegation		 // a negated constraint
	};

private:
	// containment map
	gpos::Ref<ConstraintContainmentMap> m_phmcontain;

	// constant true
	static BOOL m_fTrue;

	// constant false
	static BOOL m_fFalse;

	// return address of static BOOL constant based on passed BOOL value
	static BOOL *
	PfVal(BOOL value)
	{
		if (value)
		{
			return &m_fTrue;
		}

		return &m_fFalse;
	}

	// add column as a new equivalence class, if it is not already in one of the
	// existing equivalence classes
	static void AddColumnToEquivClasses(CMemoryPool *mp, const CColRef *colref,
										CColRefSetArray *pdrgpcrs);

	// create constraint from scalar comparison
	static gpos::Ref<CConstraint> PcnstrFromScalarCmp(
		CMemoryPool *mp, gpos::Ref<CExpression> pexpr,
		gpos::Ref<CColRefSetArray> *ppdrgpcrs, BOOL infer_nulls_as = false);

	// create constraint from scalar boolean expression
	static gpos::Ref<CConstraint> PcnstrFromScalarBoolOp(
		CMemoryPool *mp, gpos::Ref<CExpression> pexpr,
		gpos::Ref<CColRefSetArray> *ppdrgpcrs, BOOL infer_nulls_as = false);

	// create conjunction/disjunction from array of constraints
	static gpos::Ref<CConstraint> PcnstrConjDisj(
		CMemoryPool *mp, gpos::Ref<CConstraintArray> pdrgpcnstr, BOOL fConj);

protected:
	// memory pool -- used for local computations
	CMemoryPool *m_mp;

	// columns used in this constraint
	gpos::Ref<CColRefSet> m_pcrsUsed;

	// equivalent scalar expression
	gpos::Ref<CExpression> m_pexprScalar;

	// print
	IOstream &PrintConjunctionDisjunction(IOstream &os,
										  CConstraintArray *pdrgpcnstr) const;

	// construct a conjunction or disjunction scalar expression from an
	// array of constraints
	static gpos::Ref<CExpression> PexprScalarConjDisj(
		CMemoryPool *mp, CConstraintArray *pdrgpcnstr, BOOL fConj);

	// flatten an array of constraints to be used as constraint children
	static gpos::Ref<CConstraintArray> PdrgpcnstrFlatten(
		CMemoryPool *mp, gpos::Ref<CConstraintArray> pdrgpcnstr,
		EConstraintType ect);

	// combine any two or more constraints that reference only one particular column
	static gpos::Ref<CConstraintArray> PdrgpcnstrDeduplicate(
		CMemoryPool *mp, gpos::Ref<CConstraintArray> pdrgpcnstr,
		EConstraintType ect);

	// mapping between columns and arrays of constraints
	static gpos::Ref<ColRefToConstraintArrayMap> Phmcolconstr(
		CMemoryPool *mp, CColRefSet *pcrs, CConstraintArray *pdrgpcnstr);

	// return a copy of the conjunction/disjunction constraint for a different column
	static gpos::Ref<CConstraint> PcnstrConjDisjRemapForColumn(
		CMemoryPool *mp, CColRef *colref, CConstraintArray *pdrgpcnstr,
		BOOL fConj);

	// create constraint from scalar array comparison expression originally generated for
	// "scalar op ANY/ALL (array)" construct
	static gpos::Ref<CConstraint> PcnstrFromScalarArrayCmp(
		CMemoryPool *mp, gpos::Ref<CExpression> pexpr, CColRef *colref,
		BOOL infer_nulls_as = false);

	static gpos::Ref<CColRefSet> PcrsFromConstraints(
		CMemoryPool *mp, CConstraintArray *pdrgpcnstr);

public:
	CConstraint(const CConstraint &) = delete;

	// ctor
	explicit CConstraint(CMemoryPool *mp, gpos::Ref<CColRefSet> pcrsUsed);

	// dtor
	~CConstraint() override;

	// constraint type accessor
	virtual EConstraintType Ect() const = 0;

	// is this constraint a contradiction
	virtual BOOL FContradiction() const = 0;

	// is this constraint unbounded
	virtual BOOL
	IsConstraintUnbounded() const
	{
		return false;
	}

	// does the current constraint contain the given one
	virtual BOOL Contains(gpos::Ref<CConstraint> pcnstr);

	// equality function
	virtual BOOL Equals(gpos::Ref<CConstraint> pcnstr);

	// columns in this constraint
	virtual CColRefSet *
	PcrsUsed() const
	{
		return m_pcrsUsed.get();
	}

	// scalar expression
	virtual gpos::Ref<CExpression> PexprScalar(CMemoryPool *mp) = 0;

	// check if there is a constraint on the given column
	virtual BOOL FConstraint(const CColRef *colref) const = 0;

	virtual BOOL
	FConstraintOnSegmentId() const
	{
		return false;
	}

	// return a copy of the constraint with remapped columns
	virtual gpos::Ref<CConstraint> PcnstrCopyWithRemappedColumns(
		CMemoryPool *mp, UlongToColRefMap *colref_mapping, BOOL must_exist) = 0;

	// return constraint on a given column
	virtual gpos::Ref<CConstraint>
	Pcnstr(CMemoryPool *,	//mp,
		   const CColRef *	//colref
	)
	{
		return nullptr;
	}

	// return constraint on a given set of columns
	virtual gpos::Ref<CConstraint>
	Pcnstr(CMemoryPool *,  //mp,
		   CColRefSet *	   //pcrs
	)
	{
		return nullptr;
	}

	// return a clone of the constraint for a different column
	virtual gpos::Ref<CConstraint> PcnstrRemapForColumn(
		CMemoryPool *mp, CColRef *colref) const = 0;

	// create constraint from scalar expression and pass back any discovered
	// equivalence classes
	static gpos::Ref<CConstraint> PcnstrFromScalarExpr(
		CMemoryPool *mp, gpos::Ref<CExpression> pexpr,
		gpos::Ref<CColRefSetArray> *ppdrgpcrs, BOOL infer_nulls_as = false);

	// create conjunction from array of constraints
	static gpos::Ref<CConstraint> PcnstrConjunction(
		CMemoryPool *mp, gpos::Ref<CConstraintArray> pdrgpcnstr);

	// create disjunction from array of constraints
	static gpos::Ref<CConstraint> PcnstrDisjunction(
		CMemoryPool *mp, gpos::Ref<CConstraintArray> pdrgpcnstr);

	// merge equivalence classes coming from children of a bool op
	static gpos::Ref<CColRefSetArray> PdrgpcrsMergeFromBoolOp(
		CMemoryPool *mp, gpos::Ref<CExpression> pexpr,
		CColRefSetArray *pdrgpcrsFst, CColRefSetArray *pdrgpcrsSnd);

	// subset of the given constraints, which reference the given column
	static gpos::Ref<CConstraintArray> PdrgpcnstrOnColumn(
		CMemoryPool *mp, CConstraintArray *pdrgpcnstr, CColRef *colref,
		BOOL fExclusive);
	virtual gpos::IOstream &OsPrint(gpos::IOstream &os) const = 0;

};	// class CConstraint

// shorthand for printing, pointer.
inline IOstream &
operator<<(IOstream &os, const gpos::Ref<CConstraint> cnstr)
{
	return cnstr->OsPrint(os);
}
// shorthand for printing
inline IOstream &
operator<<(IOstream &os, const CConstraint &cnstr)
{
	return cnstr.OsPrint(os);
}
}  // namespace gpopt

#endif	// !GPOPT_CConstraint_H

// EOF
