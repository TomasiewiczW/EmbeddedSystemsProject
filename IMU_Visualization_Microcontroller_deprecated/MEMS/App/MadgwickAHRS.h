/*
 * MadgwickAHRS.h
 *
 *  Created on: Nov 26, 2020
 *      Author: Szczena
 *      Comments: Based on the excellent implementation by SOH Madgwick
 */

#ifndef APP_MADGWICKAHRS_H_
#define APP_MADGWICKAHRS_H_

#include <motion_fx.h>
//----------------------------------------------------------------------------------------------------
// Variable declaration

extern volatile float beta;				// algorithm gain
extern volatile float q0, q1, q2, q3;	// quaternion of sensor frame relative to auxiliary frame

//---------------------------------------------------------------------------------------------------
// Function declarations

void MadgwickAHRS(MFX_input_t *pdata_in, MFX_output_t *pdata_out, float delta_time);
void MadgwickAHRS_6x_Fallback(float gx, float gy, float gz, float ax, float ay, float az, MFX_output_t *pdata_out, float delta_time);

#endif /* APP_MADGWICKAHRS_H_ */
