#include "mbed.h"
#include "millis.h"

using namespace std::chrono;

/*
 millis.cpp
 Copyright (c) 2016 Zoltan Hudak <hudakz@inbox.com>
 All rights reserved.

 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.
  */

unsigned long millis(void) {
    auto now_ms = time_point_cast<milliseconds>(Kernel::Clock::now());
    return now_ms.time_since_epoch().count();;
}


