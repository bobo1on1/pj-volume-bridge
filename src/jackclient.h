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

#ifndef JACKCLIENT_H
#define JACKCLIENT_H

#include "pulsevolume.h"

#include <jack/jack.h>
#include <vector>
#include <string>
#include <mutex>
#include <condition_variable>

class CJackClient
{
  public:
    CJackClient(CPulseVolume& pulsevolume);
    ~CJackClient();

    bool                      Setup(std::string jackname, std::vector<std::string> portnames);
    void                      Run();
    void                      Cleanup();

  private:
    CPulseVolume&             m_pulsevolume;

    jack_client_t*            m_client;
    std::vector<jack_port_t*> m_inports;
    std::vector<jack_port_t*> m_outports;
    bool                      m_connected;
    std::mutex                m_mutex;
    std::condition_variable   m_condition;
    float                     m_prevvolume;
    int                       m_samplerate;

    bool                      CreatePort(std::string name);

    static void               SJackInfoShutdownCallback(jack_status_t code, const char *reason, void *arg);
    void                      PJackInfoShutdownCallback(jack_status_t code, const char *reason);

    static int                SJackSamplerateCallback(jack_nframes_t nframes, void *arg);
    int                       PJackSamplerateCallback(jack_nframes_t nframes);

    static int                SJackProcessCallback(jack_nframes_t nframes, void *arg);
    int                       PJackProcessCallback(jack_nframes_t nframes);
};

#endif //JACKCLIENT_H
