#include "intelhex.h"
#include "stdio.h"
#include "string.h"

#define LOG(args...)     //   printf(args)

// logic - Every buf 512 bytes
//  - Build and return structure
//  - partial structure needs to be persistent (2 of struct)
//  - line size doesnt matter
//  - always write decoded data to RAM

#define MAX_RECORD_SIZE   0x25

typedef enum {
    DATA_RECORD = 0,
	EOF_RECORD  = 0x01, /* End Of File */
	ESA_RECORD  = 0x02, /* Extended Segment Address */
	SSA_RECORD  = 0x03, /* Start Segment Address */
	ELA_RECORD  = 0x04, /* Extended Linear Address */
	SLA_RECORD  = 0x05,  /* Start Linear Address */
} hex_record_t;


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
    for ( ; i < (record->byte_count + 5); i++) {
        result += record->buf[i];
    }
    return result == 0;
}

void reset_hex_parser(void)
{

}

int32_t intelhex_parse(uint8_t **p_hex_buf, uint8_t **p_bin_buf, uint32_t *address, uint32_t hex_buf_size,  uint32_t bin_buf_size)
{
    static hex_line_t line;
    static uint8_t low_nibble = 0, idx = 0;
    static uint32_t base_address = 0;
    static uint8_t record_processed = 0;
    
    uint8_t *hex_ptr = *p_hex_buf;
    uint8_t *bin_ptr = *p_bin_buf;
    uint8_t *input_buf = hex_ptr;
    uint8_t *hex_end = hex_ptr + hex_buf_size;
    uint32_t bin_size = 0;
    
    int32_t return_value = INTELHEX_TO_CONTINUE;
    *address = 0;

    while (hex_ptr < hex_end) {
        uint8_t ch = *hex_ptr;
        hex_ptr++;
        
        if (':' == ch) {
            memset(line.buf, 0, sizeof(hex_line_t));
            low_nibble = 0;
            idx = 0;
            record_processed = 0;
        } else if (('\r' == ch) || ('\n' == ch)) {  // End Of Line
            if (1 == record_processed) {   // line is already processed 
                continue;
            } else if (0 == record_processed) {
                line.address = (line.buf[1] << 8) + line.buf[2];  // fix address
            }
            
            record_processed = 1;

            if (!validate_checksum(&line)) {
                return INTELHEX_CHECKSUM_MISMATCH;
            }

            if (DATA_RECORD == line.record_type) {
                // find discrete blocks, process current line next time
                if (bin_size && ((base_address + line.address) != *address)) {
                    LOG("Memory from 0x%X to 0x%X is skipped.\n", *address, base_address + line.address);
                
                    hex_ptr--;             // roll back EOF, leave current line to process next time
                    record_processed = 2;  // avoid changing line.address next time
                    
                    return_value = INTELHEX_TO_WRITE;
                    break;
                }
                
                // bin buffer has not enough space left, process current line next time
                if ((bin_size + line.byte_count) > bin_buf_size) {
                    LOG("bin buffer has not enough space.\n");
                    
                    hex_ptr--;             // roll back EOF, leave current line to process next time
                    record_processed = 2;  // avoid changing line.address next time
                    
                    return_value = INTELHEX_TO_WRITE;
                    break;
                }

                // move data from line buffer to bin buffer
                memcpy(bin_ptr, (uint8_t *)line.data, line.byte_count);
                bin_ptr  += line.byte_count;
                bin_size += line.byte_count;
                
                // store the end address of bin data
                *address = base_address + line.address + line.byte_count;
            } else if (EOF_RECORD == line.record_type) {
                return_value = INTELHEX_DONE;
                break;
            } else {
                if (ELA_RECORD == line.record_type) {
                    base_address = (line.data[0] << 24) | (line.data[1] << 16);
                    
                    LOG("Extend Lienar Address: 0x%X\n", base_address);
                } else if (ESA_RECORD == line.record_type) {
                    base_address = (line.data[0] << 12) | (line.data[1] << 4);
                    
                    LOG("Extend Segment Address: 0x%X\n", base_address);
                }
                                
                return_value = INTELHEX_TO_WRITE;
                break;
            }
        } else {
            if (low_nibble) {
                if (idx < sizeof(line)) {
                    line.buf[idx] |= ctoh(ch) & 0xf;
                    idx++;
                } else {
                    LOG("Line is too long. Line buffer is not enough.\n");
                    return INTELHEX_LINE_TOO_LONG;
                }
            }
            else {
                line.buf[idx] = ctoh(ch) << 4;
            }
            low_nibble = !low_nibble;
        }
    }

    *address = *address - bin_size; // calculate the start address for bin data
    *p_bin_buf = bin_ptr;
    *p_hex_buf = hex_ptr;
    
    return return_value;
}

