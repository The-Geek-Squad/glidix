/*
	Glidix kernel

	Copyright (c) 2014-2017, Madd Games.
	All rights reserved.
	
	Redistribution and use in source and binary forms, with or without
	modification, are permitted provided that the following conditions are met:
	
	* Redistributions of source code must retain the above copyright notice, this
	  list of conditions and the following disclaimer.
	
	* Redistributions in binary form must reproduce the above copyright notice,
	  this list of conditions and the following disclaimer in the documentation
	  and/or other materials provided with the distribution.
	
	THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
	AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
	IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
	DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
	FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
	DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
	SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
	CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
	OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
	OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef SDIDE_H_
#define SDIDE_H_

#include <glidix/util/common.h>
#include <glidix/storage/storage.h>
#include <glidix/hw/pci.h>

/* status register */
#define ATA_SR_BSY			0x80    // Busy
#define ATA_SR_DRDY			0x40    // Drive ready
#define ATA_SR_DF			0x20    // Drive write fault
#define ATA_SR_DSC			0x10    // Drive seek complete
#define ATA_SR_DRQ			0x08    // Data request ready
#define ATA_SR_CORR			0x04    // Corrected data
#define ATA_SR_IDX			0x02    // Index
#define ATA_SR_ERR			0x01    // Error

/* error register */
#define ATA_ER_BBK			0x80    // Bad sector
#define ATA_ER_UNC			0x40    // Uncorrectable data
#define ATA_ER_MC			0x20    // No media
#define ATA_ER_IDNF			0x10    // ID mark not found
#define ATA_ER_MCR			0x08    // No media
#define ATA_ER_ABRT			0x04    // Command aborted
#define ATA_ER_TK0NF			0x02    // Track 0 not found
#define ATA_ER_AMNF			0x01    // No address mark

/* ATA commands */
#define ATA_CMD_READ_PIO		0x20
#define ATA_CMD_READ_PIO_EXT		0x24
#define ATA_CMD_READ_DMA		0xC8
#define ATA_CMD_READ_DMA_EXT		0x25
#define ATA_CMD_WRITE_PIO		0x30
#define ATA_CMD_WRITE_PIO_EXT		0x34
#define ATA_CMD_WRITE_DMA		0xCA
#define ATA_CMD_WRITE_DMA_EXT		0x35
#define ATA_CMD_CACHE_FLUSH		0xE7
#define ATA_CMD_CACHE_FLUSH_EXT		0xEA
#define ATA_CMD_PACKET			0xA0
#define ATA_CMD_IDENTIFY_PACKET		0xA1
#define ATA_CMD_IDENTIFY		0xEC

/* ATAPI commands */
#define ATAPI_CMD_READ			0xA8
#define ATAPI_CMD_EJECT			0x1B
#define	ATAPI_CMD_READ_CAPACITY		0x25

/* identification packet */
#define ATA_IDENT_DEVICETYPE		0
#define ATA_IDENT_CYLINDERS		2
#define ATA_IDENT_HEADS			6
#define ATA_IDENT_SECTORS		12
#define ATA_IDENT_SERIAL		20
#define ATA_IDENT_MODEL			54
#define ATA_IDENT_CAPABILITIES		98
#define ATA_IDENT_FIELDVALID		106
#define ATA_IDENT_MAX_LBA		120
#define ATA_IDENT_COMMANDSETS		164
#define ATA_IDENT_MAX_LBA_EXT		200

/* interfaces types */
#define IDE_ATA				0x00
#define IDE_ATAPI			0x01

/* channels */
#define	ATA_PRIMARY			0
#define	ATA_SECONDARY			1

/* master/slave */
#define ATA_MASTER			0x00
#define ATA_SLAVE			0x01

/* ATA registers, from I/O base */
#define	ATA_IOREG_DATA			0x00
#define	ATA_IOREG_FEATURES		0x01
#define	ATA_IOREG_SECCOUNT		0x02
#define ATA_IOREG_LBA0			0x03
#define ATA_IOREG_LBA1			0x04
#define ATA_IOREG_LBA2			0x05
#define	ATA_IOREG_HDDEVSEL		0x06
#define	ATA_IOREG_COMMAND		0x07
#define	ATA_IOREG_STATUS		0x07

/* ATA registers, from control base */
#define ATA_CREG_CONTROL		0x02

/* control bits */
#define	ATA_CTRL_NIEN			0x02
#define	ATA_CTRL_HOB			0x80

/* command sets */
#define	ATA_CMDSET_LBA_EXT		(1 << 26)

/**
 * Describes a channel.
 */
typedef struct
{
	uint16_t				base;	// I/O base
	uint16_t				ctrl;	// control base
} IDEChannel;

struct IDEController_;

/**
 * Describes an attached device.
 */
typedef struct
{
	/**
	 * Device type (IDE_ATA or IDE_ATAPI).
	 */
	int					type;
	
	/**
	 * Channel (primary or secondary) and slot (master or slave).
	 */
	int					channel, slot;
	
	/**
	 * The controller.
	 */
	struct IDEController_*			ctrl;
	
	/**
	 * The storage device object.
	 */
	StorageDevice*				sd;
	
	/**
	 * The thread which handles this device.
	 */
	Thread*					thread;
} IDEDevice;

/**
 * Describes an IDE controller.
 */
typedef struct IDEController_
{
	/**
	 * Next controller.
	 */
	struct IDEController_*			next;
	
	/**
	 * The PCI device description associated with the controller.
	 */
	PCIDevice*				pcidev;
	
	/**
	 * Primary and secondary channels.
	 */
	IDEChannel				channels[2];
	
	/**
	 * Lock for accessing devices on this controller.
	 */
	Semaphore				lock;
	
	/**
	 * Devices, and the number of them.
	 */
	IDEDevice				devs[4];
	int					numDevs;
} IDEController;

extern volatile int ideInts[2];

#endif
