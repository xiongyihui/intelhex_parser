
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "intelhex.h"

#define BIN_PAGE_SIZE   512

uint8_t hex_buf[512];
uint8_t bin_buf[BIN_PAGE_SIZE];

int main(int argc, char *argv[]) {
    int status;
    uint8_t *hex_ptr;
    uint8_t *hex_end;
    uint8_t *bin_ptr = bin_buf;
    uint8_t *bin_end = bin_buf + sizeof(bin_buf);
    uint32_t addr = 0;
    uint32_t continuous_data_start = 0;
    uint32_t continuous_data_size = 0;
    uint32_t bin_size;
    uint32_t bin_start = 0;
    
    FILE *f;
    char *filename = "example.hex";

    if (argc > 1) {
        filename = argv[1];
    }

    f = fopen(filename, "r");
    if (NULL == f) {
        printf("Can not open %s.\n", filename);
        return 1;
    }
    

	while (1) {
        int len = fread(hex_buf, 1, sizeof(hex_buf), f);
        if (len <= 0) {
            fclose(f);
            printf("End Of File. File is incomplete\n");
            return -1;
        }
        
        hex_end = hex_buf + len;
        hex_ptr = hex_buf;
        do {
            uint32_t hex_buf_size = hex_end - hex_ptr;
            uint32_t bin_buf_size = bin_end - bin_ptr;
            
            uint8_t *last_bin_ptr = bin_ptr;
            uint32_t new_bin;
            
            status = intelhex_parse(&hex_ptr, &bin_ptr, &addr, hex_buf_size, bin_buf_size);

            if (last_bin_ptr == bin_buf) {
                bin_start = addr;
            }
            
            bin_size = bin_ptr - bin_buf;
            
            new_bin = bin_ptr - last_bin_ptr;
            if (new_bin && (continuous_data_start + continuous_data_size) != addr) {
                if (continuous_data_size) {
                    printf("0x%X: 0x%X bytes data block\n", continuous_data_start, continuous_data_size);
                }
                continuous_data_start = addr;
                continuous_data_size  = new_bin;
            } else {
                continuous_data_size += new_bin;
            }
            
            if (0 > status) {
                printf("Invalid record. Exit\n");
                fclose(f);
                return -1;
            }
            
            if (INTELHEX_DONE == status) {                
                // write bin data
                printf("--> 0x%X: 0x%X bin\n", bin_start, bin_size);
                
                if (continuous_data_size) {
                    printf("0x%X: 0x%X bytes data block\n", continuous_data_start, continuous_data_size);
                }
            
                printf("Get EOF record. Mission Complete\n");
                fclose(f);
                return 0;
            }
            
            if (INTELHEX_TO_WRITE == status) {
                if (bin_size) {
                    // write bin data
                    printf("--> 0x%X: 0x%X bin\n", bin_start, bin_size);
                    
                    bin_ptr = bin_buf;
                }
                
                continue;
            }
            
            if (BIN_PAGE_SIZE <= bin_size) {
                // write a page of bin data
                printf("--> 0x%X: 0x%X bin\n", bin_start, bin_size);
                
                memcpy(bin_buf, bin_buf + BIN_PAGE_SIZE, bin_size - BIN_PAGE_SIZE);
                bin_ptr -= BIN_PAGE_SIZE;
            }

        } while (hex_ptr < hex_end); 

	}
	
	return EXIT_SUCCESS;
}

