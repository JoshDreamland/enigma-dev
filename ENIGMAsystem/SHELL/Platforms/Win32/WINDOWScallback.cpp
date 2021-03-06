/** Copyright (C) 2008 Josh Ventura
*** Copyright (C) 2013 Robert B. Colton
***
*** This file is a part of the ENIGMA Development Environment.
***
*** ENIGMA is free software: you can redistribute it and/or modify it under the
*** terms of the GNU General Public License as published by the Free Software
*** Foundation, version 3 of the license or any later version.
***
*** This application and its source code is distributed AS-IS, WITHOUT ANY
*** WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
*** FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
*** details.
***
*** You should have received a copy of the GNU General Public License along
*** with this code. If not, see <http://www.gnu.org/licenses/>
**/

#include <map>
#include <string>
using std::string;
using std::map;
#include <windows.h>

#include "../General/PFwindow.h"
#include "WINDOWScallback.h"
#include "Universal_System/CallbackArrays.h" // For those damn vk_ constants.

#include "Universal_System/instance_system.h"
#include "Universal_System/instance.h"

#ifndef WM_MOUSEHWHEEL
  #define WM_MOUSEHWHEEL 0x020E
#endif

extern short mouse_hscrolls;
extern short mouse_vscrolls;
namespace enigma_user {
extern int keyboard_lastkey;
extern string keyboard_lastchar;
extern string keyboard_string;
}

namespace enigma
{
	using enigma_user::keyboard_lastkey;
    using enigma_user::keyboard_lastchar;
	using enigma_user::keyboard_string;
    extern char mousestatus[3],last_mousestatus[3],keybdstatus[256],last_keybdstatus[256];
	map<int,int> keybdmap;
    extern int windowX, windowY, windowWidth, windowHeight;
    extern double  scaledWidth, scaledHeight;
    extern char* currentCursor;
    extern HWND hWnd,hWndParent;
    extern void setchildsize(bool adapt);
	extern void WindowResized();
	extern unsigned int pausedSteps;
    extern bool gameWindowFocused, treatCloseAsEscape;
    static short hdeltadelta = 0, vdeltadelta = 0;
    int tempLeft = 0, tempTop = 0, tempRight = 0, tempBottom = 0, tempWidth, tempHeight;
    RECT tempWindow;

