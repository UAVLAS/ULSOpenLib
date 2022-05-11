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

#ifndef ULSOBJECT_H
#define ULSOBJECT_H

#include <inttypes.h>
#include <string.h>

#include "ULSBusTypes.h"

#ifdef PCQT_BUILD
#include <QHash>
#include <QList>
#include <QString>
#include <QVariantMap>
#endif

#define __ULS_DEVICE_TYPE_PCR1 (0x0001)

#define __ULS_DEVICE_TYPE_PCR1_NAME "PCR1"

#ifdef __GNUC__
#define __ULS_PACKET( __Declaration__ ) __Declaration__ __attribute__((packed))
#endif

#ifdef _MSC_VER
#define __ULS_PACKET( __Declaration__ ) __pragma( pack(push, 1) ) __Declaration__ __pragma( pack(pop))
#endif


class ULSObjectBase : public ULSListItem {
 public:
  ULSObjectBase(uint16_t id, const char *name, const char *description,
                _ulsbus_obj_permitions permition);
  uint16_t id;
  uint16_t size;
  uint16_t len;
  uint8_t *_pxData;
  const char *_name;
  const char *_description;
  _ulsbus_obj_permitions _permition;

// #ifdef PCQT_BUILD
//   virtual QVariantMap get(uint8_t *buf) {
//     QVariantMap v;
//     (void)v;
//     (void)buf;
//     return v;
//   };
//   virtual uint32_t set(QVariantMap vars) {
//     (void)vars;
//     return 0;
//   };
//   QString name() { return QString().fromLatin1(_name); };
//   QString description() { return QString().fromLatin1(_description); };
// #endif
  uint32_t getData(uint8_t *buf) {
    memcpy(buf, _pxData, size * len);
    return size * len;
  };
  void setData(uint8_t *buf) { memcpy(_pxData, buf, size * len); };
  virtual void defaultConfig(){};
  virtual void validateConfig(){};

 protected:
  float checkConfigF(float val, float min, float max) {
    if (val > max) return max;
    if (val < min) return min;
    return val;
  };
};

class ULSDBase : public ULSList<ULSObjectBase> {
 public:
  ULSDBase(const char *tn, const uint16_t tc);
  ULSObjectBase *getObject(uint16_t obj_id);
// #ifdef PCQT_BUILD
//   QVariantMap getVar(QString *objName, uint16_t obj_id, uint8_t *buf);
//   QVariantMap getVar(QString objName, uint8_t *buf);
//   QVariantMap getObjectVars(QString objName);

//   QString name() { return QString().fromLatin1(typeName); }
//   uint16_t getObjId(QString objName);
//   ULSObjectBase *getObject(QString objName);
//   ;
// #endif
  const char *devname;
  const char *typeName;
  uint16_t typeCode;
  uint8_t *pxCfg;
  uint32_t lenCfg;
};

// Standart Objects and devices
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
// #ifdef PCQT_BUILD
//   QVariantMap get(uint8_t *buf) override {
//     QVariantMap out;
//     __ULSObjectSignature *pxSign = (__ULSObjectSignature *)buf;
//     pxSign->fw[31] = 0;
//     pxSign->ldr[31] = 0;
//     out["fw"] = QString().fromLatin1(pxSign->fw);
//     out["ldr"] = QString().fromLatin1(pxSign->ldr);
//     out["serial"] = QString("%1-%2-%3-%4")
//                         .arg(pxSign->serial[0], 8, 16, QLatin1Char('0'))
//                         .arg(pxSign->serial[1], 8, 16, QLatin1Char('0'))
//                         .arg(pxSign->serial[2], 8, 16, QLatin1Char('0'))
//                         .arg(pxSign->serial[3], 8, 16, QLatin1Char('0'));
//     out["key"] = pxSign->serial[0] ^ pxSign->serial[1] ^ pxSign->serial[2] ^
//                  pxSign->serial[3];
//     out["progflashingtime"] = pxSign->progflashingtime;
//     out["progsize"] = pxSign->progsize;
//     out["progcrc"] = pxSign->progcrc;
//     out["type"] = pxSign->type;
//     return out;
//   };
// #endif
};
class ULSD_ULSX : public ULSDBase {
 public:
  ULSD_ULSX(const char *tn, const uint16_t tc)
      : ULSDBase(tn, tc), o_sys_signature(0x0001) {
    add(&o_sys_signature);
  }
  ULSObjectSignature o_sys_signature;
};

class ULSD_PC : public ULSDBase {
 public:
  ULSD_PC() : ULSDBase(__ULS_DEVICE_TYPE_PCR1_NAME, __ULS_DEVICE_TYPE_PCR1) {}
};

#endif  // ULSOBJECT_H
