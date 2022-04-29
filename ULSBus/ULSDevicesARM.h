/** 
 *  Copyright: 2022 by UAVLAS  <www.uavlas.com> 
 *  Author: Yury Kapacheuski <yk@uavlas.com> 
 *
 * This file is part of UAVLAS project applications.
 *
 * This is free software: you can redistribute
 * it and/or modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 *
 * Some open source application is distributed in the hope that it will
 * be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with Foobar.  If not, see <http://www.gnu.org/licenses/>.
 *
 * @license LGPL-3.0+ <https://spdx.org/licenses/LGPL-3.0+>
 */
 
// THIS FILE GENERATED AUTOMATICALY DO NOT EDIT

#include "ULSObject.h"

#ifndef ULSDEVICE_ULSQX_H
#define ULSDEVICE_ULSQX_H


#define __ULS_DEVICE_TYPE_ULSQT1R1 (0x0010)
#define __ULS_DEVICE_TYPE_ULSQR1R1 (0x0011)
#define __ULS_DEVICE_TYPE_ULSQG1R1 (0x0012)
#define __ULS_DEVICE_TYPE_ULSQM1R1 (0x0013)

#define __ULS_DEVICE_TYPE_ULSQT1R1_NAME "ULSQT1R1"
#define __ULS_DEVICE_TYPE_ULSQR1R1_NAME "ULSQR1R1"
#define __ULS_DEVICE_TYPE_ULSQG1R1_NAME "ULSQG1R1"
#define __ULS_DEVICE_TYPE_ULSQM1R1_NAME "ULSQM1R1"

//ULSQT1-R1 status information
typedef __ULS_PACKET( struct {
    uint32_t status; // Device status bitfield
    uint32_t error; // Device error bitfield
    uint32_t Iled[37]; // Leds current
    float vs; // Supply Voltage
    float ih; // Leds current monitor hi side
    float il; // Leds current monitor low side
    float tb; // Board temperatire
    float timu; // IMU temperatire
    float imua[3]; // IMU accelerations
    float imug[3]; // IMU gyroscopes
    float imum[3]; // IMU magnetometers
    float imu[3]; // IMU attitude
}) __ULSObjectStruct_ULSQT1R1Status;

//ULSQT1-R1 configuration
typedef __ULS_PACKET( struct {
    char name[16]; // Name
    float power; // Illumination power
    float Voff; // Voltage to switch-off device
    float Vlow; // Low voltage warning
    float magCalOffset[3]; // Compass calibration offset
    float magCalScale[3]; // Compass calibration scale
    uint8_t ctrl; // Transmitter controll byte
}) __ULSObjectStruct_ULSQT1R1Config;

//ULSQT1-R1 status information
typedef __ULS_PACKET( struct {
    uint32_t status; // Device status bitfield
    uint32_t error; // Device error bitfield
    uint32_t blitzTest; // Debug: blitzTest
    uint32_t packCntr; // Debug:
    uint16_t bitMax; // Debug:
    uint8_t qtId; // Debug:
    uint8_t synqChannel; // Debug:
    uint32_t posTime; // Debug:
    uint32_t emsTime; // Debug:
    uint32_t synqMax; // Debug:
    float level; // Signal level
    float levelA; // Signal level A channel
    float levelB; // Signal level B channel
    float snrA; // Signal to noise ratio for channel A
    float snrB; // Signal to noise ratio for channel B
    float prob; // Signal probability
    float mrxDistance; // Distance provided by MRX algorithm
    float mrxYaw; // Yaw orintation provided by MRX algorithm
    float distance; // Distance to transmitter
    float ang[3]; // Receiver angular position in transmitter frame
    float pos[3]; // Receiver position in transmitter frame
    float vel[3]; // Receiver velocity in transmitter frame
    float gimu[3]; // Transmitter orientation (roll, pitch, yaw)
    float rel_pos_ned[3]; // Relative Position of transmitter (offset from receiver)in NED (North East Down)
    float rel_vel_ned[3]; // Ralative velocity of tranmitter (offset from receiver) in NED (North East Down)
    float rel_pos_frd[3]; // Relative Position of transmitter (offset from receiver) in FRD (Forward Right Down)
    float rel_vel_frd[3]; // Ralative velocity of tranmitter (offset from receiver) in FRD (Forward Right Down)
    uint32_t pos_wld[3]; // World Position of transmitter  (Lat Lon Msl)
    float vel_wld[3]; // World velocity of tranmitter in NED (North East Down)
    float abs_pos_ned[3]; // Absolute position of transmitter (vehicle frame) in NED (North East Down)
    float abs_vel_ned[3]; // Absolute velocity of tranmitter (vehicle frame)in NED (North East Down)
    float vehicle_abs_pos_ned[3]; // Absolute Position of vehicle in NED (North East Down)
    float vehicle_abs_vel_ned[3]; // Absolute velocity of vehicle in NED (North East Down)
    float vehicle_heading; // Vehicle heading information
}) __ULSObjectStruct_ULSQR1R1Status;

