/** \file algorithm.cpp ******************************************************
*
* Project: MAXREFDES117#
* Filename: algorithm.cpp
* Description: This module calculates the heart rate/SpO2 level
*
*
* --------------------------------------------------------------------
*
* This code follows the following naming conventions:
*
* char              ch_pmod_value
* char (array)      s_pmod_s_string[16]
* float             f_pmod_value
* int32_t           n_pmod_value
* int32_t (array)   an_pmod_value[16]
* int16_t           w_pmod_value
* int16_t (array)   aw_pmod_value[16]
* uint16_t          uw_pmod_value
* uint16_t (array)  auw_pmod_value[16]
* uint8_t           uch_pmod_value
* uint8_t (array)   auch_pmod_buffer[16]
* uint32_t          un_pmod_value
* int32_t *         pn_pmod_value
*
* ------------------------------------------------------------------------- */
/*******************************************************************************
* Copyright (C) 2016 Maxim Integrated Products, Inc., All Rights Reserved.
*
* Permission is hereby granted, free of charge, to any person obtaining a
* copy of this software and associated documentation files (the "Software"),
* to deal in the Software without restriction, including without limitation
* the rights to use, copy, modify, merge, publish, distribute, sublicense,
* and/or sell copies of the Software, and to permit persons to whom the
* Software is furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included
* in all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
* OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
* IN NO EVENT SHALL MAXIM INTEGRATED BE LIABLE FOR ANY CLAIM, DAMAGES
* OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
* ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
* OTHER DEALINGS IN THE SOFTWARE.
*
* Except as contained in this notice, the name of Maxim Integrated
* Products, Inc. shall not be used except as stated in the Maxim Integrated
* Products, Inc. Branding Policy.
*
* The mere transfer of this software does not imply any licenses
* of trade secrets, proprietary technology, copyrights, patents,
* trademarks, maskwork rights, or any other form of intellectual
* property whatsoever. Maxim Integrated Products, Inc. retains all
* ownership rights.
*******************************************************************************
*/

#include <stdint.h>
#include <math.h>
#include "algorithm.h"
#include "uart.h"
#include <stdint.h>
#include <stdio.h>
//#include "arduino.h"


void maxim_heart_rate_and_oxygen_saturation(uint32_t *pun_ir_buffer, int32_t n_ir_buffer_length, uint32_t *pun_red_buffer, float *pn_spo2, int8_t *pch_spo2_valid,
                int32_t *pn_heart_rate, int8_t *pch_hr_valid, int32_t* sys, int32_t* dia, bool onlyheartrate, bool diasyscalc_en)
