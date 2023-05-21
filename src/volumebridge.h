/*  This file is part of pj-volume-bridge.

    pj-volume-bridge is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    pj-volume-bridge is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with pj-volume-bridge.  If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef VOLUMEBRIDGE_H
#define VOLUMEBRIDGE_H

#include "pulsevolume.h"
#include "jackclient.h"

#include <vector>
#include <string>

class CVolumeBridge
{
  public:
    CVolumeBridge();
    ~CVolumeBridge();

    bool                     Setup(int argc, char *argv[]);
    void                     Run();

  private:
    void                     PrintHelpMessage();

    void                     ConnectJackClient();
    void                     ConnectPulseClient();

    CJackClient              m_jackclient;
    CPulseVolume             m_pulsevolume;

    std::string              m_jackname;
    std::vector<std::string> m_portnames;
    std::string              m_sinkname;
};

#endif //VOLUMEBRIDGE_H
