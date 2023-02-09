//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2009 Greenplum, Inc.
//
//	@filename:
//		CXformFactory.h
//
//	@doc:
//		Management of global xform set
//---------------------------------------------------------------------------
#ifndef GPOPT_CXformFactory_H
#define GPOPT_CXformFactory_H

#include "gpos/base.h"
#include "gpos/common/owner.h"

#include "gpopt/xforms/CXform.h"

namespace gpopt
{
using namespace gpos;

//---------------------------------------------------------------------------
//	@class:
//		CXformFactory
//
//	@doc:
//		Factory class to manage xforms
//
//---------------------------------------------------------------------------
class CXformFactory
{
private:
	// definition of hash map to maintain mappings
	using XformNameToXformMap =
		gpos::UnorderedMap<const CHAR *, gpos::Ref<CXform>,
						   gpos::PtrHash<CHAR, gpos::HashValue<CHAR>>,
						   gpos::PtrEqual<CHAR, CXform::FEqualIds>>;

	// memory pool
	CMemoryPool *m_mp;

	// range of all xforms
	gpos::Ref<CXform> m_rgpxf[CXform::ExfSentinel];

	// name -> xform map
	gpos::Ref<XformNameToXformMap> m_phmszxform;

	// bitset of exploration xforms
	gpos::Ref<CXformSet> m_pxfsExploration;

	// bitset of implementation xforms
	gpos::Ref<CXformSet> m_pxfsImplementation;

	// ensure that xforms are inserted in order
	ULONG m_lastAddedOrSkippedXformId;

	// global instance
	static CXformFactory *m_pxff;

	// private ctor
	explicit CXformFactory(CMemoryPool *mp);

	// actual adding of xform
	void Add(gpos::Ref<CXform> pxform);

	// skip unused xforms that have been removed, preserving
	// xform ids of the remaining ones
	void
	SkipUnused(ULONG numXformsToSkip)
	{
		m_lastAddedOrSkippedXformId += numXformsToSkip;
	}

public:
	CXformFactory(const CXformFactory &) = delete;

	// dtor
	~CXformFactory();

	// create all xforms
	void Instantiate();

	// accessor by xform id
	gpos::Ref<CXform> Pxf(CXform::EXformId exfid) const;

	// accessor by xform name
	gpos::Ref<CXform> Pxf(const CHAR *szXformName) const;

	// accessor of exploration xforms
	CXformSet *
	PxfsExploration() const
	{
		return m_pxfsExploration.get();
	}

	// accessor of implementation xforms
	CXformSet *
	PxfsImplementation() const
	{
		return m_pxfsImplementation.get();
	}

	// is this xform id still used?
	BOOL IsXformIdUsed(CXform::EXformId exfid);

	// global accessor
	static CXformFactory *
	Pxff()
	{
		return m_pxff;
	}

	// initialize global factory instance
	static GPOS_RESULT Init();

	// destroy global factory instance
	static void Shutdown();

};	// class CXformFactory

}  // namespace gpopt


#endif	// !GPOPT_CXformFactory_H

// EOF
