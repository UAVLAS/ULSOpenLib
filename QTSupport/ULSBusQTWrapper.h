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

#ifndef ULSBUSQTWRAPPER_H
#define ULSBUSQTWRAPPER_H

#include "ULSBus.h"
#include "ULSDevice_ULSQX.h"
#include <QTextStream>
#include <QElapsedTimer>

class ULSBusQTWrapper:public ULSBus
{
public:
    ULSBusQTWrapper(const char *name,ULSDeviceBase *selfDevice,_ulsbus_obj_updated_callback callback);
    void addDevice(_ulsbus_device_status *status) override;

private:
    ULSBusTransaction _transactions[512];
    ULSBusObjectBuffer _objectsBuffers[512];
    _ulsbus_obj_updated_callback _callback;
};


#endif // ULSBUSQTWRAPPER_H