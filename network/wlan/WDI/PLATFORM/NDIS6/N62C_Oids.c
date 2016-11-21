#include "Mp_Precomp.h"

/* We use this OID table structure to check if the OID is supported at this point & what the 
 * size should
 */
typedef struct PORT_QUERY_SET_OID_ENTRY {
    NDIS_OID                    Oid;                // Oid value
    
    BOOLEAN                     InitSettable:1;     // Settable in Init state (for atleast one op mode)
    
    BOOLEAN                     OpSettable:1;       // Settable in Op state (for atleast one op mode)
    
    BOOLEAN                     Queryable:1;        // Queryable in Init/Op state
    
    BOOLEAN                     ExtSTASupported:1;  // Valid in ExtSTA
    
    BOOLEAN                     ExtAPSupported:1;   // Valid in ExtAP
    
    BOOLEAN                     NetmonSupported:1;  // Valid in Netmon mode

    BOOLEAN                     PhySpecific:1;      // Is this OID only acceptable for certain phys

    ULONG                       MinBufferSize;      // Minimum size required for the buffer (query & set)
    
} PORT_QUERY_SET_OID_ENTRY, * PPORT_QUERY_SET_OID_ENTRY;






PORT_QUERY_SET_OID_ENTRY OidQuerySetTable[] = {
    // NDIS OIDs
    {
        OID_GEN_CURRENT_LOOKAHEAD,                  // Oid
        TRUE,                                       // InitSettable
        TRUE,                                       // OpSettable
        TRUE,                                       // Queryable
        TRUE,                                       // ExtSTASupported
        TRUE,                                       // ExtAPSupported
        TRUE,                                       // NetmonSupported
        FALSE,                                      // PhySpecific
        sizeof(ULONG)                               // MinBufferSize
    },
    {
        OID_GEN_CURRENT_PACKET_FILTER,              // Oid
        TRUE,                                       // InitSettable
        TRUE,                                       // OpSettable
        TRUE,                                       // Queryable
        TRUE,                                       // ExtSTASupported
        TRUE,                                       // ExtAPSupported
        TRUE ,                                      // NetmonSupported
        FALSE,                                      // PhySpecific
        sizeof(ULONG)                               // MinBufferSize
    },
    {
        OID_GEN_HARDWARE_STATUS,                    // Oid
        FALSE,                                      // InitSettable
        FALSE,                                      // OpSettable
        TRUE,                                       // Queryable
        TRUE,                                       // ExtSTASupported
        TRUE,                                       // ExtAPSupported
        TRUE,                                       // NetmonSupported
        FALSE,                                      // PhySpecific
        sizeof(NDIS_HARDWARE_STATUS)                // MinBufferSize
    },
    {
        OID_GEN_INTERRUPT_MODERATION,               // Oid
        TRUE,                                       // InitSettable
        TRUE,                                       // OpSettable
        TRUE,                                       // Queryable
        TRUE,                                       // ExtSTASupported
        TRUE,                                       // ExtAPSupported
        TRUE,                                       // NetmonSupported
        FALSE,                                      // PhySpecific
        sizeof(NDIS_INTERRUPT_MODERATION_PARAMETERS)    // MinBufferSize
    },
    {
        OID_GEN_LINK_PARAMETERS,                    // Oid
        TRUE,                                       // InitSettable
        TRUE,                                       // OpSettable
        TRUE,                                       // Queryable
        TRUE,                                       // ExtSTASupported
        TRUE,                                       // ExtAPSupported
        TRUE,                                       // NetmonSupported
        FALSE,                                      // PhySpecific
        sizeof(NDIS_LINK_PARAMETERS)                // MinBufferSize
    },
    {
        OID_GEN_MAXIMUM_FRAME_SIZE,                 // Oid
        FALSE,                                      // InitSettable
        FALSE,                                      // OpSettable
        TRUE,                                       // Queryable
        TRUE,                                       // ExtSTASupported
        TRUE,                                       // ExtAPSupported
        TRUE,                                       // NetmonSupported
        FALSE,                                      // PhySpecific
        sizeof(ULONG)                               // MinBufferSize
    },
    {
        OID_GEN_MAXIMUM_TOTAL_SIZE,                 // Oid
        FALSE,                                      // InitSettable
        FALSE,                                      // OpSettable
        TRUE,                                       // Queryable
        TRUE,                                       // ExtSTASupported
        TRUE,                                       // ExtAPSupported
        TRUE,                                       // NetmonSupported
        FALSE,                                      // PhySpecific
        sizeof(ULONG)                               // MinBufferSize
    },
    {
        OID_GEN_RECEIVE_BLOCK_SIZE,                 // Oid
        FALSE,                                      // InitSettable
        FALSE,                                      // OpSettable
        TRUE,                                       // Queryable
        TRUE,                                       // ExtSTASupported
        TRUE,                                       // ExtAPSupported
        TRUE,                                       // NetmonSupported
        FALSE,                                      // PhySpecific
        sizeof(ULONG)                               // MinBufferSize
    },
    {
        OID_GEN_RECEIVE_BUFFER_SPACE,               // Oid
        FALSE,                                      // InitSettable
        FALSE,                                      // OpSettable
        TRUE,                                       // Queryable
        TRUE,                                       // ExtSTASupported
        TRUE,                                       // ExtAPSupported
        TRUE,                                       // NetmonSupported
        FALSE,                                      // PhySpecific
        sizeof(ULONG)                               // MinBufferSize
    },
    {
        OID_GEN_SUPPORTED_GUIDS,                    // Oid
        FALSE,                                      // InitSettable
        FALSE,                                      // OpSettable
        TRUE,                                       // Queryable
        TRUE,                                       // ExtSTASupported
        TRUE,                                       // ExtAPSupported
        TRUE,                                       // NetmonSupported
        FALSE,                                      // PhySpecific
        0                                           // MinBufferSize // Variable
    },
    {
        OID_GEN_TRANSMIT_BLOCK_SIZE,                // Oid
        FALSE,                                      // InitSettable
        FALSE,                                      // OpSettable
        TRUE,                                       // Queryable
        TRUE,                                       // ExtSTASupported
        TRUE,                                       // ExtAPSupported
        TRUE,                                       // NetmonSupported
        FALSE,                                      // PhySpecific
        sizeof(ULONG)                               // MinBufferSize
    },
    {
        OID_GEN_TRANSMIT_BUFFER_SPACE,              // Oid
        FALSE,                                      // InitSettable
        FALSE,                                      // OpSettable
        TRUE,                                       // Queryable
        TRUE,                                       // ExtSTASupported
        TRUE,                                       // ExtAPSupported
        TRUE,                                       // NetmonSupported
        FALSE,                                      // PhySpecific
        sizeof(ULONG)                               // MinBufferSize
    },
    {
        OID_GEN_TRANSMIT_QUEUE_LENGTH,              // Oid
        FALSE,                                      // InitSettable
        FALSE,                                      // OpSettable
        TRUE,                                       // Queryable
        FALSE,                                       // ExtSTASupported
        FALSE,                                       // ExtAPSupported
        FALSE,                                       // NetmonSupported
        FALSE,                                      // PhySpecific
        sizeof(ULONG)                               // MinBufferSize
    },
    {
        OID_GEN_VENDOR_DESCRIPTION,                 // Oid
        FALSE,                                      // InitSettable
        FALSE,                                      // OpSettable
        TRUE,                                       // Queryable
        TRUE,                                       // ExtSTASupported
        TRUE,                                       // ExtAPSupported
        TRUE,                                       // NetmonSupported
        FALSE,                                      // PhySpecific
        0                                           // MinBufferSize // Variable
    },
    {
        OID_GEN_VENDOR_DRIVER_VERSION,              // Oid
        FALSE,                                      // InitSettable
        FALSE,                                      // OpSettable
        TRUE,                                       // Queryable
        TRUE,                                       // ExtSTASupported
        TRUE,                                       // ExtAPSupported
        TRUE,                                       // NetmonSupported
        FALSE,                                      // PhySpecific
        sizeof(ULONG)                               // MinBufferSize
    },
    {
        OID_GEN_VENDOR_ID,                          // Oid
        FALSE,                                      // InitSettable
        FALSE,                                      // OpSettable
        TRUE,                                       // Queryable
        TRUE,                                       // ExtSTASupported
        TRUE,                                       // ExtAPSupported
        TRUE,                                       // NetmonSupported
        FALSE,                                      // PhySpecific
        sizeof(ULONG)                               // MinBufferSize
    },
    {
        OID_802_3_CURRENT_ADDRESS,                  // Oid
        FALSE,                                      // InitSettable
        FALSE,                                      // OpSettable
        TRUE,                                       // Queryable
        TRUE,                                       // ExtSTASupported
        TRUE,                                       // ExtAPSupported
        FALSE,                                      // NetmonSupported
        FALSE,                                      // PhySpecific
        sizeof(DOT11_MAC_ADDRESS)                   // MinBufferSize
    },

    // PNP handlers
    {
        OID_PNP_SET_POWER,                          // Oid
        TRUE,                                       // InitSettable
        TRUE,                                       // OpSettable
        FALSE,                                      // Queryable
        TRUE,                                       // ExtSTASupported
        TRUE,                                       // ExtAPSupported
        TRUE,                                       // NetmonSupported
        FALSE,                                      // PhySpecific
        sizeof(NDIS_DEVICE_POWER_STATE)             // MinBufferSize
    },
    {
        OID_PNP_QUERY_POWER,                        // Oid
        FALSE,                                      // InitSettable
        FALSE,                                      // OpSettable
        TRUE,                                       // Queryable
        TRUE,                                       // ExtSTASupported
        TRUE,                                       // ExtAPSupported
        TRUE,                                       // NetmonSupported
        FALSE,                                      // PhySpecific
        sizeof(NDIS_DEVICE_POWER_STATE)             // MinBufferSize
    },

    // Operation Oids
    {
        OID_DOT11_CURRENT_ADDRESS,                  // Oid
        FALSE,                                      // InitSettable
        FALSE,                                      // OpSettable
        TRUE,                                       // Queryable
        TRUE,                                       // ExtSTASupported
        TRUE,                                       // ExtAPSupported
        FALSE,                                      // NetmonSupported
        FALSE,                                      // PhySpecific
        sizeof(DOT11_MAC_ADDRESS)                   // MinBufferSize
    },
    {
        OID_DOT11_CURRENT_OPERATION_MODE,           // Oid
        TRUE,                                       // InitSettable
        FALSE,                                      // OpSettable
        TRUE,                                       // Queryable
        TRUE,                                       // ExtSTASupported
        TRUE,                                       // ExtAPSupported
        TRUE,                                       // NetmonSupported
        FALSE,                                      // PhySpecific
        sizeof(DOT11_CURRENT_OPERATION_MODE)        // MinBufferSize
    },
    {
        OID_DOT11_CURRENT_OPTIONAL_CAPABILITY,      // Oid
        FALSE,                                      // InitSettable
        FALSE,                                      // OpSettable
        TRUE,                                       // Queryable
        TRUE,                                       // ExtSTASupported
        TRUE,                                       // ExtAPSupported
        FALSE,                                      // NetmonSupported
        FALSE,                                      // PhySpecific
        sizeof(DOT11_CURRENT_OPTIONAL_CAPABILITY)   // MinBufferSize
    },
    {
        OID_DOT11_DATA_RATE_MAPPING_TABLE,          // Oid
        FALSE,                                      // InitSettable
        FALSE,                                      // OpSettable
        TRUE,                                       // Queryable
        TRUE,                                       // ExtSTASupported
        TRUE,                                       // ExtAPSupported
        FALSE,                                      // NetmonSupported
        FALSE,                                      // PhySpecific
        sizeof(DOT11_DATA_RATE_MAPPING_TABLE)       // MinBufferSize
    },
    {
        OID_DOT11_MAXIMUM_LIST_SIZE,                // Oid
        FALSE,                                      // InitSettable
        FALSE,                                      // OpSettable
        TRUE,                                       // Queryable
        TRUE,                                       // ExtSTASupported
        TRUE,                                       // ExtAPSupported
        FALSE,                                      // NetmonSupported
        FALSE,                                      // PhySpecific
        sizeof(ULONG)                               // MinBufferSize
    },
    {
        OID_DOT11_MPDU_MAX_LENGTH,                  // Oid
        FALSE,                                      // InitSettable
        FALSE,                                      // OpSettable
        TRUE,                                       // Queryable
        TRUE,                                       // ExtSTASupported
        TRUE,                                       // ExtAPSupported
        FALSE,                                      // NetmonSupported
        FALSE,                                      // PhySpecific
        sizeof(ULONG)                               // MinBufferSize
    },
    {
        OID_DOT11_MULTICAST_LIST,                   // Oid
        TRUE,                                       // InitSettable
        TRUE,                                       // OpSettable
        TRUE,                                       // Queryable
        TRUE,                                       // ExtSTASupported
        TRUE,                                       // ExtAPSupported
        FALSE,                                      // NetmonSupported
        FALSE,                                      // PhySpecific
        0                                           // MinBufferSize // Variable
    },
    {
        OID_DOT11_NIC_POWER_STATE,                  // Oid: msDot11NICPowerState
        TRUE,                                       // InitSettable
        TRUE,                                       // OpSettable
        TRUE,                                       // Queryable
        TRUE,                                       // ExtSTASupported
        TRUE,                                       // ExtAPSupported
        FALSE,                                      // NetmonSupported
        FALSE,                                      // PhySpecific
        sizeof(BOOLEAN)                             // MinBufferSize
    },
    {
        OID_DOT11_OPERATION_MODE_CAPABILITY,        // Oid
        FALSE,                                      // InitSettable
        FALSE,                                      // OpSettable
        TRUE,                                       // Queryable
        TRUE,                                       // ExtSTASupported
        TRUE,                                       // ExtAPSupported
        TRUE,                                       // NetmonSupported (need not be supported, but is queried)
        FALSE,                                      // PhySpecific
        sizeof(DOT11_OPERATION_MODE_CAPABILITY)     // MinBufferSize
    },
    {
        OID_DOT11_OPTIONAL_CAPABILITY,              // Oid
        FALSE,                                      // InitSettable
        FALSE,                                      // OpSettable
        TRUE,                                       // Queryable
        TRUE,                                       // ExtSTASupported
        TRUE,                                       // ExtAPSupported
        FALSE,                                      // NetmonSupported
        FALSE,                                      // PhySpecific
        sizeof(DOT11_OPTIONAL_CAPABILITY)           // MinBufferSize
    },
    {
        OID_DOT11_PERMANENT_ADDRESS,                // Oid
        FALSE,                                      // InitSettable
        FALSE,                                      // OpSettable
        TRUE,                                       // Queryable
        TRUE,                                       // ExtSTASupported
        TRUE,                                       // ExtAPSupported
        FALSE,                                      // NetmonSupported
        FALSE,                                      // PhySpecific
        sizeof(DOT11_MAC_ADDRESS)                   // MinBufferSize
    },
    {
        OID_DOT11_RECV_SENSITIVITY_LIST,            // Oid
        FALSE,                                      // InitSettable // Method
        FALSE,                                      // OpSettable   // Method
        FALSE,                                      // Queryable
        TRUE,                                       // ExtSTASupported
        TRUE,                                       // ExtAPSupported
        FALSE,                                      // NetmonSupported
        FALSE,                                      // PhySpecific
        sizeof(DOT11_RECV_SENSITIVITY_LIST)         // MinBufferSize
    },
    {
        OID_DOT11_RESET_REQUEST,                    // Oid
        FALSE,                                      // InitSettable // Method
        FALSE,                                      // OpSettable   // Method
        FALSE,                                      // Queryable
        TRUE,                                       // ExtSTASupported
        TRUE,                                       // ExtAPSupported
        TRUE,                                       // NetmonSupported
        FALSE,                                      // PhySpecific
        MIN(sizeof(DOT11_RESET_REQUEST), sizeof(DOT11_STATUS_INDICATION))   // MinBufferSize
    },
    {
        OID_DOT11_RF_USAGE,                         // Oid
        FALSE,                                      // InitSettable
        FALSE,                                      // OpSettable
        TRUE,                                       // Queryable
        TRUE,                                       // ExtSTASupported
        TRUE,                                       // ExtAPSupported
        FALSE,                                      // NetmonSupported
        FALSE,                                      // PhySpecific
        sizeof(ULONG)                               // MinBufferSize
    },
    {
        OID_DOT11_SCAN_REQUEST,                     // Oid
        TRUE,                                       // InitSettable
        TRUE,                                       // OpSettable
        FALSE,                                      // Queryable
        TRUE,                                       // ExtSTASupported
        TRUE,                                       // ExtAPSupported
        FALSE,                                      // NetmonSupported
        FALSE,                                      // PhySpecific
        FIELD_OFFSET(DOT11_SCAN_REQUEST_V2, ucBuffer)   // MinBufferSize
    },
    {
        OID_DOT11_SUPPORTED_DATA_RATES_VALUE,       // Oid
        FALSE,                                      // InitSettable
        FALSE,                                      // OpSettable
        TRUE,                                       // Queryable
        TRUE,                                       // ExtSTASupported
        TRUE,                                       // ExtAPSupported
        FALSE,                                      // NetmonSupported
        FALSE,                                      // PhySpecific
        sizeof(DOT11_SUPPORTED_DATA_RATES_VALUE_V2),// MinBufferSize
    },
    {
        OID_DOT11_SUPPORTED_PHY_TYPES,              // Oid: msDot11SupportedPhyTypes
        FALSE,                                      // InitSettable
        FALSE,                                      // OpSettable
        TRUE,                                       // Queryable
        TRUE,                                       // ExtSTASupported
        TRUE,                                       // ExtAPSupported
        FALSE,                                      // NetmonSupported
        FALSE,                                      // PhySpecific
        sizeof(DOT11_SUPPORTED_PHY_TYPES)           // MinBufferSize
    },
    {
        OID_DOT11_SUPPORTED_POWER_LEVELS,           // Oid
        FALSE,                                      // InitSettable
        FALSE,                                      // OpSettable
        TRUE,                                       // Queryable
        TRUE,                                       // ExtSTASupported
        TRUE,                                       // ExtAPSupported
        FALSE,                                      // NetmonSupported
        FALSE,                                      // PhySpecific
        sizeof(DOT11_SUPPORTED_POWER_LEVELS)        // MinBufferSize
    },
    {
        OID_DOT11_SUPPORTED_RX_ANTENNA,             // Oid
        FALSE,                                      // InitSettable
        FALSE,                                      // OpSettable
        TRUE,                                       // Queryable
        TRUE,                                       // ExtSTASupported
        TRUE,                                       // ExtAPSupported
        FALSE,                                      // NetmonSupported
        FALSE,                                      // PhySpecific
        sizeof(DOT11_SUPPORTED_ANTENNA_LIST)        // MinBufferSize
    },
    {
        OID_DOT11_SUPPORTED_TX_ANTENNA,             // Oid
        FALSE,                                      // InitSettable
        FALSE,                                      // OpSettable
        TRUE,                                       // Queryable
        TRUE,                                       // ExtSTASupported
        TRUE,                                       // ExtAPSupported
        FALSE,                                      // NetmonSupported
        FALSE,                                      // PhySpecific
        sizeof(DOT11_SUPPORTED_ANTENNA_LIST)        // MinBufferSize
    },

    // MIB Oids
    {
        OID_DOT11_BEACON_PERIOD,                    // Oid: dot11BeaconPeriod
        TRUE,                                       // InitSettable
        FALSE,                                      // OpSettable
        TRUE,                                       // Queryable
        TRUE,                                       // ExtSTASupported
        TRUE,                                       // ExtAPSupported
        FALSE,                                      // NetmonSupported
        FALSE,                                      // PhySpecific
        sizeof(ULONG)                               // MinBufferSize
    },
    {
        OID_DOT11_CCA_MODE_SUPPORTED,               // Oid: dot11CCAModeSupported
        FALSE,                                      // InitSettable
        FALSE,                                      // OpSettable
        TRUE,                                       // Queryable
        TRUE,                                       // ExtSTASupported
        TRUE,                                       // ExtAPSupported
        FALSE,                                      // NetmonSupported
        TRUE,                                       // PhySpecific
        sizeof(ULONG)                               // MinBufferSize
    },
    {
        OID_DOT11_CF_POLLABLE,                      // Oid: dot11CFPollable
        FALSE,                                      // InitSettable
        FALSE,                                      // OpSettable
        TRUE,                                       // Queryable
        TRUE,                                       // ExtSTASupported
        TRUE,                                       // ExtAPSupported
        FALSE,                                      // NetmonSupported
        FALSE,                                      // PhySpecific
        sizeof(BOOLEAN)                             // MinBufferSize
    },
    {
        OID_DOT11_CHANNEL_AGILITY_ENABLED,          // Oid: dot11ChannelAgilityEnabled
        FALSE,                                      // InitSettable
        FALSE,                                      // OpSettable
        TRUE,                                       // Queryable
        TRUE,                                       // ExtSTASupported
        TRUE,                                       // ExtAPSupported
        FALSE,                                      // NetmonSupported
        TRUE,                                       // PhySpecific
        sizeof(BOOLEAN)                             // MinBufferSize
    },
    {
        OID_DOT11_CHANNEL_AGILITY_PRESENT,          // Oid: dot11ChannelAgilityPresent
        FALSE,                                      // InitSettable
        FALSE,                                      // OpSettable
        TRUE,                                       // Queryable
        TRUE,                                       // ExtSTASupported
        TRUE,                                       // ExtAPSupported
        FALSE,                                      // NetmonSupported
        TRUE,                                       // PhySpecific
        sizeof(BOOLEAN)                             // MinBufferSize
    },
    {
        OID_DOT11_COUNTRY_STRING,                   // Oid: dot11CountryString
        FALSE,                                      // InitSettable
        FALSE,                                      // OpSettable
        TRUE,                                       // Queryable
        TRUE,                                       // ExtSTASupported
        TRUE,                                       // ExtAPSupported
        FALSE,                                      // NetmonSupported
        FALSE,                                      // PhySpecific
        sizeof(DOT11_COUNTRY_OR_REGION_STRING)      // MinBufferSize
    },
    {
        OID_DOT11_CURRENT_CCA_MODE,                 // Oid: dot11CurrentCCAMode
        FALSE,                                      // InitSettable
        FALSE,                                      // OpSettable
        TRUE,                                       // Queryable
        TRUE,                                       // ExtSTASupported
        TRUE,                                       // ExtAPSupported
        FALSE,                                      // NetmonSupported
        TRUE,                                       // PhySpecific
        sizeof(ULONG)                               // MinBufferSize
    },
    {
        OID_DOT11_CURRENT_CHANNEL,                  // Oid: dot11CurrentChannel
        TRUE,                                       // InitSettable
        TRUE, //2008.08.19, haich.	//FALSE, // OpSettable
        TRUE,                                       // Queryable
        TRUE,                                       // ExtSTASupported
        TRUE,                                       // ExtAPSupported
        TRUE,                                       // NetmonSupported
        TRUE,                                       // PhySpecific
        sizeof(ULONG)                               // MinBufferSize
    },
    {
        OID_DOT11_CURRENT_FREQUENCY,                // Oid: dot11CurrentFrequency
        TRUE,                                       // InitSettable
        FALSE,                                      // OpSettable
        TRUE,                                       // Queryable
        TRUE,                                       // ExtSTASupported
        TRUE,                                       // ExtAPSupported
        TRUE,                                       // NetmonSupported
        TRUE,                                       // PhySpecific
        sizeof(ULONG)                               // MinBufferSize
    },
    {
        OID_DOT11_CURRENT_REG_DOMAIN,               // Oid: dot11CurrentRegDomain
        FALSE,                                      // InitSettable
        FALSE,                                      // OpSettable
        TRUE,                                       // Queryable
        TRUE,                                       // ExtSTASupported
        TRUE,                                       // ExtAPSupported
        FALSE,                                      // NetmonSupported
        FALSE,                                      // PhySpecific
        sizeof(ULONG)                               // MinBufferSize
    },
    {
        OID_DOT11_CURRENT_TX_POWER_LEVEL,           // Oid: dot11CurrentTxPowerLevel
        FALSE,                                      // InitSettable
        FALSE,                                      // OpSettable
        TRUE,                                       // Queryable
        TRUE,                                       // ExtSTASupported
        TRUE,                                       // ExtAPSupported
        FALSE,                                      // NetmonSupported
        FALSE,                                      // PhySpecific
        sizeof(ULONG)                               // MinBufferSize
    },
    {
        OID_DOT11_DIVERSITY_SELECTION_RX,           // Oid: dot11DiversitySelectionRx
        FALSE,                                      // InitSettable
        FALSE,                                      // OpSettable
        TRUE,                                       // Queryable
        TRUE,                                       // ExtSTASupported
        TRUE,                                       // ExtAPSupported
        FALSE,                                      // NetmonSupported
        FALSE,                                      // PhySpecific
        sizeof(DOT11_DIVERSITY_SELECTION_RX_LIST)   // MinBufferSize
    },
    {
        OID_DOT11_DIVERSITY_SUPPORT,                // Oid: dot11DiversitySupport 
        FALSE,                                      // InitSettable
        FALSE,                                      // OpSettable
        TRUE,                                       // Queryable
        TRUE,                                       // ExtSTASupported
        TRUE,                                       // ExtAPSupported
        FALSE,                                      // NetmonSupported
        FALSE,                                      // PhySpecific
        sizeof(DOT11_DIVERSITY_SUPPORT)             // MinBufferSize
    },
    {
        OID_DOT11_DSSS_OFDM_OPTION_ENABLED,         // Oid: dot11DSSSOFDMOptionEnabled
        FALSE,                                      // InitSettable
        FALSE,                                      // OpSettable
        TRUE,                                       // Queryable
        TRUE,                                       // ExtSTASupported
        TRUE,                                       // ExtAPSupported
        FALSE,                                      // NetmonSupported
        TRUE,                                       // PhySpecific
        sizeof(BOOLEAN)                             // MinBufferSize
    },
    {
        OID_DOT11_DSSS_OFDM_OPTION_IMPLEMENTED,     // Oid: dot11DSSSOFDMOptionImplemented
        FALSE,                                      // InitSettable
        FALSE,                                      // OpSettable
        TRUE,                                       // Queryable
        TRUE,                                       // ExtSTASupported
        TRUE,                                       // ExtAPSupported
        FALSE,                                      // NetmonSupported
        TRUE,                                       // PhySpecific
        sizeof(BOOLEAN)                             // MinBufferSize
    },
    {
        OID_DOT11_ED_THRESHOLD,                     // Oid: dot11EDThreshold
        FALSE,                                      // InitSettable
        FALSE,                                      // OpSettable
        TRUE,                                       // Queryable
        TRUE,                                       // ExtSTASupported
        TRUE,                                       // ExtAPSupported
        FALSE,                                      // NetmonSupported
        TRUE,                                       // PhySpecific
        sizeof(ULONG)                               // MinBufferSize
    },
    {
        OID_DOT11_ERP_PBCC_OPTION_ENABLED,          // Oid: dot11ERPBCCOptionEnabled
        FALSE,                                      // InitSettable
        FALSE,                                      // OpSettable
        TRUE,                                       // Queryable
        TRUE,                                       // ExtSTASupported
        TRUE,                                       // ExtAPSupported
        FALSE,                                      // NetmonSupported
        TRUE,                                       // PhySpecific
        sizeof(BOOLEAN)                             // MinBufferSize
    },
    {
        OID_DOT11_ERP_PBCC_OPTION_IMPLEMENTED,      // Oid: dot11ERPPBCCOptionImplemented
        FALSE,                                      // InitSettable
        FALSE,                                      // OpSettable
        TRUE,                                       // Queryable
        TRUE,                                       // ExtSTASupported
        TRUE,                                       // ExtAPSupported
        FALSE,                                      // NetmonSupported
        TRUE,                                       // PhySpecific
        sizeof(BOOLEAN)                             // MinBufferSize
    },
    {
        OID_DOT11_FRAGMENTATION_THRESHOLD,          // Oid: dot11FragmentationThreshold
        TRUE,                                       // InitSettable
        TRUE,                                       // OpSettable
        TRUE,                                       // Queryable
        TRUE,                                       // ExtSTASupported
        TRUE,                                       // ExtAPSupported
        FALSE,                                      // NetmonSupported
        FALSE,                                      // PhySpecific
        sizeof(ULONG)                               // MinBufferSize
    },
    {
        OID_DOT11_FREQUENCY_BANDS_SUPPORTED,        // Oid: dot11FrequencyBandsSupported
        FALSE,                                      // InitSettable
        FALSE,                                      // OpSettable
        TRUE,                                       // Queryable
        TRUE,                                       // ExtSTASupported
        TRUE,                                       // ExtAPSupported
        FALSE,                                      // NetmonSupported
        TRUE,                                       // PhySpecific
        sizeof(ULONG)                               // MinBufferSize
    },
    {
        OID_DOT11_LONG_RETRY_LIMIT,                 // Oid: dot11LongtRetryLimit
        FALSE,                                      // InitSettable
        FALSE,                                      // OpSettable
        TRUE,                                       // Queryable
        TRUE,                                       // ExtSTASupported
        TRUE,                                       // ExtAPSupported
        FALSE,                                      // NetmonSupported
        FALSE,                                      // PhySpecific
        sizeof(ULONG)                               // MinBufferSize
    },
    {
        OID_DOT11_MAC_ADDRESS,                      // Oid: dot11MACAddress
        FALSE,                                      // InitSettable
        FALSE,                                      // OpSettable
        TRUE,                                       // Queryable
        TRUE,                                       // ExtSTASupported
        TRUE,                                       // ExtAPSupported
        FALSE,                                      // NetmonSupported
        FALSE,                                      // PhySpecific
        sizeof(DOT11_MAC_ADDRESS)                   // MinBufferSize
    },
    {
        OID_DOT11_MAX_RECEIVE_LIFETIME,             // Oid: dot11MaxReceiveLifetime
        FALSE,                                      // InitSettable
        FALSE,                                      // OpSettable
        TRUE,                                       // Queryable
        TRUE,                                       // ExtSTASupported
        TRUE,                                       // ExtAPSupported
        FALSE,                                      // NetmonSupported
        FALSE,                                      // PhySpecific
        sizeof(ULONG)                               // MinBufferSize
    },
    {
        OID_DOT11_MAX_TRANSMIT_MSDU_LIFETIME,       // Oid: dot11MaxTransmitMSDULifetime
        FALSE,                                      // InitSettable
        FALSE,                                      // OpSettable
        TRUE,                                       // Queryable
        TRUE,                                       // ExtSTASupported
        TRUE,                                       // ExtAPSupported
        FALSE,                                      // NetmonSupported
        FALSE,                                      // PhySpecific
        sizeof(ULONG)                               // MinBufferSize
    },
    {
        OID_DOT11_MULTI_DOMAIN_CAPABILITY_ENABLED,  // Oid: dot11MultiDomainCapabilityEnabled
        TRUE,                                       // InitSettable
        FALSE,                                      // OpSettable
        TRUE,                                       // Queryable
        TRUE,                                       // ExtSTASupported
        TRUE,                                       // ExtAPSupported
        FALSE,                                      // NetmonSupported
        FALSE,                                      // PhySpecific
        sizeof(BOOLEAN)                             // MinBufferSize
    },
    {
        OID_DOT11_MULTI_DOMAIN_CAPABILITY_IMPLEMENTED,  // Oid: dot11MultiDomainCapabilityImplemented
        FALSE,                                      // InitSettable
        FALSE,                                      // OpSettable
        TRUE,                                       // Queryable
        TRUE,                                       // ExtSTASupported
        TRUE,                                       // ExtAPSupported
        FALSE,                                      // NetmonSupported
        FALSE,                                      // PhySpecific
        sizeof(BOOLEAN)                             // MinBufferSize
    },
    {
        OID_DOT11_OPERATIONAL_RATE_SET,             // Oid: dot11OperationalRateSet
        TRUE,                                       // InitSettable
        FALSE,                                      // OpSettable
        TRUE,                                       // Queryable
        TRUE,                                       // ExtSTASupported
        TRUE,                                       // ExtAPSupported
        FALSE,                                      // NetmonSupported
        FALSE,                                      // PhySpecific
        sizeof(DOT11_RATE_SET)                      // MinBufferSize
    },
    {
        OID_DOT11_PBCC_OPTION_IMPLEMENTED,          // Oid: dot11PBCCOptionImplemented
        FALSE,                                      // InitSettable
        FALSE,                                      // OpSettable
        TRUE,                                       // Queryable
        TRUE,                                       // ExtSTASupported
        TRUE,                                       // ExtAPSupported
        FALSE,                                      // NetmonSupported
        TRUE,                                       // PhySpecific
        sizeof(BOOLEAN)                             // MinBufferSize
    },
    {
        OID_DOT11_REG_DOMAINS_SUPPORT_VALUE,        // Oid: dot11RegDomainValue
        FALSE,                                      // InitSettable
        FALSE,                                      // OpSettable
        TRUE,                                       // Queryable
        TRUE,                                       // ExtSTASupported
        TRUE,                                       // ExtAPSupported
        FALSE,                                      // NetmonSupported
        FALSE,                                      // PhySpecific
        sizeof(DOT11_REG_DOMAINS_SUPPORT_VALUE)     // MinBufferSize
    },
    {
        OID_DOT11_RTS_THRESHOLD,                    // Oid: dot11RTSThreshold
        TRUE,                                       // InitSettable
        TRUE,                                       // OpSettable
        TRUE,                                       // Queryable
        TRUE,                                       // ExtSTASupported
        TRUE,                                       // ExtAPSupported
        FALSE,                                      // NetmonSupported
        FALSE,                                      // PhySpecific
        sizeof(ULONG)                               // MinBufferSize
    },
    {
        OID_DOT11_SHORT_PREAMBLE_OPTION_IMPLEMENTED,// Oid: dot11ShortPreambleOptionImplemented
        FALSE,                                      // InitSettable
        FALSE,                                      // OpSettable
        TRUE,                                       // Queryable
        TRUE,                                       // ExtSTASupported
        TRUE,                                       // ExtAPSupported
        FALSE,                                      // NetmonSupported
        TRUE,                                       // PhySpecific
        sizeof(BOOLEAN)                             // MinBufferSize
    },
    {
        OID_DOT11_SHORT_RETRY_LIMIT,                // Oid: dot11ShortRetryLimit
        FALSE,                                      // InitSettable
        FALSE,                                      // OpSettable
        TRUE,                                       // Queryable
        TRUE,                                       // ExtSTASupported
        TRUE,                                       // ExtAPSupported
        FALSE,                                      // NetmonSupported
        FALSE,                                      // PhySpecific
        sizeof(ULONG)                               // MinBufferSize
    },
    {
        OID_DOT11_SHORT_SLOT_TIME_OPTION_ENABLED,   // Oid: dot11ShortSlotTimeOptionEnabled
        FALSE,                                      // InitSettable
        FALSE,                                      // OpSettable
        TRUE,                                       // Queryable
        TRUE,                                       // ExtSTASupported
        TRUE,                                       // ExtAPSupported
        FALSE,                                      // NetmonSupported
        TRUE ,                                      // PhySpecific
        sizeof(BOOLEAN)                             // MinBufferSize
    },
    {
        OID_DOT11_SHORT_SLOT_TIME_OPTION_IMPLEMENTED, // Oid: dot11ShortSlotTimeOptionImplemented
        FALSE,                                      // InitSettable
        FALSE,                                      // OpSettable
        TRUE,                                       // Queryable
        TRUE,                                       // ExtSTASupported
        TRUE,                                       // ExtAPSupported
        FALSE,                                      // NetmonSupported
        TRUE,                                       // PhySpecific
        sizeof(BOOLEAN)                             // MinBufferSize
    },
    {
        OID_DOT11_STATION_ID,                       // Oid: dot11StationID
        FALSE,                                      // InitSettable
        FALSE,                                      // OpSettable
        TRUE,                                       // Queryable
        TRUE,                                       // ExtSTASupported
        TRUE,                                       // ExtAPSupported
        FALSE,                                      // NetmonSupported
        FALSE,                                      // PhySpecific
        sizeof(DOT11_MAC_ADDRESS)                   // MinBufferSize
    },
    {
        OID_DOT11_TEMP_TYPE,                        // Oid: dot11TempType
        FALSE,                                      // InitSettable
        FALSE,                                      // OpSettable
        TRUE,                                       // Queryable
        TRUE,                                       // ExtSTASupported
        TRUE,                                       // ExtAPSupported
        FALSE,                                      // NetmonSupported
        FALSE,                                      // PhySpecific
        sizeof(DOT11_TEMP_TYPE)                     // MinBufferSize
    },

    // ExtSTA Operation OIDs
    {
        OID_DOT11_ACTIVE_PHY_LIST,                  // Oid: msDot11ActivePhyList
        FALSE,                                      // InitSettable
        FALSE,                                      // OpSettable
        TRUE,                                       // Queryable
        TRUE,                                       // ExtSTASupported
        FALSE,                                      // ExtAPSupported
        FALSE,                                      // NetmonSupported
        FALSE,                                      // PhySpecific
        sizeof(DOT11_PHY_ID_LIST)                   // MinBufferSize
    },
    {
        OID_DOT11_ATIM_WINDOW,                      // Oid
        TRUE,                                       // InitSettable
        FALSE,                                      // OpSettable
        TRUE,                                       // Queryable
        TRUE,                                       // ExtSTASupported
        FALSE,                                      // ExtAPSupported
        FALSE,                                      // NetmonSupported
        FALSE,                                      // PhySpecific
        sizeof(ULONG)                               // MinBufferSize
    },
    {
        OID_DOT11_AUTO_CONFIG_ENABLED,              // Oid: msDot11AutoConfigEnabled
        TRUE,                                       // InitSettable
        FALSE,                                      // OpSettable
        TRUE,                                       // Queryable
        TRUE,                                       // ExtSTASupported
        TRUE,                                       // ExtAPSupported
        TRUE,                                       // NetmonSupported (need not be supported, but is set)
        FALSE,                                      // PhySpecific
        sizeof(ULONG)                               // MinBufferSize
    },
    {
        OID_DOT11_CIPHER_DEFAULT_KEY,               // Oid
        TRUE,                                       // InitSettable
        TRUE,                                       // OpSettable
        FALSE,                                      // Queryable
        TRUE,                                       // ExtSTASupported
        TRUE,                                       // ExtAPSupported
        FALSE,                                      // NetmonSupported
        FALSE,                                      // PhySpecific
        FIELD_OFFSET(DOT11_CIPHER_DEFAULT_KEY_VALUE, ucKey)     // MinBufferSize
    },
    {
        OID_DOT11_CIPHER_DEFAULT_KEY_ID,            // Oid: dot11DefaultKeyID
        TRUE,                                       // InitSettable
        TRUE,                                       // OpSettable
        TRUE,                                       // Queryable
        TRUE,                                       // ExtSTASupported
        TRUE,                                       // ExtAPSupported
        FALSE,                                      // NetmonSupported
        FALSE,                                      // PhySpecific
        sizeof(ULONG)                               // MinBufferSize
    },
    {
        OID_DOT11_CIPHER_KEY_MAPPING_KEY,           // Oid
        TRUE,                                       // InitSettable
        TRUE,                                       // OpSettable
        FALSE,                                      // Queryable
        TRUE,                                       // ExtSTASupported
        TRUE,                                       // ExtAPSupported
        FALSE,                                      // NetmonSupported
        FALSE,                                      // PhySpecific
        FIELD_OFFSET(DOT11_BYTE_ARRAY, ucBuffer)    // MinBufferSize
    },
    {
        OID_DOT11_CONNECT_REQUEST,                  // Oid
        TRUE,                                       // InitSettable
        FALSE,                                      // OpSettable
        FALSE,                                      // Queryable
        TRUE,                                       // ExtSTASupported
        FALSE,                                      // ExtAPSupported
        FALSE,                                      // NetmonSupported
        FALSE,                                      // PhySpecific
        0                                           // MinBufferSize
    },
    {
        OID_DOT11_CURRENT_PHY_ID,                   // Oid: msDot11CurrentPhyID
        TRUE,                                       // InitSettable
        TRUE,                                       // OpSettable
        TRUE,                                       // Queryable
        TRUE,                                       // ExtSTASupported
        TRUE,                                       // ExtAPSupported
        TRUE,                                       // NetmonSupported
        FALSE,                                      // PhySpecific
        sizeof(ULONG)                               // MinBufferSize
    },
    {
        OID_DOT11_DESIRED_BSS_TYPE,                 // Oid: dot11DesiredBSSType
        TRUE,                                       // InitSettable
        FALSE,                                      // OpSettable
        TRUE,                                       // Queryable
        TRUE,                                       // ExtSTASupported
        FALSE,                                      // ExtAPSupported
        FALSE,                                      // NetmonSupported
        FALSE,                                      // PhySpecific
        sizeof(DOT11_BSS_TYPE)                      // MinBufferSize
    },
    {
        OID_DOT11_DESIRED_BSSID_LIST,               // Oid: msDot11DesiredBSSIDList
        TRUE,                                       // InitSettable
        FALSE,                                      // OpSettable
        TRUE,                                       // Queryable
        TRUE,                                       // ExtSTASupported
        FALSE,                                      // ExtAPSupported
        FALSE,                                      // NetmonSupported
        FALSE,                                      // PhySpecific
        FIELD_OFFSET(DOT11_BSSID_LIST, BSSIDs)      // MinBufferSize
    },
    {
        OID_DOT11_DESIRED_PHY_LIST,                 // Oid: msDot11DesiredPhyList
        TRUE,                                       // InitSettable
        FALSE,                                      // OpSettable
        TRUE,                                       // Queryable
        TRUE,                                       // ExtSTASupported
        TRUE,                                       // ExtAPSupported
        TRUE,                                       // NetmonSupported
        FALSE,                                      // PhySpecific
        FIELD_OFFSET(DOT11_PHY_ID_LIST, dot11PhyId)     // MinBufferSize
    },
    {
        OID_DOT11_DESIRED_SSID_LIST,                // Oid: msDot11DesiredSSIDList
        TRUE,                                       // InitSettable
        FALSE,                                      // OpSettable
        TRUE,                                       // Queryable
        TRUE,                                       // ExtSTASupported
        TRUE,                                       // ExtAPSupported
        FALSE,                                      // NetmonSupported
        FALSE,                                      // PhySpecific
        FIELD_OFFSET(DOT11_SSID_LIST, SSIDs)        // MinBufferSize
    },
    {
        OID_DOT11_DISCONNECT_REQUEST,               // Oid
        FALSE,                                      // InitSettable
        TRUE,                                       // OpSettable
        FALSE,                                      // Queryable
        TRUE,                                       // ExtSTASupported
        FALSE,                                      // ExtAPSupported
        FALSE,                                      // NetmonSupported
        FALSE,                                      // PhySpecific
        0                                           // MinBufferSize
    },
    {
        OID_DOT11_ENABLED_AUTHENTICATION_ALGORITHM, // Oid: msDot11EnabledAuthAlgo
        TRUE,                                       // InitSettable
        FALSE,                                      // OpSettable
        TRUE,                                       // Queryable
        TRUE,                                       // ExtSTASupported
        TRUE,                                       // ExtAPSupported
        FALSE,                                      // NetmonSupported
        FALSE,                                      // PhySpecific
        FIELD_OFFSET(DOT11_AUTH_ALGORITHM_LIST, AlgorithmIds)    // MinBufferSize
    },
    {
        OID_DOT11_ENABLED_MULTICAST_CIPHER_ALGORITHM,   // Oid: msDot11EnabledMulticastCipherAlgo
        TRUE,                                       // InitSettable
        FALSE,                                      // OpSettable
        TRUE,                                       // Queryable
        TRUE,                                       // ExtSTASupported
        TRUE,                                       // ExtAPSupported
        FALSE,                                      // NetmonSupported
        FALSE,                                      // PhySpecific
        FIELD_OFFSET(DOT11_CIPHER_ALGORITHM_LIST, AlgorithmIds)    // MinBufferSize
    },
    {
        OID_DOT11_ENABLED_UNICAST_CIPHER_ALGORITHM, // Oid: msDot11EnabledUnicastCipherAlgo
        TRUE,                                       // InitSettable
        FALSE,                                      // OpSettable
        TRUE,                                       // Queryable
        TRUE,                                       // ExtSTASupported
        TRUE,                                       // ExtAPSupported
        FALSE,                                      // NetmonSupported
        FALSE,                                      // PhySpecific
        FIELD_OFFSET(DOT11_CIPHER_ALGORITHM_LIST, AlgorithmIds)    // MinBufferSize
    },
    {
        OID_DOT11_ENUM_ASSOCIATION_INFO,            // Oid
        FALSE,                                      // InitSettable
        FALSE,                                      // OpSettable
        TRUE,                                       // Queryable
        TRUE,                                       // ExtSTASupported
        FALSE,                                      // ExtAPSupported
        FALSE,                                      // NetmonSupported
        FALSE,                                      // PhySpecific
        sizeof(DOT11_ASSOCIATION_INFO_LIST)         // MinBufferSize
    },
    {
        OID_DOT11_ENUM_BSS_LIST,                    // Oid
        FALSE,                                      // InitSettable // Method
        FALSE,                                      // OpSettable    // Method
        FALSE,                                      // Queryable
        TRUE,                                       // ExtSTASupported
        TRUE,                                       // ExtAPSupported
        FALSE,                                      // NetmonSupported
        FALSE,                                      // PhySpecific
        FIELD_OFFSET(DOT11_BYTE_ARRAY, ucBuffer)    // MinBufferSize
    },
    {
        OID_DOT11_EXCLUDE_UNENCRYPTED,              // Oid: dot11ExcludeUnencrypted
        TRUE,                                       // InitSettable
        FALSE,                                      // OpSettable
        TRUE,                                       // Queryable
        TRUE,                                       // ExtSTASupported
        TRUE,                                       // ExtAPSupported
        FALSE,                                      // NetmonSupported
        FALSE,                                      // PhySpecific
        sizeof(BOOLEAN)                             // MinBufferSize
    },
    {
        OID_DOT11_EXCLUDED_MAC_ADDRESS_LIST,        // Oid: msDot11ExcludedMacAddressList
        TRUE,                                       // InitSettable
        TRUE,                                       // OpSettable
        TRUE,                                       // Queryable
        TRUE,                                       // ExtSTASupported
        FALSE,                                      // ExtAPSupported
        FALSE,                                      // NetmonSupported
        FALSE,                                      // PhySpecific
        FIELD_OFFSET(DOT11_MAC_ADDRESS_LIST, MacAddrs)  // MinBufferSize
    },
    {
        OID_DOT11_EXTSTA_CAPABILITY,                // Oid
        FALSE,                                      // InitSettable
        FALSE,                                      // OpSettable
        TRUE,                                       // Queryable
        TRUE,                                       // ExtSTASupported
        FALSE,                                      // ExtAPSupported
        FALSE,                                      // NetmonSupported
        FALSE,                                      // PhySpecific
        sizeof(DOT11_EXTSTA_CAPABILITY)             // MinBufferSize
    },
    {
        OID_DOT11_FLUSH_BSS_LIST,                   // Oid
        TRUE,                                       // InitSettable
        TRUE,                                       // OpSettable
        FALSE,                                      // Queryable
        TRUE,                                       // ExtSTASupported
        TRUE,                                       // ExtAPSupported
        FALSE,                                      // NetmonSupported
        FALSE,                                      // PhySpecific
        0                                           // MinBufferSize
    },
    {
        OID_DOT11_HARDWARE_PHY_STATE,               // Oid: msDot11HardwarePHYState
        FALSE,                                      // InitSettable
        FALSE,                                      // OpSettable
        TRUE,                                       // Queryable
        TRUE,                                       // ExtSTASupported
        FALSE,                                      // ExtAPSupported
        FALSE,                                      // NetmonSupported
        FALSE,                                      // PhySpecific
        sizeof(BOOLEAN)                             // MinBufferSize
    },
    {
        OID_DOT11_HIDDEN_NETWORK_ENABLED,           // Oid: msDot11HiddenNetworkEnabled
        TRUE,                                       // InitSettable
        FALSE,                                      // OpSettable
        TRUE,                                       // Queryable
        TRUE,                                       // ExtSTASupported
        FALSE,                                      // ExtAPSupported
        FALSE,                                      // NetmonSupported
        FALSE,                                      // PhySpecific
        sizeof(BOOLEAN)                             // MinBufferSize
    },
    {
        OID_DOT11_IBSS_PARAMS,                      // Oid
        TRUE,                                       // InitSettable
        FALSE,                                      // OpSettable
        TRUE,                                       // Queryable
        TRUE,                                       // ExtSTASupported
        FALSE,                                      // ExtAPSupported
        FALSE,                                      // NetmonSupported
        FALSE,                                      // PhySpecific
        sizeof(DOT11_IBSS_PARAMS)                   // MinBufferSize
    },
    {
        OID_DOT11_MEDIA_STREAMING_ENABLED,          // Oid: msDot11MediaStreamingEnabled
        TRUE,                                       // InitSettable
        TRUE,                                       // OpSettable
        TRUE,                                       // Queryable
        TRUE,                                       // ExtSTASupported
        FALSE,                                      // ExtAPSupported
        FALSE,                                      // NetmonSupported
        FALSE,                                      // PhySpecific
        sizeof(BOOLEAN)                             // MinBufferSize
    },
    {
        OID_DOT11_PMKID_LIST,                       // Oid
        TRUE,                                       // InitSettable
        TRUE,                                       // OpSettable
        TRUE,                                       // Queryable
        TRUE,                                       // ExtSTASupported
        FALSE,                                      // ExtAPSupported
        FALSE,                                      // NetmonSupported
        FALSE,                                      // PhySpecific
        FIELD_OFFSET(DOT11_PMKID_LIST,PMKIDs)       // MinBufferSize
    },
    {
        OID_DOT11_POWER_MGMT_REQUEST,               // Oid: msDot11PowerSavingLevel
        TRUE,                                       // InitSettable
        TRUE,                                       // OpSettable
        TRUE,                                       // Queryable
        TRUE,                                       // ExtSTASupported
        FALSE,                                      // ExtAPSupported
        FALSE,                                      // NetmonSupported
        FALSE,                                      // PhySpecific
        sizeof(ULONG)                               // MinBufferSize
    },
    {
        OID_DOT11_PRIVACY_EXEMPTION_LIST,           // Oid: msDot11PrivacyExemptionList
        TRUE,                                       // InitSettable
        FALSE,                                      // OpSettable
        TRUE,                                       // Queryable
        TRUE,                                       // ExtSTASupported
        TRUE,                                       // ExtAPSupported
        FALSE,                                      // NetmonSupported
        FALSE,                                      // PhySpecific
        FIELD_OFFSET(DOT11_PRIVACY_EXEMPTION_LIST, PrivacyExemptionEntries)     // MinBufferSize
    },
    {
        OID_DOT11_SAFE_MODE_ENABLED,                // Oid: msDot11SafeModeEnabled
        TRUE,                                       // InitSettable
        FALSE,                                      // OpSettable
        TRUE,                                       // Queryable
        TRUE,                                       // ExtSTASupported
        FALSE,                                      // ExtAPSupported
        FALSE,                                      // NetmonSupported
        FALSE,                                      // PhySpecific
        sizeof(BOOLEAN)                             // MinBufferSize
    },
    {
        OID_DOT11_STATISTICS,                       // Oid
        FALSE,                                      // InitSettable
        FALSE,                                      // OpSettable
        TRUE,                                       // Queryable
        TRUE,                                       // ExtSTASupported
        TRUE,                                       // ExtAPSupported
        TRUE,                                       // NetmonSupported
        FALSE,                                      // PhySpecific
        sizeof(DOT11_STATISTICS)                    // MinBufferSize
    },
    {
        OID_DOT11_SUPPORTED_MULTICAST_ALGORITHM_PAIR,   // Oid
        FALSE,                                      // InitSettable
        FALSE,                                      // OpSettable
        TRUE,                                       // Queryable
        TRUE,                                       // ExtSTASupported
        FALSE,                                      // ExtAPSupported
        FALSE,                                      // NetmonSupported
        FALSE,                                      // PhySpecific
        FIELD_OFFSET(DOT11_AUTH_CIPHER_PAIR_LIST, AuthCipherPairs)  // MinBufferSize
    },
    {
        OID_DOT11_SUPPORTED_UNICAST_ALGORITHM_PAIR, // Oid
        FALSE,                                      // InitSettable
        FALSE,                                      // OpSettable
        TRUE,                                       // Queryable
        TRUE,                                       // ExtSTASupported
        FALSE,                                      // ExtAPSupported
        FALSE,                                      // NetmonSupported
        FALSE,                                      // PhySpecific
        FIELD_OFFSET(DOT11_AUTH_CIPHER_PAIR_LIST, AuthCipherPairs)  // MinBufferSize
    },
    {
        OID_DOT11_UNICAST_USE_GROUP_ENABLED,        // Oid: msDot11UnicastUseGroupEnabled
        TRUE,                                       // InitSettable
        FALSE,                                      // OpSettable
        TRUE,                                       // Queryable
        TRUE,                                       // ExtSTASupported
        FALSE,                                      // ExtAPSupported
        FALSE,                                      // NetmonSupported
        FALSE,                                      // PhySpecific
        sizeof(BOOLEAN)                             // MinBufferSize
    },
    {
        OID_DOT11_UNREACHABLE_DETECTION_THRESHOLD,  // Oid: msDot11UnreachableDetectionThreshold
        TRUE,                                       // InitSettable
        TRUE,                                       // OpSettable
        TRUE,                                       // Queryable
        TRUE,                                       // ExtSTASupported
        FALSE,                                      // ExtAPSupported
        FALSE,                                      // NetmonSupported
        FALSE,                                      // PhySpecific
        sizeof(ULONG)                               // MinBufferSize
    },
    // ExtAP specific OIDs
    {
        OID_DOT11_DTIM_PERIOD,                      // Oid: dot11DTIMPeriod
        TRUE,                                       // InitSettable
        FALSE,                                      // OpSettable
        TRUE,                                       // Queryable
        FALSE,                                      // ExtSTASupported
        TRUE,                                       // ExtAPSupported
        FALSE,                                      // NetmonSupported
        FALSE,                                      // PhySpecific
        sizeof(ULONG)                               // MinBufferSize
    },
    {
        OID_DOT11_AVAILABLE_CHANNEL_LIST,           // Oid: msDot11AvailableChannelList
        FALSE,                                      // InitSettable
        FALSE,                                      // OpSettable
        TRUE,                                       // Queryable
        FALSE,                                      // ExtSTASupported
        TRUE,                                       // ExtAPSupported
        FALSE,                                      // NetmonSupported
        FALSE,                                      // PhySpecific
        FIELD_OFFSET(DOT11_AVAILABLE_CHANNEL_LIST, uChannelNumber)  // MinBufferSize
    },
    {
        OID_DOT11_AVAILABLE_FREQUENCY_LIST,         // Oid: msDot11AvailableFrequencyList
        FALSE,                                      // InitSettable
        FALSE,                                      // OpSettable
        TRUE,                                       // Queryable
        FALSE,                                      // ExtSTASupported
        TRUE,                                       // ExtAPSupported
        FALSE,                                      // NetmonSupported
        FALSE,                                      // PhySpecific
        FIELD_OFFSET(DOT11_AVAILABLE_FREQUENCY_LIST, uFrequencyValue)   // MinBufferSize
    },
    {
        OID_DOT11_ENUM_PEER_INFO,                   // Oid
        FALSE,                                      // InitSettable
        FALSE,                                      // OpSettable
        TRUE,                                       // Queryable
        FALSE,                                      // ExtSTASupported
        TRUE,                                       // ExtAPSupported
        FALSE,                                      // NetmonSupported
        FALSE,                                      // PhySpecific
        FIELD_OFFSET(DOT11_PEER_INFO_LIST, PeerInfo)    // MinBufferSize
    },
    {
        OID_DOT11_DISASSOCIATE_PEER_REQUEST,        // Oid
        FALSE,                                      // InitSettable
        TRUE,                                       // OpSettable
        FALSE,                                      // Queryable
        FALSE,                                      // ExtSTASupported
        TRUE,                                       // ExtAPSupported
        FALSE,                                      // NetmonSupported
        FALSE,                                      // PhySpecific
        sizeof(DOT11_DISASSOCIATE_PEER_REQUEST)     // MinBufferSize
    },
    {
        OID_DOT11_PORT_STATE_NOTIFICATION,          // Oid
        FALSE,                                      // InitSettable
        TRUE,                                       // OpSettable
        FALSE,                                      // Queryable
        FALSE,                                      // ExtSTASupported
        FALSE,                                       // ExtAPSupported
        FALSE,                                      // NetmonSupported
        FALSE,                                      // PhySpecific
        sizeof(DOT11_PORT_STATE_NOTIFICATION)       // MinBufferSize
    },
    {
        OID_DOT11_INCOMING_ASSOCIATION_DECISION,    // Oid
        FALSE,                                      // InitSettable
        TRUE,                                       // OpSettable
        FALSE,                                      // Queryable
        FALSE,                                      // ExtSTASupported
        TRUE,                                       // ExtAPSupported
        FALSE,                                      // NetmonSupported
        FALSE,                                      // PhySpecific
        sizeof(DOT11_INCOMING_ASSOC_DECISION)       // MinBufferSize
    },
    {
        OID_DOT11_ADDITIONAL_IE,                    // Oid
        TRUE,                                       // InitSettable
        TRUE,                                       // OpSettable
        TRUE,                                       // Queryable
        FALSE,                                      // ExtSTASupported
        TRUE,                                       // ExtAPSupported
        FALSE,                                      // NetmonSupported
        FALSE,                                      // PhySpecific
        sizeof(DOT11_ADDITIONAL_IE)                 // MinBufferSize
    },
    {
        OID_DOT11_WPS_ENABLED,                      // Oid
        TRUE,                                       // InitSettable
        TRUE,                                       // OpSettable
        TRUE,                                       // Queryable
        FALSE,                                      // ExtSTASupported
        TRUE,                                       // ExtAPSupported
        FALSE,                                      // NetmonSupported
        FALSE,                                      // PhySpecific
        sizeof(BOOLEAN)                             // MinBufferSize
    },
    {
        OID_DOT11_START_AP_REQUEST,                 // Oid
        TRUE,                                       // InitSettable
        FALSE,                                      // OpSettable
        FALSE,                                      // Queryable
        FALSE,                                      // ExtSTASupported
        TRUE,                                       // ExtAPSupported
        FALSE,                                      // NetmonSupported
        FALSE,                                      // PhySpecific
        0                                           // MinBufferSize
    },
/*    
    {
	OID_DOT11_AP_BEACON_MODE,                 // Oid
	TRUE,                                       // InitSettable
	TRUE,                                      // OpSettable
	TRUE,                                      // Queryable
	FALSE,                                      // ExtSTASupported
	TRUE,                                       // ExtAPSupported
	FALSE,                                      // NetmonSupported
	FALSE,                                      // PhySpecific
	sizeof(BOOLEAN)                       // MinBufferSize
    },
*/    
    {
	OID_DOT11_ASSOCIATION_PARAMS,                 // Oid
	TRUE,                                       // InitSettable
	FALSE,                                      // OpSettable
	FALSE,                                      // Queryable
	TRUE,                                      // ExtSTASupported
	FALSE,                                       // ExtAPSupported
	FALSE,                                      // NetmonSupported
	FALSE,                                      // PhySpecific
	sizeof(DOT11_ASSOCIATION_PARAMS)                       // MinBufferSize
    },
    {
	OID_PM_ADD_WOL_PATTERN,						// Oid
	TRUE,										// InitSettable
	TRUE,										// OpSettable
	FALSE,										// Queryable
	TRUE,										// ExtSTASupported
	FALSE,										// ExtAPSupported
	FALSE,										// NetmonSupported
	FALSE,										// PhySpecific
	sizeof(NDIS_PM_WOL_PATTERN) 			// MinBufferSize
    },
    {
	OID_PM_REMOVE_WOL_PATTERN,						// Oid
	TRUE,										// InitSettable
	TRUE,										// OpSettable
	FALSE,										// Queryable
	TRUE,										// ExtSTASupported
	FALSE,										// ExtAPSupported
	FALSE,										// NetmonSupported
	FALSE,										// PhySpecific
	sizeof(NDIS_PM_WOL_PATTERN) 			// MinBufferSize
    },
    {
	OID_PM_PARAMETERS,							// Oid
	TRUE,										// InitSettable
	TRUE,										// OpSettable
	TRUE,										// Queryable
	TRUE,										// ExtSTASupported
	FALSE,										// ExtAPSupported
	FALSE,										// NetmonSupported
	FALSE,										// PhySpecific
	sizeof(NDIS_PM_PARAMETERS) 			// MinBufferSize
    },
    {
	OID_PM_ADD_PROTOCOL_OFFLOAD, 					// Oid
	TRUE,										// InitSettable
	TRUE,										// OpSettable
	FALSE,										// Queryable
	TRUE,										// ExtSTASupported
	FALSE,										// ExtAPSupported
	FALSE,										// NetmonSupported
	FALSE,										// PhySpecific
	sizeof(NDIS_PM_PROTOCOL_OFFLOAD) 			// MinBufferSize
    },
    {
	OID_PM_GET_PROTOCOL_OFFLOAD, 					// Oid
	FALSE,										// InitSettable
	FALSE,										// OpSettable
	TRUE,										// Queryable
	TRUE,										// ExtSTASupported
	FALSE,										// ExtAPSupported
	FALSE,										// NetmonSupported
	FALSE,										// PhySpecific
	sizeof(NDIS_PM_PROTOCOL_OFFLOAD) 			// MinBufferSize
    },
    {
	OID_PM_REMOVE_PROTOCOL_OFFLOAD, 					// Oid
	TRUE,										// InitSettable
	TRUE,										// OpSettable
	FALSE,										// Queryable
	TRUE,										// ExtSTASupported
	FALSE,										// ExtAPSupported
	FALSE,										// NetmonSupported
	FALSE,										// PhySpecific
	sizeof(NDIS_PM_PROTOCOL_OFFLOAD) 			// MinBufferSize
    },
    {
	OID_PM_PROTOCOL_OFFLOAD_LIST, 					// Oid
	TRUE,										// InitSettable
	TRUE,										// OpSettable
	FALSE,										// Queryable
	TRUE,										// ExtSTASupported
	FALSE,										// ExtAPSupported
	FALSE,										// NetmonSupported
	FALSE,										// PhySpecific
	sizeof(NDIS_PM_PROTOCOL_OFFLOAD) 			// MinBufferSize
    },

    
	{
	OID_RECEIVE_FILTER_SET_FILTER, 					// Oid
	TRUE,										// InitSettable
	TRUE,										// OpSettable
	TRUE,										// Queryable
	TRUE,										// ExtSTASupported
	FALSE,										// ExtAPSupported
	FALSE,										// NetmonSupported
	FALSE,										// PhySpecific
	sizeof(NDIS_RECEIVE_FILTER_PARAMETERS) 			// MinBufferSize
    },	
    {
	OID_RECEIVE_FILTER_CLEAR_FILTER,						// Oid
	TRUE,										// InitSettable
	TRUE,										// OpSettable
	FALSE,										// Queryable
	TRUE,										// ExtSTASupported
	FALSE,										// ExtAPSupported
	FALSE,										// NetmonSupported
	FALSE,										// PhySpecific
	sizeof(NDIS_RECEIVE_FILTER_CLEAR_PARAMETERS) 			// MinBufferSize
    },
    {
	 OID_RECEIVE_FILTER_HARDWARE_CAPABILITIES,	// Oid
	 FALSE,                                      				// InitSettable
     FALSE,                                      				// OpSettable
     TRUE,                                       				// Queryable
     TRUE,                                      				// ExtSTASupported
     FALSE,                                       			// ExtAPSupported
     FALSE,                                      				// NetmonSupported
     FALSE,                                      				// PhySpecific
	 sizeof(NDIS_RECEIVE_FILTER_CAPABILITIES)  			// MinBufferSize
    },
    {
	 OID_RECEIVE_FILTER_CURRENT_CAPABILITIES,	// Oid
	 FALSE,                                      				// InitSettable
     FALSE,                                      				// OpSettable
     TRUE,                                       				// Queryable
     TRUE,                                      				// ExtSTASupported
     FALSE,                                       			// ExtAPSupported
     FALSE,                                      				// NetmonSupported
     FALSE,                                      				// PhySpecific
	 sizeof(NDIS_RECEIVE_FILTER_CAPABILITIES)  			// MinBufferSize
    },
};

