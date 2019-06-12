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

#include "pulsevolume.h"
#include <cstdio>
#include <cmath>

CPulseVolume::CPulseVolume()
{
  m_mainloop       = nullptr;
  m_context        = nullptr;
  m_usedefaultsink = false;
  m_state          = PA_CONTEXT_UNCONNECTED;
  m_volume         = 0.0f;
}

CPulseVolume::~CPulseVolume()
{
}

bool CPulseVolume::Setup(std::string sinkname)
{
  if (sinkname.empty())
  {
    m_usedefaultsink = true;
    m_sinkname.clear();
  }
  else
  {
    m_sinkname = sinkname;
  }

  //set up pulseaudio as threaded mainloop
  m_mainloop = pa_threaded_mainloop_new();
  pa_threaded_mainloop_start(m_mainloop);

  pa_threaded_mainloop_lock(m_mainloop);

  pa_mainloop_api *api = pa_threaded_mainloop_get_api(m_mainloop);
  m_context = pa_context_new(api, "pulse-jack-volume-bridge");

  pa_context_set_state_callback(m_context, SStateCallback, this);
  pa_context_set_subscribe_callback(m_context, SSubscribeCallback, this);

  //try to connect to the pulseaudio daemon
  pa_context_connect(m_context, NULL, PA_CONTEXT_NOAUTOSPAWN, NULL);
  while(m_state != PA_CONTEXT_READY && m_state != PA_CONTEXT_FAILED && m_state != PA_CONTEXT_TERMINATED)
  {
    pa_threaded_mainloop_wait(m_mainloop);
    if (m_state == PA_CONTEXT_FAILED || m_state == PA_CONTEXT_TERMINATED)
    {
      printf("Pulse: pa_context_connect failed\n");
      return false;
    }
  }

  //subscribe to sink messages, to get a callback when the volume changes
  pa_subscription_mask_t mask = (pa_subscription_mask_t)((int)PA_SUBSCRIPTION_MASK_SINK | (int)PA_SUBSCRIPTION_MASK_SERVER);
  pa_operation* o = pa_context_subscribe(m_context, mask, SSuccessCallback, this);
  pa_operation_state_t state;
  while ((state = pa_operation_get_state(o)) != PA_OPERATION_DONE)
  {
    pa_threaded_mainloop_wait(m_mainloop);

    if (state == PA_OPERATION_CANCELLED)
    {
      printf("Pulse: pa_context_subscribe failed\n");
      return false;
    }
  }
  pa_operation_unref (o);

  o = pa_context_get_server_info(m_context, SServerInfoCallback, this);
  while ((state = pa_operation_get_state(o)) != PA_OPERATION_DONE)
  {
    pa_threaded_mainloop_wait(m_mainloop);

    if (state == PA_OPERATION_CANCELLED)
    {
      printf("Pulse: pa_context_get_server_info failed\n");
      return false;
    }
  }
  pa_operation_unref (o);

  o = pa_context_get_sink_info_by_name(m_context, m_sinkname.c_str(), SSinkInfoCallback, this);
  pa_operation_unref(o);

  pa_threaded_mainloop_unlock(m_mainloop);

  return true;
}

void CPulseVolume::Run()
{
  for(;;)
  {
    //wait until the pulseaudio volume changes
    pa_threaded_mainloop_lock(m_mainloop);

    while (m_state == PA_CONTEXT_READY)
      pa_threaded_mainloop_wait(m_mainloop);

    pa_threaded_mainloop_unlock(m_mainloop);

    if (m_state != PA_CONTEXT_READY)
    {
      printf("Pulse: context failed\n");
      return;
    }
  }
}

void CPulseVolume::Cleanup()
{
  if (m_context && m_mainloop)
  {
    pa_threaded_mainloop_lock(m_mainloop);
    pa_context_disconnect(m_context);
    pa_context_unref(m_context);
    m_context = nullptr;
    pa_threaded_mainloop_unlock(m_mainloop);
  }

  if (m_mainloop)
  {
    pa_threaded_mainloop_stop(m_mainloop);
    pa_threaded_mainloop_free(m_mainloop);
    m_mainloop= nullptr;
  }

  m_state = PA_CONTEXT_UNCONNECTED;
  
  m_volume = 0.0f;
}

