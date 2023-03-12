//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2013 EMC Corp.
//
//	@filename:
//		CMDScCmpGPDB.h
//
//	@doc:
//		Implementation of GPDB-specific scalar comparison operators in the MD cache
//---------------------------------------------------------------------------


#ifndef GPMD_CMDScCmpGPDB_H
#define GPMD_CMDScCmpGPDB_H

#include "gpos/base.h"

#include "naucrates/dxl/CDXLUtils.h"
#include "naucrates/md/IMDScCmp.h"

namespace gpmd
{
using namespace gpdxl;

//---------------------------------------------------------------------------
//	@class:
//		CMDScCmpGPDB
//
//	@doc:
//		Implementation for GPDB-specific scalar comparison operators in the
//		MD cache
//
//---------------------------------------------------------------------------
class CMDScCmpGPDB : public IMDScCmp
{
private:
	// memory pool
	CMemoryPool *m_mp;

	// DXL for object
	mutable CWStringDynamic *m_dxl_str{nullptr};

	// object id
	gpos::Ref<IMDId> m_mdid;

	// operator name
	CMDName *m_mdname;

	// left type
	gpos::Ref<IMDId> m_mdid_left;

	// right type
	gpos::Ref<IMDId> m_mdid_right;

	// comparison type
	IMDType::ECmpType m_comparision_type;

	// comparison operator id
	gpos::Ref<IMDId> m_mdid_op;

public:
	CMDScCmpGPDB(const CMDScCmpGPDB &) = delete;

	// ctor
	CMDScCmpGPDB(CMemoryPool *mp, gpos::Ref<IMDId> mdid, CMDName *mdname,
				 gpos::Ref<IMDId> left_mdid, gpos::Ref<IMDId> right_mdid,
				 IMDType::ECmpType cmp_type, gpos::Ref<IMDId> mdid_op);

	// dtor
	~CMDScCmpGPDB() override;

	// accessors
	const CWStringDynamic *
	GetStrRepr() const override
	{
		if (nullptr == m_dxl_str)
		{
			m_dxl_str = CDXLUtils::SerializeMDObj(
				m_mp, gpos::dyn_cast<IMDCacheObject>(SharedFromThis()),
				false /*fSerializeHeader*/, false /*indentation*/);
		}
		return m_dxl_str;
	}

	// copmarison object id
	gpos::Ref<IMDId> MDId() const override;

	// cast object name
	CMDName Mdname() const override;

	// left type
	gpos::Ref<IMDId> GetLeftMdid() const override;

	// right type
	gpos::Ref<IMDId> GetRightMdid() const override;

	// comparison type
	IMDType::ECmpType ParseCmpType() const override;

	// comparison operator id
	gpos::Ref<IMDId> MdIdOp() const override;

	// serialize object in DXL format
	void Serialize(gpdxl::CXMLSerializer *xml_serializer) const override;

#ifdef GPOS_DEBUG
	// debug print of the type in the provided stream
	void DebugPrint(IOstream &os) const override;
#endif
};
}  // namespace gpmd

#endif	// !GPMD_CMDScCmpGPDB_H

// EOF
