
#ifndef _ATOMIC_H_
#define _ATOMIC_H_

/*
int8_t / uint8_t
int16_t / uint16_t
int32_t / uint32_t
int64_t / uint64_t

在用gcc编译的时候要加上选项 -march=i686
type __sync_fetch_and_add (type *ptr, type value);
type __sync_fetch_and_sub (type *ptr, type value);
type __sync_fetch_and_or (type *ptr, type value);
type __sync_fetch_and_and (type *ptr, type value);
type __sync_fetch_and_xor (type *ptr, type value);
type __sync_fetch_and_nand (type *ptr, type value);

type __sync_add_and_fetch (type *ptr, type value);
type __sync_sub_and_fetch (type *ptr, type value);
type __sync_or_and_fetch (type *ptr, type value);
type __sync_and_and_fetch (type *ptr, type value);
type __sync_xor_and_fetch (type *ptr, type value);
type __sync_nand_and_fetch (type *ptr, type value);

bool __sync_bool_compare_and_swap (type *ptr, type oldval, type newval, ...)
type __sync_val_compare_and_swap (type *ptr, type oldval, type newval, ...)
type __sync_lock_test_and_set(type *ptr, type value, ....);
void __sync_lock_release(type* ptr, ....);
*/


#ifdef WIN32
#include <Windows.h>
#include <WinNT.h>
#include <WinBase.h>
#include <intrin.h>
#include <stdint.h>

/*
*The InterlockedAdd function is only available on the Itanium platform. 
*On x86 and x86-64 platforms you can use InterlockedExchangeAdd instead
*/

//add
template<typename T>
T __sync_fetch_and_add(volatile T* ptr, T value)
{
	throw "";
}

template<>
int32_t __sync_fetch_and_add<int32_t>(volatile int32_t* ptr, int32_t value)
{
	return InterlockedExchangeAdd((LONG*)ptr, value); 
}

template<>
uint32_t __sync_fetch_and_add<uint32_t>(volatile uint32_t* ptr, uint32_t value)
{
	return InterlockedExchangeAdd((LONG*)ptr, value); 
}

template<>
int64_t __sync_fetch_and_add<int64_t>(volatile int64_t* ptr, int64_t value)
{
	return InterlockedExchangeAdd64((LONGLONG*)ptr, value); 
}

template<>
uint64_t __sync_fetch_and_add<uint64_t>(volatile uint64_t* ptr, uint64_t value)
{
	return InterlockedExchangeAdd64((LONGLONG*)ptr, value); 
}

//sub
template<typename T>
T __sync_fetch_and_sub(volatile T* ptr, T value)
{
	throw "";
}

template<>
int32_t __sync_fetch_and_sub<int32_t>(volatile int32_t* ptr, int32_t value)
{
	return InterlockedExchangeSubtract((unsigned long*)ptr, value); 
}

template<>
uint32_t __sync_fetch_and_sub<uint32_t>(volatile uint32_t* ptr, uint32_t value)
{
	return InterlockedExchangeSubtract(ptr, value); 
}

template<>
int64_t __sync_fetch_and_sub<int64_t>(volatile int64_t* ptr, int64_t value)
{
	return InterlockedExchangeAdd64((LONGLONG*)ptr, -1*value); 
}

template<>
uint64_t __sync_fetch_and_sub<uint64_t>(volatile uint64_t* ptr, uint64_t value)
{
	return InterlockedExchangeAdd64((LONGLONG*)ptr, -1*value); 
}

//or
template<typename T>
T __sync_fetch_and_or(volatile T* ptr, T value)
{
	throw "";
}

template<>
int8_t __sync_fetch_and_or<int8_t>(volatile int8_t* ptr, int8_t value)
{
	return _InterlockedOr8((char*)ptr, value); 
}

template<>
int16_t __sync_fetch_and_or<int16_t>(volatile int16_t* ptr, int16_t value)
{
	return InterlockedOr16(ptr, value);
}

template<>
int32_t __sync_fetch_and_or<int32_t>(volatile int32_t* ptr, int32_t value)
{
	return _InterlockedOr((long*)ptr, value);
}

template<>
int64_t __sync_fetch_and_or<int64_t>(volatile int64_t* ptr, int64_t value)
{
	return InterlockedOr64(ptr, value);
}

//and
template<typename T>
T __sync_fetch_and_and(volatile T* ptr, T value)
{
	throw "";
}

template<>
int8_t __sync_fetch_and_and<int8_t>(volatile int8_t* ptr, int8_t value)
{
	return _InterlockedAnd8((char*)ptr, value); 
}

