//---------------------------------------------------------------------------
// Greenplum Database
// Copyright (c) 2020 VMware, Inc. or its affiliates
//---------------------------------------------------------------------------
#ifndef GPOS_Owner_H
#define GPOS_Owner_H
namespace gpos
{
template <class T>
using owner = T;

template <class T>
using pointer = T;

// Like an owner, but used for manually suppressing pointer / observer
// detection. It indicates that the normal assumption that an owner cannot leak
// doesn't apply here.
template <class T>
using leaked = T;

// our useless little "Convert" or "cast" helper functions
// convert to using uniform cast from Casting.h
template <class T>
using cast_func = T;
}  // namespace gpos
#endif
