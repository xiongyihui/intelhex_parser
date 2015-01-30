
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "intelhex.h"

uint8_t buf[1024];

int main(int argc, char *argv[]) {
    int status;
    uint8_t *ptr;
    uint8_t *end;
    uint32_t size = 0;
    uint32_t addr = 0;
    uint32_t start_addr = 0;
    uint32_t data_size = 0;
    
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
        int len = fread(buf, 1, sizeof(buf), f);
        if (len <= 0) {
            fclose(f);
            printf("End Of File. File is incomplete\n");
            return -1;
        }
        
        end = buf + len;

        ptr = buf;
        do {
            size = end - ptr;
            status = parse_hex_blob(&ptr, &size, &addr);
            
            if (DATA_RECORD != status || 0 != size) {
                if ((start_addr + data_size) == addr) {
                    data_size += size;
                } else {
                    printf("0x%X: 0x%X bytes data block\n", start_addr, data_size);
                    start_addr = addr;
                    data_size = size;
                }
            }
            
            
            
            if (EOF_RECORD == status) {
                if (data_size != 0) {
                    printf("0x%X: 0x%X bytes data block\n", start_addr, data_size);
                }
            
                printf("End Of File. Mission Complete\n");
                fclose(f);
                return 0;
            } else if (INVALID_RECORD == status) {
                printf("Invalid record. Exit\n");
                fclose(f);
                return -2;
            }

        } while (ptr < end); 

	}
	
	return EXIT_SUCCESS;
}

