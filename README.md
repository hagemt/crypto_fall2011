Description
===========

  Plouton is designed to be a secure client/server/gateway solution
  for managing simple banking transactions. Its protocol consists of
  Diffie-Hellman style key exchange atop a public key cryptosystem,
  with a discrete intermediatary element that may be built alongside
  the client and server to simulate compartmentalization of
  communication at boundary points of a computer network.

  Most importantly, this facilitates quality assurance through
  easing the implementation of performance testing and analysis
  of possible attack vectors. Its author(s) aim(s) to deliver
  reliable and transparent security features.

  Plouton leverages the readline, gcrypt, pthread, and SQLite3 libraries.

Changelog
=========

  Initial Release:

  * Developing: ATM client operates in plaintext mode. [atm]
  * Backend: Account information is stored in a SQL database. [bank]
  * Feature: A robust shell for simple user interaction. [bank]
  * Feature: Worker threads to handle simultaneous connections. [bank]
  * Developing: A transparent courier between local ports. [proxy]

License
=======

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

Contributors
============

  * Tor E. Hagemann <hagemt@rpi.edu>

