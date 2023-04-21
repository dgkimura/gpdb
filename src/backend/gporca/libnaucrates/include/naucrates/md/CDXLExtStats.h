//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2023 VMware Inc.
//
//	@filename:
//		CDXLExtStats.h
//
//	@doc:
//		Class representing extended stats
//---------------------------------------------------------------------------



#ifndef GPMD_CDXLExtStats_H
#define GPMD_CDXLExtStats_H

#include "gpos/base.h"
#include "gpos/common/CDouble.h"
#include "gpos/string/CWStringDynamic.h"

#include "naucrates/md/CMDExtStatsInfo.h"
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
	gpos::Ref<IMDId> m_rel_stats_mdid;

	// table name
	CMDName *m_mdname;

	// DXL string for object
	mutable CWStringDynamic *m_dxl_str{nullptr};

	gpos::Ref<CMDDependencyArray> m_dependency_array;

	gpos::Ref<CMDNDistinctArray> m_ndistinct_array;

public:
	CDXLExtStats(const CDXLExtStats &) = delete;

	CDXLExtStats(CMemoryPool *mp, gpos::Ref<IMDId> rel_stats_mdid,
				 CMDName *mdname,
				 gpos::Ref<CMDDependencyArray> extstats_dependency_array,
				 gpos::Ref<CMDNDistinctArray> ndistinct_array);

	~CDXLExtStats() override;

	// the metadata id
	gpos::Ref<IMDId> MDId() const override;

	// relation name
	CMDName Mdname() const override;

	// DXL string representation of cache object
	const CWStringDynamic *GetStrRepr() const override;

	// serialize relation stats in DXL format given a serializer object
	void Serialize(gpdxl::CXMLSerializer *) const override;

	gpos::Ref<CMDDependencyArray>
	GetDependencies() const override
	{
		return m_dependency_array;
	}

	gpos::Ref<CMDNDistinctArray>
	GetNDistinctList() const override
	{
		return m_ndistinct_array;
	}

#ifdef GPOS_DEBUG
	// debug print of the metadata ext stats
	void
	DebugPrint(IOstream &os GPOS_UNUSED) const override
	{
	}
#endif

	// dummy relstats
	static gpos::Ref<CDXLExtStats> CreateDXLDummyExtStats(
		CMemoryPool *mp, gpos::Ref<IMDId> mdid);
};

}  // namespace gpmd



#endif	// !GPMD_CDXLExtStats_H

// EOF
