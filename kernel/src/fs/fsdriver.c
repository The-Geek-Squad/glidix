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

#include <glidix/fs/fsdriver.h>
#include <glidix/thread/semaphore.h>
#include <glidix/thread/sched.h>
#include <glidix/util/memory.h>
#include <glidix/util/string.h>
#include <glidix/int/syscall.h>
#include <glidix/display/console.h>

static Semaphore semFS;
static FSDriver *firstDriver;

static Inode* fsbind(const char *image, int flags, const void *options, size_t optlen, int *error)
{
	File *fp = vfsOpen(VFS_NULL_IREF, image, O_RDWR | O_NONBLOCK, 0, error);
	if (fp == NULL)
	{
		return NULL;
	};
	
	Inode *inode = fp->iref.inode;
	vfsUprefInode(inode);
	vfsClose(fp);
	
	if ((inode->mode & VFS_MODE_TYPEMASK) != VFS_MODE_DIRECTORY)
	{
		vfsDownrefInode(inode);
		*error = ENOTDIR;
		return NULL;
	};
	
	return inode;
};

static FSDriver drvBind = {
	.fsname = "bind",
	.openroot = fsbind
};

void initFSDrivers()
{
	firstDriver = NULL;
	semInit(&semFS);
	
	registerFSDriver(&drvBind);
};

void registerFSDriver(FSDriver *drv)
{
	semWait(&semFS);
	if (firstDriver == NULL)
	{
		firstDriver = drv;
		drv->prev = NULL;
		drv->next = NULL;
	}
	else
	{
		FSDriver *last = firstDriver;
		while (last->next != NULL) last = last->next;
		last->next = drv;
		drv->prev = last;
		drv->next = NULL;
	};
	semSignal(&semFS);
};

void unregisterFSDriver(FSDriver *drv)
{
	semWait(&semFS);
	if (firstDriver == drv) firstDriver = drv->next;
	if (drv->prev != NULL) drv->prev->next = drv->next;
	if (drv->next != NULL) drv->next->prev = drv->prev;
	semSignal(&semFS);
};

int sys_mount(const char *ufsname, const char *uimage, const char *umountpoint, int flags, const void *uoptions, size_t optlen)
{
	char fsname[USER_STRING_MAX];
	char image[USER_STRING_MAX];
	char mountpoint[USER_STRING_MAX];
	char options[1024];
	
	if (optlen > 1024)
	{
		ERRNO = EOVERFLOW;
		return -1;
	};
	
	if (memcpy_u2k(options, uoptions, optlen) != 0)
	{
		ERRNO = EFAULT;
		return -1;
	};
	
	if (strcpy_u2k(fsname, ufsname) != 0)
	{
		ERRNO = EFAULT;
		return -1;
	};
	
	if (strcpy_u2k(image, uimage) != 0)
	{
		ERRNO = EFAULT;
		return -1;
	};
	
	if (strcpy_u2k(mountpoint, umountpoint) != 0)
	{
		ERRNO = EFAULT;
		return -1;
	};
	
	if (!havePerm(XP_MOUNT))
	{
		ERRNO = EACCES;
		return -1;
	};

	if ((flags & ~MNT_ALL) != 0)
	{
		ERRNO = EINVAL;
		return -1;
	};

	semWait(&semFS);
	FSDriver *drv;
	for (drv=firstDriver; drv!=NULL; drv=drv->next)
	{
		if (strcmp(drv->fsname, fsname) == 0) break;
	};
	
	if (drv == NULL)
	{
		semSignal(&semFS);
		ERRNO = EINVAL;
		return -1;
	};
	
	int error;
	Inode *inode = drv->openroot(image, flags, options, optlen, &error);
	semSignal(&semFS);
	
	if (inode == NULL)
	{
		ERRNO = error;
		return -1;
	};
	
	DentryRef dref = vfsGetDentry(VFS_NULL_IREF, mountpoint, !!(flags & MNT_TEMP), &error);
	if (dref.dent == NULL)
	{
		vfsDownrefInode(inode);
		ERRNO = error;
		return -1;
	};
	
	int status = vfsMount(dref, inode, flags);
	vfsDownrefInode(inode);
	if (status != 0)
	{
		if (dref.dent->ino == 0) vfsRemoveDentry(dref);
		else vfsUnrefDentry(dref);
		
		ERRNO = status;
		return -1;
	};

	return 0;
};

int sys_fsdrv(char *buffer, int num)
{
	int outNum = 0;
	if (num < 0)
	{
		ERRNO = EINVAL;
		return -1;
	};
	
	semWait(&semFS);
	FSDriver *drv;
	for (drv=firstDriver; drv!=NULL; drv=drv->next)
	{
		if (num != 0)
		{
			char entry[16];
			memset(entry, 16, 0);
			strcpy(entry, drv->fsname);
			
			if (memcpy_k2u(buffer, entry, 16) != 0)
			{
				semSignal(&semFS);
				ERRNO = EFAULT;
				return -1;
			};
			
			buffer += 16;
		};
		
		outNum++;
	};
	semSignal(&semFS);
	
	return outNum;
};
