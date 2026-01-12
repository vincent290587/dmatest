#include <string.h>
#include <unistd.h>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <linux/can/error.h>
#include <linux/can/netlink.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

/////////////////////////////////////////////////////////

#ifndef __PRINTF__
#define __PRINTF__(...)  printf(__VA_ARGS__)
#endif

#ifndef __VPRINTF__
#define __VPRINTF__(...) vprintf(__VA_ARGS__)
#endif

typedef enum {
    eTraceTypeRaw = 0,
    eTraceTypeComplete,
} eTraceType;

typedef enum {
    eTraceLevelDebug = 0,
    eTraceLevelInfo,
    eTraceLevelWarning,
    eTraceLevelError,
    eTraceLevelFatal,
} eTraceLevel;

#define TRACE_BASIC(level, ...)         do{ trace_printf_internal(level, eTraceTypeComplete, __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__); } while(0)

#define TRACE_INFO(...)  	                TRACE_BASIC(eTraceLevelInfo, __VA_ARGS__)
#define TRACE_ERROR(...)  	                TRACE_BASIC(eTraceLevelError, __VA_ARGS__)

/////////////////////////////////////////////////////////

#include "cants_frame_types.h"

/**
 * Makes CAN-TS external ID: 0b000.dddddddd.ttt.ssssssss.cc.cccccccc
 */
#define CANTS_EXT_ID(dst, trans_type, src, cmd_type, cmd_channel)\
                    (((uint32_t)(dst)        << 21U) |\
                    ((uint32_t)(trans_type)  << 18U) |\
                    ((uint32_t)(src)         << 10U) |\
                    ((uint32_t)(cmd_type)    <<  8U) |\
                    ((uint32_t)(cmd_channel) <<  0U))

struct can_descr {
    struct sockaddr_can addr;
    struct ifreq ifr;
    int s;
    int channel;
    int to_terminate;
};

static uint32_t flt_keepalive = 0U;
static uint32_t flt_mask      = 0U;
static uint32_t flt_timesync  = 0U;
static uint32_t flt_tlc       = 0U;
static uint32_t flt_tlm       = 0U;
static uint32_t flt_gblk      = 0U;
static uint32_t flt_sblk      = 0U;
static uint32_t flt_unsol_tlm = 0U;

static uint8_t local_addr  = 0U;                                                /* Indicates CAN-TS address of local device */
static uint8_t master_addr = 0U;                                                /* Indicates CAN-TS address of master device, or 0 if master mode used */

static struct can_descr can = {0};

/////////////////////////////////////////////////////////

static void trace_printf_internal(eTraceLevel level, eTraceType type, const char * const file, 
    const char * const function, const int line, const char * const format, ...) {

    va_list args;
    va_start( args, format );

    __VPRINTF__(format, args);
    __PRINTF__("\r\n");

    va_end(args);

}