/**
* \brief        Calculate the heart rate and SpO2 level, Robert Fraczkiewicz version
* \par          Details
*               By detecting  peaks of PPG cycle and corresponding AC/DC of red/infra-red signal, the xy_ratio for the SPO2 is computed.
*
* \param[in]    *pun_ir_buffer           - IR sensor data buffer
* \param[in]    n_ir_buffer_length      - IR sensor data buffer length
* \param[in]    *pun_red_buffer          - Red sensor data buffer
* \param[out]    *pn_spo2                - Calculated SpO2 value
* \param[out]    *pch_spo2_valid         - 1 if the calculated SpO2 value is valid
* \param[out]    *pn_heart_rate          - Calculated heart rate value
* \param[out]    *pch_hr_valid           - 1 if the calculated heart rate value is valid
*
* \retval       None
*/
{
  volatile float un_ir_mean;
  uint32_t un_only_once ;
  int32_t k, j, n_i_ratio_count;
  int32_t i, s, m, n_exact_ir_valley_locs_count, n_middle_idx;
  volatile int32_t n_th1;
  int32_t n_npks, n_c_min;
  int32_t an_ir_valley_locs[15] ;
  float n_peak_interval_sum;

  int32_t n_y_ac, n_x_ac;
  int32_t n_spo2_calc;
  int32_t n_y_dc_max, n_x_dc_max;
  int32_t n_y_dc_max_idx, n_x_dc_max_idx;
  int32_t an_ratio[5];
  int32_t n_ratio_average;
  int32_t n_nume, n_denom ;

  int32_t A1[100];
  int32_t A4[100];
  int32_t A5[100];

  float f_ir_mean,f_red_mean,f_ir_sumsq,f_red_sumsq;
  float f_y_ac, f_x_ac, xy_ratio;



  // calculates DC mean and subtract DC from ir
  un_ir_mean =0;
  for (k=0 ; k<n_ir_buffer_length ; k++ ) un_ir_mean += pun_ir_buffer[k] ;
  un_ir_mean =un_ir_mean/n_ir_buffer_length ;

  // remove DC and invert signal so that we can use peak detector as valley detector
  for (k=0 ; k<n_ir_buffer_length ; k++ )
    an_x[k] = -1*(pun_ir_buffer[k] - un_ir_mean) ;


  // 4 pt Moving Average
  for(k=0; k< BUFFER_SIZE-MA4_SIZE; k++){
    an_x[k]=( an_x[k]+an_x[k+1]+ an_x[k+2]+ an_x[k+3])/(int)4;
  }
  

  for(k=0; k<BUFFER_SIZE; k++) {
      if(A1[k] <0 & A4[k] > 0) {
          A5[k] = an_x[k];
      } else {
          A5[k] = 0;
      }
   }


  for (k=0; k<BUFFER_SIZE; k++){
      if(A5[k] > 0){
          for (j=0; j<BUFFER_SIZE; j++)
          {
              if(A5[j] <0){
                  *sys = A5[i];
                  *dia = A5[j];
              }
              else
              {   if(A5[j] >0);
                      break;
              }
          }
      }
  }


  // calculate threshold
    n_th1=0;
    for ( k=0 ; k<BUFFER_SIZE ;k++){
      n_th1 +=  an_x[k];
    }
    n_th1=  n_th1/ ( BUFFER_SIZE);
    if( n_th1<30) n_th1=30; // min allowed
    if( n_th1>60) n_th1=60; // max allowed

    for ( k=0 ; k<15;k++) an_ir_valley_locs[k]=0;
    // since we flipped signal, we use peak detector as valley detector
    maxim_find_peaks( an_ir_valley_locs, &n_npks, an_x, BUFFER_SIZE, n_th1, 14, 15 );//peak_height, peak_distance, max_num_peaks
    n_peak_interval_sum =0;
    if (n_npks>=2){
      for (k=1; k<n_npks; k++) n_peak_interval_sum += (an_ir_valley_locs[k] -an_ir_valley_locs[k -1] ) ;
      n_peak_interval_sum =n_peak_interval_sum/(n_npks-1);
      *pn_heart_rate =(int32_t)( (FS*60)/ n_peak_interval_sum );
      *pch_hr_valid  = 1;
    }
    else  {
      *pn_heart_rate = -999; // unable to calculate because # of peaks are too small
      *pch_hr_valid  = 0;
    }


  //using exact_ir_valley_locs , find ir-red DC andir-red AC for SPO2 calibration an_ratio
  //finding AC/DC maximum of raw
  get_spo2(an_x, an_y, n_ir_buffer_length, pn_spo2, pch_spo2_valid);


  return;
//  n_ratio_average =0;
//  n_i_ratio_count = 0;
//  for(k=0; k< 5; k++) an_ratio[k]=0;
//  for (k=0; k< n_exact_ir_valley_locs_count; k++){
//    if (an_ir_valley_locs[k] > BUFFER_SIZE ){
//      *pn_spo2 =  -999 ; // do not use SPO2 since valley loc is out of range
//      *pch_spo2_valid  = 0;
//      return;
//    }
//  }
//  // find max between two valley locations
//  // and use an_ratio betwen AC compoent of Ir & Red and DC compoent of Ir & Red for SPO2
//  for (k=0; k< n_exact_ir_valley_locs_count-1; k++){
//    n_y_dc_max= -16777216 ;
//    n_x_dc_max= -16777216;
//    if (an_ir_valley_locs[k+1]-an_ir_valley_locs[k] >3){
//        for (i=an_ir_valley_locs[k]; i< an_ir_valley_locs[k+1]; i++){
//          if (an_x[i]> n_x_dc_max) {n_x_dc_max =an_x[i]; n_x_dc_max_idx=i;}
//          if (an_y[i]> n_y_dc_max) {n_y_dc_max =an_y[i]; n_y_dc_max_idx=i;}
//      }
//      n_y_ac= (an_y[an_ir_valley_locs[k+1]] - an_y[an_ir_valley_locs[k] ] )*(n_y_dc_max_idx -an_ir_valley_locs[k]); //red
//      n_y_ac=  an_y[an_ir_valley_locs[k]] + n_y_ac/ (an_ir_valley_locs[k+1] - an_ir_valley_locs[k])  ;
//      n_y_ac=  an_y[n_y_dc_max_idx] - n_y_ac;    // subracting linear DC compoenents from raw
//      n_x_ac= (an_x[an_ir_valley_locs[k+1]] - an_x[an_ir_valley_locs[k] ] )*(n_x_dc_max_idx -an_ir_valley_locs[k]); // ir
//      n_x_ac=  an_x[an_ir_valley_locs[k]] + n_x_ac/ (an_ir_valley_locs[k+1] - an_ir_valley_locs[k]);
//      n_x_ac=  an_x[n_y_dc_max_idx] - n_x_ac;      // subracting linear DC compoenents from raw
//      n_nume=( n_y_ac *n_x_dc_max)>>7 ; //prepare X100 to preserve floating value
//      n_denom= ( n_x_ac *n_y_dc_max)>>7;
//      if (n_denom>0  && n_i_ratio_count <5 &&  n_nume != 0)
//      {
//        an_ratio[n_i_ratio_count]= (n_nume*100)/n_denom ; //formular is ( n_y_ac *n_x_dc_max) / ( n_x_ac *n_y_dc_max) ;
//        n_i_ratio_count++;
//      }
//    }
//  }
//  // choose median value since PPG signal may varies from beat to beat
//  maxim_sort_ascend(an_ratio, n_i_ratio_count);
//  n_middle_idx= n_i_ratio_count/2;
//
//  if (n_middle_idx >1)
//    n_ratio_average =( an_ratio[n_middle_idx-1] +an_ratio[n_middle_idx])/2; // use median
//  else
//    n_ratio_average = an_ratio[n_middle_idx ];
//
//  if( n_ratio_average>2 && n_ratio_average <184){
//    n_spo2_calc= uch_spo2_table[n_ratio_average] ;
//    *pn_spo2 = n_spo2_calc ;
//    *pch_spo2_valid  = 1;//  float_SPO2 =  -45.060*n_ratio_average* n_ratio_average/10000 + 30.354 *n_ratio_average/100 + 94.845 ;  // for comparison with table
//  }
//  else{
//    *pn_spo2 =  -999 ; // do not use SPO2 since signal an_ratio is out of range
//    *pch_spo2_valid  = 0;
//  }

}

