/*
	Glidix GUI

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

#include <libgwm.h>
#include <poll.h>

void* timerThread(void *context)
{
	GWMTimer *timer = (GWMTimer*) context;
	
	int cycles = 0;
	while (timer->running)
	{
		poll(NULL, 0, timer->period);
		gwmPostUpdateEx(timer->win, timer->evtype, cycles++);
	};
	
	free(timer);
	return NULL;
};

GWMTimer* gwmCreateTimer(GWMWindow *win, int period)
{
	return gwmCreateTimerEx(win, period, GWM_EVENT_UPDATE);
};

GWMTimer* gwmCreateTimerEx(GWMWindow *win, int period, int evtype)
{
	GWMTimer *timer = (GWMTimer*) malloc(sizeof(GWMTimer));
	if (timer == NULL) return NULL;
	
	timer->period = period;
	timer->running = 1;
	timer->win = win;
	timer->evtype = evtype;
	
	if (pthread_create(&timer->thread, NULL, timerThread, timer) != 0)
	{
		free(timer);
		return NULL;
	};
	
	pthread_detach(timer->thread);
	return timer;
};

void gwmDestroyTimer(GWMTimer *timer)
{
	timer->running = 0;
};
