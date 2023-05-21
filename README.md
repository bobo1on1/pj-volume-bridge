# pj-volume-bridge
Jack client that uses pulseaudio for volume control.<br>
Its main use is to apply the same volume control as the module-jack-sink pulseaudio module.<br>
This way, the line-in volume and the volume of jack clients can be controlled using pulseaudio.<br>

Installing
==========

Install the libpulse and libjack development files.

Check out the repository:<br>
```$git clone https://github.com/bobo1on1/pj-volume-bridge.git```
  
This will create a new directory called pj-volume-bridge, to compile:<br>
```
$ cd pj-volume-bridge
$ ./waf configure
$ ./waf
```
  
To install on your system:<br>
```$ sudo ./waf install```

Using
=====

To start pj-volume-bridge, simply enter the pj-volume-bridge command in a terminal without arguments:<br>
```
$ pj-volume-bridge
Connecting to jackd
Pulse: Connecting
Pulse: Authorizing
Pulse: Setting name
Pulse: Ready
default sink is "jack_out"
Pulse: volume: 45%, factor: 0.091077
Connected to jackd
```
It will connect to jackd with the name pulsevolume, and creates 2 input ports and 2 output ports.<br>
The volume from the default pulseaudio sink is used.<br>
Audio from the input ports is multiplied with the factor from the [pa_sw_volume_to_linear()](https://freedesktop.org/software/pulseaudio/doxygen/volume_8h.html#a04da6c4572a758a0244bbfc81d370cfb) function.<br>
Its advised to set the sink to the jack_out sink using the --sink=jack_out command line argument.<br>

Command line options
=====
```
    -h, --help:      Print this message.

    -n, --name       Set the name of the jack client to create.

    -p, --port:      Adds a jack input-output port pair.
                     The input port is suffixed with -in.
                     The output port is suffixed with -out.
                     If no ports are added, left and right ports
                     are added by default.

    -s, --sink:      The name of the sink to use, if no name is given,
                     the default pulseaudio sink is used.

  example:
    pj-volume-bridge -s jack_out -p left -p right
```