void get_spo2(float* an_x, float* an_y, int32_t n_ir_buffer_length, float* pn_spo2, int8_t* pch_spo2_valid) {
    float f_ir_mean,f_red_mean,f_ir_sumsq,f_red_sumsq;
      f_ir_mean=0.0;
      f_red_mean=0.0;
      int k;
      for (k=0; k<n_ir_buffer_length; ++k) {
        f_ir_mean += an_x[k];
        f_red_mean += an_y[k];
      }
      f_ir_mean=f_ir_mean/n_ir_buffer_length ;
      f_red_mean=f_red_mean/n_ir_buffer_length ;


    float f_y_ac=rf_rms(an_y,n_ir_buffer_length,&f_red_sumsq);
    float f_x_ac=rf_rms(an_x,n_ir_buffer_length,&f_ir_sumsq);

    float xy_ratio= (f_y_ac*f_ir_mean)/(f_x_ac*f_red_mean);  //formula is (f_y_ac*f_x_dc) / (f_x_ac*f_y_dc) ;

    if(xy_ratio>0.02 && xy_ratio<1.84) { // Check boundaries of applicability
        char string[50];
        sprintf(string, "ratioxy=%f\n ", xy_ratio);
           uca0WriteString(string);
        *pn_spo2 = (-45.060*xy_ratio + 30.354)*xy_ratio + 94.845;
//       *pch_spo2_valid = 1;
     } else {
       *pn_spo2 =  -999 ; // do not use SPO2 since signal an_ratio is out of range
       *pch_spo2_valid  = 0;
     }

     return;

}



