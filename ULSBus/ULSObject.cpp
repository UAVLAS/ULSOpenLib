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

#include "ULSObject.h"



ULSObjectBase::ULSObjectBase(uint16_t id, const char *name, const char *description, _ulsbus_obj_permitions permition):
    ULSListItem(),
    id(id),
    _name(name),
    _description(description),
    _permition(permition)
{
    size = 0;
    len = 0;
}

ULSDBase::ULSDBase(const char *tn, const uint16_t tc):
    ULSList(),
    typeName(tn),
    typeCode(tc)
{

}

ULSObjectBase *ULSDBase::getObject(uint16_t obj_id)
{
    begin();
    while(next()){
        if(current->id == obj_id)return current;
    }
    return nullptr;
}

#ifdef PCQT_BUILD
QVariantMap ULSDBase::getVar(QString *objName, uint16_t obj_id, uint8_t *buf)
{
    begin();
    while(next()){
        if(current->id == obj_id){
            *objName = current->name();
            return current->get(buf);
        }
    }
    return QVariantMap();
}

QVariantMap ULSDBase::getVar(QString objName, uint8_t *buf)
{
    begin();
    while(next()){
        if(current->name() == objName)return current->get(buf);
    }
    return QVariantMap();
}

uint16_t ULSDBase::getObjId(QString objName)
{
    begin();
    while(next()){
        if(current->name() == objName)return current->id;
    }
    return 0xFFFF;
}
ULSObjectBase *ULSDBase::getObject(QString objName)
{
    begin();
    while(next()){
        if(current->name()  == objName)return current;
    }
    return nullptr;
}

#endif
