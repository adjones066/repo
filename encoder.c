#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main( int argc, char *argv[]) {
    const char* inFile = argv[1];
    const char* outFile = argv[2];
    char *secret = argv[3];

    FILE* fp= fopen(argv[1], "r");
    FILE* fp2 = fopen(argv[2], "w+");

   // printf("%s\n",argv[1]);
   // printf("%s\n",argv[2]);
   // printf("%s\n",argv[3]);



   int pixarray;

   int imgwidth;

   int imgheight;

   int pixarrayoffset = 10;

   int widthoffset = 18;

   //fseek finds offset and puts pointer at offset in the file
   //fread then takes that pointer and reads the amount of bytes specified after that pointer

   fseek(fp, pixarrayoffset, SEEK_SET);
   fread(&pixarray,4,1,fp);

   fseek(fp,widthoffset,SEEK_SET);
   fread(&imgwidth,4,1,fp);

   fread(&imgheight,4,1,fp);
   printf("Pixel array starts at %d\n",pixarray);
   printf("Height of image is %d\n",imgheight);
   printf("Width of image is %d\n",imgwidth);



   char header[54];
   for(int i = 0; i < 54; i++){
       fseek(fp,i,SEEK_SET);
       header[i] = getc(fp);
       fputc(header[i],fp2); // fputc(input char, output file stream)
   }

    fseek(fp,pixarray,SEEK_SET);
   int charIn = 0;
   int bitIn = 0;
   int null = 0;

   for(int i = 0; i < abs(imgheight*imgwidth)*4; i++){
       int byte = getc(fp);
       if(i % 4 != 3 && charIn <= strlen(secret)){  // skipping alpha channel
           char leastsigbit = (secret[charIn] >> bitIn) & 1;
           // put leastsigbit into rgb values and then write it
           byte = (byte >> 1) << 1;
           byte = byte | leastsigbit;
           fputc(byte,fp2);
           bitIn++;
           if (bitIn > 7){
               bitIn = 0;
               charIn++;
           }
       } else if(charIn >= strlen(secret) && null < 8){ //putting in null bytes by clearing leastsigbit color byte to encode null
           byte = (byte >> 1) << 1;
           fputc(byte,fp2);
           null++;
       } else {
           fputc(byte,fp2);
       }
   }

   fclose(fp);
   fclose(fp2);

   return 0;
}
