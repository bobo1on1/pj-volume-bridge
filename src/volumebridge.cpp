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
#include <thread>
#include <chrono>
#include <cstdio>
#include <getopt.h>

CVolumeBridge::CVolumeBridge() : m_jackclient(m_pulsevolume)
{
}

CVolumeBridge::~CVolumeBridge()
{
}

bool CVolumeBridge::Setup(int argc, char *argv[])
{
  struct option longoptions[] =
  {
    {"help",        no_argument,       NULL, 'h'},
    {"name",        required_argument, NULL, 'n'},
    {"port",        required_argument, NULL, 'p'},
    {"sink",        required_argument, NULL, 's'},
    {0, 0, 0, 0}
  };

  const char* shortoptions = "hn:p:s:";
  int         optionindex = 0;
  int         c;

  while ((c = getopt_long(argc, argv, shortoptions, longoptions, &optionindex)) != -1)
  {
    if (c == 'h' || c == '?')
    {
      PrintHelpMessage();
      return false;
    }
    else if (c == 'n')
    {
      m_jackname = optarg;
    }
    else if (c == 'p')
    {
      m_portnames.push_back(optarg);
    }
    else if (c == 's')
    {
      m_sinkname = optarg;
    }
  }

  //set the default jack name if not set by command line argument
  if (m_jackname.empty())
    m_jackname = "pulsevolume";

  //set default port names if no ports are given
  if (m_portnames.empty())
  {
    m_portnames.push_back("left");
    m_portnames.push_back("right");
  }

  return true;
}

void CVolumeBridge::PrintHelpMessage()
{
  printf("usage: pulse-jack-volume-bridge [option]\n");
  printf("\n");
  printf("Creates jack input-output port pairs, copies audio from input to output ports,\n");
  printf("scales the audio with the volume from a pulseaudio sink.\n");
  printf("The volume is only correct for sinks that use software volume control.\n");
  printf("\n");
  printf("  options:\n");
  printf("\n");
  printf("    -h, --help:      Print this message.\n");
  printf("\n");
  printf("    -n, --name       Set the name of the jack client to create.\n");
  printf("\n");
  printf("    -p, --port:      Adds a jack input-output port pair.\n");
  printf("                     The input port is suffixed with -in.\n");
  printf("                     The output port is suffixed with -out.\n");
  printf("                     If no ports are added, left and right ports\n");
  printf("                     are added by default.\n");
  printf("\n");
  printf("    -s, --sink:      The name of the sink to use, if no name is given,\n");
  printf("                     the default pulseaudio sink is used.\n");
  printf("\n");
  printf("  example:\n");
  printf("    pulse-jack-volume-bridge -s jack_out -p left -p right\n");
  printf("\n");
}

void CVolumeBridge::Run()
{
  //start a thread to connect to jackd
  std::thread jackthread(&CVolumeBridge::ConnectJackClient, this);

  //connect to pulseaudio from this thread
  ConnectPulseClient();
}

void CVolumeBridge::ConnectJackClient()
{
  for(;;)
  {
    if (!m_jackclient.Setup(m_jackname, m_portnames))
    {
      printf("ERROR: Setting up jack client failed, retrying in 10 seconds\n");
      m_jackclient.Cleanup();
      std::this_thread::sleep_for(std::chrono::seconds(10));
      continue;
    }

    m_jackclient.Run();

    //prevent busy looping in case Run() fails quickly after Setup() succeeds
    std::this_thread::sleep_for(std::chrono::seconds(1));

    m_jackclient.Cleanup();
  }
}

void CVolumeBridge::ConnectPulseClient()
{
  for(;;)
  {
    if (!m_pulsevolume.Setup(m_sinkname))
    {
      printf("ERROR: Setup pulse client failed, retrying in 10 seconds\n");
      m_pulsevolume.Cleanup();
      std::this_thread::sleep_for(std::chrono::seconds(10));
      continue;
    }

    m_pulsevolume.Run();

    //prevent busy looping in case Run() fails quickly after Setup() succeeds
    std::this_thread::sleep_for(std::chrono::seconds(1));

    m_pulsevolume.Cleanup();
  }
}