NDIS_STATUS
N62CValidateOIDCorrectness(
	PADAPTER pTargetAdapter,
	PNDIS_OID_REQUEST NdisRequest
)
{
	NDIS_STATUS ndisStatus = NDIS_STATUS_INVALID_OID;

	PULONG							bytesNeededLocation;
	NDIS_OID						oid;
	ULONG							infoBufferLength;
	PPORT_QUERY_SET_OID_ENTRY		oidTableEntry = NULL;
	ULONG							i;
	DOT11_PHY_TYPE					currentPhy;
	PRT_NDIS62_COMMON				pNdis62Common=pTargetAdapter->pNdis62Common;
	PRT_NDIS6_COMMON				pNdisCommon = pTargetAdapter->pNdisCommon;

	oid = NdisRequest->DATA.QUERY_INFORMATION.Oid; // Oid is at same offset for all RequestTypes

	// Find the OID in the oid table
	for (i = 0; i < ((ULONG)sizeof(OidQuerySetTable)/(ULONG)sizeof(PORT_QUERY_SET_OID_ENTRY)); i++)
	{
		if (OidQuerySetTable[i].Oid == oid)
		{
			oidTableEntry = &OidQuerySetTable[i];
			break;
		}
	}

	if (oidTableEntry == NULL)
	{
		RT_TRACE(COMP_OID_QUERY | COMP_OID_SET, DBG_LOUD, ("OID 0x%08x not found in oid table\n", oid));
		return NDIS_STATUS_FAILURE;
	}

	// Verify that the OID is applicable for the current operation mode mode
	switch(pNdis62Common->PortType)
	{
		case EXTSTA_PORT:
		if (!oidTableEntry->ExtSTASupported)
		{
			RT_TRACE((COMP_OID_QUERY | COMP_OID_SET), DBG_SERIOUS, 
				("OID 0x%08x not supported in ExtSTA mode\n", oid));
			return NDIS_STATUS_INVALID_STATE;
		}
		break;


		case EXT_P2P_ROLE_PORT:
			// No Checking
		break;
/*
		case DOT11_OPERATION_MODE_NETWORK_MONITOR :
		if (!oidTableEntry->NetmonSupported)
		{
			MpTrace(COMP_OID, DBG_SERIOUS, ("OID 0x%08x not supported in Network Monitor mode\n", oid));
			return NDIS_STATUS_INVALID_STATE;
		}
		break;
*/
		default:
		// Currently we assume this is AP mode
		if (!oidTableEntry->ExtAPSupported)
		{
			RT_TRACE((COMP_OID_QUERY | COMP_OID_SET), DBG_SERIOUS, 
				("OID 0x%08x not supported in ExtAP mode\n", oid));
			return NDIS_STATUS_INVALID_STATE;
		}
		break;
	}

	// Verify if the OID is supported in current operating state
	switch(NdisRequest->RequestType)
	{
		case NdisRequestQueryInformation:
		case NdisRequestQueryStatistics:
			if (!oidTableEntry->Queryable)
			{
				RT_TRACE(COMP_OID_QUERY | COMP_OID_SET, DBG_LOUD, ("Query of OID 0x%08x not supported\n", oid));
				return NDIS_STATUS_INVALID_STATE;
			}

			// Determine buffer length that will be used later
			bytesNeededLocation = (PULONG)&NdisRequest->DATA.QUERY_INFORMATION.BytesNeeded;
			infoBufferLength = NdisRequest->DATA.QUERY_INFORMATION.InformationBufferLength;
			break;

		case NdisRequestSetInformation:
			if (pNdis62Common->CurrentOpState == INIT_STATE)
			{// INIT_STATE
				if (!oidTableEntry->InitSettable)
				{
					RT_TRACE(COMP_OID_QUERY | COMP_OID_SET, DBG_LOUD,  
						("Set of OID 0x%08x not supported in INIT state.\n", oid));
					return NDIS_STATUS_INVALID_STATE;
				}
			}
			else
			{// OP_STATE
				if (!oidTableEntry->OpSettable)
				{
					RT_TRACE(COMP_OID_QUERY | COMP_OID_SET, DBG_LOUD,  
						("Set of OID 0x%08x not supported in OP state.\n", oid));
					return NDIS_STATUS_INVALID_STATE;
				}
			}

			// Determine buffer length that will be used later
			bytesNeededLocation = (PULONG)&NdisRequest->DATA.SET_INFORMATION.BytesNeeded;
			infoBufferLength = NdisRequest->DATA.SET_INFORMATION.InformationBufferLength;
			break;

		case NdisRequestMethod:
			// Only these OIDs are supported as methods
			if ( (oid != OID_DOT11_RESET_REQUEST) &&
				(oid != OID_DOT11_ENUM_BSS_LIST) &&
				(oid != OID_DOT11_RECV_SENSITIVITY_LIST) &&
				(oid != OID_PM_GET_PROTOCOL_OFFLOAD) &&
				(oid != OID_RECEIVE_FILTER_SET_FILTER)) //tynli add OID_PM_GET_PROTOCOL_OFFLOAD
			{
				RT_TRACE(COMP_OID_QUERY | COMP_OID_SET, DBG_LOUD,  ("Method request for OID 0x%08x not supported\n", oid));
				return NDIS_STATUS_INVALID_OID;
			}

			// Determine buffer length that will be used later
			bytesNeededLocation = (PULONG)&NdisRequest->DATA.METHOD_INFORMATION.BytesNeeded;

			// Larger of the two buffer lengths
			infoBufferLength = MAX(NdisRequest->DATA.METHOD_INFORMATION.InputBufferLength,
			NdisRequest->DATA.METHOD_INFORMATION.OutputBufferLength);            
			break;

		default:
			RT_TRACE(COMP_OID_QUERY | COMP_OID_SET, DBG_LOUD,  
					("NDIS_OID_REQUEST contains unknown request type %d\n", NdisRequest->RequestType)
				);
			return NDIS_STATUS_NOT_SUPPORTED;
	}

	// Verify that this OID is applicable for the current PHY
	if (oidTableEntry->PhySpecific)
	{
		// Verify that this OID is valid for the currently SELECTED PHY
		// TODO: Win 7
		//currentPhy = BasePortGetPhyTypeFromId(Port, VNic11QuerySelectedPhyId(Port->VNic));
		//currentPhy=dot11_phy_type_erp; //temp add by Maddest

		if(pTargetAdapter->bInHctTest)
			currentPhy = dot11_phy_type_erp;
		else	
			currentPhy=pNdisCommon->pDot11SelectedPhyMIB->PhyType; //dot11_phy_type_erp; //temp add by Maddest

		switch (oid)
		{
			// OIDs valid only for ERP
			case OID_DOT11_DSSS_OFDM_OPTION_ENABLED:
			case OID_DOT11_DSSS_OFDM_OPTION_IMPLEMENTED:
			case OID_DOT11_ERP_PBCC_OPTION_ENABLED:
			case OID_DOT11_ERP_PBCC_OPTION_IMPLEMENTED:
			case OID_DOT11_SHORT_SLOT_TIME_OPTION_ENABLED:
			case OID_DOT11_SHORT_SLOT_TIME_OPTION_IMPLEMENTED:
				if (currentPhy != dot11_phy_type_erp)
				{
					RT_TRACE(COMP_OID_QUERY | COMP_OID_SET, DBG_LOUD, ("OID 0x%08x only valid on ERP phy. Current phy type is %d\n", 
					oid,
					currentPhy));
					return NDIS_STATUS_INVALID_DATA;
				}
				break;

			case OID_DOT11_PBCC_OPTION_IMPLEMENTED:
			case OID_DOT11_SHORT_PREAMBLE_OPTION_IMPLEMENTED:
			case OID_DOT11_CHANNEL_AGILITY_PRESENT:
			case OID_DOT11_CHANNEL_AGILITY_ENABLED:
				if ((currentPhy != dot11_phy_type_erp) && (currentPhy != dot11_phy_type_hrdsss))
				{
					RT_TRACE(COMP_OID_QUERY | COMP_OID_SET, DBG_LOUD, ("OID 0x%08x only valid on ERP or HRDSS phy. Current phy type is %d\n", 
					oid,
					currentPhy));

					return NDIS_STATUS_INVALID_DATA;
				}
				break;

			// OIDs valid only for ERP or DSSS or HRDSS
			case OID_DOT11_CCA_MODE_SUPPORTED:
			case OID_DOT11_CURRENT_CCA_MODE:
			case OID_DOT11_CURRENT_CHANNEL:
			case OID_DOT11_ED_THRESHOLD:
				if ((currentPhy != dot11_phy_type_erp) && (currentPhy != dot11_phy_type_hrdsss) && (currentPhy != dot11_phy_type_dsss))
				{
					RT_TRACE(COMP_OID_QUERY | COMP_OID_SET, DBG_LOUD, ("OID 0x%08x only valid on ERP, HRDSS or DSS phy. Current phy type is %d\n", 
					oid,
					currentPhy));

					return NDIS_STATUS_INVALID_DATA;
				}
				break;

			case OID_DOT11_FREQUENCY_BANDS_SUPPORTED:
			case OID_DOT11_CURRENT_FREQUENCY:
				if ((currentPhy != dot11_phy_type_ofdm) && (currentPhy != dot11_phy_type_ht ))
				{
					RT_TRACE(COMP_OID_QUERY | COMP_OID_SET, DBG_LOUD, ("OID 0x%08x only valid on OFDM phy. Current phy type is %d\n", 
					oid,
					currentPhy));

					return NDIS_STATUS_INVALID_DATA;
				}
			
			default:
				break;
		}
	}
	
	return NDIS_STATUS_SUCCESS;
}


