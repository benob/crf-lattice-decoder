#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

namespace macaon {
    class CDB {
        FILE* fp;
        uint32_t table_location[256];
        uint32_t table_length[256];

        static uint32_t hash_function(const char* data, int length) {
            uint32_t h = 5381;
            int i;
            for(i = 0; i < length; i++) {
                h = ((h << 5) + h) ^ data[i];
            }
            return h;
        }

        public:
        CDB(const char* filename) : fp(NULL) {
            size_t i;
            fp = fopen(filename, "r");
            uint32_t header[512];
            fread(header, sizeof(uint32_t), 512, fp);
            for(i = 0; i < 256; i++) {
                table_location[i] = header[i * 2];
                table_length[i] = header[i * 2 + 1];
            }
        }

        ~CDB() {
            if(fp) fclose(fp);
        }

        char* find(const char* target) {
            size_t target_length = strlen(target);
            uint32_t h = hash_function(target, strlen(target));
            uint32_t n = h & 0xff;
            uint32_t h_in_table = h >> 8;
            uint32_t len = table_length[n];
            uint32_t pos = table_location[n];
            uint32_t offset = 0;
            while(offset < len) {
                uint32_t base = pos + 8 * ((offset + h_in_table) % len);
                fseek(fp, base, SEEK_SET);
                uint32_t values[2];
                fread(values, sizeof(uint32_t), 2, fp);
                if(values[0] == 0) return NULL;
                if(values[0] == h) {
                    uint32_t record = values[1];
                    fseek(fp, record, SEEK_SET);
                    fread(values, sizeof(uint32_t), 2, fp);
                    uint32_t key_length = values[0];
                    if(key_length == target_length) {
                        char key[key_length + 1];
                        fread(key, key_length, 1, fp);
                        key[key_length] = '\0';
                        if(strcmp(key, target) == 0) {
                            uint32_t data_length = values[1];
                            char* data = (char*) malloc(data_length + 1);
                            fread(data, data_length, 1, fp);
                            data[data_length] = '\0';
                            return data;
                        }
                    }
                }
                offset++;
            }
            return NULL;
        }
    };
}
