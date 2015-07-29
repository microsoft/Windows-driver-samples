//===========================================================================
// HARDWARE OFFLOAD PIN DEFINITIONS
//===========================================================================
#define STATIC_KSPROPSETID_OffloadPin\
    0xfb05301,   0x5a11, 0x400a, 0x99, 0x6b, 0x12,0xdf, 0x99, 0x28, 0x36, 0xde

DEFINE_GUIDSTRUCT("0FB05301-5A11-400a-996B-12DF992836DE", KSPROPSETID_OffloadPin);

#define KSPROPSETID_OffloadPin DEFINE_GUIDNAMED(KSPROPSETID_OffloadPin)

typedef enum {
    KSPROPERTY_OFFLOAD_PIN_GET_STREAM_OBJECT_POINTER,
    KSPROPERTY_OFFLOAD_PIN_VERIFY_STREAM_OBJECT_POINTER
} KSPROPERTY_OFFLOAD_PIN;