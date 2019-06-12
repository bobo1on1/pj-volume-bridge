/*  This file is part of pulse-jack-volume-bridge.

    pulse-jack-volume-bridge is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    pulse-jack-volume-bridge is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with pulse-jack-volume-bridge.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "volumebridge.h"

int main(int argc, char *argv[])
{
  CVolumeBridge volumebridge;
  if (!volumebridge.Setup(argc, argv))
    return 1;

  volumebridge.Run();

  return 0;
}