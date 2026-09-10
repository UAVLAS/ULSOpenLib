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

ULSObjectBase::ULSObjectBase(uint16_t id, const char *name,
                             const char *description,
                             _ulsbus_obj_permissions permission)
    : ULSListItem(),
      id(id),
      _name(name),
      _description(description),
      _permission(permission) {
  size = 0;
  len = 0;
}

/*
 * devname is the *instance* name a device announces in its explorer answer,
 * and it was left out of this list entirely. A firmware device always assigns
 * it - from the config's name field, or "Loader" in a bootloader - so the
 * omission never showed there. The Qt side does not: ULSD_PC and ULSD_ULSX
 * only forward the type name and code, so the pointer held whatever the
 * surrounding storage happened to contain, and cnProcessExplorer() read 16
 * bytes through it as soon as a peer explored us. Default it to the type name
 * so it is always a valid string, and let a device override it as before.
 *
 * pxCfg/lenCfg are the same shape of trap: the generated device constructors
 * set them, but a device with no config object left them indeterminate and
 * the config save path copies lenCfg bytes through pxCfg.
 */
ULSDBase::ULSDBase(const char *tn, const uint16_t tc)
    : ULSList(),
      devname(tn),
      typeName(tn),
      typeCode(tc),
      pxCfg(nullptr),
      lenCfg(0) {}

ULSObjectBase *ULSDBase::getObject(uint16_t obj_id) {
  begin();
  while (next()) {
    if (current->id == obj_id) return current;
  }
  return nullptr;
}

void ULSDBase::setData(uint16_t obj_id, uint8_t *buf) {
  ULSObjectBase *obj = getObject(obj_id);
  if (obj == nullptr) return;  // an id this device does not carry
  obj->setData(buf);
}
