#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "OutputGenerator.h"

void generateOutput(resultInfo_t *res){
  FILE *fpOut = fopen(OUTPUT_FN, OUTPUT_MODE);
  if(fpOut == NULL){
    printf("%s open error\n", OUTPUT_FN);
    return;
  }
  fprintf(fpOut, "seg%d, %8.3Lf, %8.3Lf, %5d, %5d\n", res->sizePerSeg,
      res->stress, res->simple, res->isSync, res->maxSeg);

  fclose(fpOut);
}