NDIS_STATUS
N62C_OID_DOT11_INCOMING_ASSOCIATION_DECISION(
	PADAPTER pAdapter,
	PNDIS_OID_REQUEST NdisRequest
)
{
	NDIS_STATUS ndisStatus = NDIS_STATUS_INVALID_OID;
	
	// Adapter selection
	PADAPTER pDefaultAdapter = GetDefaultAdapter(pAdapter);
	PADAPTER pTargetAdapter = GetAdapterByPortNum(pAdapter, (u1Byte)NdisRequest->PortNumber);

	do
	{
		// Validate if the OID is issued in the correct state and mode
		ndisStatus = N62CValidateOIDCorrectness(pTargetAdapter, NdisRequest);
		if(ndisStatus != NDIS_STATUS_SUCCESS) {
			RT_TRACE((COMP_OID_SET | COMP_OID_QUERY), DBG_LOUD, ("%s: State Error!\n", __FUNCTION__));
			break;
		}

		switch (NdisRequest->RequestType)
		{
			// Query
			case NdisRequestQueryInformation:
			case NdisRequestQueryStatistics:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;
			
			// Set
			case NdisRequestSetInformation:
				ndisStatus = N62C_SET_OID_DOT11_INCOMING_ASSOCIATION_DECISION(
						pTargetAdapter,
						NdisRequest->DATA.SET_INFORMATION.Oid,
						NdisRequest->DATA.SET_INFORMATION.InformationBuffer,
						NdisRequest->DATA.SET_INFORMATION.InformationBufferLength,
						(PULONG)&NdisRequest->DATA.SET_INFORMATION.BytesRead,
						(PULONG)&NdisRequest->DATA.SET_INFORMATION.BytesNeeded
					);
				
				break;

			// Method
			case NdisRequestMethod:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;

			default:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;

		}
	}while(FALSE);

	return ndisStatus;
}

