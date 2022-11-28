//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CDXLExtStats.h
//
//	@doc:
//		Class representing column stats
//---------------------------------------------------------------------------

#ifndef GPMD_CDXLExtStats_H
#define GPMD_CDXLExtStats_H

#include "gpos/base.h"
#include "gpos/common/CDouble.h"
#include "gpos/common/CDynamicPtrArray.h"
#include "gpos/string/CWStringDynamic.h"

#include "naucrates/md/CDXLStatsDependency.h"
#include "naucrates/md/CMDIdExtStats.h"
#include "naucrates/md/IMDExtStats.h"

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
//		CDXLExtStats
//
//	@doc:
//		Class representing extended stats
//
//---------------------------------------------------------------------------
class CDXLExtStats : public IMDExtStats
{
private:
	// memory pool
	CMemoryPool *m_mp;

	// metadata id of the object
	CMDIdExtStats *m_mdid_ext_stats;

	// column name
	CMDName *m_mdname;

	// DXL string for object
	CWStringDynamic *m_dxl_str;

	CDXLStatsDependencyArray *m_dependencies;

public:
	CDXLExtStats(const CDXLExtStats &) = delete;

	// ctor
	CDXLExtStats(CMemoryPool *mp, CMDIdExtStats *mdid_ext_stats,
				 CMDName *mdname, CDXLStatsDependencyArray *dependencies);

	// dtor
	~CDXLExtStats() override;

	// the metadata id
	IMDId *MDId() const override;

	// relation name
	CMDName Mdname() const override;

	// DXL string representation of cache object
	const CWStringDynamic *GetStrRepr() const override;

	// serialize column stats in DXL format
	void Serialize(gpdxl::CXMLSerializer *) const override;

#ifdef GPOS_DEBUG
	// debug print of the column stats
	void DebugPrint(IOstream &os) const override;
#endif
};

// array of dxl column stats
using CDXLExtStatsArray = CDynamicPtrArray<CDXLExtStats, CleanupRelease>;
}  // namespace gpmd



#endif	// !GPMD_CDXLExtStats_H

// EOF