void maxim_find_peaks( int32_t *pn_locs, int32_t *n_npks,  float  *pn_x, int32_t n_size, int32_t n_min_height, int32_t n_min_distance, int32_t n_max_num )
/**
* \brief        Find peaks
* \par          Details
*               Find at most MAX_NUM peaks above MIN_HEIGHT separated by at least MIN_DISTANCE
*
* \retval       None
*/
{
  maxim_peaks_above_min_height( pn_locs, n_npks, pn_x, n_size, n_min_height );
  maxim_remove_close_peaks( pn_locs, n_npks, pn_x, n_min_distance );
  *n_npks = min( *n_npks, n_max_num );
}

void maxim_peaks_above_min_height( int32_t *pn_locs, int32_t *n_npks,  float  *pn_x, int32_t n_size, int32_t n_min_height )
/**
* \brief        Find peaks above n_min_height
* \par          Details
*               Find all peaks above MIN_HEIGHT
*
* \retval       None
*/
{
  int32_t i = 1, n_width;
  *n_npks = 0;

  while (i < n_size-1){
    if (pn_x[i] > n_min_height && pn_x[i] > pn_x[i-1]){      // find left edge of potential peaks
      n_width = 1;
      while (i+n_width < n_size && pn_x[i] == pn_x[i+n_width])  // find flat peaks
        n_width++;
      if (pn_x[i] > pn_x[i+n_width] && (*n_npks) < 15 ){      // find right edge of peaks
        pn_locs[(*n_npks)++] = i;
        // for flat peaks, peak location is left edge
        i += n_width+1;
      }
      else
        i += n_width;
    }
    else
      i++;
  }
}

void maxim_remove_close_peaks(int32_t *pn_locs, int32_t *pn_npks, float *pn_x, int32_t n_min_distance)
/**
* \brief        Remove peaks
* \par          Details
*               Remove peaks separated by less than MIN_DISTANCE
*
* \retval       None
*/
{

  int32_t i, j, n_old_npks, n_dist;

  /* Order peaks from large to small */
  maxim_sort_indices_descend( pn_x, pn_locs, *pn_npks );

  for ( i = -1; i < *pn_npks; i++ ){
    n_old_npks = *pn_npks;
    *pn_npks = i+1;
    for ( j = i+1; j < n_old_npks; j++ ){
      n_dist =  pn_locs[j] - ( i == -1 ? -1 : pn_locs[i] ); // lag-zero peak of autocorr is at index -1
      if ( n_dist > n_min_distance || n_dist < -n_min_distance )
        pn_locs[(*pn_npks)++] = pn_locs[j];
    }
  }

  // Resort indices int32_to ascending order
  maxim_sort_ascend( pn_locs, *pn_npks );
}

void maxim_sort_ascend(int32_t  *pn_x, int32_t n_size)
/**
* \brief        Sort array
* \par          Details
*               Sort array in ascending order (insertion sort algorithm)
*
* \retval       None
*/
{
  int32_t i, j, n_temp;
  for (i = 1; i < n_size; i++) {
    n_temp = pn_x[i];
    for (j = i; j > 0 && n_temp < pn_x[j-1]; j--)
        pn_x[j] = pn_x[j-1];
    pn_x[j] = n_temp;
  }
}

void maxim_sort_indices_descend(  float  *pn_x, int32_t *pn_indx, int32_t n_size)
/**
* \brief        Sort indices
* \par          Details
*               Sort indices according to descending order (insertion sort algorithm)
*
* \retval       None
*/
{
  int32_t i, j, n_temp;
  for (i = 1; i < n_size; i++) {
    n_temp = pn_indx[i];
    for (j = i; j > 0 && pn_x[n_temp] > pn_x[pn_indx[j-1]]; j--)
      pn_indx[j] = pn_indx[j-1];
    pn_indx[j] = n_temp;
  }
}

float rf_rms(float *pn_x, int32_t n_size, float *sumsq)
/**
* \brief        Root-mean-square variation
* \par          Details
*               Compute root-mean-square variation for a given series pn_x
*               Robert Fraczkiewicz, 12/25/2017
* \retval       RMS value and raw sum of squares
*/
{
  int16_t i;
  float r,*pn_ptr;
  (*sumsq)=0.0;
  for (i=0,pn_ptr=pn_x; i<n_size; ++i,++pn_ptr) {
    r=(*pn_ptr);
    (*sumsq) += r*r;
  }
  (*sumsq)/=n_size; // This corresponds to autocorrelation at lag=0
  return sqrt(*sumsq);
}


