//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CPartConstraint.h
//
//	@doc:
//		Part constraints for partitioned tables
//---------------------------------------------------------------------------
#ifndef GPOPT_CPartConstraint_H
#define GPOPT_CPartConstraint_H

#include "gpos/base.h"
#include "gpos/common/CHashMap.h"
#include "gpos/common/CHashMapIter.h"
#include "gpos/common/owner.h"

#include "gpopt/base/CColRef.h"
#include "gpopt/base/CConstraint.h"

namespace gpopt
{
using namespace gpos;

// fwd decl
class CColRef;
class CPartConstraint;

// hash maps of part constraints indexed by part index id
using UlongToPartConstraintMap =
	CHashMap<ULONG, CPartConstraint, gpos::HashValue<ULONG>,
			 gpos::Equals<ULONG>, CleanupDelete<ULONG>,
			 CleanupRelease<CPartConstraint>>;

// map iterator
using UlongToPartConstraintMapIter =
	CHashMapIter<ULONG, CPartConstraint, gpos::HashValue<ULONG>,
				 gpos::Equals<ULONG>, CleanupDelete<ULONG>,
				 CleanupRelease<CPartConstraint>>;

//---------------------------------------------------------------------------
//	@class:
//		CPartConstraint
//
//	@doc:
//		metadata abstraction for tables
//
//---------------------------------------------------------------------------
class CPartConstraint : public CRefCount, public DbgPrintMixin<CPartConstraint>
{
private:
	// constraints for different levels
	gpos::owner<UlongToConstraintMap *> m_phmulcnstr;

	// levels at which the default partitions are included
	gpos::owner<CBitSet *> m_pbsDefaultParts;

	// number of levels;
	ULONG m_num_of_part_levels;

	// is constraint unbounded
	BOOL m_is_unbounded;

	// is a dummy (not to be used) constraint
	BOOL m_fUninterpreted;

	// partition keys
	gpos::owner<CColRef2dArray *> m_pdrgpdrgpcr;

	// combined constraint
	gpos::owner<CConstraint *> m_pcnstrCombined;

#ifdef GPOS_DEBUG
	// are all default partitions on all levels included
	BOOL FAllDefaultPartsIncluded() const;
#endif	//GPOS_DEBUG

	// does the current constraint overlap with given one at the given level
	BOOL FOverlapLevel(CMemoryPool *mp,
					   gpos::pointer<const CPartConstraint *> ppartcnstr,
					   ULONG ulLevel) const;

	// check whether or not the current part constraint can be negated. A part
	// constraint can be negated only if it has constraints on the first level
	// since negation destroys the independence between the levels
	BOOL FCanNegate() const;

	// construct the combined constraint
	CConstraint *PcnstrBuildCombined(CMemoryPool *mp);

	// return the remaining part of the first constraint that is not covered by
	// the second constraint
	static gpos::owner<CConstraint *> PcnstrRemaining(CMemoryPool *mp,
													  CConstraint *pcnstrFst,
													  CConstraint *pcnstrSnd);

	// check if two constaint maps have the same constraints
	static BOOL FEqualConstrMaps(
		gpos::pointer<UlongToConstraintMap *> phmulcnstrFst,
		gpos::pointer<UlongToConstraintMap *> phmulcnstrSnd, ULONG ulLevels);

	// check if it is possible to produce a disjunction of the two given part
	// constraints. This is possible if the first ulLevels-1 have the same
	// constraints and default flags for both part constraints
	static BOOL FDisjunctionPossible(
		gpos::pointer<CPartConstraint *> ppartcnstrFst,
		gpos::pointer<CPartConstraint *> ppartcnstrSnd);

public:
	CPartConstraint(const CPartConstraint &) = delete;

	// ctors
	CPartConstraint(CMemoryPool *mp, UlongToConstraintMap *phmulcnstr,
					CBitSet *pbsDefaultParts, BOOL is_unbounded,
					CColRef2dArray *pdrgpdrgpcr);
	CPartConstraint(CMemoryPool *mp, CConstraint *pcnstr,
					BOOL fDefaultPartition, BOOL is_unbounded);

	CPartConstraint(BOOL fUninterpreted);

	// dtor
	~CPartConstraint() override;

	// constraint at given level
	gpos::pointer<CConstraint *> Pcnstr(ULONG ulLevel) const;

	// combined constraint
	gpos::pointer<CConstraint *>
	PcnstrCombined() const
	{
		return m_pcnstrCombined;
	}

	// is default partition included on the given level
	BOOL
	IsDefaultPartition(ULONG ulLevel) const
	{
		return m_pbsDefaultParts->Get(ulLevel);
	}

	// partition keys
	gpos::pointer<CColRef2dArray *>
	Pdrgpdrgpcr() const
	{
		return m_pdrgpdrgpcr;
	}

	// is constraint unbounded
	BOOL IsConstraintUnbounded() const;

	// is constraint uninterpreted
	BOOL
	FUninterpreted() const
	{
		return m_fUninterpreted;
	}

	// are constraints equivalent
	BOOL FEquivalent(gpos::pointer<const CPartConstraint *> ppartcnstr) const;

	// does constraint overlap with given constraint
	BOOL FOverlap(CMemoryPool *mp,
				  gpos::pointer<const CPartConstraint *> ppartcnstr) const;

	// does constraint subsume given one
	BOOL FSubsume(gpos::pointer<const CPartConstraint *> ppartcnstr) const;

	// return what remains of the current part constraint after taking out
	// the given part constraint. Returns NULL is the difference cannot be
	// performed
	gpos::owner<CPartConstraint *> PpartcnstrRemaining(
		CMemoryPool *mp, CPartConstraint *ppartcnstr);

	// return a copy of the part constraint with remapped columns
	gpos::owner<CPartConstraint *> PpartcnstrCopyWithRemappedColumns(
		CMemoryPool *mp, UlongToColRefMap *colref_mapping, BOOL must_exist);

	// print
	IOstream &OsPrint(IOstream &os) const;

	// construct a disjunction of the two constraints
	static gpos::owner<CPartConstraint *> PpartcnstrDisjunction(
		CMemoryPool *mp, gpos::pointer<CPartConstraint *> ppartcnstrFst,
		gpos::pointer<CPartConstraint *> ppartcnstrSnd);

	// combine the two given part constraint maps and return the result
	static gpos::owner<UlongToPartConstraintMap *> PpartcnstrmapCombine(
		CMemoryPool *mp,
		gpos::pointer<UlongToPartConstraintMap *> ppartcnstrmapFst,
		gpos::pointer<UlongToPartConstraintMap *> ppartcnstrmapSnd);

	// copy the part constraints from the source map into the destination map
	static void CopyPartConstraints(
		CMemoryPool *mp, UlongToPartConstraintMap *ppartcnstrmapDest,
		UlongToPartConstraintMap *ppartcnstrmapSource);

};	// class CPartConstraint

}  // namespace gpopt

#endif	// !GPOPT_CPartConstraint_H

// EOF
