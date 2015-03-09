/*
	Glidix kernel

	Copyright (c) 2014-2015, Madd Games.
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

#include <glidix/module.h>
#include <glidix/console.h>
#include <glidix/idt.h>
#include <glidix/common.h>
#include <glidix/port.h>
#include <glidix/sched.h>
#include <glidix/ktty.h>

static IRQHandler oldHandler;

// US keyboard layout
static unsigned char keymap[128] =
{
    0,  0, '1', '2', '3', '4', '5', '6', '7', '8',	/* 9 */
  '9', '0', '-', '=', '\b',	/* Backspace */
  '\t',			/* Tab */
  'q', 'w', 'e', 'r',	/* 19 */
  't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',	/* Enter key */
    0,			/* 29   - Control */
  'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';',	/* 39 */
 '\'', '`',   0x80,		/* Left shift */
 '\\', 'z', 'x', 'c', 'v', 'b', 'n',			/* 49 */
  'm', ',', '.', '/',   0x80,				/* Right shift */
  '*',
    0,	/* Alt */
  ' ',	/* Space bar */
    0,	/* Caps lock */
    0,	/* 59 - F1 key ... > */
    0,   0,   0,   0,   0,   0,   0,   0,
    0,	/* < ... F10 */
    0,	/* 69 - Num lock*/
    0,	/* Scroll Lock */
    0,	/* Home key */
    0,	/* Up Arrow */
    0,	/* Page Up */
  '-',
    0,	/* Left Arrow */
    0,
    0,	/* Right Arrow */
  '+',
    0,	/* 79 - End key*/
    0,	/* Down Arrow */
    0,	/* Page Down */
    0,	/* Insert Key */
    0,	/* Delete Key */
    0,   0,   0,
    0,	/* F11 Key */
    0,	/* F12 Key */
    0,	/* All other keys are undefined */
};

// when shift is pressed
static unsigned char keymapShift[128] =
{
    0,  0, '!', '@', '#', '$', '%', '^', '&', '*',	/* 9 */
  '(', ')', '_', '+', '\b',	/* Backspace */
  '\t',			/* Tab */
  'Q', 'W', 'E', 'R',	/* 19 */
  'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',	/* Enter key */
    0,			/* 29   - Control */
  'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':',	/* 39 */
 '\"', '`',   0x80,		/* Left shift */
 '\\', 'Z', 'X', 'C', 'V', 'B', 'N',			/* 49 */
  'M', '<', '>', '?',   0x80,				/* Right shift */
  '*',
    0,	/* Alt */
  ' ',	/* Space bar */
    0,	/* Caps lock */
    0,	/* 59 - F1 key ... > */
    0,   0,   0,   0,   0,   0,   0,   0,
    0,	/* < ... F10 */
    0,	/* 69 - Num lock*/
    0,	/* Scroll Lock */
    0,	/* Home key */
    0,	/* Up Arrow */
    0,	/* Page Up */
  '-',
    0,	/* Left Arrow */
    0,
    0,	/* Right Arrow */
  '+',
    0,	/* 79 - End key*/
    0,	/* Down Arrow */
    0,	/* Page Down */
    0,	/* Insert Key */
    0,	/* Delete Key */
    0,   0,   0,
    0,	/* F11 Key */
    0,	/* F12 Key */
    0,	/* All other keys are undefined */
};

static volatile ATOMIC(uint8_t)	kbdbuf[64];
static volatile ATOMIC(int)	kbdput = 0;
static volatile ATOMIC(int)	kbdread = 0;

static void onKeyboardIRQ(int irq)
{
	uint8_t code = inb(0x60);
	int nextPut = kbdput + 1;
	if (nextPut == 64) nextPut = 0;
	kbdbuf[kbdput] = code;
	kbdput = nextPut;
};

static void kbdThread(void *data)
{
	int ctrl = 0;
	int shift = 0;
	while (1)
	{
		while (kbdput == kbdread)
		{
			//kprintf_debug("WAIT kbdput=%d, kbdread=%d\n", kbdput, kbdread);
		};		// wait for keyboard event
		uint8_t code = kbdbuf[kbdread];
		//kprintf_debug("keybaord code: %a\n", code);
		kbdread++;
		if (kbdread == 64) kbdread = 0;

		if (code == 0x1D)
		{
			ctrl = 1;
		}
		else if (code == 0x9D)
		{
			ctrl = 0;
		}
		else if ((code & 0x80) == 0)
		{
			unsigned char key = keymap[code];
			if (shift) key = keymapShift[code];
			if (key == 0x80)
			{
				shift = 1;
				continue;
			};
			if (key == 0) continue;

			if (ctrl)
			{
				termPutChar('^');
				termPutChar(keymapShift[code]);

				if (key == 'c')
				{
					signalPid(1, SIGINT);
				};
			}
			else
			{
				termPutChar(key);
			};
		}
		else if (code & 0x80)
		{
			unsigned char key = keymap[code & 0x7F];
			if (key == 0x80) shift = 0;
		};
	};

	//kprintf_debug("here or something?\n");
};

MODULE_INIT()
{
	kprintf("Initializing the PS/2 keyboard\n");
	oldHandler = registerIRQHandler(1, onKeyboardIRQ);

	CreateKernelThread(kbdThread, NULL, NULL);
};

MODULE_FINI()
{
	kprintf("Shutting down PS/2 keyboard\n");
	registerIRQHandler(1, oldHandler);
};