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
#define __ULS_GENERIC_VAR_TO_QVM(VNAME) {out[#VNAME] = px->VNAME;};
#define __ULS_GENERIC_STRING_TO_QVM(SNAME) {out[#SNAME] = QString().fromLatin1(px->SNAME);};
#define __ULS_GENERIC_V3F_TO_QVM(VNAME) {out[#VNAME] = QList<QVariant>()<<(px->VNAME[0])<<(px->VNAME[1])<<(px->VNAME[2]);};


#define __ULS_GENERIC_VARRAY_TO_QVM(VNAME,SIZE) {QList<QVariant> ql_##VNAME;for(int i=0;i<SIZE;i++)ql_##VNAME.append(px->VNAME[i]);\
    out[#VNAME] = ql_##VNAME;};

// SET DEFINES
#define __ULS_QVM_TO_STRING(SNAME) if(QString("%1").arg(vars[#SNAME].toString()).size() < 16){\
    memcpy( var.SNAME, QString("%1").arg(vars[#SNAME].toString()).toStdString().c_str()\
    ,QString("%1").arg(vars[#SNAME].toString()).size());var.SNAME[QString("%1").arg(vars[#SNAME].toString()).size()] = 0; }

#define __ULS_QVM_TO_FLOAT(VNAME) {var.VNAME = vars[#VNAME].toFloat();};
#endif

class ULSObjectSignature: public ULSObjectBase
{
public:
    typedef struct __attribute__((packed)){
        char fw[32];
        char ldr[32];
        uint32_t serial[4];
        uint32_t progflashingtime;
        uint32_t progsize;
        uint32_t progcrc;
        uint32_t type;
    }__ULSObjectSignature;  // Total 128 bytes;
    __ULSObjectSignature var;

public:
    ULSObjectSignature(uint16_t id):
        ULSObjectBase(id,"System_signature","SystemSignature Information",ULSBUS_OBJECT_PERMITION_READONLY)
    {
        size = sizeof (__ULSObjectSignature);
        len = 1;
        _pxData = (uint8_t*)&var;
    }
#ifdef PCQT_BUILD
    QVariantMap get(uint8_t *buf)override
    {
        QVariantMap out;
        __ULSObjectSignature *pxSign = (__ULSObjectSignature*)buf;
        pxSign->fw[31] = 0;
        pxSign->ldr[31] = 0;
        out["fw"]   = QString().fromLatin1(pxSign->fw);
        out["ldr"]  =  QString().fromLatin1(pxSign->ldr);
        out["serial"] = QString("%1-%2-%3-%4").arg(pxSign->serial[0],8,16,QLatin1Char('0'))
                .arg(pxSign->serial[1],8,16,QLatin1Char('0'))
                .arg(pxSign->serial[2],8,16,QLatin1Char('0'))
                .arg(pxSign->serial[3],8,16,QLatin1Char('0'));
        out["key"] = pxSign->serial[0]^pxSign->serial[1]^pxSign->serial[2]^pxSign->serial[3];
        out["progflashingtime"] = pxSign->progflashingtime;
        out["progsize"] = pxSign->progsize;
        out["progcrc"] = pxSign->progcrc;
        out["type"] = pxSign->type;
        return out;
    };
#endif
};

class ULSObjectULSQT1R1Status: public ULSObjectBase
{
public:
    typedef struct __attribute__((packed)){
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

    }__ULSObjectULSQT1R1Status;  // Total 128 bytes;
    __ULSObjectULSQT1R1Status var;

public:
    ULSObjectULSQT1R1Status(uint16_t id):
        ULSObjectBase(id,"Status","System status Information",ULSBUS_OBJECT_PERMITION_READONLY)
    {
        size = sizeof (__ULSObjectULSQT1R1Status);
        len = 1;
        _pxData = (uint8_t*)&var;
    }
#ifdef PCQT_BUILD
    QVariantMap get(uint8_t *buf)override
    {
        QVariantMap out;
        __ULSObjectULSQT1R1Status *px = (__ULSObjectULSQT1R1Status*)buf;
        __ULS_GENERIC_VAR_TO_QVM(status);
        __ULS_GENERIC_VAR_TO_QVM(errorr);
        __ULS_GENERIC_VAR_TO_QVM(vs);
        __ULS_GENERIC_VAR_TO_QVM(ih);
        __ULS_GENERIC_VAR_TO_QVM(il);
        __ULS_GENERIC_VAR_TO_QVM(tb);
        __ULS_GENERIC_VAR_TO_QVM(timu);

        __ULS_GENERIC_VARRAY_TO_QVM(Iled,37);

        __ULS_GENERIC_V3F_TO_QVM(imua);
        __ULS_GENERIC_V3F_TO_QVM(imug);
        __ULS_GENERIC_V3F_TO_QVM(imum);
        __ULS_GENERIC_V3F_TO_QVM(imu);

        return out;
    };
#endif
};
class ULSObjectULSQT1R1Config: public ULSObjectBase
{
public:
    typedef struct __attribute__((packed)){
        char name[16];
        float Voff;
        float Vlow;
    }__ULSObjectULSQT1R1Config;  // Total 128 bytes;
    __ULSObjectULSQT1R1Config var;

public:
    ULSObjectULSQT1R1Config(uint16_t id):
        ULSObjectBase(id,"Config","System configuration",ULSBUS_OBJECT_PERMITION_READWRITE)
    {
        size = sizeof (__ULSObjectULSQT1R1Config);
        len = 1;
        _pxData = (uint8_t*)&var;
    }
#ifdef PCQT_BUILD
    QVariantMap get(uint8_t *buf)override
    {
        QVariantMap out;
        __ULSObjectULSQT1R1Config *px = (__ULSObjectULSQT1R1Config*)buf;
        px->name[15] = 0;
        __ULS_GENERIC_STRING_TO_QVM(name);
        __ULS_GENERIC_VAR_TO_QVM(Voff);
        __ULS_GENERIC_VAR_TO_QVM(Vlow);
        return out;
    };
    uint32_t set(QVariantMap vars)override
    {

        QString nm = QString("%1").arg(vars["name"].toString());
        if(nm.size()<16)memcpy( var.name, nm.toStdString().c_str() ,nm.size());
        var.name[15] = 0;
        __ULS_QVM_TO_FLOAT(Voff);
        __ULS_QVM_TO_FLOAT(Vlow);

        return size;
    };
#endif
};
// ULS-QR1-R1

class ULSObjectULSQR1R1Status: public ULSObjectBase
{
public:
    typedef struct __attribute__((packed)){
        uint32_t status;
        uint32_t errorr;

        uint32_t blitzTest;
        uint32_t packCntr;
        uint16_t bitMax;
        uint8_t qtId;
        uint8_t synqChannel;
        uint32_t posTime;
        uint32_t emsTime;
        uint32_t synqMax;

        float level;
        float levelA;
        float levelB;
        float prob;
        float trianDistance;
        float posDistance;
        float posHeight;
        float posAx;
        float posAy;
        float posDx;
        float posDy;
        float velDx;
        float velDy;
        float imuR;
        float imuP;
        float Dx;
        float Dy;
        float Dz;
        float R;



    }__ULSObjectULSQR1R1Status;  // Total 128 bytes;
    __ULSObjectULSQR1R1Status var;

public:
    ULSObjectULSQR1R1Status(uint16_t id):
        ULSObjectBase(id,"Status","System status Information",ULSBUS_OBJECT_PERMITION_READONLY)
    {
        size = sizeof (__ULSObjectULSQR1R1Status);
        len = 1;
        _pxData = (uint8_t*)&var;
    }
#ifdef PCQT_BUILD

    QVariantMap get(uint8_t *buf)override
    {
        QVariantMap out;
        __ULSObjectULSQR1R1Status *px = (__ULSObjectULSQR1R1Status*)buf;
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
        __ULS_GENERIC_VAR_TO_QVM(prob);
        __ULS_GENERIC_VAR_TO_QVM(trianDistance);
        __ULS_GENERIC_VAR_TO_QVM(posDistance);
        __ULS_GENERIC_VAR_TO_QVM(posHeight);
        __ULS_GENERIC_VAR_TO_QVM(posAx);
        __ULS_GENERIC_VAR_TO_QVM(posAy);
        __ULS_GENERIC_VAR_TO_QVM(posDx);
        __ULS_GENERIC_VAR_TO_QVM(posDy);
        __ULS_GENERIC_VAR_TO_QVM(velDx);
        __ULS_GENERIC_VAR_TO_QVM(velDy);
        __ULS_GENERIC_VAR_TO_QVM(imuR);
        __ULS_GENERIC_VAR_TO_QVM(imuP);
        __ULS_GENERIC_VAR_TO_QVM(Dx);
        __ULS_GENERIC_VAR_TO_QVM(Dy);
        __ULS_GENERIC_VAR_TO_QVM(Dz);
        __ULS_GENERIC_VAR_TO_QVM(R);

        return out;
    };
#endif
};
class ULSObjectULSQR1R1Config: public ULSObjectBase
{
public:
    typedef struct __attribute__((packed)){
        char name[16];
        float posOffsetX;
        float posOffsetY;
    }__ULSObjectULSQR1R1Config;  // Total 128 bytes;
    __ULSObjectULSQR1R1Config var;

public:
    ULSObjectULSQR1R1Config(uint16_t id):
        ULSObjectBase(id,"Config","System configuration",ULSBUS_OBJECT_PERMITION_READWRITE)
    {
        size = sizeof (__ULSObjectULSQR1R1Config);
        len = 1;
        _pxData = (uint8_t*)&var;
    }
#ifdef PCQT_BUILD
    QVariantMap get(uint8_t *buf)override
    {
        QVariantMap out;
        __ULSObjectULSQR1R1Config *px = (__ULSObjectULSQR1R1Config*)buf;
        px->name[15] = 0;
        __ULS_GENERIC_STRING_TO_QVM(name);
        __ULS_GENERIC_VAR_TO_QVM(posOffsetX);
        __ULS_GENERIC_VAR_TO_QVM(posOffsetY);

        return out;
    };
    uint32_t set(QVariantMap vars)override
    {
        __ULS_QVM_TO_STRING(name);
        var.name[15] = 0;
        __ULS_QVM_TO_FLOAT(posOffsetX);
        __ULS_QVM_TO_FLOAT(posOffsetY);
        return size;
    };
#endif
};


class ULSD_ULSX:public ULSDBase
{
public:
    ULSD_ULSX(const char* tn,const uint16_t tc):
        ULSDBase(tn,tc),
        o_sys_signature(0x0001)
    {
        add(&o_sys_signature);
    }

    ULSObjectSignature o_sys_signature;
    uint8_t *pxCfg;
    uint32_t lenCfg;
};


class ULSD_PC:public ULSDBase
{
public:
    ULSD_PC():
        ULSDBase(__ULS_DEVICE_TYPE_PCR1_NAME,__ULS_DEVICE_TYPE_PCR1)
    {

    }
};
class ULSD_ULSQT1R1:public ULSD_ULSX
{
public:
    ULSD_ULSQT1R1():
        ULSD_ULSX(__ULS_DEVICE_TYPE_ULSQT1R1_NAME,__ULS_DEVICE_TYPE_ULSQT1R1),
        o_status(0x0010),
        o_cfg(0x0020)
    {
        add(&o_status);
        add(&o_cfg);
        pxCfg = o_cfg._pxData;
        lenCfg = o_cfg.size;
    }
    ULSObjectULSQT1R1Status o_status;
    ULSObjectULSQT1R1Config o_cfg;
};
class ULSD_ULSQR1R1:public ULSD_ULSX
{
public:
    ULSD_ULSQR1R1():
        ULSD_ULSX(__ULS_DEVICE_TYPE_ULSQR1R1_NAME,__ULS_DEVICE_TYPE_ULSQR1R1),
        o_status(0x0010),
        o_cfg(0x0020)
    {
        add(&o_status);
        add(&o_cfg);
        pxCfg = o_cfg._pxData;
        lenCfg = o_cfg.size;
    }
    ULSObjectULSQR1R1Status o_status;
    ULSObjectULSQR1R1Config o_cfg;
};

#ifdef PCQT_BUILD


class ULSQTDevicesLibrary
{
public:
    ULSQTDevicesLibrary(){
        devTypes[__ULS_DEVICE_TYPE_ULSQT1R1] = (ULSDBase*)&devULSQT1R1;
        devTypes[__ULS_DEVICE_TYPE_ULSQR1R1] = (ULSDBase*)&devULSQR1R1;
    };
    ULSD_ULSQT1R1 devULSQT1R1;
    ULSD_ULSQR1R1 devULSQR1R1;
    QHash<uint,ULSDBase*> devTypes;
};
#endif

#endif // ULSDEVICE_ULSQX_H
