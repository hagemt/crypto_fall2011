##
# Copyright 2011 by Tor E. Hagemann <hagemt@rpi.edu>
# This file is part of Plouton.
#
# Plouton is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# Plouton is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with Plouton.  If not, see <http://www.gnu.org/licenses/>.
##

if(BUILD_ATM)
	add_executable(atm atm.c)
	target_link_libraries(atm plouton)
	install(TARGETS atm
		RUNTIME DESTINATION bin
		COMPONENT "ATM"
	)
endif(BUILD_ATM)

if(BUILD_BANK)
	add_executable(bank bank.c)
	target_link_libraries(bank plouton)
	install(TARGETS bank
		RUNTIME DESTINATION bin
		COMPONENT "Bank"
	)
endif(BUILD_BANK)

if(BUILD_PROXY)
	add_executable(proxy proxy.c)
	target_link_libraries(proxy plouton)
	install(TARGETS proxy
		RUNTIME DESTINATION bin
		COMPONENT "Proxy"
	)
endif(BUILD_PROXY)
