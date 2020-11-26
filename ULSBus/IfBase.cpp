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

#include "IfBase.h"

IfBase::IfBase(){
    rxBufInstance.buf = __null;
    rxBufInstance.lenght = 0;
    txBufInstance.buf = __null;
    txBufInstance.lenght = 0;
}
bool IfBase::send(uint8_t *buf, uint32_t lenght){
    _if_buffer_instance bi;
    bi.buf = buf;
    bi.lenght = lenght;
    return send(&bi);
}

bool IfBase::send(){
    return send(&txBufInstance);
}


