/* this file is part of lector, a cafe document viewer
 *
 *  Copyright (C) 2004 Red Hat, Inc
 *
 * Lector is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Lector is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include "ev-view-cursor.h"

CdkCursor *
ev_view_cursor_new (CdkDisplay  *display,
		    EvViewCursor new_cursor)
{
	CdkCursor *cursor = NULL;

	switch (new_cursor) {
	case EV_VIEW_CURSOR_NORMAL:
		break;
	case EV_VIEW_CURSOR_IBEAM:
		cursor = cdk_cursor_new_for_display (display, CDK_XTERM);
		break;
	case EV_VIEW_CURSOR_LINK:
		cursor = cdk_cursor_new_for_display (display, CDK_HAND2);
		break;
	case EV_VIEW_CURSOR_WAIT:
		cursor = cdk_cursor_new_for_display (display, CDK_WATCH);
		break;
	case EV_VIEW_CURSOR_HIDDEN:
		cursor = cdk_cursor_new_for_display (display, CDK_BLANK_CURSOR);
		break;
	case EV_VIEW_CURSOR_DRAG:
		cursor = cdk_cursor_new_for_display (display, CDK_FLEUR);
		break;
	case EV_VIEW_CURSOR_AUTOSCROLL:
		cursor = cdk_cursor_new_for_display (display, CDK_DOUBLE_ARROW);
		break;
	case EV_VIEW_CURSOR_ADD:
		cursor = cdk_cursor_new_for_display (display, CDK_PLUS);
		break;
	}

	return cursor;
}
