#!/bin/bash
#### tested on Trisquel 7

sudo apt-get install libglib2.0-dev libsmf-dev libjack-jackd2-dev qjackctl libsndfile1-dev libsamplerate0-dev libboost-all-dev libespeak-dev cmake sox lmms samplerate-programs libfftw3-dev kakasi

mkdir vconnect
cd vconnect
dget http://mentors.debian.net/debian/pool/main/v/vconnectstand/vconnectstand_20151108a-1.dsc
dpkg-source -x vconnectstand_20151108a-1.dsc
cd vconnectstand-20151108a
debuild -us -uc
cd ..
sudo dpkg -i *.deb
cd ..

git clone https://github.com/kbinani/libvsq
cd libvsq
patch -p1 < ../libvsq.patch
cmake .
make
sudo make install
sudo ldconfig
cd ..

git clone https://github.com/divVerent/ecantorix
cd ecantorix
rm -rf .git
cd ..
sudo cp -r ecantorix /opt
sudo cpan install Config::Tiny
sudo cpan install MIDI::Opus
sudo cpan install Math::FFT
