/*
 * filter.cpp
 *
 *  Created on: May 2, 2023
 *      Author: shash
 */




void filter(double *b, int b_length, double *x, int x_length, double* filter) {

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
//     for(int i=1; i<x_length; i++) filter[i] = filter[i]+filter[i-1];
}