void CPulseVolume::SStateCallback(pa_context* c, void *userdata)
{
  ((CPulseVolume*)userdata)->StateCallback(c);
}

void CPulseVolume::StateCallback(pa_context* c)
{
  m_state = pa_context_get_state(c);

  if (m_state == PA_CONTEXT_UNCONNECTED)
    printf("Pulse: Unconnected\n");
  else if (m_state == PA_CONTEXT_CONNECTING)
    printf("Pulse: Connecting\n");
  else if (m_state == PA_CONTEXT_AUTHORIZING)
    printf("Pulse: Authorizing\n");
  else if (m_state == PA_CONTEXT_SETTING_NAME)
    printf("Pulse: Setting name\n");
  else if (m_state == PA_CONTEXT_READY)
    printf("Pulse: Ready\n");
  else if (m_state == PA_CONTEXT_FAILED)
    printf("Pulse: Connect failed\n");
  else if (m_state == PA_CONTEXT_TERMINATED)
    printf("Pulse: Connection terminated\n");

  pa_threaded_mainloop_signal(m_mainloop, 0);
}

void CPulseVolume::SSubscribeCallback(pa_context *c, pa_subscription_event_type_t t, uint32_t idx, void *userdata)
{
  ((CPulseVolume*)userdata)->SubscribeCallback(c, t, idx);
}

void CPulseVolume::SubscribeCallback(pa_context *c, pa_subscription_event_type_t t, uint32_t idx)
{
  if ((t & PA_SUBSCRIPTION_EVENT_TYPE_MASK) != PA_SUBSCRIPTION_EVENT_CHANGE)
    return;

  //default sink might have changed, check it
  if ((t & PA_SUBSCRIPTION_EVENT_FACILITY_MASK) == PA_SUBSCRIPTION_EVENT_SERVER)
  {
    pa_operation* o = pa_context_get_server_info(m_context, SServerInfoCallback, this);
    pa_operation_unref(o);
  }

  //get the current volume
  pa_operation* o = pa_context_get_sink_info_by_name(m_context, m_sinkname.c_str(), SSinkInfoCallback, this);
  pa_operation_unref(o);
}

void CPulseVolume::SSuccessCallback(pa_context *c, int success, void *userdata)
{
  ((CPulseVolume*)userdata)->SuccessCallback(c, success);
}

void CPulseVolume::SuccessCallback(pa_context *c, int success)
{
  pa_threaded_mainloop_signal(m_mainloop, 0);
}

void CPulseVolume::SSinkInfoCallback(pa_context *c, const pa_sink_info *i, int eol, void *userdata)
{
  ((CPulseVolume*)userdata)->SinkInfoCallback(c, i, eol);
}

void CPulseVolume::SinkInfoCallback(pa_context *c, const pa_sink_info *i, int eol)
{
  if (eol)
    return;

  pa_volume_t pulsevolume;
  if (i->mute)
    pulsevolume = 0;
  else
    pulsevolume = pa_cvolume_avg(&i->volume);

  float volume = pa_sw_volume_to_linear(pulsevolume);
  if (volume != m_volume)
  {
    int volumepercentage = lround((double)pulsevolume / PA_VOLUME_NORM * 100.0);
    printf("Pulse: volume: %i%%, factor: %f\n", volumepercentage, volume);
    m_volume = volume;
  }
}

void CPulseVolume::SServerInfoCallback(pa_context *c, const pa_server_info*i, void *userdata)
{
  ((CPulseVolume*)userdata)->ServerInfoCallback(c, i);
}

void CPulseVolume::ServerInfoCallback(pa_context *c, const pa_server_info*i)
{
  if (m_usedefaultsink)
  {
    printf("default sink is \"%s\"\n", i->default_sink_name);
    m_sinkname = i->default_sink_name;
  }

  pa_threaded_mainloop_signal(m_mainloop, 0);
}
