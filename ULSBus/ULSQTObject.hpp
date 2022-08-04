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
#include <QObject>
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

#define __ULS_QVM_TO_CHAR_ARRAY(SN,CN,LEN)\
    if (SN.size() < (LEN - 1)) {              \
    memcpy(CN,SN.toStdString().c_str(),SN.size()); \
    CN[SN.size()] = 0;        \
    }
#define __ULS_QVM_TO_VARRAYF(V,VNAME, SIZE)  \
{\
    QVariantList ql = V.toList();\
    for (int i = 0; i < SIZE; i++)\
    VNAME[i] = ql.at(i).toFloat();\
    };
#define __ULS_QVM_TO_VARRAYUI(V,VNAME, SIZE)  \
{\
    QVariantList ql = V.toList();\
    for (int i = 0; i < SIZE; i++)\
    VNAME[i] = ql.at(i).toUInt();\
    };
#define __ULS_QVM_TO_VARRAYI(V,VNAME, SIZE)  \
{\
    QVariantList ql = V.toList();\
    for (int i = 0; i < SIZE; i++)\
    VNAME[i] = ql.at(i).toInt();\
    };
//----------------------------------------------------------------------
// ULSOBJECTS COMMON DATA

class ULSObjectBase : public QObject,public ULSListItem {

public:
    ULSObjectBase(uint16_t id, const char *name, const char *description,
                  _ulsbus_obj_permitions permition):
        ULSListItem(),
        id(id),
        _name(name),
        _description(description),
        _permition(permition)
    {
        size = 0;
        len = 0;
    }

    uint16_t id;
    uint16_t size;
    uint16_t len;
    uint8_t *_pxData;
    const char *_name;
    const char *_description;
    _ulsbus_obj_permitions _permition;

    virtual void defaultConfig(){};
    virtual void validateConfig(){};
    virtual void dataUpdated(){};

    uint32_t getData(uint8_t *buf) {
        memcpy(buf, _pxData, size * len);
        return size * len;
    };
    void setData(uint8_t *buf) { memcpy(_pxData, buf, size * len); dataUpdated();};

    virtual QVariantMap getVars() {
        QVariantMap v;
        (void)v;
        return v;
    };
    virtual void setVars(const QVariantMap& vars) {
        (void)vars;
    };
    QString name() { return QString().fromLatin1(_name); };
    QString description() { return QString().fromLatin1(_description); };


protected:
    float checkConfigF(float val, float min, float max) {
        if (val > max) return max;
        if (val < min) return min;
        return val;
    };
};

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

    Q_OBJECT

public:
    Q_PROPERTY(QString var_fw READ var_fw NOTIFY var_fwChanged)
    Q_PROPERTY(QString var_ldr READ var_ldr NOTIFY var_ldrChanged)
    Q_PROPERTY(QString var_serial READ var_serial NOTIFY var_serialChanged)
    Q_PROPERTY(QString var_key READ var_key NOTIFY var_keyChanged)
    Q_PROPERTY(uint32_t var_progflashingtime READ var_progflashingtime NOTIFY var_progflashingtimeChanged)

    Q_PROPERTY(uint32_t var_progsize READ var_progsize NOTIFY var_progsizeChanged)
    Q_PROPERTY(uint32_t var_progcrc READ var_progcrc NOTIFY var_progcrcChanged)
    Q_PROPERTY(uint32_t var_type READ var_type NOTIFY var_typeChanged)

    QString var_fw(){
        var.fw[31] = 0;
        return QString().fromLatin1(var.fw);
    };
    QString var_ldr(){
        var.ldr[31] = 0;
        return QString().fromLatin1(var.ldr);
    };
    QString var_serial(){
        return QString("%1-%2-%3-%4")
                .arg(var.serial[0], 8, 16, QLatin1Char('0'))
                .arg(var.serial[1], 8, 16, QLatin1Char('0'))
                .arg(var.serial[2], 8, 16, QLatin1Char('0'))
                .arg(var.serial[3], 8, 16, QLatin1Char('0'));
    };
    uint32_t var_key(){
        return var.serial[0] ^ var.serial[1] ^ var.serial[2] ^
                var.serial[3];
    }
    uint32_t var_progflashingtime(){
        return var.progflashingtime;
    }
    uint32_t var_progsize(){
        return var.progsize;
    }
    uint32_t var_progcrc(){
        return var.progcrc;
    }
    uint32_t var_type(){
        return var.type;
    }
