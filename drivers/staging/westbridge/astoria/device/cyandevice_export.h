/*
## cyandevice_export.h - Linux Antioch device driver file
##
## ===========================
## Copyright (C) 2010  Cypress Semiconductor
##
## This program is free software; you can redistribute it and/or
## modify it under the terms of the GNU General Public License
## as published by the Free Software Foundation; either version 2
## of the License, or (at your option) any later version.
##
## This program is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.
##
## You should have received a copy of the GNU General Public License
## along with this program; if not, write to the Free Software
## Foundation, Inc., 51 Franklin Street
## Fifth Floor, Boston, MA  02110-1301, USA.
## ===========================
*/

/*
 * Export Misc APIs that can be used from the other driver modules.
 * The APIs to create a device handle and download firmware are not exported
 * because they are expected to be used only by this kernel module.
 */

EXPORT_SYMBOL(cy_as_hal_alloc) ;
EXPORT_SYMBOL(cy_as_hal_free) ;
EXPORT_SYMBOL(cy_as_hal_mem_set);
/*[]*/
