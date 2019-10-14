#define OUTPUT_FN "output/result.csv"
#define OUTPUT_MODE "a+"

typedef struct resultInfo{
  int pid;
  int maxSeg;
  int sizePerSeg;
  long double simple;
  long double stress;
  int isSync;
}resultInfo_t;

void generateOutput(resultInfo_t *res);
