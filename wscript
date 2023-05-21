#! /usr/bin/env python
# encoding: utf-8

# the following two variables are used by the target "waf dist"
VERSION='0.0.1'
APPNAME='pj-volume-bridge'

# these variables are mandatory ('/' are converted automatically)
top = '.'
out = 'build'

def options(opt):
  opt.load('compiler_cxx')

def configure(conf):
  conf.load('compiler_cxx')

  conf.check(header_name='jack/jack.h')
  conf.check(header_name='pulse/pulseaudio.h')

  conf.check(lib='m', uselib_store='m', mandatory=False)
  conf.check(lib='pulse', uselib_store='pulse')
  conf.check(lib='jack', uselib_store='jack')
  conf.check(lib='pthread', uselib_store='pthread', mandatory=False)

def build(bld):
  bld.program(source='src/main.cpp\
                      src/pulsevolume.cpp\
                      src/jackclient.cpp\
                      src/volumebridge.cpp',
              use=['jack','m','pthread','pulse'],
              includes='./src',
              cxxflags='-Wall -g',
              target='pj-volume-bridge')

