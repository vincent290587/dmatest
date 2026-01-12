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

struct can_descr {
    struct sockaddr_can addr;
    struct ifreq ifr;
    int s;
    int channel;
    int to_terminate;
};

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

    // ... after socket() and bind() ...

    struct can_filter rfilter[3]; // Array size = number of filters

    // Local address filter
    rfilter[0].can_id   = (local_addr << 21) | CAN_EFF_FLAG;
    rfilter[0].can_mask = (0xFF << 21) | CAN_EFF_FLAG;

    TRACE_INFO("CAN filter0: 0x%08X", rfilter[0].can_id);

    // Timesync filter
    rfilter[1].can_id   = (CANTS_TT_TIMESYNC << 18) | CAN_EFF_FLAG;
    rfilter[1].can_mask = (0x7 << 18) | CAN_EFF_FLAG; 

    TRACE_INFO("CAN filter1: 0x%08X", rfilter[1].can_id);

    // Unsolicitied TM filter
    // TODO now these are always received even for other destinations
    rfilter[2].can_id   = (CANTS_TT_UNSOLICITIED_TELEMETRY << 18) | CAN_EFF_FLAG;
    rfilter[2].can_mask = (0x7 << 18) | CAN_EFF_FLAG; 

    TRACE_INFO("CAN filter2: 0x%08X", rfilter[2].can_id);

    // Apply the filter array to the socket
    setsockopt(can.s, SOL_CAN_RAW, CAN_RAW_FILTER, &rfilter, sizeof(rfilter));

    return 0;
}

static void _process(struct can_descr *pcan, const struct can_frame pcanmsg)
{
    uint32_t ext_id = pcanmsg.can_id & CAN_EFF_MASK;

    TRACE_INFO("CAN-TS received message full ext. ID: 0x%08X", pcanmsg.can_id);

    TRACE_INFO("CAN-TS received message:");
    TRACE_INFO("ID  : 0x%08X", ext_id);
    TRACE_INFO("Len : 0x%08X", pcanmsg.len);

    uint8_t src = (ext_id >> 10u) & 0xFFu;
    TRACE_INFO("Src : 0x%08X", src);
    uint8_t dst = (ext_id >> 21u) & 0xFFu;
    TRACE_INFO("Dst : 0x%08X", dst);

    ///
    
    uint32_t ext_type = (ext_id >> 18) & 0x7u;

    if(ext_type == CANTS_TT_UNSOLICITIED_TELEMETRY)                 /* Checks Destination Address (Local Address), Transfer Type 0x01 (Unsolicited Telemetry), Source Address (Master Address or any), Frame Type (any), Command Channel (any) */
    {
        if (dst == 1u) {
            TRACE_INFO("CANTS_KA");
        } else {
            TRACE_INFO("CANTS_TT_UNSOLICITIED_TELEMETRY");
        }
    }
    else if(ext_type == CANTS_TT_TELECOMMAND)                  /* Checks Destination Address (Local Address), Transfer Type 0x02 (Telecommand), Source Address (Master Address or any), Frame Type (any), Command Channel (any) */
    {
        TRACE_INFO("CANTS_TT_TELECOMMAND");
    }
    else if(ext_type == CANTS_TT_TELEMETRY)                  /* Checks Destination Address (Local Address), Transfer Type 0x03 (Telemetry), Source Address (Master Address or any), Frame Type (any), Command Channel (any) */
    {
        TRACE_INFO("CANTS_TT_TELEMETRY");
    }
    else if(ext_type == CANTS_TT_GETBLOCK)                 /* Checks Destination Address (Local Address), Transfer Type 0x03 (Telemetry), Source Address (Master Address or any), Frame Type (any), Command Channel (any) */
    {
        TRACE_INFO("CANTS_TT_GETBLOCK");
    }
    else if(ext_type == CANTS_TT_SETBLOCK)                 /* Checks Destination Address (Local Address), Transfer Type 0x03 (Telemetry), Source Address (Master Address or any), Frame Type (any), Command Channel (any) */
    {
        TRACE_INFO("CANTS_TT_SETBLOCK");
    }
    else if(ext_type == CANTS_TT_TIMESYNC)             /* Checks Destination Address (Time Synchronization Address), Transfer Type 0x00 (Time Synchronization), Source Address (Master Address or any), Frame Type (any), Command Channel (any) */
    {
        TRACE_INFO("CANTS_TT_TIMESYNC");
    }
    else {
        TRACE_ERROR("Unknown CAN message");
    }

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

    setvbuf(stdout, NULL, _IONBF, 0); // turn off buffering for stdout

    // Other: local 0x09 -> 0x18

    local_addr  = 0x18u;
    master_addr = 0x02u;

    int ret = _init();
    TRACE_INFO("CAN init ret=%d", ret);

    _receive_thread(&can);

    return 0;
}