NDIS_STATUS
N62C_OID_DOT11_WPS_ENABLED(
	PADAPTER pAdapter,
	PNDIS_OID_REQUEST NdisRequest
)
{
	NDIS_STATUS ndisStatus = NDIS_STATUS_INVALID_OID;
	
	// Adapter selection
	PADAPTER pDefaultAdapter = GetDefaultAdapter(pAdapter);
	PADAPTER pTargetAdapter = GetAdapterByPortNum(pAdapter, (u1Byte)NdisRequest->PortNumber);

	do
	{
		// Validate if the OID is issued in the correct state and mode
		ndisStatus = N62CValidateOIDCorrectness(pTargetAdapter, NdisRequest);
		if(ndisStatus != NDIS_STATUS_SUCCESS) {
			RT_TRACE((COMP_OID_SET | COMP_OID_QUERY), DBG_LOUD, ("%s: State Error!\n", __FUNCTION__));
			break;
		}

		switch (NdisRequest->RequestType)
		{
			// Query
			case NdisRequestQueryInformation:
			case NdisRequestQueryStatistics:
				ndisStatus = N62C_QUERY_OID_DOT11_WPS_ENABLED(
						pTargetAdapter,
						NdisRequest->DATA.QUERY_INFORMATION.Oid,
						NdisRequest->DATA.QUERY_INFORMATION.InformationBuffer,
						NdisRequest->DATA.QUERY_INFORMATION.InformationBufferLength,
						(PULONG)&NdisRequest->DATA.QUERY_INFORMATION.BytesWritten,
						(PULONG)&NdisRequest->DATA.QUERY_INFORMATION.BytesNeeded 
					);
				break;
			
			// Set
			case NdisRequestSetInformation:
				ndisStatus = N62C_SET_OID_DOT11_WPS_ENABLED(
						pTargetAdapter,
						NdisRequest->DATA.SET_INFORMATION.Oid,
						NdisRequest->DATA.SET_INFORMATION.InformationBuffer,
						NdisRequest->DATA.SET_INFORMATION.InformationBufferLength,
						(PULONG)&NdisRequest->DATA.SET_INFORMATION.BytesRead,
						(PULONG)&NdisRequest->DATA.SET_INFORMATION.BytesNeeded
					);
				
				break;

			// Method
			case NdisRequestMethod:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;

			default:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;

		}
	}while(FALSE);

	return ndisStatus;
}

