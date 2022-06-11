//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2009 - 2012 EMC CORP.
//
//	@filename:
//		CKeyCollection.h
//
//	@doc:
//		Encodes key sets for a relation
//---------------------------------------------------------------------------
#ifndef GPOPT_CKeyCollection_H
#define GPOPT_CKeyCollection_H

#include "gpos/base.h"
#include "gpos/common/CDynamicPtrArray.h"
#include "gpos/common/CRefCount.h"
#include "gpos/common/owner.h"

#include "gpopt/base/CColRefSet.h"


namespace gpopt
{
using namespace gpos;

//---------------------------------------------------------------------------
//	@class:
//		CKeyCollection
//
//	@doc:
//		Captures sets of keys for a relation
//
//---------------------------------------------------------------------------
class CKeyCollection : public CRefCount, public DbgPrintMixin<CKeyCollection>
{
private:
	// array of key sets
	gpos::owner<CColRefSetArray *> m_pdrgpcrs;

public:
	CKeyCollection(const CKeyCollection &) = delete;

	// ctors
	explicit CKeyCollection(CMemoryPool *mp);
	CKeyCollection(CMemoryPool *mp, gpos::owner<CColRefSet *> pcrs);
	CKeyCollection(CMemoryPool *mp, gpos::owner<CColRefArray *> colref_array);

	// dtor
	~CKeyCollection() override;

	// add individual set -- takes ownership
	void Add(gpos::owner<CColRefSet *> pcrs);

	// check if set forms a key
	BOOL FKey(gpos::pointer<const CColRefSet *> pcrs,
			  BOOL fExactMatch = true) const;

	// check if an array of columns constitutes a key
	BOOL FKey(CMemoryPool *mp,
			  gpos::pointer<const CColRefArray *> colref_array) const;

	// trim off non-key columns
	gpos::owner<CColRefArray *> PdrgpcrTrim(
		CMemoryPool *mp,
		gpos::pointer<const CColRefArray *> colref_array) const;

	// extract a key
	gpos::owner<CColRefArray *> PdrgpcrKey(CMemoryPool *mp) const;

	// extract a hashable key
	gpos::owner<CColRefArray *> PdrgpcrHashableKey(CMemoryPool *mp) const;

	// extract key at given position
	gpos::owner<CColRefArray *> PdrgpcrKey(CMemoryPool *mp, ULONG ul) const;

	// extract key at given position
	gpos::owner<CColRefSet *> PcrsKey(CMemoryPool *mp, ULONG ul) const;

	// number of keys
	ULONG
	Keys() const
	{
		return m_pdrgpcrs->Size();
	}

	// print
	IOstream &OsPrint(IOstream &os) const;

};	// class CKeyCollection

// shorthand for printing
inline IOstream &
operator<<(IOstream &os, CKeyCollection &kc)
{
	return kc.OsPrint(os);
}

}  // namespace gpopt

#endif	// !GPOPT_CKeyCollection_H

// EOF