signals:
    void var_fwChanged();
    void var_ldrChanged();
    void var_serialChanged();
    void var_keyChanged();
    void var_progflashingtimeChanged();
    void var_progsizeChanged();
    void var_progcrcChanged();
    void var_typeChanged();
    void updated();
public:
     void dataUpdated() override{
         emit var_fwChanged();
         emit var_ldrChanged();
         emit var_serialChanged();
         emit var_keyChanged();
         emit var_progflashingtimeChanged();
         emit var_progsizeChanged();
         emit var_progcrcChanged();
         emit var_typeChanged();
         emit updated();
     };

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

    QVariantMap getVars() override {
        QVariantMap out;

        out["fw"] = var_fw();
        out["ldr"] = var_ldr();
        out["serial"] = var_serial();
        out["key"] = var_key();
        out["progflashingtime"] = var_progflashingtime();
        out["progsize"] = var_progsize();
        out["progcrc"] = var_progcrc();
        out["type"] = var_type();
        return out;
    };


    void serial(QList<QVariant> &ql){
        for (int i = 0; i < 4; i++) var.serial[i] = ql[i].toInt();
    };

};

//----------------------------------------------------------------------
// ULSDEVICES COMMON DATA


class ULSDBase : public QObject,public ULSList<ULSObjectBase> {

    Q_OBJECT

public:
    ULSDBase(const char *tn, const uint16_t tc):
        //QObject(this),
        ULSList(),
        typeCode(tc),
        typeName(tn){
    }
    ULSObjectBase *getObject(uint16_t obj_id){
        begin();
        while(next()){
            if(current->id == obj_id)return current;
        }
        return nullptr;
    }

    void setData(uint16_t obj_id,uint8_t *buf){

        ULSObjectBase *obj = getObject(obj_id);
        obj->setData(buf);
    }
public:
    uint16_t typeCode;
    uint8_t *pxCfg;
    uint32_t lenCfg;
    const char *devname;
    const char *typeName;




    // SERVICE DATA
public:
    Q_PROPERTY(QString route READ route NOTIFY routeChanged)
    Q_PROPERTY(QString name READ name NOTIFY nameChanged)
    Q_PROPERTY(QString type READ type NOTIFY typeChanged)
    Q_PROPERTY(bool isLoader READ isLoader NOTIFY isLoaderChanged)
    Q_PROPERTY(quint32 typeNumber READ typeNumber NOTIFY typeNumberChanged)

    const QString& route(){return m_route;};
    const QString& name(){return m_name;};
    const QString& type(){return m_type;};
    quint16 typeNumber(){return typeCode;};
    bool isLoader(){return m_isLoader;};


    void route(const QString& n){m_route = n;};
    void name(const QString& n){m_name = n;};
    void type(const QString& t){m_type = t;};
    void typeNumber(quint16 t){typeCode = t;};
    void isLoader(bool l){m_isLoader = l;};

signals:
    void routeChanged();
    void nameChanged();
    void typeChanged();
    void typeNumberChanged();
    void isLoaderChanged();

    void disconnected();
    void connected();

public:
    void disconnect(){
        emit disconnected();
    }
    void connect(){
        emit connected();
    }

    uint32_t timeout;
private:
    QString m_route;
    bool m_isLoader;
    QString m_name;
    QString m_type;
public:

    QVariantMap getObjectVars(uint16_t obj_id){
        begin();
        while(next()){
            if(current->id == obj_id){
                return current->getVars();
            }
        }
        return QVariantMap();
    }

    QVariantMap getObjectVars(QString objName){
        begin();
        while(next()){
            if(current->name() == objName)return current->getVars();
        }
        return QVariantMap();
    }

    uint16_t getObjId(QString objName){
        begin();
        while(next()){
            if(current->name() == objName)return current->id;
        }
        return 0xFFFF;
    }
    ULSObjectBase *getObject(QString objName) {
        begin();
        while(next()){
            if(current->name()  == objName)return current;
        }
        return nullptr;
    }

    static bool compare(const ULSDBase * const & a, const ULSDBase * const & b)
    {
        return (a->m_route.compare(b->m_route) < 0);
    }
};


class ULSD_ULSX : public ULSDBase {

    Q_OBJECT
public:
    Q_PROPERTY(ULSObjectSignature* o_signature READ o_signature NOTIFY o_signatureChanged)

    ULSObjectSignature* o_signature(){return &o_sys_signature;};
signals:
    void o_signatureChanged();
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
