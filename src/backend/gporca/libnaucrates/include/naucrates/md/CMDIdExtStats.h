//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CMDIdExtStats.h
//
//	@doc:
//		Class for representing mdids for extended statistics
//---------------------------------------------------------------------------



#ifndef GPMD_CMDIdExtStats_H
#define GPMD_CMDIdExtStats_H

#include "gpos/base.h"
#include "gpos/common/CDynamicPtrArray.h"
#include "gpos/string/CWStringConst.h"

#include "naucrates/dxl/gpdb_types.h"
#include "naucrates/md/CMDIdGPDB.h"
#include "naucrates/md/CSystemId.h"

namespace gpmd
{
using namespace gpos;


//---------------------------------------------------------------------------
//	@class:
//		CMDIdExtStats
//
//	@doc:
//		Class for representing ids of extended stats objects
//
//---------------------------------------------------------------------------
class CMDIdExtStats : public IMDId
{
private:
	// mdid of base relation
	CMDIdGPDB *m_rel_mdid;

	// position of the attribute in the base relation
	ULONG m_attr_pos;

	// buffer for the serialized mdid
	WCHAR m_mdid_buffer[GPDXL_MDID_LENGTH];

	// string representation of the mdid
	CWStringStatic m_str;

	// serialize mdid
	void Serialize();

public:
	CMDIdExtStats(const CMDIdExtStats &) = delete;

	// ctor
	CMDIdExtStats(CMDIdGPDB *rel_mdid, ULONG attno);

	// dtor
	~CMDIdExtStats() override;

	EMDIdType
	MdidType() const override
	{
		return EmdidExtStats;
	}

	// string representation of mdid
	const WCHAR *GetBuffer() const override;

	// source system id
	CSystemId
	Sysid() const override
	{
		return m_rel_mdid->Sysid();
	}

	// accessors
	IMDId *GetRelMdId() const;

	// equality check
	BOOL Equals(const IMDId *mdid) const override;

	// computes the hash value for the metadata id
	ULONG
	HashValue() const override
	{
		return gpos::CombineHashes(m_rel_mdid->HashValue(),
								   gpos::HashValue(&m_attr_pos));
	}

	// is the mdid valid
	BOOL
	IsValid() const override
	{
		return IMDId::IsValid(m_rel_mdid);
	}

	// serialize mdid in DXL as the value of the specified attribute
	void Serialize(CXMLSerializer *xml_serializer,
				   const CWStringConst *attribute_str) const override;

	// debug print of the metadata id
	IOstream &OsPrint(IOstream &os) const override;

	// const converter
	static const CMDIdExtStats *
	CastMdid(const IMDId *mdid)
	{
		GPOS_ASSERT(nullptr != mdid && EmdidExtStats == mdid->MdidType());

		return dynamic_cast<const CMDIdExtStats *>(mdid);
	}

	// non-const converter
	static CMDIdExtStats *
	CastMdid(IMDId *mdid)
	{
		GPOS_ASSERT(nullptr != mdid && EmdidExtStats == mdid->MdidType());

		return dynamic_cast<CMDIdExtStats *>(mdid);
	}

	// make a copy in the given memory pool
	IMDId *
	Copy(CMemoryPool *mp) const override
	{
		CMDIdGPDB *mdid_rel = CMDIdGPDB::CastMdid(m_rel_mdid->Copy(mp));
		return GPOS_NEW(mp) CMDIdExtStats(mdid_rel, m_attr_pos);
	}
};

}  // namespace gpmd



#endif	// !GPMD_CMDIdExtStats_H

// EOF
