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

#ifndef IFBASE_H
#define IFBASE_H

#include<inttypes.h>

typedef struct
{
    uint8_t*  buf;
    uint32_t  lenght;
}_if_buffer_instance;

class IfBase
{
public:
    IfBase();
    virtual bool open(){return false;};
    virtual void task(){};
    virtual uint32_t read(){return 0;};
    virtual bool send(_if_buffer_instance *ifBufferInstace){(void)ifBufferInstace;return false;};
    virtual uint32_t maxTxPacketLenght(){return 0;}
    virtual void enableEscIfSupprted(bool esc){(void)esc;};
    virtual uint32_t maxFrameSize(){return 8;};

    bool send(uint8_t *buf, uint32_t lenght);
    bool send();
public:
    _if_buffer_instance rxBufInstance;
    _if_buffer_instance txBufInstance;
};

#endif // IFBASE_H