    LRESULT CALLBACK WndProc (HWND hWnd, UINT message,WPARAM wParam, LPARAM lParam)
    {
      switch (message)
      {
        case WM_CREATE:
            return 0;
        case WM_CLOSE:
			instance_event_iterator = new inst_iter(NULL,NULL,NULL);
			for (enigma::iterator it = enigma::instance_list_first(); it; ++it)
			{
			  it->myevent_closebutton();
			}
			// Game Maker actually checks this first I am making the decision to check if after, since that is how it is expected to work
			// so the user can execute something before the escape is processed, no sense in an override if user is going to call game_end() anyway.
			// - Robert
			if (treatCloseAsEscape) {
				PostQuitMessage (0);
			}
            return 0;

        case WM_DESTROY:
            return 0;

        case WM_SIZE:
			instance_event_iterator = new inst_iter(NULL,NULL,NULL);
			for (enigma::iterator it = enigma::instance_list_first(); it; ++it)
			{
			  it->myevent_drawresize();
			}
			WindowResized();
            return 0;

        case WM_SETFOCUS:
            input_initialize();
            gameWindowFocused = true;
			pausedSteps = 0;
            return 0;

        case WM_KILLFOCUS:
            for (int i = 0; i < 255; i++)
            {
                last_keybdstatus[i] = keybdstatus[i];
                keybdstatus[i] = 0;
            }
            for(int i=0; i < 3; i++)
            {
                last_mousestatus[i] = mousestatus[i];
                mousestatus[i] = 0;
            }
            gameWindowFocused = false;
            return 0;

        case WM_ENTERSIZEMOVE:
            GetWindowRect(hWnd,&tempWindow);
            tempLeft = tempWindow.left;
            tempTop = tempWindow.top;
            tempRight = tempWindow.right;
            tempBottom = tempWindow.bottom;
            return 0;

        case WM_EXITSIZEMOVE:
            GetWindowRect(hWnd,&tempWindow);
            tempWidth = windowWidth + (tempWindow.right - tempWindow.left) - (tempRight - tempLeft);
            tempHeight = windowHeight + (tempWindow.bottom - tempWindow.top) - (tempBottom - tempTop);
            if (tempWidth < scaledWidth)
                tempWidth = scaledWidth;
            if (tempHeight < scaledHeight)
                tempHeight = scaledHeight;

            windowX += tempWindow.left - tempLeft;
            windowY += tempWindow.top - tempTop;
            windowWidth = tempWidth;
            windowHeight = tempHeight;
            setchildsize(false);
            return 0;

        case WM_SETCURSOR:
			// Set the user cursor if the mouse is in the client area of the window, otherwise let Windows handle setting the cursor
			// since it knows how to set the gripper cursor for window resizing. This is exactly how GM handles it.
			if (LOWORD(lParam) == HTCLIENT) {
				SetCursor(LoadCursor(NULL, currentCursor));
			} else {
				DefWindowProc(hWnd, message, wParam, lParam);
			}
            return 0;
        case WM_CHAR:
            keyboard_lastchar = string(1,wParam);
			if (keyboard_lastkey == enigma_user::vk_backspace) {
				keyboard_string = keyboard_string.substr(0, keyboard_string.length() - 1);
			} else {
				keyboard_string += keyboard_lastchar;
			}
            return 0;

        case WM_KEYDOWN: {
			int key = enigma_user::keyboard_get_map(wParam);
			keyboard_lastkey = key;
            last_keybdstatus[key]=keybdstatus[key];
            keybdstatus[key]=1;
            return 0;
		}
        case WM_KEYUP: {
			int key = enigma_user::keyboard_get_map(wParam);
			keyboard_lastkey = key;
            last_keybdstatus[key]=keybdstatus[key];
            keybdstatus[key]=0;
            return 0;
		}
        case WM_SYSKEYDOWN: {
			int key = enigma_user::keyboard_get_map(wParam);
			keyboard_lastkey = key;
            last_keybdstatus[key]=keybdstatus[key];
            keybdstatus[key]=1;
            if (key!=18)
            {
              if ((lParam&(1<<29))>0)
                   last_keybdstatus[18]=keybdstatus[18], keybdstatus[18]=1;
              else last_keybdstatus[18]=keybdstatus[18], keybdstatus[18]=0;
            }
            return 0;
		}
        case WM_SYSKEYUP: {
			int key = enigma_user::keyboard_get_map(wParam);
			keyboard_lastkey = key;
            last_keybdstatus[key]=keybdstatus[key];
            keybdstatus[key]=0;
            if (key!=(unsigned int)18)
            {
              if ((lParam&(1<<29))>0)
                   last_keybdstatus[18]=keybdstatus[18], keybdstatus[18]=0;
              else last_keybdstatus[18]=keybdstatus[18], keybdstatus[18]=1;
            }
            return 0;
		}
        case WM_MOUSEWHEEL:
             vdeltadelta += (int)HIWORD(wParam);
             mouse_vscrolls += vdeltadelta / WHEEL_DELTA;
             vdeltadelta %= WHEEL_DELTA;
             return 0;

        case WM_MOUSEHWHEEL:
             hdeltadelta += (int)HIWORD(wParam);
             mouse_hscrolls += hdeltadelta / WHEEL_DELTA;
             hdeltadelta %= WHEEL_DELTA;
             return 0;

        case WM_LBUTTONUP:   mousestatus[0]=0; return 0;
        case WM_LBUTTONDOWN: mousestatus[0]=1; return 0;
        case WM_RBUTTONUP:   mousestatus[1]=0; return 0;
        case WM_RBUTTONDOWN: mousestatus[1]=1; return 0;
        case WM_MBUTTONUP:   mousestatus[2]=0; return 0;
        case WM_MBUTTONDOWN: mousestatus[2]=1; return 0;

		//case WM_TOUCH:
		//TODO: touchscreen stuff
		//return 0;

		//#ifdef DSHOW_EXT
		//#include <dshow.h>
		//case WM_GRAPHNOTIFY:
			//TODO: Handle DirectShow media events
		//return 0;

        default:
            return DefWindowProc (hWnd, message, wParam, lParam);
      }
    }

    void input_initialize()
    {
      //Clear the input arrays
      for(int i=0;i<3;i++){
        last_mousestatus[i]=0;
        mousestatus[i]=0;
      }
      for(int i=0;i<256;i++){
        last_keybdstatus[i]=0;
        keybdstatus[i]=0;
      }
    }

    void input_push()
    {
      for(int i=0;i<3;i++){
        last_mousestatus[i] = mousestatus[i];
      }
      for(int i=0;i<256;i++){
        last_keybdstatus[i] = keybdstatus[i];
      }
      mouse_hscrolls = mouse_vscrolls = 0;
    }
}
