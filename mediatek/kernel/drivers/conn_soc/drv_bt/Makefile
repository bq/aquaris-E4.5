#
# Copyright MediaTek Inc. 2010.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA


#
# Makefile for the Linux Bluetooth HCI device drivers.
#
#ccflags-y := -I$(src)/include -I$(src)/../common/include -I$(src)/../common/linux/include 
ccflags-y :=-I$(src)/../common/core/include \
					  -I$(src)/include \
					  -I$(src)/../common/include \
					  -I$(src)/../common/linux/include \
					  -I$(src)/../../common_detect \
					  -I$(src)/../../btif/inc \
					  -I$(src)/../common/linux/pri/include
obj-$(CONFIG_MTK_COMBO_BT_HCI) += hci_stp.o
#obj-y += hci_stp.o
hci_stp-objs	:= linux/hci_stp.o