template<>
int16_t __sync_fetch_and_and<int16_t>(volatile int16_t* ptr, int16_t value)
{
	return InterlockedAnd16(ptr, value); 
}

template<>
int32_t __sync_fetch_and_and<int32_t>(volatile int32_t* ptr, int32_t value)
{
	return _InterlockedAnd((long*)ptr, value); 
}

template<>
int64_t __sync_fetch_and_and<int64_t>(volatile int64_t* ptr, int64_t value)
{
	return InterlockedAnd64(ptr, value); 
}

//xor
template<typename T>
T __sync_fetch_and_xor(volatile T* ptr, T value)
{
	throw "";
}

template<>
int8_t __sync_fetch_and_xor<int8_t>(volatile int8_t* ptr, int8_t value)
{
	return _InterlockedXor8((char*)ptr, value); 
}

template<>
int16_t __sync_fetch_and_xor<int16_t>(volatile int16_t* ptr, int16_t value)
{
	return _InterlockedXor16(ptr, value); 
}

template<>
int32_t __sync_fetch_and_xor<int32_t>(volatile int32_t* ptr, int32_t value)
{
	return _InterlockedXor((long*)ptr, value); 
}

template<>
int64_t __sync_fetch_and_xor<int64_t>(volatile int64_t* ptr, int64_t value)
{
	return InterlockedXor64(ptr, value); 
}

//nand
template<typename T>
T __sync_fetch_and_nand(volatile T* ptr, T value)
{
	throw "";
}

template<>
int8_t __sync_fetch_and_nand<int8_t>(volatile int8_t* ptr, int8_t value)
{
	return _InterlockedAnd8((char*)ptr, ((~(value<<1))>>1)); 
}

template<>
int16_t __sync_fetch_and_nand<int16_t>(volatile int16_t* ptr, int16_t value)
{
	return InterlockedAnd16(ptr, ((~(value<<1))>>1)); 
}

template<>
int32_t __sync_fetch_and_nand<int32_t>(volatile int32_t* ptr, int32_t value)
{
	return _InterlockedAnd((long*)ptr, ((~(value<<1))>>1)); 
}

template<>
int64_t __sync_fetch_and_nand<int64_t>(volatile int64_t* ptr, int64_t value)
{
	return InterlockedAnd64(ptr, ((~(value<<1))>>1)); 
}

//CAS
template<typename T>
bool __sync_bool_compare_and_swap(volatile T* ptr, T oldval, T newval)
{
	throw "";
}

template<>
bool __sync_bool_compare_and_swap(volatile int8_t* ptr, int8_t oldval, int8_t newval)
{
	char val = _InterlockedCompareExchange8((char*)ptr, newval, oldval);
	return val == oldval;
}

template<>
bool __sync_bool_compare_and_swap(volatile int16_t* ptr, int16_t oldval, int16_t newval)
{
	short val = _InterlockedCompareExchange16(ptr, newval, oldval);
	return val == oldval;
}

template<>
bool __sync_bool_compare_and_swap(volatile int32_t* ptr, int32_t oldval, int32_t newval)
{
	long val = _InterlockedCompareExchange((long*)ptr, newval, oldval);
	return val == oldval;
}

template<>
bool __sync_bool_compare_and_swap(volatile int64_t* ptr, int64_t oldval, int64_t newval)
{
	long long val = _InterlockedCompareExchange64(ptr, newval, oldval);
	return val == oldval;
}

//test and set
template<typename T>
T __sync_lock_test_and_set(volatile T* ptr, T newval)
{
	throw "";
}

template<>
int8_t __sync_lock_test_and_set(volatile int8_t* ptr, int8_t newval)
{
	return InterlockedBitTestAndSet((long*)ptr, newval); 
}

template<>
int16_t __sync_lock_test_and_set(volatile int16_t* ptr, int16_t newval)
{
	return InterlockedBitTestAndSet((long*)ptr, newval); 
}

template<>
int32_t __sync_lock_test_and_set(volatile int32_t* ptr, int32_t newval)
{
	return InterlockedBitTestAndSet((long*)ptr, newval); 
}

#ifdef _WIN64
template<>
int64_t __sync_lock_test_and_set(volatile int64_t* ptr, int64_t newval)
{
	return InterlockedBitTestAndSet64((LONG64*)ptr, (LONG64)newval); 
}
#endif


//inc dec
template<typename T>
T atomic_inc(volatile T* ptr)
{
	return InterlockedIncrement(ptr); 
}

template<typename T>
T atomic_dec(volatile T* ptr)
{
	return InterlockedDecrement(ptr); 
}

#endif //WIN32

#endif//_ATOMIC_H_