NDIS_STATUS
N62C_OID_DOT11_ADDITIONAL_IE(
	PADAPTER pAdapter,
	PNDIS_OID_REQUEST NdisRequest
)
{
	NDIS_STATUS ndisStatus = NDIS_STATUS_INVALID_OID;
	
	// Adapter selection
	PADAPTER pDefaultAdapter = GetDefaultAdapter(pAdapter);
	PADAPTER pTargetAdapter = GetAdapterByPortNum(pAdapter, (u1Byte)NdisRequest->PortNumber);

	do
	{
		// Validate if the OID is issued in the correct state and mode
		ndisStatus = N62CValidateOIDCorrectness(pTargetAdapter, NdisRequest);
		if(ndisStatus != NDIS_STATUS_SUCCESS) {
			RT_TRACE((COMP_OID_SET | COMP_OID_QUERY), DBG_LOUD, ("%s: State Error!\n", __FUNCTION__));
			break;
		}

		switch (NdisRequest->RequestType)
		{
			// Query
			case NdisRequestQueryInformation:
			case NdisRequestQueryStatistics:
				ndisStatus = N62C_QUERY_OID_DOT11_ADDITIONAL_IE(
						pTargetAdapter,
						NdisRequest->DATA.QUERY_INFORMATION.Oid,
						NdisRequest->DATA.QUERY_INFORMATION.InformationBuffer,
						NdisRequest->DATA.QUERY_INFORMATION.InformationBufferLength,
						(PULONG)&NdisRequest->DATA.QUERY_INFORMATION.BytesWritten,
						(PULONG)&NdisRequest->DATA.QUERY_INFORMATION.BytesNeeded 
					);
				break;
			
			// Set
			case NdisRequestSetInformation:
				ndisStatus = N62C_SET_OID_DOT11_ADDITIONAL_IE(
						pTargetAdapter,
						NdisRequest->DATA.SET_INFORMATION.Oid,
						NdisRequest->DATA.SET_INFORMATION.InformationBuffer,
						NdisRequest->DATA.SET_INFORMATION.InformationBufferLength,
						(PULONG)&NdisRequest->DATA.SET_INFORMATION.BytesRead,
						(PULONG)&NdisRequest->DATA.SET_INFORMATION.BytesNeeded
					);
				
				break;

			// Method
			case NdisRequestMethod:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;

			default:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;

		}
	}while(FALSE);

	return ndisStatus;
}

