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

set(CONFIGURATION_HEADER "${CMAKE_SOURCE_DIR}/libplouton.h")
configure_file("libplouton.h.in" "${CONFIGURATION_HEADER}" @ONLY)
set_property(SOURCE "${CONFIGURATION_HEADER}" PROPERTY GENERATED TRUE)
install(
	FILES "${CONFIGURATION_HEADER}"
	PUBLIC_HEADER DESTINATION include
	COMPONENT "Headers"
)

file(GLOB LIBPLOUTON_HEADERS "*.h")
install(
	FILES ${LIBPLOUTON_HEADERS}
	PUBLIC_HEADER DESTINATION include/libplouton
	COMPONENT "Headers"
)
