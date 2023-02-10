//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2009 - 2012 EMC CORP.
//
//	@filename:
//		CFunctionalDependency.h
//
//	@doc:
//		Functional dependency representation
//---------------------------------------------------------------------------
#ifndef GPOPT_CFunctionalDependency_H
#define GPOPT_CFunctionalDependency_H

#include "gpos/base.h"
#include "gpos/common/CDynamicPtrArray.h"
#include "gpos/common/CRefCount.h"
#include "gpos/common/owner.h"

#include "gpopt/base/CColRefSet.h"


namespace gpopt
{
// fwd declarations
class CFunctionalDependency;

// definition of array of functional dependencies
using CFunctionalDependencyArray =
	gpos::Vector<gpos::Ref<CFunctionalDependency>>;

using namespace gpos;

//---------------------------------------------------------------------------
//	@class:
//		CFunctionalDependency
//
//	@doc:
//		Functional dependency representation
//
//---------------------------------------------------------------------------
class CFunctionalDependency : public CRefCount,
							  public DbgPrintMixin<CFunctionalDependency>
{
private:
	// the left hand side of the FD
	gpos::Ref<CColRefSet> m_pcrsKey;

	// the right hand side of the FD
	gpos::Ref<CColRefSet> m_pcrsDetermined;

public:
	CFunctionalDependency(const CFunctionalDependency &) = delete;

	// ctor
	CFunctionalDependency(gpos::Ref<CColRefSet> pcrsKey,
						  gpos::Ref<CColRefSet> pcrsDetermined);

	// dtor
	~CFunctionalDependency() override;

	// key set accessor
	gpos::Ref<CColRefSet>
	PcrsKey() const
	{
		return m_pcrsKey.get();
	}

	// determined set accessor
	gpos::Ref<CColRefSet>
	PcrsDetermined() const
	{
		return m_pcrsDetermined.get();
	}

	// determine if all FD columns are included in the given column set
	BOOL FIncluded(gpos::Ref<CColRefSet> pcrs) const;

	// hash function
	virtual ULONG HashValue() const;

	// equality function
	BOOL Equals(const gpos::Ref<CFunctionalDependency> pfd) const;

	// do the given arguments form a functional dependency
	BOOL
	FFunctionallyDependent(gpos::Ref<CColRefSet> pcrsKey, CColRef *colref)
	{
		GPOS_ASSERT(nullptr != pcrsKey);
		GPOS_ASSERT(nullptr != colref);

		return m_pcrsKey->Equals(pcrsKey) && m_pcrsDetermined->FMember(colref);
	}

	// print
	IOstream &OsPrint(IOstream &os) const;

	// hash function
	static ULONG HashValue(const CFunctionalDependencyArray *pdrgpfd);

	// equality function
	static BOOL Equals(const CFunctionalDependencyArray *pdrgpfdFst,
					   const CFunctionalDependencyArray *pdrgpfdSnd);

	// create a set of all keys in the passed FD's array
	static gpos::Ref<CColRefSet> PcrsKeys(
		CMemoryPool *mp, const CFunctionalDependencyArray *pdrgpfd);

	// create an array of all keys in the passed FD's array
	static gpos::Ref<CColRefArray> PdrgpcrKeys(
		CMemoryPool *mp, const CFunctionalDependencyArray *pdrgpfd);


};	// class CFunctionalDependency

// shorthand for printing
inline IOstream &
operator<<(IOstream &os, CFunctionalDependency &fd)
{
	return fd.OsPrint(os);
}

}  // namespace gpopt

#endif	// !GPOPT_CFunctionalDependency_H

// EOF
