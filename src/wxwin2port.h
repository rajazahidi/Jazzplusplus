/*

the purpose of this file is to ease porting to wxwin2.
it includes the standard wxwin2 headers, and defines some compatibility
types.

it replaces the old wx.h include

/jave

 */

#ifndef wxwin2port_h
#define wxwin2port_h

//this define makes wxwin2 define compatibilty types for wxwin2, but seems flawed so dont use it
//#define  WXWIN_COMPATIBILITY 1

#include "wx/wx.h" 

# define Bool int
# define True  1
# define False 0

#ifndef TRUE
# define TRUE  1
# define FALSE 0
#endif

//im not really sure, but i think this needs to be defined to compile on unix (see trackwin.cpp for use)
#define wx_x 1

/*
these defines make it so wx_help.h and wx_form.h  arent included anymore
these are aparently no longer used in wxwin 2
*/

#define wxb_helph 1
#define wxb_formh 1
#endif
