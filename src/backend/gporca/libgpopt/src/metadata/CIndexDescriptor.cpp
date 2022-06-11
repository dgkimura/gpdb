//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2012 EMC Corp.
//
//	@filename:
//		CIndexDescriptor.cpp
//
//	@doc:
//		Implementation of index description
//---------------------------------------------------------------------------

#include "gpopt/metadata/CIndexDescriptor.h"

#include "gpos/base.h"
#include "gpos/common/owner.h"

#include "gpopt/base/CColumnFactory.h"
#include "gpopt/base/COptCtxt.h"
#include "gpopt/base/CUtils.h"

using namespace gpopt;

FORCE_GENERATE_DBGSTR(CIndexDescriptor);

//---------------------------------------------------------------------------
//	@function:
//		CIndexDescriptor::CIndexDescriptor
//
//	@doc:
//		Ctor
//
//---------------------------------------------------------------------------
CIndexDescriptor::CIndexDescriptor(
	CMemoryPool *mp, gpos::Ref<IMDId> pmdidIndex, const CName &name,
	gpos::Ref<CColumnDescriptorArray> pdrgcoldescKeyCols,
	gpos::Ref<CColumnDescriptorArray> pdrgcoldescIncludedCols,
	BOOL is_clustered, IMDIndex::EmdindexType index_type)
	: m_pmdidIndex(std::move(pmdidIndex)),
	  m_name(mp, name),
	  m_pdrgpcoldescKeyCols(std::move(pdrgcoldescKeyCols)),
	  m_pdrgpcoldescIncludedCols(std::move(pdrgcoldescIncludedCols)),
	  m_clustered(is_clustered),
	  m_index_type(index_type)
{
	GPOS_ASSERT(nullptr != mp);
	GPOS_ASSERT(m_pmdidIndex->IsValid());
	GPOS_ASSERT(nullptr != m_pdrgpcoldescKeyCols);
	GPOS_ASSERT(nullptr != m_pdrgpcoldescIncludedCols);
}

//---------------------------------------------------------------------------
//	@function:
//		CIndexDescriptor::~CIndexDescriptor
//
//	@doc:
//		Dtor
//
//---------------------------------------------------------------------------
CIndexDescriptor::~CIndexDescriptor()
{
	;

	;
	;
}

//---------------------------------------------------------------------------
//	@function:
//		CIndexDescriptor::Keys
//
//	@doc:
//		number of key columns
//
//---------------------------------------------------------------------------
ULONG
CIndexDescriptor::Keys() const
{
	return m_pdrgpcoldescKeyCols->Size();
}

//---------------------------------------------------------------------------
//	@function:
//		CIndexDescriptor::UlIncludedColumns
//
//	@doc:
//		Number of included columns
//
//---------------------------------------------------------------------------
ULONG
CIndexDescriptor::UlIncludedColumns() const
{
	// array allocated in ctor
	GPOS_ASSERT(nullptr != m_pdrgpcoldescIncludedCols);

	return m_pdrgpcoldescIncludedCols->Size();
}

//---------------------------------------------------------------------------
//	@function:
//		CIndexDescriptor::Pindexdesc
//
//	@doc:
//		Create the index descriptor from the table descriptor and index
//		information from the catalog
//
//---------------------------------------------------------------------------
gpos::Ref<CIndexDescriptor>
CIndexDescriptor::Pindexdesc(CMemoryPool *mp, const CTableDescriptor *ptabdesc,
							 const IMDIndex *pmdindex)
{
	CWStringConst strIndexName(mp, pmdindex->Mdname().GetMDName()->GetBuffer());

	CColumnDescriptorArray *pdrgpcoldesc = ptabdesc->Pdrgpcoldesc();

	;

	// array of index column descriptors
	gpos::Ref<CColumnDescriptorArray> pdrgcoldescKey =
		GPOS_NEW(mp) CColumnDescriptorArray(mp);

	for (ULONG ul = 0; ul < pmdindex->Keys(); ul++)
	{
		gpos::Ref<CColumnDescriptor> pcoldesc = (*pdrgpcoldesc)[ul];
		;
		pdrgcoldescKey->Append(pcoldesc);
	}

	// array of included column descriptors
	gpos::Ref<CColumnDescriptorArray> pdrgcoldescIncluded =
		GPOS_NEW(mp) CColumnDescriptorArray(mp);
	for (ULONG ul = 0; ul < pmdindex->IncludedCols(); ul++)
	{
		gpos::Ref<CColumnDescriptor> pcoldesc = (*pdrgpcoldesc)[ul];
		;
		pdrgcoldescIncluded->Append(pcoldesc);
	}


	// create the index descriptors
	gpos::Ref<CIndexDescriptor> pindexdesc = GPOS_NEW(mp) CIndexDescriptor(
		mp, pmdindex->MDId(), CName(&strIndexName), std::move(pdrgcoldescKey),
		std::move(pdrgcoldescIncluded), pmdindex->IsClustered(),
		pmdindex->IndexType());
	return pindexdesc;
}

BOOL
CIndexDescriptor::SupportsIndexOnlyScan() const
{
	return m_index_type == IMDIndex::EmdindBtree;
}

//---------------------------------------------------------------------------
//	@function:
//		CIndexDescriptor::OsPrint
//
//	@doc:
//		debug print
//
//---------------------------------------------------------------------------
IOstream &
CIndexDescriptor::OsPrint(IOstream &os) const
{
	m_name.OsPrint(os);
	os << ": (Keys :";
	CUtils::OsPrintDrgPcoldesc(os, m_pdrgpcoldescKeyCols.get(),
							   m_pdrgpcoldescKeyCols->Size());
	os << "); ";

	os << "(Included Columns :";
	CUtils::OsPrintDrgPcoldesc(os, m_pdrgpcoldescIncludedCols.get(),
							   m_pdrgpcoldescIncludedCols->Size());
	os << ")";

	os << " [ Clustered :";
	if (m_clustered)
	{
		os << "true";
	}
	else
	{
		os << "false";
	}
	os << " ]";
	return os;
}

// EOF
