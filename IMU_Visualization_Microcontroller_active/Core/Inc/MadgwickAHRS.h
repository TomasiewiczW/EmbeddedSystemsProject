/*
 * MadgwickAHRS.h
 *
 *  Created on: Nov 26, 2020
 *      Author: Szczena
 *      Comments: Based on the excellent implementation by SOH Madgwick
 */

#ifndef APP_MADGWICKAHRS_H_
#define APP_MADGWICKAHRS_H_

//----------------------------------------------------------------------------------------------------
// Variable declaration

extern volatile float beta;				// algorithm gain
extern volatile float q0, q1, q2, q3;	// quaternion of sensor frame relative to auxiliary frame

//---------------------------------------------------------------------------------------------------
// Function declarations

void MadgwickAHRS(float ax, float ay, float az, float gx, float gy, float gz, float mx, float my, float mz); // agm order
void MadgwickAHRS_6x_Fallback(float ax, float ay, float az, float gx, float gy, float gz);

#endif /* APP_MADGWICKAHRS_H_ */