NDIS_STATUS
N62C_OID_DOT11_ENUM_PEER_INFO(
	PADAPTER pAdapter,
	PNDIS_OID_REQUEST NdisRequest
)
{
	NDIS_STATUS ndisStatus = NDIS_STATUS_INVALID_OID;
	
	// Adapter selection
	PADAPTER pDefaultAdapter = GetDefaultAdapter(pAdapter);
	PADAPTER pTargetAdapter = GetAdapterByPortNum(pAdapter, (u1Byte)NdisRequest->PortNumber);

	do
	{
		// Validate if the OID is issued in the correct state and mode
		ndisStatus = N62CValidateOIDCorrectness(pTargetAdapter, NdisRequest);
		if(ndisStatus != NDIS_STATUS_SUCCESS) {
			RT_TRACE((COMP_OID_SET | COMP_OID_QUERY), DBG_LOUD, ("%s: State Error!\n", __FUNCTION__));
			break;
		}

		switch (NdisRequest->RequestType)
		{
			// Query
			case NdisRequestQueryInformation:
			case NdisRequestQueryStatistics:
				ndisStatus = N62C_QUERY_OID_DOT11_ENUM_PEER_INFO(
						pTargetAdapter,
						NdisRequest->DATA.QUERY_INFORMATION.Oid,
						NdisRequest->DATA.QUERY_INFORMATION.InformationBuffer,
						NdisRequest->DATA.QUERY_INFORMATION.InformationBufferLength,
						(PULONG)&NdisRequest->DATA.QUERY_INFORMATION.BytesWritten,
						(PULONG)&NdisRequest->DATA.QUERY_INFORMATION.BytesNeeded 
					);
				break;
			
			// Set
			case NdisRequestSetInformation:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;

			// Method
			case NdisRequestMethod:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;

			default:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;

		}
	}while(FALSE);

	return ndisStatus;
}

NDIS_STATUS
N62C_OID_DOT11_AVAILABLE_CHANNEL_LIST(
	PADAPTER pAdapter,
	PNDIS_OID_REQUEST NdisRequest
)
{
	NDIS_STATUS ndisStatus = NDIS_STATUS_INVALID_OID;
	
	// Adapter selection
	PADAPTER pDefaultAdapter = GetDefaultAdapter(pAdapter);
	PADAPTER pTargetAdapter = GetAdapterByPortNum(pAdapter, (u1Byte)NdisRequest->PortNumber);

	do
	{
		// Validate if the OID is issued in the correct state and mode
		ndisStatus = N62CValidateOIDCorrectness(pTargetAdapter, NdisRequest);
		if(ndisStatus != NDIS_STATUS_SUCCESS) {
			RT_TRACE((COMP_OID_SET | COMP_OID_QUERY), DBG_LOUD, ("%s: State Error!\n", __FUNCTION__));
			break;
		}

		switch (NdisRequest->RequestType)
		{
			// Query
			case NdisRequestQueryInformation:
			case NdisRequestQueryStatistics:
				ndisStatus = N62C_QUERY_OID_DOT11_AVAILABLE_CHANNEL_LIST(
						pTargetAdapter,
						NdisRequest->DATA.QUERY_INFORMATION.Oid,
						NdisRequest->DATA.QUERY_INFORMATION.InformationBuffer,
						NdisRequest->DATA.QUERY_INFORMATION.InformationBufferLength,
						(PULONG)&NdisRequest->DATA.QUERY_INFORMATION.BytesWritten,
						(PULONG)&NdisRequest->DATA.QUERY_INFORMATION.BytesNeeded 
					);
				break;
			
			// Set
			case NdisRequestSetInformation:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;

			// Method
			case NdisRequestMethod:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;

			default:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;

		}
	}while(FALSE);

	return ndisStatus;
}

NDIS_STATUS
N62C_OID_DOT11_AVAILABLE_FREQUENCY_LIST(
	PADAPTER pAdapter,
	PNDIS_OID_REQUEST NdisRequest
)
{
	NDIS_STATUS ndisStatus = NDIS_STATUS_INVALID_OID;
	
	// Adapter selection
	PADAPTER pDefaultAdapter = GetDefaultAdapter(pAdapter);
	PADAPTER pTargetAdapter = GetAdapterByPortNum(pAdapter, (u1Byte)NdisRequest->PortNumber);

	do
	{
		// Validate if the OID is issued in the correct state and mode
		ndisStatus = N62CValidateOIDCorrectness(pTargetAdapter, NdisRequest);
		if(ndisStatus != NDIS_STATUS_SUCCESS) {
			RT_TRACE((COMP_OID_SET | COMP_OID_QUERY), DBG_LOUD, ("%s: State Error!\n", __FUNCTION__));
			break;
		}

		switch (NdisRequest->RequestType)
		{
			// Query
			case NdisRequestQueryInformation:
			case NdisRequestQueryStatistics:
				ndisStatus = N62C_QUERY_OID_DOT11_AVAILABLE_FREQUENCY_LIST(
						pTargetAdapter,
						NdisRequest->DATA.QUERY_INFORMATION.Oid,
						NdisRequest->DATA.QUERY_INFORMATION.InformationBuffer,
						NdisRequest->DATA.QUERY_INFORMATION.InformationBufferLength,
						(PULONG)&NdisRequest->DATA.QUERY_INFORMATION.BytesWritten,
						(PULONG)&NdisRequest->DATA.QUERY_INFORMATION.BytesNeeded 
					);
				break;
			
			// Set
			case NdisRequestSetInformation:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;

			// Method
			case NdisRequestMethod:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;

			default:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;

		}
	}while(FALSE);

	return ndisStatus;
}

NDIS_STATUS
N62C_OID_DOT11_START_AP_REQUEST(
	PADAPTER pAdapter,
	PNDIS_OID_REQUEST NdisRequest
)
{
	NDIS_STATUS ndisStatus = NDIS_STATUS_INVALID_OID;
	
	// Adapter selection
	PADAPTER pDefaultAdapter = GetDefaultAdapter(pAdapter);
	PADAPTER pTargetAdapter = GetAdapterByPortNum(pAdapter, (u1Byte)NdisRequest->PortNumber);

	do
	{
		// Validate if the OID is issued in the correct state and mode
		ndisStatus = N62CValidateOIDCorrectness(pTargetAdapter, NdisRequest);
		if(ndisStatus != NDIS_STATUS_SUCCESS) {
			RT_TRACE((COMP_OID_SET | COMP_OID_QUERY), DBG_LOUD, ("%s: State Error!\n", __FUNCTION__));
			break;
		}

		switch (NdisRequest->RequestType)
		{
			// Query
			case NdisRequestQueryInformation:
			case NdisRequestQueryStatistics:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;
			
			// Set
			case NdisRequestSetInformation:
				ndisStatus = N62C_SET_OID_DOT11_START_AP_REQUEST(
						pTargetAdapter,
						NdisRequest->DATA.SET_INFORMATION.Oid,
						NdisRequest->DATA.SET_INFORMATION.InformationBuffer,
						NdisRequest->DATA.SET_INFORMATION.InformationBufferLength,
						(PULONG)&NdisRequest->DATA.SET_INFORMATION.BytesRead,
						(PULONG)&NdisRequest->DATA.SET_INFORMATION.BytesNeeded
					);
				
				break;

			// Method
			case NdisRequestMethod:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;

			default:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;

		}
	}while(FALSE);

	return ndisStatus;
}

