//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CMDIdExtStats.cpp
//
//	@doc:
//		Implementation of mdids for column statistics
//---------------------------------------------------------------------------


#include "naucrates/md/CMDIdExtStats.h"

#include "naucrates/dxl/xml/CXMLSerializer.h"

using namespace gpos;
using namespace gpmd;

//---------------------------------------------------------------------------
//	@function:
//		CMDIdExtStats::CMDIdExtStats
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CMDIdExtStats::CMDIdExtStats(CMDIdGPDB *rel_mdid, CColRefSet *pcrs,
							 CBitSet *attno_bms)
	: m_rel_mdid(rel_mdid),
	  m_pcrs(pcrs),
	  m_attno_bms(attno_bms),
	  m_str(m_mdid_buffer, GPOS_ARRAY_SIZE(m_mdid_buffer))
{
	GPOS_ASSERT(rel_mdid->IsValid());

	// serialize mdid into static string
	Serialize();
}

//---------------------------------------------------------------------------
//	@function:
//		CMDIdExtStats::~CMDIdExtStats
//
//	@doc:
//		Dtor
//
//---------------------------------------------------------------------------
CMDIdExtStats::~CMDIdExtStats()
{
	m_rel_mdid->Release();
}

//---------------------------------------------------------------------------
//	@function:
//		CMDIdExtStats::Serialize
//
//	@doc:
//		Serialize mdid into static string
//
//---------------------------------------------------------------------------
void
CMDIdExtStats::Serialize()
{
	// serialize mdid as SystemType.Oid.Major.Minor.Attno
	m_str.AppendFormat(
		GPOS_WSZ_LIT("%d.%d.%d.%d.%d"), MdidType(), m_rel_mdid->Oid(),
		m_rel_mdid->VersionMajor(), m_rel_mdid->VersionMinor(),
		m_pcrs->HashValue());  // FIXME:KIMURA m_pcrs->HashValue().
}

//---------------------------------------------------------------------------
//	@function:
//		CMDIdExtStats::GetBuffer
//
//	@doc:
//		Returns the string representation of the mdid
//
//---------------------------------------------------------------------------
const WCHAR *
CMDIdExtStats::GetBuffer() const
{
	return m_str.GetBuffer();
}

//---------------------------------------------------------------------------
//	@function:
//		CMDIdExtStats::GetRelMdId
//
//	@doc:
//		Returns the base relation id
//
//---------------------------------------------------------------------------
IMDId *
CMDIdExtStats::GetRelMdId() const
{
	return m_rel_mdid;
}

//---------------------------------------------------------------------------
//	@function:
//		CMDIdExtStats::Equals
//
//	@doc:
//		Checks if the mdids are equal
//
//---------------------------------------------------------------------------
BOOL
CMDIdExtStats::Equals(const IMDId *mdid) const
{
	if (nullptr == mdid || EmdidExtStats != mdid->MdidType())
	{
		return false;
	}

	const CMDIdExtStats *mdid_ext_stats = CMDIdExtStats::CastMdid(mdid);

	// TODO:KIMURA
	return m_rel_mdid->Equals(mdid_ext_stats->GetRelMdId());
}

//---------------------------------------------------------------------------
//	@function:
//		CMDIdExtStats::Serialize
//
//	@doc:
//		Serializes the mdid as the value of the given attribute
//
//---------------------------------------------------------------------------
void
CMDIdExtStats::Serialize(CXMLSerializer *xml_serializer,
						 const CWStringConst *attribute_str) const
{
	xml_serializer->AddAttribute(attribute_str, &m_str);
}

//---------------------------------------------------------------------------
//	@function:
//		CMDIdExtStats::OsPrint
//
//	@doc:
//		Debug print of the id in the provided stream
//
//---------------------------------------------------------------------------
IOstream &
CMDIdExtStats::OsPrint(IOstream &os) const
{
	os << "(" << m_str.GetBuffer() << ")";
	return os;
}

// EOF
