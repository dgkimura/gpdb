//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CDXLStatsDependency.h
//
//	@doc:
//		Class representing column stats
//---------------------------------------------------------------------------

#ifndef GPMD_CDXLStatsDependency_H
#define GPMD_CDXLStatsDependency_H

#include <vector>

#include "gpos/base.h"
#include "gpos/common/CDouble.h"
#include "gpos/common/CDynamicPtrArray.h"
#include "gpos/string/CWStringDynamic.h"

namespace gpdxl
{
class CXMLSerializer;
}

namespace gpmd
{
using namespace gpos;
using namespace gpdxl;

//---------------------------------------------------------------------------
//	@class:
//		CDXLStatsDependency
//
//	@doc:
//		Class representing dependency stats
//
//---------------------------------------------------------------------------
class CDXLStatsDependency : public CRefCount
{
private:
	// memory pool
	CMemoryPool *m_mp;

	std::vector<ULONG> m_from_attno;

	ULONG m_to_attno;

	CDouble m_degree;

public:
	CDXLStatsDependency(const CDXLStatsDependency &) = delete;

	// ctor
	CDXLStatsDependency(CMemoryPool *mp, std::vector<ULONG> from_attno,
						ULONG to_attno, CDouble degree);

	// dtor
	~CDXLStatsDependency();

	// serialize bucket in DXL format
	void Serialize(gpdxl::CXMLSerializer *) const;

	CDouble
	GetDegree() const
	{
		return m_degree;
	}

	std::vector<ULONG>
	GetFromAttno() const
	{
		return m_from_attno;
	}

	ULONG
	GetToAttno() const
	{
		return m_to_attno;
	}

#ifdef GPOS_DEBUG
	// debug print of the column stats
	void DebugPrint(IOstream &os) const;
#endif
};

// array of dxl column stats
using CDXLStatsDependencyArray =
	CDynamicPtrArray<CDXLStatsDependency, CleanupRelease>;
}  // namespace gpmd



#endif	// !GPMD_CDXLStatsDependency_H

// EOF
