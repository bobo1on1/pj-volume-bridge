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

#include "jackclient.h"
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <thread>

CJackClient::CJackClient(CPulseVolume& pulsevolume) : m_pulsevolume(pulsevolume)
{
  m_client    = nullptr;
  m_connected = false;
}

CJackClient::~CJackClient()
{
}

bool CJackClient::Setup(std::string jackname, std::vector<std::string> portnames)
{
  printf("Connecting to jackd\n");

  //truncate the name to the maximum allowed length
  std::string name = jackname.substr(0, jack_client_name_size() - 2);

  //try to connect to jackd
  m_client = jack_client_open(name.c_str(), JackNoStartServer, nullptr);
  if (m_client == nullptr)
  {
    printf("ERROR: unable to connect to jackd: %i:%s\n", (int)errno, strerror(errno));
    return false;
  }

  //register a callback for when jackd shuts down, so that the jack client can be restarted
  jack_on_info_shutdown(m_client, SJackInfoShutdownCallback, this);

  //SJackProcessCallback gets called when jack has new audio data to process
  jack_set_process_callback(m_client, SJackProcessCallback, this);

  //register jack input and output ports
  for (uint32_t i = 0; i < portnames.size(); i++)
  {
    if (!CreatePort(portnames[i].c_str()))
      return false;
  }

  //activate the jack client, so that it starts processing audio
  jack_activate(m_client);

  m_connected = true;

  printf("Connected to jackd\n");

  return true;
}

void CJackClient::Run()
{
  //wait on the condition variable until m_connected becomes false
  std::unique_lock<std::mutex> lock(m_mutex);
  m_condition.wait(lock, [this] { return !m_connected; });
}

void CJackClient::Cleanup()
{
  if (m_client)
  {
    jack_deactivate(m_client);
    jack_client_close(m_client);
    m_client = nullptr;
  }

  m_inports.clear();
  m_outports.clear();
}

bool CJackClient::CreatePort(std::string name)
{
  jack_port_t* jackport;

  //add an input port
  std::string inname = name + "-in";
  jackport = jack_port_register(m_client, inname.c_str(), JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0);
  if (jackport == nullptr)
  {
    printf("ERROR: unable to register input port with name \"%s\"\n", inname.c_str());
    return false;
  }
  m_inports.push_back(jackport);

  //add an output port
  std::string outname = name + "-out";
  jackport = jack_port_register(m_client, outname.c_str(), JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0);
  if (jackport == nullptr)
  {
    printf("ERROR: unable to register output port with name \"%s\"\n", outname.c_str());
    return false;
  }
  m_outports.push_back(jackport);

  return true;
}

void CJackClient::SJackInfoShutdownCallback(jack_status_t code, const char *reason, void *arg)
{
  ((CJackClient*)arg)->PJackInfoShutdownCallback(code, reason);
}

void CJackClient::PJackInfoShutdownCallback(jack_status_t code, const char *reason)
{
  //signal the main thread that the jack client has been disconnected
  std::unique_lock<std::mutex> lock(m_mutex);
  m_connected = false;
  m_condition.notify_all();
}

int CJackClient::SJackProcessCallback(jack_nframes_t nframes, void *arg)
{
  return ((CJackClient*)arg)->PJackProcessCallback(nframes);
}

int CJackClient::PJackProcessCallback(jack_nframes_t nframes)
{
  float volume = m_pulsevolume.Volume();

  //copy all samples from each input port to each output port,
  //multiplied by the volume from pulseaudio
  for (uint32_t i = 0; i < m_inports.size(); i++)
  {
    float* inptr  = (float*)jack_port_get_buffer(m_inports[i], nframes);
    float* outptr = (float*)jack_port_get_buffer(m_outports[i], nframes);

    float* endptr = inptr + nframes;
    while (inptr != endptr)
      *(outptr++) = *(inptr++) * volume;
  }

  return 0;
}