//ULSQR1-R1 configuration
typedef __ULS_PACKET( struct {
    char name[16]; // Name
    uint8_t ctrl; // Transmitter controll byte
    float sensitivity; // Receiver sensitivity
    float rxOffsetF; // Receiver offset (front)
    float rxOffsetR; // Receiver offset (right)
    float txOffsetF; // Transmitter offset (front)
    float txOffsetR; // Transmitter offset (right)
    float relPosGain; // Relative position Gain
    float relVelGain; // Relative velocity Gain
    float predictionTime; // Interval to predict positions if no signal
    float platformAcc; // Grount platform acceleration
}) __ULSObjectStruct_ULSQR1R1Config;

//Receiver Debug data
typedef __ULS_PACKET( struct {
    float beansA[37]; // Debug datareceiver A
    float beansB[37]; // Debug datareceiver B
}) __ULSObjectStruct_ULSQR1R1Debug;

class ULSObjectULSQT1R1Status : public ULSObjectBase {
 public:
 ULSObjectULSQT1R1Status(uint16_t id)
    : ULSObjectBase(id,"ULSQT1R1Status","ULSQT1-R1 status information",ULSBUS_OBJECT_PERMITION_READONLY){
   size = sizeof(__ULSObjectStruct_ULSQT1R1Status);
   len = 1;
   _pxData = (uint8_t *)&var;
  }
 __ULSObjectStruct_ULSQT1R1Status var;
};
// End of ULSObjectULSQT1R1Status

class ULSObjectULSQT1R1Config : public ULSObjectBase {
 public:
 ULSObjectULSQT1R1Config(uint16_t id)
    : ULSObjectBase(id,"ULSQT1R1Config","ULSQT1-R1 configuration",ULSBUS_OBJECT_PERMITION_READWRITE){
   size = sizeof(__ULSObjectStruct_ULSQT1R1Config);
   len = 1;
   _pxData = (uint8_t *)&var;
  }
  void defaultConfig() override {
   char name_def[9] = {'U','L','S','Q','T','1','-','R','1'};
   for(int i=0;i<9;i++)var.name[i] = name_def[i];
   var.power = 100;
   var.Voff = 6;
   var.Vlow = 6;
   float magCalOffset_def[3] = {0,0,0};
   for(int i=0;i<3;i++)var.magCalOffset[i] = magCalOffset_def[i];
   float magCalScale_def[3] = {1,1,1};
   for(int i=0;i<3;i++)var.magCalScale[i] = magCalScale_def[i];
   var.ctrl = 1;
  };
  void validateConfig() override {
   var.power = checkConfigF(var.power,50,150);
   var.Voff = checkConfigF(var.Voff,5,30);
   var.Vlow = checkConfigF(var.Vlow,5,30);
   for(int i=0;i<3;i++)var.magCalOffset[i] = checkConfigF(var.magCalOffset[i],0.0,3.0);
   for(int i=0;i<3;i++)var.magCalScale[i] = checkConfigF(var.magCalScale[i],0.1,3.0);
  };
 __ULSObjectStruct_ULSQT1R1Config var;
};
// End of ULSObjectULSQT1R1Config

class ULSObjectULSQR1R1Status : public ULSObjectBase {
 public:
 ULSObjectULSQR1R1Status(uint16_t id)
    : ULSObjectBase(id,"ULSQR1R1Status","ULSQT1-R1 status information",ULSBUS_OBJECT_PERMITION_READONLY){
   size = sizeof(__ULSObjectStruct_ULSQR1R1Status);
   len = 1;
   _pxData = (uint8_t *)&var;
  }
 __ULSObjectStruct_ULSQR1R1Status var;
};
// End of ULSObjectULSQR1R1Status

