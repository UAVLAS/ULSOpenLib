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


class ULSObjectSignature: public ULSObjectBase
{
public:
    typedef struct __attribute__((packed)){
        char hw[32];
        char fw[32];
        char ldr[32];
        uint32_t serial[4];
        uint32_t progflashingtime;
        uint32_t progsize;
        uint32_t progcrc;
        uint32_t devclass;
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
        pxSign->hw[31] = 0;
        pxSign->fw[31] = 0;
        pxSign->ldr[31] = 0;

        out["hw"]   = QString().fromLatin1(pxSign->hw);
        out["fw"]   = QString().fromLatin1(pxSign->fw);
        out["ldr"]  =  QString().fromLatin1(pxSign->ldr);
        out["serial"] = QString("%1-%2-%3-%4").arg(pxSign->serial[0],8,16,QLatin1Char('0'))
                .arg(pxSign->serial[1],8,16,QLatin1Char('0'))
                .arg(pxSign->serial[2],8,16,QLatin1Char('0'))
                .arg(pxSign->serial[3],8,16,QLatin1Char('0'));
        out["progflashingtime"] = pxSign->progflashingtime;
        out["progsize"] = pxSign->progsize;
        out["progcrc"] = pxSign->progcrc;
        out["devclass"] = pxSign->devclass;
        return out;
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
        ULSD_ULSX(__ULS_DEVICE_TYPE_ULSQT1R1_NAME,__ULS_DEVICE_TYPE_ULSQT1R1)
    {
    }
};
class ULSD_ULSQR1R1:public ULSD_ULSX
{
public:
    ULSD_ULSQR1R1():
        ULSD_ULSX(__ULS_DEVICE_TYPE_ULSQR1R1_NAME,__ULS_DEVICE_TYPE_ULSQR1R1)
    {
    }
};

#ifdef PCQT_BUILD
class ULSQTDevice_X
{
public:
    ULSQTDevice_X()
    {
        devTypeName[__ULS_DEVICE_TYPE_PCR1] = __ULS_DEVICE_TYPE_PCR1_NAME;
        devTypeName[__ULS_DEVICE_TYPE_ULSQT1R1] =__ULS_DEVICE_TYPE_ULSQT1R1_NAME;
        devTypeName[__ULS_DEVICE_TYPE_ULSQR1R1] =__ULS_DEVICE_TYPE_ULSQR1R1_NAME;
        devTypeName[__ULS_DEVICE_TYPE_ULSQG1R1] =__ULS_DEVICE_TYPE_ULSQG1R1_NAME;
        devTypeName[__ULS_DEVICE_TYPE_ULSQM1R1] =__ULS_DEVICE_TYPE_ULSQM1R1_NAME;
        dev[__ULS_DEVICE_TYPE_ULSQT1R1_NAME] = (ULSDBase*)&devULSQT1R1;
        dev[__ULS_DEVICE_TYPE_ULSQR1R1_NAME] = (ULSDBase*)&devULSQR1R1;
    };

    ULSD_ULSQT1R1 devULSQT1R1;
    ULSD_ULSQR1R1 devULSQR1R1;

    QHash<int,QString> devTypeName;
    QHash<QString,ULSDBase*> dev;

    QVariantMap getVars(QString devName,QString objName,uint8_t *buf){
        if(dev[devName] != nullptr)
            return dev[devName]->getVar(objName,buf);
        return QVariantMap();
    }
    QVariantMap getVars(QString devName,QString *objName,uint16_t obj_id,uint8_t *buf){
        if(dev[devName] != nullptr)
            return dev[devName]->getVar(objName,obj_id,buf);
        return QVariantMap();
    }
};
#endif

#endif // ULSDEVICE_ULSQX_H
