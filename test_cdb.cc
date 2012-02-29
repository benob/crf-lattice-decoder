#include "cdb.h"

using namespace macaon;

int main(int argc, char** argv) {
    if(argc != 2) {
        fprintf(stderr, "usage: %s <cdb>\n", argv[0]);
        return 1;
    }
    CDB db(argv[1]);

    FILE* fp = fopen(argv[1], "r");
    uint32_t limit;
    fread(&limit, 4, 1, fp);
    fseek(fp, 2048, SEEK_SET);
    int i = 0;
    while(i < 665438) {
        uint32_t key_length;
        uint32_t data_length;
        fread(&key_length, 4, 1, fp);
        fread(&data_length, 4, 1, fp);
        //fprintf(stderr, "%u %u\n", key_length, data_length);
        char key[key_length + 1];
        fread(key, key_length, 1, fp);
        key[key_length] = '\0';
        char data[data_length + 1];
        fread(data, data_length, 1, fp);
        data[data_length] = '\0';

        char* found = db.find(key);
        if(found != NULL) {
            if(strcmp(found, data) != 0) fprintf(stdout, "error: %s\n", key);
            free(found);
        }
        i++;
    }
    fprintf(stdout, "%d elements\n", i);
    fclose(fp);
}