static int _init() {

    // Create CAN RAW socket
    can.s = socket(PF_CAN, SOCK_RAW, CAN_RAW);
    
    can.channel = 0;
    can.to_terminate = 0;

    char channame[] = "canX";
    channame[3] = '0' + can.channel;

    strcpy(can.ifr.ifr_name, channame);
    ioctl(can.s, SIOCGIFINDEX, &can.ifr);

    char buffer[256];

    snprintf(buffer, sizeof(buffer), "ip link set can%d down", can.channel);
    (void)system(buffer);

    snprintf(buffer, sizeof(buffer), "ip link set can%d up type can bitrate 1000000", can.channel);
    int ret = system(buffer);
    if (ret) {
        return ret;
    }

    can.addr.can_family  = AF_CAN;
    can.addr.can_ifindex = can.ifr.ifr_ifindex;

    ret = bind(can.s, (struct sockaddr *)&can.addr, sizeof(can.addr));
    if (ret) {
        return ret;
    }

    
    flt_mask = CANTS_EXT_ID(0xFFU, 0x07U, ((master_addr == 0u) ? 0U : 0xFFU),   /* Masks only Destination Address and Frame Type, also masks Source Address in slave mode */
                            0U, 0U);

    flt_timesync = CANTS_EXT_ID(CANTS_BROADCAST_TIMESYNC_ADDR,                  /* Checks Destination Address (Broadcast Time Synchronization Address), Transfer Type 0x00 (Time Synchronization), Source Address (Master Address or any), Frame Type (any), Command Channel (any) */
                                 CANTS_TT_TIMESYNC,
                                 master_addr, 0U, 0U);

    flt_keepalive = CANTS_EXT_ID(CANTS_BROADCAST_KEEPALIVE_ADDR,                /* Checks Destination Address (Broadcast Keep-Alive Address), Transfer Type 0x01 (Unsolicited Telemetry), Source Address (Master Address or any), Frame Type (any), Command Channel (any) */
                                 CANTS_TT_UNSOLICITIED_TELEMETRY,
                                 master_addr, 0U, 0U);

    flt_tlc = CANTS_EXT_ID(local_addr, CANTS_TT_TELECOMMAND,                    /* Checks Destination Address (Local Address), Transfer Type 0x02 (Telecommand), Source Address (Master Address or any), Frame Type (any), Command Channel (any) */
                           master_addr, 0U, 0U);

    flt_tlm = CANTS_EXT_ID(local_addr, CANTS_TT_TELEMETRY,                      /* Checks Destination Address (Local Address), Transfer Type 0x03 (Telemetry), Source Address (Master Address or any), Frame Type (any), Command Channel (any) */
                           master_addr, 0U, 0U);

    flt_gblk = CANTS_EXT_ID(local_addr, CANTS_TT_GETBLOCK,                      /* Checks Destination Address (Local Address), Transfer Type 0x03 (Telemetry), Source Address (Master Address or any), Frame Type (any), Command Channel (any) */
                          master_addr, 0U, 0U);

    flt_sblk = CANTS_EXT_ID(local_addr, CANTS_TT_SETBLOCK,                      /* Checks Destination Address (Local Address), Transfer Type 0x03 (Telemetry), Source Address (Master Address or any), Frame Type (any), Command Channel (any) */
                          master_addr, 0U, 0U);

    flt_unsol_tlm = CANTS_EXT_ID(local_addr,                                    /* Checks Destination Address (Local Address), Transfer Type 0x01 (Unsolicited Telemetry), Source Address (Master Address or any), Frame Type (any), Command Channel (any) */
                                 CANTS_TT_UNSOLICITIED_TELEMETRY,
                                 master_addr, 0U, 0U);


    return 0;
}

static void _process(struct can_descr *pcan, const struct can_frame pcanmsg)
{
    uint32_t ext_id = pcanmsg.can_id & CAN_EFF_MASK;
    uint32_t ext_flt = (ext_id & flt_mask);

    TRACE_INFO("CAN-TS received message ext. ID: 0x%07X", ext_id);

    TRACE_INFO("CAN-TS received message:");
    TRACE_INFO("ID  : 0x%08X", pcanmsg.can_id);
    TRACE_INFO("Len : 0x%08X", pcanmsg.len);
    TRACE_INFO("Type: 0x%08X", pcanmsg.data);

}

static void _receive_thread(struct can_descr *pcan) {

    struct can_frame frame;

    while (true) {

        int nbytes = read(pcan->s, &frame, sizeof(frame));
        if (nbytes < 0) {
            perror("read");
            return;
        } else if (nbytes == sizeof(frame)) {
            _process(pcan, frame);
        } else {
            TRACE_INFO("Read %u butes", nbytes);
        }

        if (pcan->to_terminate) {
            close(pcan->s);
            return;
        }
    }

    char buffer[256];
    snprintf(buffer, sizeof(buffer), "ip link set can%d down", pcan->channel);
    int ret = system(buffer);
    if (ret) {
        TRACE_ERROR("can get ip link set failed");
    }
}

/////////////////////////////////////////////////////////

int main(void) {

    // Other: local 0x09 -> 0x18

    local_addr  = 0x10u;   // 0x10u
    master_addr = 0x18u;   // 0x18u

    _init();

    _receive_thread(&can);

    return 0;
}
