
#ifndef CANTS_FRAME_TYPES_H
#define CANTS_FRAME_TYPES_H

#include <stdint.h>

/*******************************************************************************
 * Definitions
 ******************************************************************************/

#define CANTS_FRAME_DATA_LENGTH 8u

/**
 * CAN-TS Broadcasts Addresses
 */
#define CANTS_BROADCAST_TIMESYNC_ADDR   0U
#define CANTS_BROADCAST_KEEPALIVE_ADDR  1U
#define CANTS_BROADCAST_ERROR_ADDR      2U
#define CANTS_BROADCAST_WARNING_ADDR    3U

/**
 * @name TC/TM Request/Acknowledge definitions
 *@{
 */
#define TCTM_RA_MASK    0x300U /**< Mask for RA field */
#define TCTM_RA_REQUEST 0x000U /**< RA request */
#define TCTM_RA_ACK     0x100U /**< RA ack */
#define TCTM_RA_NACK    0x200U /**< RA nack */
/**
 *@}
*/

/*******************************************************************************
 * Enumerations
 ******************************************************************************/

/**
 * CAN-TS Bus mode
 */
enum cants_bus_id
{
    CANTS_BUS_CAN1                  = 0,
    CANTS_BUS_CAN2                  = 1
};

/**
 * CAN-TS Transfer Types
 */
enum cants_transfer_type
{
    CANTS_TT_TIMESYNC               = 0,
    CANTS_TT_UNSOLICITIED_TELEMETRY = 1,
    CANTS_TT_TELECOMMAND            = 2,
    CANTS_TT_TELEMETRY              = 3,
    CANTS_TT_SETBLOCK               = 4,
    CANTS_TT_GETBLOCK               = 5
};

/**
 * CAN-TS Telecommand Frame Types
 */
enum cants_frame_type_tc
{
    CANTS_FT_TC_REQUEST             = 0,
    CANTS_FT_TC_ACK                 = 1,
    CANTS_FT_TC_NACK                = 2
};

/**
 * CAN-TS Telemetry Frame Types
 */
enum cants_frame_type_tm
{
    CANTS_FT_TM_REQUEST             = 0,
    CANTS_FT_TM_ACK                 = 1,
    CANTS_FT_TM_NACK                = 2
};

/**
 * CAN-TS Get Block Frame Types
 */
enum cants_frame_type_gb
{
    CANTS_FT_GB_REQUEST             = 0,
    CANTS_FT_GB_ACK                 = 2,
    CANTS_FT_GB_ABORT               = 3,
    CANTS_FT_GB_NACK                = 4,
    CANTS_FT_GB_START               = 6,
    CANTS_FT_GB_TRANSFER            = 7
};

/**
 * CAN-TS Set Block Frame Types
 */
enum cants_frame_type_sb
{
    CANTS_FT_SB_REQUEST             = 0,
    CANTS_FT_SB_TRANSFER            = 1,
    CANTS_FT_SB_ACK                 = 2,
    CANTS_FT_SB_ABORT               = 3,
    CANTS_FT_SB_NACK                = 4,
    CANTS_FT_SB_STATUSREQUEST       = 6,
    CANTS_FT_SB_REPORT              = 7
};

/**
 * @enum cants_type
 * @brief CAN-TS transfer types
 */
enum cants_type {
    cants_type_time_sync = 0, /**< Time Sync transfer type */
    cants_type_unsolicited_tm, /**< Unsolicited Telemetry transfer type */
    cants_type_telecommand, /**< Telecommand transfer type */
    cants_type_telemetry, /**< Telemetry transfer type */
    cants_type_set_block, /**< Set Block transfer type */
    cants_type_get_block, /**< Get Block transfer type */
};

/**
 * @struct cants_msg
 * @brief CAN-TS message
 */
struct cants_msg {
    uint8_t destination; /**< CAN-TS destination ID */
    uint8_t type; /**< CAN-TS frame type */
    uint8_t source; /**< CAN-TS source ID */
    uint16_t command; /**< CAN-TS command */
    uint8_t length; /**< length of data array */
    uint8_t data[CANTS_FRAME_DATA_LENGTH]; /**< data in CAN-TS message */
};

#endif //CANTS_FRAME_TYPES_H