NDIS_STATUS
N62C_OID_DOT11_DISASSOCIATE_PEER_REQUEST(
	PADAPTER pAdapter,
	PNDIS_OID_REQUEST NdisRequest
)
{
	NDIS_STATUS ndisStatus = NDIS_STATUS_INVALID_OID;
	
	// Adapter selection
	PADAPTER pDefaultAdapter = GetDefaultAdapter(pAdapter);
	PADAPTER pTargetAdapter = GetAdapterByPortNum(pAdapter, (u1Byte)NdisRequest->PortNumber);

	do
	{
		// Validate if the OID is issued in the correct state and mode
		ndisStatus = N62CValidateOIDCorrectness(pTargetAdapter, NdisRequest);
		if(ndisStatus != NDIS_STATUS_SUCCESS) {
			RT_TRACE((COMP_OID_SET | COMP_OID_QUERY), DBG_LOUD, ("%s: State Error!\n", __FUNCTION__));
			break;
		}

		switch (NdisRequest->RequestType)
		{
			// Query
			case NdisRequestQueryInformation:
			case NdisRequestQueryStatistics:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;
			
			// Set
			case NdisRequestSetInformation:
				ndisStatus = N62C_SET_OID_DOT11_DISASSOCIATE_PEER_REQUEST(
						pTargetAdapter,
						NdisRequest->DATA.SET_INFORMATION.Oid,
						NdisRequest->DATA.SET_INFORMATION.InformationBuffer,
						NdisRequest->DATA.SET_INFORMATION.InformationBufferLength,
						(PULONG)&NdisRequest->DATA.SET_INFORMATION.BytesRead,
						(PULONG)&NdisRequest->DATA.SET_INFORMATION.BytesNeeded
					);
				
				break;

			// Method
			case NdisRequestMethod:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;

			default:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;

		}
	}while(FALSE);

	return ndisStatus;
}

NDIS_STATUS
N62C_OID_PM_GET_PROTOCOL_OFFLOAD(
	PADAPTER pAdapter,
	PNDIS_OID_REQUEST NdisRequest
)
{
	NDIS_STATUS ndisStatus = NDIS_STATUS_INVALID_OID;
	
	// Adapter selection
	PADAPTER pDefaultAdapter = GetDefaultAdapter(pAdapter);
	PADAPTER pTargetAdapter = GetAdapterByPortNum(pAdapter, (u1Byte)NdisRequest->PortNumber);

	do
	{
		// Validate if the OID is issued in the correct state and mode
		ndisStatus = N62CValidateOIDCorrectness(pTargetAdapter, NdisRequest);
		if(ndisStatus != NDIS_STATUS_SUCCESS) {
			RT_TRACE((COMP_OID_SET | COMP_OID_QUERY), DBG_LOUD, ("%s: State Error!\n", __FUNCTION__));
			break;
		}
		
		switch (NdisRequest->RequestType)
		{
			// Query
			case NdisRequestQueryInformation:
			case NdisRequestQueryStatistics:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;
			
			// Set
			case NdisRequestSetInformation:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;

			// Method
			case NdisRequestMethod:
				ndisStatus = N62C_METHOD_OID_PM_GET_PROTOCOL_OFFLOAD(
						pTargetAdapter,
						NdisRequest->DATA.METHOD_INFORMATION.Oid,
						NdisRequest->DATA.METHOD_INFORMATION.InformationBuffer,
						NdisRequest->DATA.METHOD_INFORMATION.InputBufferLength,
						NdisRequest->DATA.METHOD_INFORMATION.OutputBufferLength,
						NdisRequest->DATA.METHOD_INFORMATION.MethodId,
						(PULONG)&NdisRequest->DATA.METHOD_INFORMATION.BytesWritten,
						(PULONG)&NdisRequest->DATA.METHOD_INFORMATION.BytesRead,
						(PULONG)&NdisRequest->DATA.METHOD_INFORMATION.BytesNeeded 
					);
				break;

			default:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;

		}
	}while(FALSE);

	return ndisStatus;
}

NDIS_STATUS
N62C_OID_DOT11_ASSOCIATION_PARAMS(
	PADAPTER pAdapter,
	PNDIS_OID_REQUEST NdisRequest
)
{
	NDIS_STATUS ndisStatus = NDIS_STATUS_INVALID_OID;
	
	// Adapter selection
	PADAPTER pDefaultAdapter = GetDefaultAdapter(pAdapter);
	PADAPTER pTargetAdapter = GetAdapterByPortNum(pAdapter, (u1Byte)NdisRequest->PortNumber);

	do
	{
		// Validate if the OID is issued in the correct state and mode
		ndisStatus = N62CValidateOIDCorrectness(pTargetAdapter, NdisRequest);
		if(ndisStatus != NDIS_STATUS_SUCCESS) {
			RT_TRACE((COMP_OID_SET | COMP_OID_QUERY), DBG_LOUD, ("%s: State Error!\n", __FUNCTION__));
			break;
		}

		switch (NdisRequest->RequestType)
		{
			// Query
			case NdisRequestQueryInformation:
			case NdisRequestQueryStatistics:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;
			
			// Set
			case NdisRequestSetInformation:
				ndisStatus = N62C_SET_OID_DOT11_ASSOCIATION_PARAMS(
						pTargetAdapter,
						NdisRequest->DATA.SET_INFORMATION.Oid,
						NdisRequest->DATA.SET_INFORMATION.InformationBuffer,
						NdisRequest->DATA.SET_INFORMATION.InformationBufferLength,
						(PULONG)&NdisRequest->DATA.SET_INFORMATION.BytesRead,
						(PULONG)&NdisRequest->DATA.SET_INFORMATION.BytesNeeded
					);
				
				break;

			// Method
			case NdisRequestMethod:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;

			default:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;

		}
	}while(FALSE);

	return ndisStatus;
}

NDIS_STATUS
N62C_OID_PM_ADD_WOL_PATTERN(
	PADAPTER pAdapter,
	PNDIS_OID_REQUEST NdisRequest
)
{
	NDIS_STATUS ndisStatus = NDIS_STATUS_INVALID_OID;
	
	// Adapter selection
	PADAPTER pDefaultAdapter = GetDefaultAdapter(pAdapter);
	PADAPTER pTargetAdapter = GetAdapterByPortNum(pAdapter, (u1Byte)NdisRequest->PortNumber);

	do
	{
		// Validate if the OID is issued in the correct state and mode
		ndisStatus = N62CValidateOIDCorrectness(pTargetAdapter, NdisRequest);
		if(ndisStatus != NDIS_STATUS_SUCCESS) {
			RT_TRACE((COMP_OID_SET | COMP_OID_QUERY), DBG_LOUD, ("%s: State Error!\n", __FUNCTION__));
			break;
		}

		switch (NdisRequest->RequestType)
		{
			// Query
			case NdisRequestQueryInformation:
			case NdisRequestQueryStatistics:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;
			
			// Set
			case NdisRequestSetInformation:
				ndisStatus = N62C_SET_OID_PM_ADD_WOL_PATTERN(
						pTargetAdapter,
						NdisRequest->DATA.SET_INFORMATION.Oid,
						NdisRequest->DATA.SET_INFORMATION.InformationBuffer,
						NdisRequest->DATA.SET_INFORMATION.InformationBufferLength,
						(PULONG)&NdisRequest->DATA.SET_INFORMATION.BytesRead,
						(PULONG)&NdisRequest->DATA.SET_INFORMATION.BytesNeeded
					);
				
				break;

			// Method
			case NdisRequestMethod:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;

			default:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;

		}
	}while(FALSE);

	return ndisStatus;
}

NDIS_STATUS
N62C_OID_PM_REMOVE_WOL_PATTERN(
	PADAPTER pAdapter,
	PNDIS_OID_REQUEST NdisRequest
)
{
	NDIS_STATUS ndisStatus = NDIS_STATUS_INVALID_OID;
	
	// Adapter selection
	PADAPTER pDefaultAdapter = GetDefaultAdapter(pAdapter);
	PADAPTER pTargetAdapter = GetAdapterByPortNum(pAdapter, (u1Byte)NdisRequest->PortNumber);

	do
	{
		// Validate if the OID is issued in the correct state and mode
		ndisStatus = N62CValidateOIDCorrectness(pTargetAdapter, NdisRequest);
		if(ndisStatus != NDIS_STATUS_SUCCESS) {
			RT_TRACE((COMP_OID_SET | COMP_OID_QUERY), DBG_LOUD, ("%s: State Error!\n", __FUNCTION__));
			break;
		}

		switch (NdisRequest->RequestType)
		{
			// Query
			case NdisRequestQueryInformation:
			case NdisRequestQueryStatistics:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;
			
			// Set
			case NdisRequestSetInformation:
				ndisStatus = N62C_SET_OID_PM_REMOVE_WOL_PATTERN(
						pTargetAdapter,
						NdisRequest->DATA.SET_INFORMATION.Oid,
						NdisRequest->DATA.SET_INFORMATION.InformationBuffer,
						NdisRequest->DATA.SET_INFORMATION.InformationBufferLength,
						(PULONG)&NdisRequest->DATA.SET_INFORMATION.BytesRead,
						(PULONG)&NdisRequest->DATA.SET_INFORMATION.BytesNeeded
					);
				
				break;

			// Method
			case NdisRequestMethod:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;

			default:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;

		}
	}while(FALSE);

	return ndisStatus;
}

NDIS_STATUS
N62C_OID_PM_PARAMETERS(
	PADAPTER pAdapter,
	PNDIS_OID_REQUEST NdisRequest
)
{
	NDIS_STATUS ndisStatus = NDIS_STATUS_INVALID_OID;
	
	// Adapter selection
	PADAPTER pDefaultAdapter = GetDefaultAdapter(pAdapter);
	PADAPTER pTargetAdapter = GetAdapterByPortNum(pAdapter, (u1Byte)NdisRequest->PortNumber);

	do
	{
		// Validate if the OID is issued in the correct state and mode
		ndisStatus = N62CValidateOIDCorrectness(pTargetAdapter, NdisRequest);
		if(ndisStatus != NDIS_STATUS_SUCCESS) {
			RT_TRACE((COMP_OID_SET | COMP_OID_QUERY), DBG_LOUD, ("%s: State Error!\n", __FUNCTION__));
			break;
		}

		switch (NdisRequest->RequestType)
		{
			// Query
			case NdisRequestQueryInformation:
			case NdisRequestQueryStatistics:
				ndisStatus = N62C_QUERY_OID_PM_PARAMETERS(
						pTargetAdapter,
						NdisRequest->DATA.QUERY_INFORMATION.Oid,
						NdisRequest->DATA.QUERY_INFORMATION.InformationBuffer,
						NdisRequest->DATA.QUERY_INFORMATION.InformationBufferLength,
						(PULONG)&NdisRequest->DATA.QUERY_INFORMATION.BytesWritten,
						(PULONG)&NdisRequest->DATA.QUERY_INFORMATION.BytesNeeded 
					);
				break;
			
			// Set
			case NdisRequestSetInformation:
				ndisStatus = N62C_SET_OID_PM_PARAMETERS(
						pTargetAdapter,
						NdisRequest->DATA.SET_INFORMATION.Oid,
						NdisRequest->DATA.SET_INFORMATION.InformationBuffer,
						NdisRequest->DATA.SET_INFORMATION.InformationBufferLength,
						(PULONG)&NdisRequest->DATA.SET_INFORMATION.BytesRead,
						(PULONG)&NdisRequest->DATA.SET_INFORMATION.BytesNeeded
					);
				
				break;

			// Method
			case NdisRequestMethod:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;

			default:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;

		}
	}while(FALSE);

	return ndisStatus;
}

NDIS_STATUS
N62C_OID_PM_ADD_PROTOCOL_OFFLOAD(
	PADAPTER pAdapter,
	PNDIS_OID_REQUEST NdisRequest
)
{
	NDIS_STATUS ndisStatus = NDIS_STATUS_INVALID_OID;
	
	// Adapter selection
	PADAPTER pDefaultAdapter = GetDefaultAdapter(pAdapter);
	PADAPTER pTargetAdapter = GetAdapterByPortNum(pAdapter, (u1Byte)NdisRequest->PortNumber);

	do
	{
		// Validate if the OID is issued in the correct state and mode
		ndisStatus = N62CValidateOIDCorrectness(pTargetAdapter, NdisRequest);
		if(ndisStatus != NDIS_STATUS_SUCCESS) {
			RT_TRACE((COMP_OID_SET | COMP_OID_QUERY), DBG_LOUD, ("%s: State Error!\n", __FUNCTION__));
			break;
		}

		switch (NdisRequest->RequestType)
		{
			// Query
			case NdisRequestQueryInformation:
			case NdisRequestQueryStatistics:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;
			
			// Set
			case NdisRequestSetInformation:
				ndisStatus = N62C_SET_OID_PM_ADD_PROTOCOL_OFFLOAD(
						pTargetAdapter,
						NdisRequest->DATA.SET_INFORMATION.Oid,
						NdisRequest->DATA.SET_INFORMATION.InformationBuffer,
						NdisRequest->DATA.SET_INFORMATION.InformationBufferLength,
						(PULONG)&NdisRequest->DATA.SET_INFORMATION.BytesRead,
						(PULONG)&NdisRequest->DATA.SET_INFORMATION.BytesNeeded
					);
				
				break;

			// Method
			case NdisRequestMethod:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;

			default:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;

		}
	}while(FALSE);

	return ndisStatus;
}

