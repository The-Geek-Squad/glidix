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
#include <libddi.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/**
 * BOX LAYOUT
 * Lays out children next to each other, either horizontall or vertically. Children with proportion
 * "0" get their minimum size on the main axis, and minimum to preferred (or larger with FILL) on
 * the other axis. Children with other proprtions take up the amount of remaining space that is
 * proportional to their proportion.
 */

typedef struct BoxChild_ BoxChild;
struct BoxChild_
{
	BoxChild *next;
	int proportion;
	int border;
	int flags;
	
	/**
	 * The nested layout.
	 */
	GWMLayout *layout;
	
	/**
	 * "Length"; cached for a single execution of gwmBoxRun().
	 */
	int len;
	
	/**
	 * "Other length"; minimum length on other axis. Cached for gwmBoxRun().
	 */
	int otherLen;
};

typedef struct
{
	int flags;
	BoxChild *firstChild;
	BoxChild *lastChild;
} BoxLayoutData;

static void gwmBoxMinSize(GWMLayout *box, int *outWidth, int *outHeight)
{
	BoxLayoutData *data = (BoxLayoutData*) box->data;
	
	int dims[2];
	int mainAxis = data->flags & GWM_BOX_VERTICAL;
	int otherAxis = mainAxis ^ 1;
	dims[0] = dims[1] = 0;
	
	int otherLeftoverMax = 0;
	int units = 0;
	
	BoxChild *child;
	for (child=data->firstChild; child!=NULL; child=child->next)
	{
		int width, height;
		child->layout->getMinSize(child->layout, &width, &height);
		
		int subdims[2];
		subdims[0] = width;
		subdims[1] = height;

		if (child->flags & GWM_BOX_LEFT)
		{
			subdims[0] += child->border;
		};
		
		if (child->flags & GWM_BOX_RIGHT)
		{
			subdims[0] += child->border;
		};
		
		if (child->flags & GWM_BOX_UP)
		{
			subdims[1] += child->border;
		};
		
		if (child->flags & GWM_BOX_DOWN)
		{
			subdims[1] += child->border;
		};

		if (child->proportion != 0)
		{
			units += child->proportion;
			if (dims[otherAxis] < subdims[otherAxis]) dims[otherAxis] = subdims[otherAxis];
			if (otherLeftoverMax < subdims[mainAxis]) otherLeftoverMax = subdims[mainAxis];
			continue;
		};

		dims[mainAxis] += subdims[mainAxis];
		if (dims[otherAxis] < subdims[otherAxis]) dims[otherAxis] = subdims[otherAxis];
	};
	
	dims[mainAxis] += otherLeftoverMax * units;
	*outWidth = dims[0];
	*outHeight = dims[1];
};

static void gwmBoxPrefSize(GWMLayout *box, int *outWidth, int *outHeight)
{
	BoxLayoutData *data = (BoxLayoutData*) box->data;
	
	int dims[2];
	int mainAxis = data->flags & GWM_BOX_VERTICAL;
	int otherAxis = mainAxis ^ 1;
	dims[0] = dims[1] = 0;
	
	int otherLeftoverMax = 0;
	int units = 0;
	
	BoxChild *child;
	for (child=data->firstChild; child!=NULL; child=child->next)
	{
		int width, height;
		child->layout->getPrefSize(child->layout, &width, &height);
		
		int subdims[2];
		subdims[0] = width;
		subdims[1] = height;

		if (child->flags & GWM_BOX_LEFT)
		{
			subdims[0] += child->border;
		};
		
		if (child->flags & GWM_BOX_RIGHT)
		{
			subdims[0] += child->border;
		};
		
		if (child->flags & GWM_BOX_UP)
		{
			subdims[1] += child->border;
		};
		
		if (child->flags & GWM_BOX_DOWN)
		{
			subdims[1] += child->border;
		};

		if (child->proportion != 0)
		{
			units += child->proportion;
			if (dims[otherAxis] < subdims[otherAxis]) dims[otherAxis] = subdims[otherAxis];
			if (otherLeftoverMax < subdims[mainAxis]) otherLeftoverMax = subdims[mainAxis];
			continue;
		};

		dims[mainAxis] += subdims[mainAxis];
		if (dims[otherAxis] < subdims[otherAxis]) dims[otherAxis] = subdims[otherAxis];
	};
	
	dims[mainAxis] += otherLeftoverMax * units;
	*outWidth = dims[0];
	*outHeight = dims[1];
};

