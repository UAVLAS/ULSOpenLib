/*
 * io_fifo.h
 *
 *  Created on: Dec 7, 2015
 *      Author: baron
 */

#ifndef __IO_FIFO_H_
#define __IO_FIFO_H_

#include "inttypes.h"
#ifndef STM32F7
#define __fifo_enter_critical() void();
#define __fifo_exit_critical() void();
#else
#include "cmsis_gcc.h"
#define __fifo_enter_critical()    \
  uint32_t prim = __get_PRIMASK(); \
  __disable_irq();
#define __fifo_exit_critical() \
  if (!prim) {                 \
    __enable_irq();            \
  }
#endif

template <class T>
class _io_fifo_base {
 public:
  _io_fifo_base(T *first, T *seeker, T *last, T *start, T *end)
      : _first(first),
        _seeker(seeker),
        _cobs(seeker),
        _last(last),
        _start(start),
        _end(end) {
    // *_last = 0xFF;
  }

  void reset() { _first = _last = _seeker = _start; }
  void flush() { _first = _last; }
  void flush(uint32_t flSize) {
    __fifo_enter_critical();
    if (flSize >= count()) {
      _first = _last;
      __fifo_exit_critical();
      return;
    }

    if (_first < _last) {
      _first = &_first[flSize];
    } else {
      if ((&_first[flSize]) <= _end) {
        _first = &_first[flSize];
      } else {
        _first = &_start[flSize - (int)(_end - _first) - 1];
      }
    }

    __fifo_exit_critical();
  }
  void flush_to_seeker() { _first = _seeker; }
  void seek_start() { _seeker = _first; }

  void set_last(uint32_t n) { _last = &_start[n]; }

  bool empty() { return (_first == _last); }
  T *pointer() { return _start; }
  T *head() { return _first; }

  uint32_t size() { return (uint32_t)(_end - _start + 1); }

  uint32_t count() {
    int rv = 0;
    ;
    __fifo_enter_critical();
    if (_first <= _last) {
      rv = (int)(_last - _first);
    } else {
      rv = (int)((_end - _first + 1) + (_last - _start));
    }
    __fifo_exit_critical();
    return rv;
  }
  uint32_t count_to_edge() {
    __fifo_enter_critical();
    int rv = 0;
    if (_first < _last) {
      rv = (int)(_last - _first);
    }
    if (_first > _last) {
      rv = (int)(_end - _first + 1);
    }
    __fifo_exit_critical();
    return rv;
  }
  uint32_t pull_edge() {
    __fifo_enter_critical();
    uint32_t rv = 0;
    if (_first < _last) {
      rv = _last - _first;
      _first = _last;
      __fifo_exit_critical();
      return rv;
    }
    if (_first > _last) {
      rv = _end - _first + 1;
      _first = _start;
      __fifo_exit_critical();
      return rv;
    }
    __fifo_exit_critical();
    return rv;
  }
  inline bool push(T ch) {
    __fifo_enter_critical();
    if (_last == _end) {
      if (_first != _start) {
        *_last = ch;
        _last = _start;
        __fifo_exit_critical();
        return true;
      } else {
        __fifo_exit_critical();
        return false;
      }
    }
    if ((_last + 1) == _first) {
      __fifo_exit_critical();
      return false;
    }
    *_last++ = ch;
    __fifo_exit_critical();
    return true;
  }
  bool pushcobs(T ch) {
    __fifo_enter_critical();
    if (_cobs == _end) {
      if (_first != _start) {
        *_cobs = ch;
        _cobs = _start;
        __fifo_exit_critical();
        return true;
      } else {
        __fifo_exit_critical();
        return false;
      }
    }
    if ((_cobs + 1) == _first) {
      __fifo_exit_critical();
      return false;
    }
    *_cobs++ = ch;
    __fifo_exit_critical();
    return true;
  }
  void releasecobs() { _last = _cobs; }
  uint32_t push(T *src, uint32_t len) {
    uint32_t tmp = len;
    while (len) {
      if (!push(*src++)) {
        return (tmp - len);
      }
      len--;
    }
    return (tmp - len);  // TODO remove - len
  }

  bool pull(T *ch) {
    __fifo_enter_critical();
    if (_first == _last) {
      __fifo_exit_critical();
      return false;
    }
    *ch = *_first;
    _first++;
    if (_first > _end) {
      _first = _start;
    }
    __fifo_exit_critical();
    return true;
  }

  uint32_t pull(T *dsn, uint32_t len) {
    uint32_t tmp = len;
    while (len) {
      if (!pull(dsn++)) {
        return (tmp - len);
      }
      len--;
    }
    return (tmp - len);
  }
  bool seek(T *ch) {
    __fifo_enter_critical();
    // check seecker outside buffer for some cases
    if (_last < _first) {
      if (!((_seeker >= _first) || (_seeker <= _last))) {
        seek_start();
      }
    } else if (_last > _first) {
      if (!((_seeker >= _first) && (_seeker <= _last))) {
        seek_start();
      }
    }
    if (_seeker == _last) {
      __fifo_exit_critical();
      return false;
    }
    *ch = *_seeker++;
    if (_seeker > _end) {
      _seeker = _start;
    }
    __fifo_exit_critical();
    return true;
  }
  T *pxcobs() { return _cobs; }

 private:
  T *_first, *_seeker, *_cobs, *_last, *_start, *_end;
};

template <class T, int SIZE>
class _io_fifo : public _io_fifo_base<T> {
 public:
  _io_fifo()
      : _io_fifo_base<T>(&this->_buf[0], &this->_buf[0], &this->_buf[0],
                         &this->_buf[0], &this->_buf[SIZE - 1]) {}

 private:
  T _buf[SIZE];  // +1 for _last space.
};

typedef _io_fifo_base<uint8_t> _io_fifo_u8;
typedef _io_fifo_base<uint16_t> _io_fifo_u16;
typedef _io_fifo_base<uint32_t> _io_fifo_u32;
typedef _io_fifo_base<uint64_t> _io_fifo_u64;

typedef _io_fifo_base<int8_t> _io_fifo_s8;
typedef _io_fifo_base<int16_t> _io_fifo_s16;
typedef _io_fifo_base<int32_t> _io_fifo_s32;
typedef _io_fifo_base<int64_t> _io_fifo_s64;

typedef _io_fifo_base<float> _io_fifo_float;

#endif /* __IO_FIFO_H_ */
