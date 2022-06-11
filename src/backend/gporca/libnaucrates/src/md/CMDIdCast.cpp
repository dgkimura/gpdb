//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2013 EMC Corp.
//
//	@filename:
//		CMDIdCast.cpp
//
//	@doc:
//		Implementation of mdids for cast functions
//---------------------------------------------------------------------------


#include "naucrates/md/CMDIdCast.h"

#include "gpos/common/owner.h"

#include "naucrates/dxl/xml/CXMLSerializer.h"

using namespace gpos;
using namespace gpmd;

//---------------------------------------------------------------------------
//	@function:
//		CMDIdCast::CMDIdCast
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CMDIdCast::CMDIdCast(gpos::owner<CMDIdGPDB *> mdid_src,
					 gpos::owner<CMDIdGPDB *> mdid_dest)
	: m_mdid_src(std::move(mdid_src)),
	  m_mdid_dest(std::move(mdid_dest)),
	  m_str(m_mdid_buffer, GPOS_ARRAY_SIZE(m_mdid_buffer))
{
	GPOS_ASSERT(m_mdid_src->IsValid());
	GPOS_ASSERT(m_mdid_dest->IsValid());

	// serialize mdid into static string
	Serialize();
}

//---------------------------------------------------------------------------
//	@function:
//		CMDIdCast::~CMDIdCast
//
//	@doc:
//		Dtor
//
//---------------------------------------------------------------------------
CMDIdCast::~CMDIdCast()
{
	m_mdid_src->Release();
	m_mdid_dest->Release();
}

//---------------------------------------------------------------------------
//	@function:
//		CMDIdCast::Serialize
//
//	@doc:
//		Serialize mdid into static string
//
//---------------------------------------------------------------------------
void
CMDIdCast::Serialize()
{
	// serialize mdid as SystemType.mdidSrc.mdidDest
	m_str.AppendFormat(GPOS_WSZ_LIT("%d.%d.%d.%d;%d.%d.%d"), MdidType(),
					   m_mdid_src->Oid(), m_mdid_src->VersionMajor(),
					   m_mdid_src->VersionMinor(), m_mdid_dest->Oid(),
					   m_mdid_dest->VersionMajor(),
					   m_mdid_dest->VersionMinor());
}

//---------------------------------------------------------------------------
//	@function:
//		CMDIdCast::GetBuffer
//
//	@doc:
//		Returns the string representation of the mdid
//
//---------------------------------------------------------------------------
const WCHAR *
CMDIdCast::GetBuffer() const
{
	return m_str.GetBuffer();
}

//---------------------------------------------------------------------------
//	@function:
//		CMDIdCast::MdidSrc
//
//	@doc:
//		Returns the source type id
//
//---------------------------------------------------------------------------
gpos::pointer<IMDId *>
CMDIdCast::MdidSrc() const
{
	return m_mdid_src;
}

//---------------------------------------------------------------------------
//	@function:
//		CMDIdCast::MdidDest
//
//	@doc:
//		Returns the destination type id
//
//---------------------------------------------------------------------------
gpos::pointer<IMDId *>
CMDIdCast::MdidDest() const
{
	return m_mdid_dest;
}

//---------------------------------------------------------------------------
//	@function:
//		CMDIdCast::Equals
//
//	@doc:
//		Checks if the mdids are equal
//
//---------------------------------------------------------------------------
BOOL
CMDIdCast::Equals(gpos::pointer<const IMDId *> mdid) const
{
	if (nullptr == mdid || EmdidCastFunc != mdid->MdidType())
	{
		return false;
	}

	gpos::pointer<const CMDIdCast *> mdid_cast_func = CMDIdCast::CastMdid(mdid);

	return m_mdid_src->Equals(mdid_cast_func->MdidSrc()) &&
		   m_mdid_dest->Equals(mdid_cast_func->MdidDest());
}

//---------------------------------------------------------------------------
//	@function:
//		CMDIdCast::Serialize
//
//	@doc:
//		Serializes the mdid as the value of the given attribute
//
//---------------------------------------------------------------------------
void
CMDIdCast::Serialize(CXMLSerializer *xml_serializer,
					 const CWStringConst *pstrAttribute) const
{
	xml_serializer->AddAttribute(pstrAttribute, &m_str);
}

//---------------------------------------------------------------------------
//	@function:
//		CMDIdCast::OsPrint
//
//	@doc:
//		Debug print of the id in the provided stream
//
//---------------------------------------------------------------------------
IOstream &
CMDIdCast::OsPrint(IOstream &os) const
{
	os << "(" << m_str.GetBuffer() << ")";
	return os;
}

// EOF
