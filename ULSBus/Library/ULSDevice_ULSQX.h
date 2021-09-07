/**
 *  Copyright: 2020 by UAVLAS  <www.uavlas.com>
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
#ifndef ULSDEVICE_ULSQX_H
#define ULSDEVICE_ULSQX_H

#include "ULSObject.h"
#ifdef PCQT_BUILD
// GET DEFINES
#define __ULS_GENERIC_VAR_TO_QVM(VNAME) \
  { out[#VNAME] = px->VNAME; };
#define __ULS_GENERIC_STRING_TO_QVM(SNAME) \
  { out[#SNAME] = QString().fromLatin1(px->SNAME); };
#define __ULS_GENERIC_V3F_TO_QVM(VNAME)                                  \
  {                                                                      \
    out[#VNAME] = QList<QVariant>()                                      \
                  << (px->VNAME[0]) << (px->VNAME[1]) << (px->VNAME[2]); \
  };

#define __ULS_GENERIC_VARRAY_TO_QVM(VNAME, SIZE)                    \
  {                                                                 \
    QList<QVariant> ql_##VNAME;                                     \
    for (int i = 0; i < SIZE; i++) ql_##VNAME.append(px->VNAME[i]); \
    out[#VNAME] = ql_##VNAME;                                       \
  };

#define __ULS_GENERIC_V3D_TO_QVM(VNAME) __ULS_GENERIC_VARRAY_TO_QVM(VNAME,3)

// SET DEFINES
#define __ULS_QVM_TO_STRING(SNAME)                                           \
  if (QString("%1").arg(vars[#SNAME].toString()).size() < 16) {              \
    memcpy(var.SNAME,                                                        \
           QString("%1").arg(vars[#SNAME].toString()).toStdString().c_str(), \
           QString("%1").arg(vars[#SNAME].toString()).size());               \
    var.SNAME[QString("%1").arg(vars[#SNAME].toString()).size()] = 0;        \
  }

#define __ULS_QVM_TO_FLOAT(VNAME) \
  { var.VNAME = vars[#VNAME].toFloat(); };
#define __ULS_QVM_TO_UINT(VNAME) \
  { var.VNAME = vars[#VNAME].toUInt(); };

#define __ULS_QVM_TO_VARRAYF(VNAME, SIZE)  \
  {\
    QVariantList ql_##VNAME = vars[#VNAME].toList();\
    for (int i = 0; i < SIZE; i++)\
    var.VNAME[i] = ql_##VNAME.at(i).toFloat();\
  };

#endif

typedef __ULS_PACKET( struct {
  char fw[32];
  char ldr[32];
  uint32_t serial[4];
  uint32_t progflashingtime;
  uint32_t progsize;
  uint32_t progcrc;
  uint32_t type;
})__ULSObjectSignature;  // Total 128 bytes;


class ULSObjectSignature : public ULSObjectBase {
 public:
  ULSObjectSignature(uint16_t id)
      : ULSObjectBase(id, "System_signature", "SystemSignature Information",
                      ULSBUS_OBJECT_PERMITION_READONLY) {
    size = sizeof(__ULSObjectSignature);
    len = 1;
    _pxData = (uint8_t *)&var;
    memset(_pxData,0,sizeof (__ULSObjectSignature));
  }
  __ULSObjectSignature var;
#ifdef PCQT_BUILD
  QVariantMap get(uint8_t *buf) override {
    QVariantMap out;
    __ULSObjectSignature *pxSign = (__ULSObjectSignature *)buf;
    pxSign->fw[31] = 0;
    pxSign->ldr[31] = 0;
    out["fw"] = QString().fromLatin1(pxSign->fw);
    out["ldr"] = QString().fromLatin1(pxSign->ldr);
    out["serial"] = QString("%1-%2-%3-%4")
                        .arg(pxSign->serial[0], 8, 16, QLatin1Char('0'))
                        .arg(pxSign->serial[1], 8, 16, QLatin1Char('0'))
                        .arg(pxSign->serial[2], 8, 16, QLatin1Char('0'))
                        .arg(pxSign->serial[3], 8, 16, QLatin1Char('0'));
    out["key"] = pxSign->serial[0] ^ pxSign->serial[1] ^ pxSign->serial[2] ^
                 pxSign->serial[3];
    out["progflashingtime"] = pxSign->progflashingtime;
    out["progsize"] = pxSign->progsize;
    out["progcrc"] = pxSign->progcrc;
    out["type"] = pxSign->type;
    return out;
  };
#endif
};

typedef __ULS_PACKET( struct {
  uint32_t status;
  uint32_t errorr;
  float Iled[37];
  float vs;
  float ih;
  float il;
  float tb;
  float timu;
  float imua[3];
  float imug[3];
  float imum[3];
  float imu[3];

})__ULSObjectULSQT1R1Status;  // Total 128 bytes;

class ULSObjectULSQT1R1Status : public ULSObjectBase {
 public:
  ULSObjectULSQT1R1Status(uint16_t id)
      : ULSObjectBase(id, "Status", "System status Information",
                      ULSBUS_OBJECT_PERMITION_READONLY) {
    size = sizeof(__ULSObjectULSQT1R1Status);
    len = 1;
    _pxData = (uint8_t *)&var;
    }
  __ULSObjectULSQT1R1Status var;

#ifdef PCQT_BUILD
  QVariantMap get(uint8_t *buf) override {
    QVariantMap out;
    __ULSObjectULSQT1R1Status *px = (__ULSObjectULSQT1R1Status *)buf;
    __ULS_GENERIC_VAR_TO_QVM(status);
    __ULS_GENERIC_VAR_TO_QVM(errorr);
    __ULS_GENERIC_VAR_TO_QVM(vs);
    __ULS_GENERIC_VAR_TO_QVM(ih);
    __ULS_GENERIC_VAR_TO_QVM(il);
    __ULS_GENERIC_VAR_TO_QVM(tb);
    __ULS_GENERIC_VAR_TO_QVM(timu);

    __ULS_GENERIC_VARRAY_TO_QVM(Iled, 37);

    __ULS_GENERIC_V3F_TO_QVM(imua);
    __ULS_GENERIC_V3F_TO_QVM(imug);
    __ULS_GENERIC_V3F_TO_QVM(imum);
    __ULS_GENERIC_V3F_TO_QVM(imu);

    return out;
  };
#endif
};

typedef enum{
  CTLR_EMMITER_EN = 1
}ULSQT1R1_CTRL;

typedef __ULS_PACKET( struct {
  char name[16];
  float power;
  float Voff;
  float Vlow;
  float magCalOffset[3];
  float magCalScale[3];
  uint8_t ctrl;
}) __ULSObjectULSQT1R1Config;  // Total 128 bytes;


class ULSObjectULSQT1R1Config : public ULSObjectBase {
 public:
  ULSObjectULSQT1R1Config(uint16_t id)
      : ULSObjectBase(id, "Config", "System configuration",
                      ULSBUS_OBJECT_PERMITION_READWRITE) {
    size = sizeof(__ULSObjectULSQT1R1Config);
    len = 1;
    _pxData = (uint8_t *)&var;
  }
  __ULSObjectULSQT1R1Config var;
  void defaultConfig() override {
    memcpy(var.name, "ULSQT1R1       ", 16);
    var.ctrl = 1;
    var.power = 110.f;
    var.Vlow = 8.f;
    var.Voff = 6.f;
    var.magCalOffset[0] = var.magCalOffset[1] = var.magCalOffset[2] =0;
    var.magCalScale[0] = var.magCalScale[1] = var.magCalScale[2] =1;
  };
  void validateConfig() override {
    var.power = checkConfigF(var.power, 50.f, 150.f);
    var.Vlow = checkConfigF(var.Vlow, 5.f, 25.f);
    var.Voff = checkConfigF(var.Voff, 5.f, var.Vlow);
  };
#ifdef PCQT_BUILD
  QVariantMap get(uint8_t *buf) override {
    QVariantMap out;
    __ULSObjectULSQT1R1Config *px = (__ULSObjectULSQT1R1Config *)buf;
    px->name[15] = 0;
    __ULS_GENERIC_STRING_TO_QVM(name);
    __ULS_GENERIC_VAR_TO_QVM(ctrl);
    __ULS_GENERIC_VAR_TO_QVM(power);
    __ULS_GENERIC_VAR_TO_QVM(Voff);
    __ULS_GENERIC_VAR_TO_QVM(Vlow);
    __ULS_GENERIC_V3F_TO_QVM(magCalOffset);
    __ULS_GENERIC_V3F_TO_QVM(magCalScale);
    return out;
  };
  uint32_t set(QVariantMap vars) override {
    QString nm = QString("%1").arg(vars["name"].toString());
    if (nm.size() < 16) memcpy(var.name, nm.toStdString().c_str(), nm.size());
    var.name[15] = 0;
    __ULS_QVM_TO_UINT(ctrl);
    __ULS_QVM_TO_FLOAT(power);
    __ULS_QVM_TO_FLOAT(Voff);
    __ULS_QVM_TO_FLOAT(Vlow);
    __ULS_QVM_TO_VARRAYF(magCalOffset,3);
    __ULS_QVM_TO_VARRAYF(magCalScale,3);
    return size;
  };
#endif
};
// ULS-QR1-R1
typedef __ULS_PACKET( struct {
  uint32_t status;
  uint32_t errorr;
  // Debug Variables
  uint32_t blitzTest;
  uint32_t packCntr;
  uint16_t bitMax;
  uint8_t  qtId;
  uint8_t  synqChannel;
  uint32_t posTime;
  uint32_t emsTime;
  uint32_t synqMax;
  // Debug Variables

  float level;
  float levelA;
  float levelB;
  float snrA;
  float snrB;
  float prob;
  float mrxDistance;// Distance provided by MRX algorithm
  float mrxYaw; // Yaw orintation provided by MRX algorithm
  float distance;
  float ang[3]; // Receiver angular position in transmitter frame [deg]
  float pos[3]; //Receiver position in transmitter frame [m]
  float vel[3]; //Receiver velocity in transmitter frame [m]
  float gimu[3]; // Transmitter orientation (roll, pitch, yaw)[deg,deg,deg]
  float  rel_pos_ned[3]; // Relative Position of transmitter (offset from receiver)in NED (North East Down)[m]
  float  rel_vel_ned[3]; // Ralative velocity of tranmitter (offset from receiver) in NED (North East Down) [m/s]
  float  rel_pos_frd[3]; // Relative Position of transmitter (offset from receiver) in FRD (Forward Right Down) [m]
  float  rel_vel_frd[3]; // Ralative velocity of tranmitter (offset from receiver) in FRD (Forward Right Down)[m/s]
  uint32_t pos_wld[3]; // World Position of transmitter  (Lat Lon Msl)[d,d,cm]
  float  vel_wld[3]; // World velocity of tranmitter in NED (North East Down) [m/s]
  float  abs_pos_ned[3]; // Absolute position of transmitter (vehicle frame) in NED (North East Down)[m]
  float  abs_vel_ned[3]; // Absolute velocity of tranmitter (vehicle frame)in NED (North East Down) [m/s]
  float  vehicle_abs_pos_ned[3]; // Absolute Position of vehicle in NED (North East Down)[m]
  float  vehicle_abs_vel_ned[3]; // Ralative velocity of vehicle in NED (North East Down) [m/s]
  float  vehicle_heading; // Vehicle heading information
  
})__ULSObjectULSQR1R1Status;  // Total 128 bytes;


class ULSObjectULSQR1R1Status : public ULSObjectBase {
 public:
  ULSObjectULSQR1R1Status(uint16_t id)
      : ULSObjectBase(id, "Status", "System status Information",
                      ULSBUS_OBJECT_PERMITION_READONLY) {
    size = sizeof(__ULSObjectULSQR1R1Status);
    len = 1;
    _pxData = (uint8_t *)&var;
  }
  __ULSObjectULSQR1R1Status var;
#ifdef PCQT_BUILD

  QVariantMap get(uint8_t *buf) override {
    QVariantMap out;
    __ULSObjectULSQR1R1Status *px = (__ULSObjectULSQR1R1Status *)buf;
    __ULS_GENERIC_VAR_TO_QVM(status);
    __ULS_GENERIC_VAR_TO_QVM(errorr);
    __ULS_GENERIC_VAR_TO_QVM(blitzTest);
    __ULS_GENERIC_VAR_TO_QVM(packCntr);
    __ULS_GENERIC_VAR_TO_QVM(bitMax);
    __ULS_GENERIC_VAR_TO_QVM(qtId);
    __ULS_GENERIC_VAR_TO_QVM(synqChannel);
    __ULS_GENERIC_VAR_TO_QVM(posTime);
    __ULS_GENERIC_VAR_TO_QVM(emsTime);
    __ULS_GENERIC_VAR_TO_QVM(synqMax);
    __ULS_GENERIC_VAR_TO_QVM(level);
    __ULS_GENERIC_VAR_TO_QVM(levelA);
    __ULS_GENERIC_VAR_TO_QVM(levelB);
    __ULS_GENERIC_VAR_TO_QVM(snrA);
    __ULS_GENERIC_VAR_TO_QVM(snrB);
    __ULS_GENERIC_VAR_TO_QVM(prob);
    __ULS_GENERIC_VAR_TO_QVM(mrxDistance);
    __ULS_GENERIC_VAR_TO_QVM(mrxYaw);
    __ULS_GENERIC_VAR_TO_QVM(distance);
    __ULS_GENERIC_V3D_TO_QVM(ang);
    __ULS_GENERIC_V3D_TO_QVM(pos);
    __ULS_GENERIC_V3D_TO_QVM(vel);
    __ULS_GENERIC_V3D_TO_QVM(gimu);
    __ULS_GENERIC_V3D_TO_QVM(rel_pos_ned); // Relative Position of transmitter in NED (North East Down)[m]
    __ULS_GENERIC_V3D_TO_QVM(rel_vel_ned); // Ralative velocity of tranmitter in NED (North East Down) [m/s] 
    __ULS_GENERIC_V3D_TO_QVM(rel_pos_frd); // Relative Position of transmitter in FRD (Forward Right Down) [m]
    __ULS_GENERIC_V3D_TO_QVM(rel_vel_frd); // Ralative velocity of tranmitter in FRD (Forward Right Down)[m/s]
    __ULS_GENERIC_V3D_TO_QVM(pos_wld); // World Position of transmitter in NED (Lat Lon Msl)[m]
    __ULS_GENERIC_V3D_TO_QVM(vel_wld); // World velocity of tranmitter in NED (Lat Lon Msl) [m/s]
    __ULS_GENERIC_V3D_TO_QVM(abs_pos_ned); // Absolute position of transmitter (vehicle frame) in NED (North East Down)[m]
    __ULS_GENERIC_V3D_TO_QVM(abs_vel_ned); // Absolute velocity of tranmitter (vehicle frame)in NED (North East Down) [m/s] 
    __ULS_GENERIC_V3D_TO_QVM(vehicle_abs_pos_ned); // Absolute Position of vehicle in NED (North East Down)[m]
    __ULS_GENERIC_V3D_TO_QVM(vehicle_abs_vel_ned); // Ralative velocity of vehicle in NED (North East Down) [m/s] 

    return out;
  };
#endif
};
typedef enum{
   CTLR_RX_EN = 1,
   CTLR_NOISE_DEBUG = 2,
 }ULSQR1R1_CTRL;

 typedef __ULS_PACKET( struct{
   char name[16];
   uint8_t ctrl;
   float sensitivity;
   float rxOffsetF;
   float rxOffsetR;
   float txOffsetF;
   float txOffsetR;
   float predictionTime;

 }) __ULSObjectULSQR1R1Config;  // Total 128 bytes;



class ULSObjectULSQR1R1Config : public ULSObjectBase {
 public:
  typedef enum{
   CTLR_RX_EN = 1,
   CTLR_NOISE_DEBUG = 2,
   CTLR_DISABLE_COMPASS = 4,
   CTLR_DISABLE_MRX = 8,
  }ULSQR1R1_CTRL;
  ULSObjectULSQR1R1Config(uint16_t id)
      : ULSObjectBase(id, "Config", "System configuration",
                      ULSBUS_OBJECT_PERMITION_READWRITE) {
    size = sizeof(__ULSObjectULSQR1R1Config);
    len = 1;
    _pxData = (uint8_t *)&var;
  }
   __ULSObjectULSQR1R1Config var;
  void defaultConfig() override {
    memcpy(var.name, "ULSQR1R1       ", 16);
    var.ctrl = 1;
    var.sensitivity = 1.f;
    var.rxOffsetF = 0.f;
    var.rxOffsetR = 0.f;
    var.txOffsetF = 0.f;
    var.txOffsetR = 0.f;
    var.predictionTime = 1.0f;
  };
  void validateConfig() override {
    var.sensitivity = checkConfigF(var.sensitivity, 0.8f, 1.5f);
    var.rxOffsetF = checkConfigF(var.rxOffsetF, -1.2f, 1.2f);
    var.rxOffsetR = checkConfigF(var.rxOffsetR, -1.2f, 1.2f);
    var.txOffsetF = checkConfigF(var.txOffsetF, -1.2f, 1.2f);
    var.txOffsetR = checkConfigF(var.txOffsetR, -1.2f, 1.2f);
    var.predictionTime = checkConfigF(var.predictionTime, 0.0f, 10.0f);
  };
#ifdef PCQT_BUILD
  QVariantMap get(uint8_t *buf) override {
    QVariantMap out;
    __ULSObjectULSQR1R1Config *px = (__ULSObjectULSQR1R1Config *)buf;
    px->name[15] = 0;
    __ULS_GENERIC_STRING_TO_QVM(name);
    __ULS_GENERIC_VAR_TO_QVM(ctrl);
    __ULS_GENERIC_VAR_TO_QVM(sensitivity);
    __ULS_GENERIC_VAR_TO_QVM(rxOffsetF);
    __ULS_GENERIC_VAR_TO_QVM(rxOffsetR);
    __ULS_GENERIC_VAR_TO_QVM(txOffsetF);
    __ULS_GENERIC_VAR_TO_QVM(txOffsetR);
    __ULS_GENERIC_VAR_TO_QVM(predictionTime);

    return out;
  };
  uint32_t set(QVariantMap vars) override {
    __ULS_QVM_TO_STRING(name);
    var.name[15] = 0;
    __ULS_QVM_TO_UINT(ctrl);
    __ULS_QVM_TO_FLOAT(sensitivity);
    __ULS_QVM_TO_FLOAT(rxOffsetF);
    __ULS_QVM_TO_FLOAT(rxOffsetR);
    __ULS_QVM_TO_FLOAT(txOffsetF);
    __ULS_QVM_TO_FLOAT(txOffsetR);
    __ULS_QVM_TO_FLOAT(predictionTime);
    return size;
  };
#endif
};

typedef __ULS_PACKET( struct {
   float beansA[37];
   float beansB[37];
 })__ULSObjectULSQR1R1Debug;  // Total 128 bytes;



class ULSObjectULSQR1R1Debug : public ULSObjectBase {

 public:
  ULSObjectULSQR1R1Debug(uint16_t id)
      : ULSObjectBase(id, "Debug", "System debug Information",
                      ULSBUS_OBJECT_PERMITION_READONLY) {
    size = sizeof(__ULSObjectULSQR1R1Debug);
    len = 1;
    _pxData = (uint8_t *)&var;
  }
   __ULSObjectULSQR1R1Debug var;
#ifdef PCQT_BUILD

  QVariantMap get(uint8_t *buf) override {
    QVariantMap out;
    __ULSObjectULSQR1R1Debug *px = (__ULSObjectULSQR1R1Debug *)buf;
    __ULS_GENERIC_VARRAY_TO_QVM(beansA, 37);
    __ULS_GENERIC_VARRAY_TO_QVM(beansB, 37);
    return out;
  };
#endif
};
class ULSD_ULSX : public ULSDBase {
 public:
  ULSD_ULSX(const char *tn, const uint16_t tc)
      : ULSDBase(tn, tc), o_sys_signature(0x0001) {
    add(&o_sys_signature);
  }

  ULSObjectSignature o_sys_signature;
  uint8_t *pxCfg;
  uint32_t lenCfg;
};

class ULSD_PC : public ULSDBase {
 public:
  ULSD_PC() : ULSDBase(__ULS_DEVICE_TYPE_PCR1_NAME, __ULS_DEVICE_TYPE_PCR1) {}
};
class ULSD_ULSQT1R1 : public ULSD_ULSX {
 public:
  ULSD_ULSQT1R1()
      : ULSD_ULSX(__ULS_DEVICE_TYPE_ULSQT1R1_NAME, __ULS_DEVICE_TYPE_ULSQT1R1),
        o_status(0x0010),
        o_cfg(0x0020) {
    add(&o_status);
    add(&o_cfg);
    pxCfg = o_cfg._pxData;
    lenCfg = o_cfg.size;
  }
  ULSObjectULSQT1R1Status o_status;
  ULSObjectULSQT1R1Config o_cfg;
};
class ULSD_ULSQR1R1 : public ULSD_ULSX {
 public:
  ULSD_ULSQR1R1()
      : ULSD_ULSX(__ULS_DEVICE_TYPE_ULSQR1R1_NAME, __ULS_DEVICE_TYPE_ULSQR1R1),
        o_status(0x0010),
        o_cfg(0x0020),
        o_debug(0x030) {
    add(&o_status);
    add(&o_cfg);
    add(&o_debug);
    
    pxCfg = o_cfg._pxData;
    lenCfg = o_cfg.size;
  }
  ULSObjectULSQR1R1Status o_status;
  ULSObjectULSQR1R1Config o_cfg;
  ULSObjectULSQR1R1Debug o_debug;
};

#ifdef PCQT_BUILD

class ULSQTDevicesLibrary {
 public:
  ULSQTDevicesLibrary() {
    devTypes[__ULS_DEVICE_TYPE_ULSQT1R1] = (const char*)__ULS_DEVICE_TYPE_ULSQT1R1_NAME;
    devTypes[__ULS_DEVICE_TYPE_ULSQR1R1] = (const char*)__ULS_DEVICE_TYPE_ULSQR1R1_NAME;
  };
//  ULSD_ULSQT1R1 devULSQT1R1;
//  ULSD_ULSQR1R1 devULSQR1R1;
  QHash<uint,const char*> devTypes;
  ULSD_ULSX* createInstance(uint typecode){

      switch(typecode){
        case __ULS_DEVICE_TYPE_ULSQT1R1: return new ULSD_ULSQT1R1();
        case __ULS_DEVICE_TYPE_ULSQR1R1: return new ULSD_ULSQR1R1();
      }
      return nullptr;

  }
};
#endif

#endif  // ULSDEVICE_ULSQX_H