NDIS_STATUS
N62C_OID_PM_REMOVE_PROTOCOL_OFFLOAD(
	PADAPTER pAdapter,
	PNDIS_OID_REQUEST NdisRequest
)
{
	NDIS_STATUS ndisStatus = NDIS_STATUS_INVALID_OID;
	
	// Adapter selection
	PADAPTER pDefaultAdapter = GetDefaultAdapter(pAdapter);
	PADAPTER pTargetAdapter = GetAdapterByPortNum(pAdapter, (u1Byte)NdisRequest->PortNumber);

	do
	{
		// Validate if the OID is issued in the correct state and mode
		ndisStatus = N62CValidateOIDCorrectness(pTargetAdapter, NdisRequest);
		if(ndisStatus != NDIS_STATUS_SUCCESS) {
			RT_TRACE((COMP_OID_SET | COMP_OID_QUERY), DBG_LOUD, ("%s: State Error!\n", __FUNCTION__));
			break;
		}

		switch (NdisRequest->RequestType)
		{
			// Query
			case NdisRequestQueryInformation:
			case NdisRequestQueryStatistics:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;
			
			// Set
			case NdisRequestSetInformation:
				ndisStatus = N62C_SET_OID_PM_REMOVE_PROTOCOL_OFFLOAD(
						pTargetAdapter,
						NdisRequest->DATA.SET_INFORMATION.Oid,
						NdisRequest->DATA.SET_INFORMATION.InformationBuffer,
						NdisRequest->DATA.SET_INFORMATION.InformationBufferLength,
						(PULONG)&NdisRequest->DATA.SET_INFORMATION.BytesRead,
						(PULONG)&NdisRequest->DATA.SET_INFORMATION.BytesNeeded
					);
				
				break;

			// Method
			case NdisRequestMethod:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;

			default:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;

		}
	}while(FALSE);

	return ndisStatus;
}


NDIS_STATUS
N62C_OID_PM_ADD_PROTOCOL_OFFLOAD_LIST(
	PADAPTER pAdapter,
	PNDIS_OID_REQUEST NdisRequest
)
{
	NDIS_STATUS ndisStatus = NDIS_STATUS_INVALID_OID;
	
	// Adapter selection
	PADAPTER pDefaultAdapter = GetDefaultAdapter(pAdapter);
	PADAPTER pTargetAdapter = GetAdapterByPortNum(pAdapter, (u1Byte)NdisRequest->PortNumber);

	RT_TRACE(COMP_POWER, DBG_LOUD, ("===> N62C_OID_PM_ADD_PROTOCOL_OFFLOAD_LIST \n"));

	do
	{
		// Validate if the OID is issued in the correct state and mode
		ndisStatus = N62CValidateOIDCorrectness(pTargetAdapter, NdisRequest);
		if(ndisStatus != NDIS_STATUS_SUCCESS) {
			RT_TRACE((COMP_OID_SET | COMP_OID_QUERY), DBG_LOUD, ("%s: State Error!\n", __FUNCTION__));
			break;
		}

		switch (NdisRequest->RequestType)
		{
			// Query
			case NdisRequestQueryInformation:
			case NdisRequestQueryStatistics:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;
			
			// Set
			case NdisRequestSetInformation:
				ndisStatus = N62C_SET_OID_PM_ADD_PROTOCOL_OFFLOAD_LIST(
						pTargetAdapter,
						NdisRequest->DATA.SET_INFORMATION.Oid,
						NdisRequest->DATA.SET_INFORMATION.InformationBuffer,
						NdisRequest->DATA.SET_INFORMATION.InformationBufferLength,
						(PULONG)&NdisRequest->DATA.SET_INFORMATION.BytesRead,
						(PULONG)&NdisRequest->DATA.SET_INFORMATION.BytesNeeded
					);
				
				break;

			// Method
			case NdisRequestMethod:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;

			default:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;

		}
	}while(FALSE);

	return ndisStatus;
}


NDIS_STATUS
N62C_OID_GEN_INTERRUPT_MODERATION(
	PADAPTER pAdapter,
	PNDIS_OID_REQUEST NdisRequest
)
{
	NDIS_STATUS ndisStatus = NDIS_STATUS_INVALID_OID;
	
	// Adapter selection
	PADAPTER pDefaultAdapter = GetDefaultAdapter(pAdapter);
	PADAPTER pTargetAdapter = GetAdapterByPortNum(pAdapter, (u1Byte)NdisRequest->PortNumber);

	do
	{
		// Validate if the OID is issued in the correct state and mode
		ndisStatus = N62CValidateOIDCorrectness(pTargetAdapter, NdisRequest);
		if(ndisStatus != NDIS_STATUS_SUCCESS) {
			RT_TRACE((COMP_OID_SET | COMP_OID_QUERY), DBG_LOUD, ("%s: State Error!\n", __FUNCTION__));
			break;
		}

		switch (NdisRequest->RequestType)
		{
			// Query
			case NdisRequestQueryInformation:
			case NdisRequestQueryStatistics:
				ndisStatus = N62C_QUERY_OID_GEN_INTERRUPT_MODERATION(
						pTargetAdapter,
						NdisRequest->DATA.QUERY_INFORMATION.Oid,
						NdisRequest->DATA.QUERY_INFORMATION.InformationBuffer,
						NdisRequest->DATA.QUERY_INFORMATION.InformationBufferLength,
						(PULONG)&NdisRequest->DATA.QUERY_INFORMATION.BytesWritten,
						(PULONG)&NdisRequest->DATA.QUERY_INFORMATION.BytesNeeded 
					);
				break;
			
			// Set
			case NdisRequestSetInformation:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;

			// Method
			case NdisRequestMethod:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;

			default:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;

		}
	}while(FALSE);

	return ndisStatus;
}

NDIS_STATUS
N62C_OID_PACKET_COALESCING_FILTER_MATCH_COUNT(
	PADAPTER pAdapter,
	PNDIS_OID_REQUEST NdisRequest
)
{
	NDIS_STATUS ndisStatus = NDIS_STATUS_INVALID_OID;
	
	// Adapter selection
	PADAPTER pDefaultAdapter = GetDefaultAdapter(pAdapter);
	PADAPTER pTargetAdapter = GetAdapterByPortNum(pAdapter, (u1Byte)NdisRequest->PortNumber);

	do
	{
		// Validate if the OID is issued in the correct state and mode
		ndisStatus = N62CValidateOIDCorrectness(pTargetAdapter, NdisRequest);
		if(ndisStatus != NDIS_STATUS_SUCCESS) {
			RT_TRACE((COMP_OID_SET | COMP_OID_QUERY), DBG_LOUD, ("%s: State Error!\n", __FUNCTION__));
			break;
		}

		switch (NdisRequest->RequestType)
		{
			// Query
			case NdisRequestQueryInformation:
			case NdisRequestQueryStatistics:
				ndisStatus = N62C_QUERY_OID_PACKET_COALESCING_FILTER_MATCH_COUNT(
						pTargetAdapter,
						NdisRequest->DATA.QUERY_INFORMATION.Oid,
						NdisRequest->DATA.QUERY_INFORMATION.InformationBuffer,
						NdisRequest->DATA.QUERY_INFORMATION.InformationBufferLength,
						(PULONG)&NdisRequest->DATA.QUERY_INFORMATION.BytesWritten,
						(PULONG)&NdisRequest->DATA.QUERY_INFORMATION.BytesNeeded 
					);
				break;
			
			// Set
			case NdisRequestSetInformation:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;

			// Method
			case NdisRequestMethod:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;

			default:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;

		}
	}while(FALSE);

	return ndisStatus;
}


NDIS_STATUS
N62C_OID_RECEIVE_FILTER_CLEAR_FILTER(
	PADAPTER pAdapter,
	PNDIS_OID_REQUEST NdisRequest
)
{
	NDIS_STATUS ndisStatus = NDIS_STATUS_INVALID_OID;
	
	// Adapter selection
	PADAPTER pDefaultAdapter = GetDefaultAdapter(pAdapter);
	PADAPTER pTargetAdapter = GetAdapterByPortNum(pAdapter, (u1Byte)NdisRequest->PortNumber);

	do
	{
		// Validate if the OID is issued in the correct state and mode
		ndisStatus = N62CValidateOIDCorrectness(pTargetAdapter, NdisRequest);
		if(ndisStatus != NDIS_STATUS_SUCCESS) {
			RT_TRACE((COMP_OID_SET | COMP_OID_QUERY), DBG_LOUD, ("%s: State Error!\n", __FUNCTION__));
			break;
		}

		switch (NdisRequest->RequestType)
		{
			// Query
			case NdisRequestQueryInformation:
			case NdisRequestQueryStatistics:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;
			
			// Set
			case NdisRequestSetInformation:
				ndisStatus = N62C_SET_OID_RECEIVE_FILTER_CLEAR_FILTER(
						pTargetAdapter,
						NdisRequest->DATA.SET_INFORMATION.Oid,
						NdisRequest->DATA.SET_INFORMATION.InformationBuffer,
						NdisRequest->DATA.SET_INFORMATION.InformationBufferLength,
						(PULONG)&NdisRequest->DATA.SET_INFORMATION.BytesRead,
						(PULONG)&NdisRequest->DATA.SET_INFORMATION.BytesNeeded
					);
				
				break;

			// Method
			case NdisRequestMethod:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;

			default:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;

		}
	}while(FALSE);

	return ndisStatus;
}

NDIS_STATUS
N62C_OID_RECEIVE_FILTER_SET_FILTER(
	PADAPTER pAdapter,
	PNDIS_OID_REQUEST NdisRequest
)
{
	NDIS_STATUS ndisStatus = NDIS_STATUS_INVALID_OID;
	
	// Adapter selection
	PADAPTER pDefaultAdapter = GetDefaultAdapter(pAdapter);
	PADAPTER pTargetAdapter = GetAdapterByPortNum(pAdapter, (u1Byte)NdisRequest->PortNumber);

	do
	{
		// Validate if the OID is issued in the correct state and mode
		ndisStatus = N62CValidateOIDCorrectness(pTargetAdapter, NdisRequest);
		if(ndisStatus != NDIS_STATUS_SUCCESS) {
			RT_TRACE((COMP_OID_SET | COMP_OID_QUERY), DBG_LOUD, ("%s: State Error!\n", __FUNCTION__));
			break;
		}

		switch (NdisRequest->RequestType)
		{
			case NdisRequestQueryInformation:
			case NdisRequestQueryStatistics:
			case NdisRequestSetInformation:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;				
				break;

			// Method
			case NdisRequestMethod:
				ndisStatus = N62C_QUERYSET_OID_RECEIVE_FILTER_SET_FILTER(
						pTargetAdapter,
						NdisRequest->DATA.METHOD_INFORMATION.Oid,
						NdisRequest->DATA.METHOD_INFORMATION.InformationBuffer,
						NdisRequest->DATA.METHOD_INFORMATION.InputBufferLength,
						NdisRequest->DATA.METHOD_INFORMATION.OutputBufferLength,
						NdisRequest->DATA.METHOD_INFORMATION.MethodId,
						(PULONG)&NdisRequest->DATA.METHOD_INFORMATION.BytesWritten,
						(PULONG)&NdisRequest->DATA.METHOD_INFORMATION.BytesRead,
						(PULONG)&NdisRequest->DATA.METHOD_INFORMATION.BytesNeeded				
					);
				break;

			default:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;

		}
	}while(FALSE);

	return ndisStatus;
}

NDIS_STATUS
N62C_OID_RECEIVE_FILTER_HARDWARE_CAPABILITIES(
	PADAPTER pAdapter,
	PNDIS_OID_REQUEST NdisRequest
)
{
	NDIS_STATUS ndisStatus = NDIS_STATUS_INVALID_OID;
	
	// Adapter selection
	PADAPTER pDefaultAdapter = GetDefaultAdapter(pAdapter);
	PADAPTER pTargetAdapter = GetAdapterByPortNum(pAdapter, (u1Byte)NdisRequest->PortNumber);

	do
	{
		// Validate if the OID is issued in the correct state and mode
		ndisStatus = N62CValidateOIDCorrectness(pTargetAdapter, NdisRequest);
		if(ndisStatus != NDIS_STATUS_SUCCESS) {
			RT_TRACE((COMP_OID_SET | COMP_OID_QUERY), DBG_LOUD, ("%s: State Error!\n", __FUNCTION__));
			break;
		}

		switch (NdisRequest->RequestType)
		{
			// Query
			case NdisRequestQueryInformation:
			case NdisRequestQueryStatistics:
				ndisStatus = N62C_QUERY_OID_RECEIVE_FILTER_HARDWARE_CAPABILITIES(
						pTargetAdapter,
						NdisRequest->DATA.QUERY_INFORMATION.Oid,
						NdisRequest->DATA.QUERY_INFORMATION.InformationBuffer,
						NdisRequest->DATA.QUERY_INFORMATION.InformationBufferLength,
						(PULONG)&NdisRequest->DATA.QUERY_INFORMATION.BytesWritten,
						(PULONG)&NdisRequest->DATA.QUERY_INFORMATION.BytesNeeded 
					);
				break;
			
			// Set
			case NdisRequestSetInformation:
				break;

			// Method
			case NdisRequestMethod:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;

			default:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;

		}
	}while(FALSE);

	return ndisStatus;
}

NDIS_STATUS
N62C_OID_RECEIVE_FILTER_CURRENT_CAPABILITIES(
	PADAPTER pAdapter,
	PNDIS_OID_REQUEST NdisRequest
)
{
	NDIS_STATUS ndisStatus = NDIS_STATUS_INVALID_OID;
	
	// Adapter selection
	PADAPTER pDefaultAdapter = GetDefaultAdapter(pAdapter);
	PADAPTER pTargetAdapter = GetAdapterByPortNum(pAdapter, (u1Byte)NdisRequest->PortNumber);

	do
	{
		// Validate if the OID is issued in the correct state and mode
		ndisStatus = N62CValidateOIDCorrectness(pTargetAdapter, NdisRequest);
		if(ndisStatus != NDIS_STATUS_SUCCESS) {
			RT_TRACE((COMP_OID_SET | COMP_OID_QUERY), DBG_LOUD, ("%s: State Error!\n", __FUNCTION__));
			break;
		}

		switch (NdisRequest->RequestType)
		{
			// Query
			case NdisRequestQueryInformation:
			case NdisRequestQueryStatistics:
				ndisStatus = N62C_QUERY_OID_RECEIVE_FILTER_HARDWARE_CAPABILITIES(
						pTargetAdapter,
						NdisRequest->DATA.QUERY_INFORMATION.Oid,
						NdisRequest->DATA.QUERY_INFORMATION.InformationBuffer,
						NdisRequest->DATA.QUERY_INFORMATION.InformationBufferLength,
						(PULONG)&NdisRequest->DATA.QUERY_INFORMATION.BytesWritten,
						(PULONG)&NdisRequest->DATA.QUERY_INFORMATION.BytesNeeded 
					);
				break;
			
			// Set
			case NdisRequestSetInformation:
				break;

			// Method
			case NdisRequestMethod:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;

			default:
				ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
				break;

		}
	}while(FALSE);

	return ndisStatus;
}