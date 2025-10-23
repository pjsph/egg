#ifndef EDEFINES_H
#define EDEFINES_H

#ifdef _MSC_VER
    #define EAPI __declspec(dllexport)
#else
    #define EAPI
#endif

#define false 0
#define true 1

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;

// TODO: check that types are at least of the expected size

#endif // EDEFINES_H
