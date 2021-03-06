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
#include <time.h>
#include <stdio.h>

int main()
{
	if (gwmInit() != 0)
	{
		fprintf(stderr, "Failed to initialize GWM!\n");
		return 1;
	};
	
	// TODO: a nice dialog which asks for parameters such as what to screenshot,
	// and how long to wait etc
	sleep(10);
	
	GWMGlobWinRef focwin;
	gwmGetDesktopWindows(&focwin, NULL);

	DDISurface *shot = gwmScreenshotWindow(&focwin);
	if (shot == NULL)
	{
		gwmMessageBox(NULL, "Screnshot", "Failed to take the screenshot.", GWM_MBUT_OK | GWM_MBICON_ERROR);
		gwmQuit();
		return 1;
	};
	
	GWMWindow *fc = gwmCreateFileChooser(NULL, "Save screenshot as...", GWM_FILE_SAVE);
	// TODO: filters
	char *path = gwmRunFileChooser(fc);
	if (path == NULL)
	{
		ddiDeleteSurface(shot);
		gwmQuit();
		return 1;
	};
	
	const char *error;
	if (ddiSavePNG(shot, path, &error) != 0)
	{
		ddiDeleteSurface(shot);
		char msg[1024];
		sprintf(msg, "Failed to save screenshot: %s", error);
		gwmMessageBox(NULL, "Screenshot", msg, GWM_MBUT_OK | GWM_MBICON_ERROR);
		gwmQuit();
		return 1;
	};
	
	gwmMessageBox(NULL, "Screenshot", "Screenshot saved.", GWM_MBUT_OK | GWM_MBICON_SUCCESS);
	return 0;
};
