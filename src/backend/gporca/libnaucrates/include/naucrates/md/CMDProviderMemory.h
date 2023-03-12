//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2014 VMware, Inc. or its affiliates.
//
//	@filename:
//		CMDProviderMemory.h
//
//	@doc:
//		Memory-based provider of metadata objects.
//---------------------------------------------------------------------------



#ifndef GPMD_CMDProviderMemory_H
#define GPMD_CMDProviderMemory_H

#include "gpos/base.h"
#include "gpos/common/RefWrapper.h"
#include "gpos/string/CWStringDynamic.h"

#include "naucrates/md/IMDCacheObject.h"
#include "naucrates/md/IMDId.h"
#include "naucrates/md/IMDProvider.h"

namespace gpmd
{
using namespace gpos;
using namespace gpnaucrates;

struct mdid_equal_to
	: public std::binary_function<gpos::Ref<IMDId>, gpos::Ref<IMDId>, bool>
{
	bool
	operator()(const gpos::Ref<IMDId> &lhs, const gpos::Ref<IMDId> &rhs) const
	{
		return lhs->Equals(rhs);
	}
};

struct mdid_hash
{
	int
	operator()(const gpos::Ref<IMDId> &obj) const
	{
		return obj->HashValue();
	}
};

//---------------------------------------------------------------------------
//	@class:
//		CMDProviderMemory
//
//	@doc:
//		Memory-based provider of metadata objects.
//
//---------------------------------------------------------------------------
class CMDProviderMemory : public IMDProvider
{
protected:
	// hash map of serialized MD objects indexed by their MD id
	using MDIdToSerializedMDIdMap =
		gpos::UnorderedMap<gpos::Ref<IMDId>,
						   gpos::Ref<gpos::WrapperCWStringDynamic>, mdid_hash,
						   mdid_equal_to>;

	// metadata objects indexed by their metadata id
	gpos::Ref<MDIdToSerializedMDIdMap> m_mdmap;

	// load MD objects in the hash map
	void LoadMetadataObjectsFromArray(
		CMemoryPool *mp, gpos::Ref<IMDCacheObjectArray> mdcache_obj_array);

	// private copy ctor
	CMDProviderMemory(const CMDProviderMemory &);

public:
	// ctor
	CMDProviderMemory(CMemoryPool *mp,
					  gpos::Ref<IMDCacheObjectArray> mdcache_obj_array);

	// ctor
	CMDProviderMemory(CMemoryPool *mp, const CHAR *file_name);

	//dtor
	~CMDProviderMemory() override;

	// returns the DXL string of the requested metadata object
	CWStringBase *GetMDObjDXLStr(CMemoryPool *mp, CMDAccessor *md_accessor,
								 gpos::Ref<IMDId> mdid) const override;

	// returns the requested metadata object
	gpos::Ref<IMDCacheObject> GetMDObj(
		CMemoryPool *mp, CMDAccessor *md_accessor, gpos::Ref<IMDId> mdid,
		IMDCacheObject::Emdtype mdtype) const override;

	// return the mdid for the specified system id and type
	gpos::Ref<IMDId> MDId(CMemoryPool *mp, CSystemId sysid,
						  IMDType::ETypeInfo type_info) const override;
};
}  // namespace gpmd



#endif	// !GPMD_CMDProviderMemory_H

// EOF
