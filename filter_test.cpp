#include <stdio.h>

#include "data.h"

void filter(float *b, int b_length, float *x, int x_length, float* filter) {
      filter[0] = b[0]*x[0];

      for (int i = 1; i < x_length; i++) {
        filter[i] = 0.0;
        for (int j = 0; j <= i; j++) {
          int k = i-j;
          if (j > 0) {
            if ((k < b_length) && (j < x_length)) {
              filter[i] += b[k]*x[j];
            }
            if ((k < x_length)) {
              filter[i] -= 1*filter[k];
            }
          } else {
            if ((k < b_length) && (j < x_length)) {
              filter[i] += (b[k]*x[j]);
            }
          }
        }
      }
}


int main() {
    float filterx[4000];
    for(int i=0; i<4000; i++) data[i] = -data[i];

    filter(b, 49, data, 4000, filterx);
   
    for(int i=1; i<4000; i++) filterx[i] = filterx[i]+filterx[i-1];
     for(int i=0; i<4000; i++) printf("%f\n", filterx[i]);
}