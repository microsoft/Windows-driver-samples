#ifndef __INC_CPPDRVREG_H
#define __INC_CPPDRVREG_H

//
// Tested by rcnjko, 2004.07.08.
//
#if defined (__cplusplus)

 // v4 (NT DDK)
 #if !defined(NDIS50_MINIPORT) && !defined(NDIS50) && !defined(NDIS51_MINIPORT) && !defined(NDIS51)
  #define KNDIS_CHARS_MPFIELD_v3(Char, Field)	Char.Ndis30Chars.Field
  #define KNDIS_CHARS_MPFIELD_v4(Char, Field)	Char.Field

  #define KNDIS_CHARS_PTFIELD_v3(Char, Field)	Char.Ndis30Chars.Field
  #define KNDIS_CHARS_PTFIELD_v4(Char, Field)	Char.Field
 #endif

 // v5 (W2K DDK)
 #if defined (NDIS50_MINIPORT) || defined (NDIS50)
  #define KNDIS_CHARS_MPFIELD_v3(Char, Field)	Char.Ndis40Chars.Ndis30Chars.Field
  #define KNDIS_CHARS_MPFIELD_v4(Char, Field)	Char.Ndis40Chars.Field
  #define KNDIS_CHARS_MPFIELD_v5(Char, Field)	Char.Field

  #define KNDIS_CHARS_PTFIELD_v3(Char, Field)	Char.Ndis40Chars.Ndis30Chars.Field
  #define KNDIS_CHARS_PTFIELD_v4(Char, Field)	Char.Ndis40Chars.Field
  #define KNDIS_CHARS_PTFIELD_v5(Char, Field)	Char.Field
 #endif

 // v51 (XP DDK)
 #if defined (NDIS51_MINIPORT) || defined (NDIS51)
  #define KNDIS_CHARS_MPFIELD_v3(Char, Field)	Char.Ndis50Chars.Ndis40Chars.Ndis30Chars.Field
  #define KNDIS_CHARS_MPFIELD_v4(Char, Field)	Char.Ndis50Chars.Ndis40Chars.Field
  #define KNDIS_CHARS_MPFIELD_v5(Char, Field)	Char.Ndis50Chars.Field
  #define KNDIS_CHARS_MPFIELD_v51(Char, Field)  Char.Field
   // same as 5.0
  #define KNDIS_CHARS_PTFIELD_v3(Char, Field)	Char.Ndis40Chars.Ndis30Chars.Field
  #define KNDIS_CHARS_PTFIELD_v4(Char, Field)	Char.Ndis40Chars.Field
  #define KNDIS_CHARS_PTFIELD_v5(Char, Field)	Char.Field
 #endif 

 // v6 (TBD)
 // ...


#else // C is cool
  #define KNDIS_CHARS_MPFIELD_v3(Char, Field)	Char.Field
  #define KNDIS_CHARS_MPFIELD_v4(Char, Field)	Char.Field
  #define KNDIS_CHARS_MPFIELD_v5(Char, Field)	Char.Field
  #define KNDIS_CHARS_MPFIELD_v51(Char, Field) Char.Field

  #define KNDIS_CHARS_PTFIELD_v3(Char, Field)	Char.Field
  #define KNDIS_CHARS_PTFIELD_v4(Char, Field)	Char.Field
  #define KNDIS_CHARS_PTFIELD_v5(Char, Field)	Char.Field
#endif // __cplusplus

#endif // #ifndef __INC_CPPDRVREG_H