class ULSObjectULSQR1R1Config : public ULSObjectBase {
 public:
 ULSObjectULSQR1R1Config(uint16_t id)
    : ULSObjectBase(id,"ULSQR1R1Config","ULSQR1-R1 configuration",ULSBUS_OBJECT_PERMITION_READWRITE){
   size = sizeof(__ULSObjectStruct_ULSQR1R1Config);
   len = 1;
   _pxData = (uint8_t *)&var;
  }
  void defaultConfig() override {
   char name_def[9] = {'U','L','S','Q','R','1','-','R','1'};
   for(int i=0;i<9;i++)var.name[i] = name_def[i];
   var.ctrl = 1;
   var.sensitivity = 1.0;
   var.rxOffsetF = 0.0;
   var.rxOffsetR = 0.0;
   var.txOffsetF = 0.0;
   var.txOffsetR = 0.0;
   var.relPosGain = 1.0;
   var.relVelGain = 1.0;
   var.predictionTime = 1.0;
   var.platformAcc = 0.1;
  };
  void validateConfig() override {
   var.sensitivity = checkConfigF(var.sensitivity,0.8,1.2);
   var.rxOffsetF = checkConfigF(var.rxOffsetF,-1.2,1.2);
   var.rxOffsetR = checkConfigF(var.rxOffsetR,-1.2,1.2);
   var.txOffsetF = checkConfigF(var.txOffsetF,-1.2,1.2);
   var.txOffsetR = checkConfigF(var.txOffsetR,-1.2,1.2);
   var.relPosGain = checkConfigF(var.relPosGain,0.1,5.0);
   var.relVelGain = checkConfigF(var.relVelGain,0.1,5.0);
   var.predictionTime = checkConfigF(var.predictionTime,0.0,5);
   var.platformAcc = checkConfigF(var.platformAcc,0.01,5.0);
  };
 __ULSObjectStruct_ULSQR1R1Config var;
};
// End of ULSObjectULSQR1R1Config

class ULSObjectULSQR1R1Debug : public ULSObjectBase {
 public:
 ULSObjectULSQR1R1Debug(uint16_t id)
    : ULSObjectBase(id,"ULSQR1R1Debug","Receiver Debug data",ULSBUS_OBJECT_PERMITION_READONLY){
   size = sizeof(__ULSObjectStruct_ULSQR1R1Debug);
   len = 1;
   _pxData = (uint8_t *)&var;
  }
 __ULSObjectStruct_ULSQR1R1Debug var;
};
// End of ULSObjectULSQR1R1Debug

//ULSQT1R1: UAVLAS transmitter version 1 and revision 1
class ULSD_ULSQT1R1 : public ULSD_ULSX {
 public:
 ULSD_ULSQT1R1():
    ULSD_ULSX(__ULS_DEVICE_TYPE_ULSQT1R1_NAME,__ULS_DEVICE_TYPE_ULSQT1R1),
    o_status(0x0010),
    o_cfg(0x0020){
  pxCfg = o_cfg._pxData;
  lenCfg = o_cfg.size;
  };
  ULSObjectULSQT1R1Status o_status;
  ULSObjectULSQT1R1Config o_cfg;
};
// End of ULSD_ULSQT1R1

//ULSQR1R1: UAVLAS receiver version 1 and revision 1
class ULSD_ULSQR1R1 : public ULSD_ULSX {
 public:
 ULSD_ULSQR1R1():
    ULSD_ULSX(__ULS_DEVICE_TYPE_ULSQR1R1_NAME,__ULS_DEVICE_TYPE_ULSQR1R1),
    o_status(0x0010),
    o_cfg(0x0020),
    o_debug(0x0030){
  pxCfg = o_cfg._pxData;
  lenCfg = o_cfg.size;
  };
  ULSObjectULSQR1R1Status o_status;
  ULSObjectULSQR1R1Config o_cfg;
  ULSObjectULSQR1R1Debug o_debug;
};
// End of ULSD_ULSQR1R1

//ULSQG1R1: UAVLAS Reserved device
class ULSD_ULSQG1R1 : public ULSD_ULSX {
 public:
 ULSD_ULSQG1R1():
    ULSD_ULSX(__ULS_DEVICE_TYPE_ULSQG1R1_NAME,__ULS_DEVICE_TYPE_ULSQG1R1){
  };
};
// End of ULSD_ULSQG1R1

//ULSQM1R1: UAVLAS Reserved device
class ULSD_ULSQM1R1 : public ULSD_ULSX {
 public:
 ULSD_ULSQM1R1():
    ULSD_ULSX(__ULS_DEVICE_TYPE_ULSQM1R1_NAME,__ULS_DEVICE_TYPE_ULSQM1R1){
  };
};
// End of ULSD_ULSQM1R1

#endif  // ULSDEVICE_ULSQX_H 