static void gwmBoxRun(GWMLayout *layout, int x, int y, int width, int height)
{
	// go over all children, computing the size of 0-proportion ones, in anticipation
	// of dividing up the leftover space. also compute how many units of leftover space
	// we need.
	int totalFixed = 0;
	int units = 0;
	
	BoxLayoutData *data = (BoxLayoutData*) layout->data;
	BoxChild *child;
	for (child=data->firstChild; child!=NULL; child=child->next)
	{
		int width, height;
		child->layout->getMinSize(child->layout, &width, &height);
		
		if (data->flags & GWM_BOX_VERTICAL)
		{
			child->len = height;
			if (child->flags & GWM_BOX_UP) child->len += child->border;
			if (child->flags & GWM_BOX_DOWN) child->len += child->border;
			child->otherLen = width;
			if (child->flags & GWM_BOX_LEFT) child->otherLen += child->border;
			if (child->flags & GWM_BOX_RIGHT) child->otherLen += child->border;
		}
		else
		{
			child->len = width;
			if (child->flags & GWM_BOX_LEFT) child->len += child->border;
			if (child->flags & GWM_BOX_RIGHT) child->len += child->border;
			child->otherLen = height;
			if (child->flags & GWM_BOX_UP) child->otherLen += child->border;
			if (child->flags & GWM_BOX_DOWN) child->otherLen += child->border;
		};
		
		if (child->proportion == 0)
		{
			totalFixed += child->len;
		}
		else
		{
			units += child->proportion;
		};
	};
	
	// figure out the remaining length
	int leftover;
	if (data->flags & GWM_BOX_VERTICAL)
	{
		leftover = height - totalFixed;
	}
	else
	{
		leftover = width - totalFixed;
	};
	
	// figure out the unit length
	int unitLen;
	if (units != 0)
	{
		unitLen = leftover / units;
	}
	else
	{
		unitLen = 0;
	};
	
	// now lay it all out
	int coords[2];
	coords[0] = x;
	coords[1] = y;
	int dims[2];
	dims[0] = width;
	dims[1] = height;
	
	int mainAxis = data->flags & GWM_BOX_VERTICAL;
	int otherAxis = mainAxis ^ 1;

	for (child=data->firstChild; child!=NULL; child=child->next)
	{
		int len;
		if (child->proportion == 0)
		{
			len = child->len;
		}
		else
		{
			len = child->proportion * unitLen;
		};
		
		int otherLen;
		if (child->flags & GWM_BOX_FILL)
		{
			otherLen = dims[otherAxis];
		}
		else
		{
			otherLen = child->otherLen;
		};
		
		int mainLen = len;
		
		// now decide the coordinates and size of the child in its allocated box
		int mainOffset;
		if (data->flags & GWM_BOX_VERTICAL)
		{
			mainOffset = coords[1];
			if (child->flags & GWM_BOX_UP)
			{
				mainOffset += child->border;
				mainLen -= child->border;
			};
			
			if (child->flags & GWM_BOX_DOWN)
			{
				mainLen -= child->border;
			};
		}
		else
		{
			mainOffset = coords[0];
			if (child->flags & GWM_BOX_LEFT)
			{
				mainOffset += child->border;
				mainLen -= child->border;
			};
			
			if (child->flags & GWM_BOX_RIGHT)
			{
				mainLen -= child->border;
			};
		};
		
		int otherOffset = (dims[otherAxis] - otherLen) / 2 + coords[otherAxis];
		if (data->flags & GWM_BOX_VERTICAL)
		{
			if (child->flags & GWM_BOX_LEFT)
			{
				otherOffset += child->border;
				otherLen -= child->border;
			};
			
			if (child->flags & GWM_BOX_RIGHT)
			{
				otherLen -= child->border;
			};
		}
		else
		{
			if (child->flags & GWM_BOX_UP)
			{
				otherOffset += child->border;
				otherLen -= child->border;
			};
			
			if (child->flags & GWM_BOX_DOWN)
			{
				otherLen -= child->border;
			};
		};

		int resultCoords[2];
		resultCoords[mainAxis] = mainOffset;
		resultCoords[otherAxis] = otherOffset;
		
		int resultSize[2];
		resultSize[mainAxis] = mainLen;
		resultSize[otherAxis] = otherLen;
		
		child->layout->run(child->layout, resultCoords[0], resultCoords[1], resultSize[0], resultSize[1]);
		
		// move on
		resultSize[mainAxis] = len;
		coords[mainAxis] += resultSize[mainAxis];
	};
};

GWMLayout* gwmCreateBoxLayout(int flags)
{
	GWMLayout *layout = gwmCreateAbstractLayout();
	BoxLayoutData *data = (BoxLayoutData*) malloc(sizeof(BoxLayoutData));
	data->flags = flags;
	data->firstChild = NULL;
	data->lastChild = NULL;
	
	layout->data = data;
	layout->getMinSize = gwmBoxMinSize;
	layout->getPrefSize = gwmBoxPrefSize;
	layout->run = gwmBoxRun;
	
	return layout;
};

void gwmDestroyBoxLayout(GWMLayout *layout)
{
	BoxLayoutData *data = (BoxLayoutData*) layout->data;
	
	while (data->firstChild != NULL)
	{
		BoxChild *child = data->firstChild;
		data->firstChild = child->next;
		free(child);
	};
	
	free(data);
	gwmDestroyAbstractLayout(layout);
};

void gwmBoxLayoutAddWindow(GWMLayout *box, GWMWindow *win, int proportion, int border, int flags)
{
	gwmBoxLayoutAddLayout(box, gwmCreateLeafLayout(win), proportion, border, flags);
};

void gwmBoxLayoutAddLayout(GWMLayout *box, GWMLayout *sub, int proportion, int border, int flags)
{
	BoxChild *child = (BoxChild*) malloc(sizeof(BoxChild));
	memset(child, 0, sizeof(BoxChild));
	child->proportion = proportion;
	child->border = border;
	child->flags = flags;
	child->layout = sub;
	
	BoxLayoutData *data = (BoxLayoutData*) box->data;
	if (data->lastChild == NULL)
	{
		data->firstChild = data->lastChild = child;
	}
	else
	{
		data->lastChild->next = child;
		data->lastChild = child;
	};
};

void gwmBoxLayoutAddSpacer(GWMLayout *box, int proportion, int border, int flags)
{
	gwmBoxLayoutAddWindow(box, NULL, proportion, border, flags);
};
