//---------------------------------------------------------------------------
//	Greenplum Database
//	Copyright (C) 2014 VMware, Inc. or its affiliates.
//
//	@filename:
//		IComparator.h
//
//	@doc:
//		Interface for comparing IDatum instances.
//
//	@owner:
//
//
//	@test:
//
//---------------------------------------------------------------------------

#ifndef GPOPT_IComparator_H
#define GPOPT_IComparator_H

#include "gpos/base.h"
#include "gpos/common/owner.h"

namespace gpnaucrates
{
// fwd declarations
class IDatum;
}  // namespace gpnaucrates

namespace gpopt
{
using gpnaucrates::IDatum;

//---------------------------------------------------------------------------
//	@class:
//		IComparator
//
//	@doc:
//		Interface for comparing IDatum instances.
//
//---------------------------------------------------------------------------
class IComparator
{
public:
	virtual ~IComparator() = default;

	// tests if the two arguments are equal
	virtual gpos::BOOL Equals(gpos::pointer<const IDatum *> datum1,
							  gpos::pointer<const IDatum *> datum2) const = 0;

	// tests if the first argument is less than the second
	virtual gpos::BOOL IsLessThan(
		gpos::pointer<const IDatum *> datum1,
		gpos::pointer<const IDatum *> datum2) const = 0;

	// tests if the first argument is less or equal to the second
	virtual gpos::BOOL IsLessThanOrEqual(
		gpos::pointer<const IDatum *> datum1,
		gpos::pointer<const IDatum *> datum2) const = 0;

	// tests if the first argument is greater than the second
	virtual gpos::BOOL IsGreaterThan(
		gpos::pointer<const IDatum *> datum1,
		gpos::pointer<const IDatum *> datum2) const = 0;

	// tests if the first argument is greater or equal to the second
	virtual gpos::BOOL IsGreaterThanOrEqual(
		gpos::pointer<const IDatum *> datum1,
		gpos::pointer<const IDatum *> datum2) const = 0;
};
}  // namespace gpopt

#endif	// !GPOPT_IComparator_H

// EOF
