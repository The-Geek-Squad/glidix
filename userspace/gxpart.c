/*
	Glidix Parition Manager

	Copyright (c) 2014-2016, Madd Games.
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

#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>

#include <glidix/storage.h>

int device;
SDParams params;

typedef struct
{
	uint8_t							flags;			// 0x80 = bootable
	uint8_t							startHead;
	uint16_t						startSectorCylinder;
	uint8_t							systemID;
	uint8_t							endHead;
	uint16_t						endSectorCylinder;
	uint32_t						lbaStart;
	uint32_t						numSectors;
} __attribute__ ((packed)) Partition;

typedef struct
{
	uint8_t							bootloader[446];
	Partition						parts[4];
	uint8_t							bootsig[2];
} __attribute__ ((packed)) MBR;

MBR mbr;

void getline(char *buf)
{
	char c = 0;
	while (1)
	{
		fread(&c, 1, 1, stdin);
		if (c == '\n')
		{
			*buf = 0;
			break;
		};
		*buf++ = c;
	};
};

void printPartitionTable()
{
#if 0
	printf("#\tBoot\tType\tStart\t\tSize\n");
	int i;
	for (i=0; i<4; i++)
	{
		uint64_t start = (((uint64_t)mbr.parts[i].startHigh << 32) + (uint64_t)mbr.parts[i].startLow) * (uint64_t)params.blockSize;
		uint64_t len = (((uint64_t)mbr.parts[i].lenHigh << 32) + (uint64_t)mbr.parts[i].lenLow) * (uint64_t)params.blockSize;
		char boot = ' ';
		if (mbr.parts[i].flags & 0x80) boot = '*';
		//printf("%d\t%c\t0x%x\t%d\t%d\n", i, boot, mbr.parts[i].systemID, start, len);
		printf("%d\t%c\t0x%x\t%lu", i, boot, mbr.parts[i].systemID, start);
		if (start <= 9999999) printf("\t");
		printf("\t%lu\n", len);
	};
#endif

	printf("# Boot Type Start      Size\n");
	int i;
	for (i=0; i<4; i++)
	{
		char boot = ' ';
		if (mbr.parts[i].flags & 0x80) boot = '*';
		
		printf("%d [%c]  0x%02hhX 0x%08x 0x%08x\n", i, boot, mbr.parts[i].systemID, mbr.parts[i].lbaStart, mbr.parts[i].numSectors);
	};
};

uint64_t _toint(const char *num)
{
	uint64_t out = 0;
	while (*num != 0)
	{
		out = out * 10 + ((*num++)-'0');
	};
	return out;
};

uint64_t getFirstUnusedOffset()
{
	uint64_t lowest = 512;
	int i;
	for (i=0; i<4; i++)
	{
		if ((mbr.parts[i].lbaStart+mbr.parts[i].numSectors) > lowest) lowest = mbr.parts[i].lbaStart+mbr.parts[i].numSectors;
	};
	return lowest;
};

uint64_t getFirstFreePartition()
{
	uint64_t i;
	for (i=0; i<4; i++)
	{
		Partition *part = &mbr.parts[i];
		if (part->numSectors == 0)
		{
			return i;
		};
	};
	return 4;
};

void runcmd(char *cmd)
{
	if (strcmp(cmd, "create") == 0)
	{
		char buffer[256];
		uint64_t i = getFirstFreePartition();
		if (i == 4)
		{
			fprintf(stderr, "The partition table is full!\n");
			return;
		};
		printf("Which partition (slot)? [%d] ", i);
		fflush(stdout);
		getline(buffer);
		if (buffer[0] != 0) i = _toint(buffer);
		if ((i < 0) || (i > 4))
		{
			fprintf(stderr, "%d is not a valid partition!\n", i);
			return;
		};

		uint64_t offset = getFirstUnusedOffset();
		printf("Offset? [%lu] ", offset);
		fflush(stdout);
		getline(buffer);
		if (buffer[0] != 0) offset = _toint(buffer);
		if ((offset % params.blockSize) != 0)
		{
			fprintf(stderr, "Offset must be aligned on block boundary (%d)!\n", params.blockSize);
			return;
		};

		uint64_t size = params.totalSize - offset;
		printf("Size? [%lu] ", size);
		fflush(stdout);
		getline(buffer);
		if (buffer[0] != 0) size = _toint(buffer);
		if (size < params.blockSize)
		{
			fprintf(stderr, "The specified partition size is too small (less than 1 block)!\n");
			return;
		};

		if ((offset < params.blockSize) || ((offset+size) > params.totalSize))
		{
			fprintf(stderr, "The specified partition does not fit on disk!\n");
			return;
		};

		uint64_t startLBA = offset / params.blockSize;
		uint64_t lenLBA = size / params.blockSize;

		Partition *part = &mbr.parts[i];
		part->flags = 0;
		part->startHead = 0;
		part->startSectorCylinder = 0;
		part->systemID = 0x7F;
		part->endHead = 0;
		part->endSectorCylinder = 0;
		part->lbaStart = startLBA;
		part->numSectors = lenLBA;
	}
	else if (strcmp(cmd, "cancel") == 0)
	{
		char buffer[256];
		printf("Exit without saving (yes/no)? [no] ");
		fflush(stdout);
		getline(buffer);

		if (strcmp(buffer, "yes") == 0)
		{
			close(device);
			printf("Changes discarded.\n");
			exit(0);
		};
	}
	else if (strcmp(cmd, "done") == 0)
	{
		char buffer[256];
		printf("Save to disk and exit (yes/no)? [no] ");
		fflush(stdout);
		getline(buffer);

		if (strcmp(buffer, "yes") == 0)
		{
			lseek(device, 0, SEEK_SET);
			write(device, &mbr, 512);
			close(device);
			printf("Partition table saved.\n");
			exit(0);
		};
	}
	else if (strcmp(cmd, "delete") == 0)
	{
		char buffer[256];
		printf("Which partition (slot)? [0] ");
		fflush(stdout);
		getline(buffer);
		uint64_t i = 0;
		if (buffer[0] != 0) i = _toint(buffer);
		if ((i < 0) || (i > 4))
		{
			fprintf(stderr, "%d is not a valid partition!\n", i);
			return;
		};

		Partition *part = &mbr.parts[i];
		memset(part, 0, sizeof(Partition));
	}
	else if (strcmp(cmd, "setboot") == 0)
	{
		char buffer[256];
		printf("Which partition should be bootable? [0] ");
		fflush(stdout);
		getline(buffer);
		uint64_t i = 0;
		if (buffer[0] != 0) i = _toint(buffer);
		if ((i < 0) || (i > 4))
		{
			fprintf(stderr, "%d is not a valid partition!\n", i);
			return;
		};

		Partition *part = &mbr.parts[i];

		int j;
		for (j=0; j<4; j++)
		{
			mbr.parts[j].flags = 0;
		};
		part->flags = 0x80;
		printf("The partition is now bootable!\n");
	}
	else if ((strcmp(cmd, "wipe") == 0) || (strcmp(cmd, "clear") == 0))
	{
		char buffer[256];
		printf("Are you sure you want to wipe the partition table (yes/no)? [no] ");
		fflush(stdout);
		getline(buffer);

		if (strcmp(buffer, "yes") == 0)
		{
			int i;
			for (i=0; i<4; i++)
			{
				Partition *part = &mbr.parts[i];
				memset(part, 0, sizeof(Partition));
			};
		};
	}
	else
	{
		fprintf(stderr, "Command not recognised!\n");
	};
};

int main(int argc, char *argv[])
{
	if (argc != 2)
	{
		fprintf(stderr, "%s <file>\n", argv[0]);
		fprintf(stderr, "  Edits the partition table in a specified file or storage device.\n");
		return 1;
	};

	const char *filename = argv[1];

	device = open(filename, O_RDWR);
	if (device < 0)
	{
		fprintf(stderr, "Could not open the specified device or image\n");
		return 1;
	};

	ioctl(device, IOCTL_SDI_IDENTITY, &params);
	read(device, &mbr, 512);

	while (1)
	{
		printf("Block size: %lu\tCapacity (bytes): %lu\n", params.blockSize, params.totalSize);
		printPartitionTable();
		printf("gxpart> ");
		fflush(stdout);
		char buffer[256];
		getline(buffer);
		runcmd(buffer);
	};

	return 0;
};
