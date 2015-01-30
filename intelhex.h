#ifndef INTELHEX_H
#define INTELHEX_H

#include "stdint.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum hex_record_t {
    INVALID_RECORD = -1,
    DATA_RECORD = 0,
	EOF_RECORD  = 0x01, /* End Of File */
	ESA_RECORD  = 0x02, /* Extended Segment Address */
	SSA_RECORD  = 0x03, /* Start Segment Address */
	ELA_RECORD  = 0x04, /* Extended Linear Address */
	SLA_RECORD  = 0x05  /* Start Linear Address */
} hex_record_t;

/** Prepare any state that is maintained for the start of a file
 *   @return none
 */
void reset_hex_parser(void);

/** Convert a blob of hex data into its binary equivelant
 *   @param pbuf The input and formatted output buffer for data
 *   @param start_address The address the data converted in the buffer starts at
 *   @param size The amount of data in the buffer
 *   @return >=0 - complete, 
 */
int32_t parse_hex_blob(uint8_t **pbuf, uint32_t *size, uint32_t *start_address);
      
#ifdef __cplusplus
}
#endif

#endif
