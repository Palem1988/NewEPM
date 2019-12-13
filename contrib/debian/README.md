
Debian
====================
This directory contains files used to package epmcoind/epmcoin-qt
for Debian-based Linux systems. If you compile epmcoind/epmcoin-qt yourself, there are some useful files here.

## epmcoin: URI support ##


epmcoin-qt.desktop  (Gnome / Open Desktop)
To install:

	sudo desktop-file-install epmcoin-qt.desktop
	sudo update-desktop-database

If you build yourself, you will either need to modify the paths in
the .desktop file or copy or symlink your epmcoin-qt binary to `/usr/bin`
and the `../../share/pixmaps/epmcoin128.png` to `/usr/share/pixmaps`

epmcoin-qt.protocol (KDE)

