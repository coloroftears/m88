//  $Id: writetag.cpp,v 1.1 2000/02/18 02:11:56 cisc Exp $

#include <stdio.h>
#include <stdint.h>

int main(int ac, char** av) {
  if (ac == 2) {
    FILE* fp = fopen(av[1], "r+b");
    if (fp) {
      int len;

      fseek(fp, 0, SEEK_END);
      len = ftell(fp);
      fseek(fp, 0, SEEK_SET);

      uint8_t* mod = new uint8_t[len];
      if (mod) {
        fread(mod, 1, len, fp);

        const int tagpos = 0x7c;
        uint32_t crc = 0xffffffff;
        *(uint32_t*)(mod + tagpos) = 0;

        uint32_t crctable[256];
        uint32_t i;
        for (i = 0; i < 256; i++) {
          uint32_t r = i << 24;
          for (int j = 0; j < 8; j++)
            r = (r << 1) ^ (r & 0x80000000 ? 0x04c11db7 : 0);
          crctable[i] = r;
        }

        // CRC 計算

        for (i = 0; i < len; i++)
          crc = (crc << 8) ^ crctable[((crc >> 24) ^ mod[i]) & 0xff];
        delete[] mod;

        printf("crc = %.8x\n", crc);
        crc = ~crc;
        fseek(fp, tagpos, SEEK_SET);
        fwrite(&crc, sizeof(crc), 1, fp);
      }
      fclose(fp);
    }
  }
  return 0;
}
