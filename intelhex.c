#include "intelhex.h"
#include "stdio.h"
#include "string.h"

#define LOG(args...)        printf(args)

// logic - Every buf 512 bytes
//  - Build and return structure
//  - partial structure needs to be persistent (2 of struct)
//  - line size doesnt matter
//  - always write decoded data to RAM

#define MAX_RECORD_SIZE   0x25


typedef union hex_line_t hex_line_t;
union __attribute__((packed)) hex_line_t {
    uint8_t buf[MAX_RECORD_SIZE];
    struct __attribute__((packed)) {
        uint8_t  byte_count;
        uint16_t address;
        uint8_t  record_type;
        uint8_t  data[MAX_RECORD_SIZE - 4];        // include checksum
    };
};

/** Converts a character representation of a hex to real value.
 *   @param c is the hex value in char format
 *   @return the value of the hex
 */
static uint8_t ctoh(char c)
{
    return (c & 0x10) ? /*0-9*/ c & 0xf : /*A-F, a-f*/ (c & 0xf) + 9;
}

/** Calculate checksum on a hex record
 *   @param data is the line of hex record
 *   @param size is the length of the data array
 *   @return 1 if the data provided is a valid hex record otherwise 0
 */
static uint8_t validate_checksum(hex_line_t *record)
{
    uint8_t i = 0, result = 0;
    for ( ; i < (record->byte_count+5); i++) {
        result += record->buf[i];
    }
    return result == 0;
}

void reset_hex_parser(void)
{

}

int32_t parse_hex_blob(uint8_t **pbuf, uint32_t *size, uint32_t *address)
{
    static hex_line_t line;
    static uint8_t low_nibble = 0, idx = 0;
    static uint32_t base_address = 0;
    static uint8_t record_processed = 0;
    
    uint8_t *ptr = *pbuf;
    uint8_t *input_buf = ptr;
    uint8_t *end = ptr + *size;
    uint8_t *line_header = ptr;
    
    int32_t return_value = DATA_RECORD;

    *size = 0;

    while (ptr < end) {
        uint8_t ch = *ptr;
        ptr++;
        
        if (':' == ch) {
            line_header = ptr - 1;
            memset(line.buf, 0, sizeof(hex_line_t));
            low_nibble = 0;
            idx = 0;
            record_processed = 0;
        } else if (('\r' == ch) || ('\n' == ch)) {  // End Of Line
            if (record_processed) {   // line is already processed 
                continue;
            }

            if (!validate_checksum(&line)) {
                return INVALID_RECORD;
            }

            line.address = (line.buf[1] << 8) + line.buf[2];  // fix address

            record_processed = 1;

            if (DATA_RECORD == line.record_type) {
                // verify this is a continous block of memory or need to exit and dump
                if ((*size != 0) && (base_address + line.address) != *address) {
                    LOG("Memory from 0x%X to 0x%X is skipped.\n", *address, base_address + line.address);
                
                    ptr = line_header;                       // leave current line to parse next time
                    return_value = DATA_RECORD;
                    break;
                }

                // move from line buffer back to input buffer
                //  memcpy((uint8_t *)input_buf + *size, (uint8_t *)line.data, line.byte_count);
                *size += line.byte_count;
                // this stores the last known end address of decoded data
                *address = base_address + line.address + line.byte_count;
            } else {
                if (ELA_RECORD == line.record_type) {
                    base_address = (line.data[0] << 24) | (line.data[1] << 16);
                    
                    LOG("Extend Lienar Address: 0x%X\n", base_address);
                } else if (ESA_RECORD == line.record_type) {
                    base_address = (line.data[0] << 12) | (line.data[1] << 4);
                    
                    LOG("Extend Segment Address: 0x%X\n", base_address);
                }
                
                return_value = line.record_type;

                break;
            }
        } else {
            if (low_nibble) {
                line.buf[idx] |= ctoh(ch) & 0xf;
                idx++;
            }
            else {
                line.buf[idx] = ctoh(ch) << 4;
            }
            low_nibble = !low_nibble;
        }
    }

    *address = *address - *size; // figure the start address for the buffer before returning
    *pbuf = ptr;
    return return_value;
}

