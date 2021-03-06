/*
	Doom Editor Utility, by Brendon Wyber and Rapha�l Quinet.

        Adapted from the Borland C original by JP Morris

	You are allowed to use any parts of this code in another program, as
	long as you give credits to the authors in the documentation and in
	the program itself.  Read the file README.1ST for more information.

	This program comes with absolutely no warranty.

	GFX.C - Graphics routines.
*/

int vp=0;

/* the includes */
#include "../ithelib.h"
#include "../graphics/iregraph.hpp"
#include "../console.hpp"
#include "../loadfile.hpp"
#include "menusys.h"
#include "../oscli.hpp"
#include "../widgets.hpp"
#include <math.h>
#ifndef _WIN32
#include <unistd.h>
#endif

#define Preserve();	_copybitmapdata->Get(swapscreen,0,0);
#define Restore();  _copybitmapdata->DrawSolid(swapscreen,0,0);
#define INVALID -2147483647
#define MAXDIV10 214748364

int GfxReadkey();
int GetFileList(char *prompt, char *filename);

static char *InputNameFromList_getter(int index, int *list_size);
static char **UserList;
static int UserListLen;

extern IREBITMAP *swapscreen;
extern IREFONT *DefaultFont;
static IREBITMAP *_copybitmapdata;
extern int CMP(const void *a,const void *b);

/* the global variables */
int OrigX;		/* the X origin */
int OrigY;		/* the Y origin */
int Scale;		/* the scale value */
int PointerX;		/* X position of pointer */
int PointerY;		/* Y position of pointer */
int ScrMaxX;		/* maximum X screen coord */
int ScrMaxY;		/* maximum Y screen coord */
int ScrCenterX;		/* X coord of screen center */
int ScrCenterY;		/* Y coord of screen center */
IRECOLOUR CURCOL;
IRECOLOUR mainface;

// Convert to Allegro format, with the ascii code as bits 0-7 and the scancode in bits 8 onwards (SDL has >8bit scancodes)

int GfxReadkey()
{
int s,a;
s=IRE_NextKey(&a);
IRE_ClearKeyBuf();
return ((s&0xfff) << 8) + (a&0xff);
}


void setfillstyle(int a,IRECOLOUR *b)
{
CURCOL.Set(b->packed);
}

void F_error(char *msg)
{
KillGFX();
puts("SYSTEM FATAL ERROR:\n");
puts(msg);
exit(1);
}

/*
	initialise the graphics display
*/

void InitGfx()
{
irecon_mouse(1);
ScrMaxX = 640;
ScrMaxY = 480;

_copybitmapdata=MakeIREBITMAP(ScrMaxX,ScrMaxY);
if(!_copybitmapdata)
	F_error("Out of memory getting copybitmap");
ScrCenterX = ScrMaxX / 2;
ScrCenterY = ScrMaxY / 2;

mainface.Set(160,160,160);
CURCOL.Set(mainface.packed);
}



/*
	terminate the graphics display
*/

void TermGfx()
{
delete _copybitmapdata;
irecon_mouse(0);
}


/*
   clear the screen
*/

void ClearScreen()
{
swapscreen->Clear(&CURCOL);
}


/*
	set the current drawing color
*/

void SetColor(IRECOLOUR *color)
{
CURCOL.Set(color->packed);
}

/*
void setcolor( int color)
{
CURCOL=color;
}
*/

IRECOLOUR *getcolor()
{
return &CURCOL;
}



/*
	draw a line on the screen from screen coords
*/

void DrawScreenLine( int Xstart, int Ystart, int Xend, int Yend)
{
	swapscreen->Line(Xstart, Ystart, Xend, Yend,&CURCOL);
}



/*
	draw a filled in box on the screen from screen coords
*/

void DrawScreenBox( int Xstart, int Ystart, int Xend, int Yend)
{
if(Xstart > Xend)
	return;
if(Ystart > Yend)
	return;
	
swapscreen->FillRect(Xstart, Ystart, Xend-Xstart, Yend-Ystart,&CURCOL);
}


/*
	draw a filled-in 3D-box on the screen from screen coords
*/

void DrawScreenBox3D( int Xstart, int Ystart, int Xend, int Yend)
{
IRECOLOUR c(0,0,0);
c.Set(mainface.packed);

swapscreen->FillRect(Xstart+1, Ystart+1, ((Xend-1)-Xstart), ((Yend-1)-Ystart),&c);

	SetColor(ITG_DARKGRAY);
	DrawScreenLine( Xstart, Yend, Xend, Yend);
	DrawScreenLine( Xend, Ystart, Xend, Yend);
	if (Xend - Xstart > 20 && Yend - Ystart > 20)
	{
		DrawScreenLine( Xstart + 1, Yend - 1, Xend - 1, Yend - 1);
		DrawScreenLine( Xend - 1, Ystart + 1, Xend - 1, Yend - 1);
		SetColor(ITG_WHITE);
		DrawScreenLine( Xstart + 1, Ystart + 1, Xstart + 1, Yend - 1);
		DrawScreenLine( Xstart + 1, Ystart + 1, Xend - 1, Ystart + 1);
	}
	SetColor(ITG_WHITE);
	DrawScreenLine( Xstart, Ystart, Xend, Ystart);
	DrawScreenLine( Xstart, Ystart, Xstart, Yend);
	SetColor(ITG_BLACK);
}

/*
	draw a filled-in 3D-box on the screen from screen coords
*/

void DrawSunkBox3D( int Xstart, int Ystart, int Xend, int Yend)
{
IRECOLOUR c(0,0,0);
c.Set(mainface.packed);

swapscreen->FillRect(Xstart+1, Ystart+1, ((Xend-1)-Xstart), ((Yend-1)-Ystart),&c);

SetColor(ITG_WHITE);
DrawScreenLine( Xstart, Yend, Xend, Yend);
DrawScreenLine( Xend, Ystart, Xend, Yend);
if (Xend - Xstart > 20 && Yend - Ystart > 20)	{
	DrawScreenLine( Xstart + 1, Yend - 1, Xend - 1, Yend - 1);
	DrawScreenLine( Xend - 1, Ystart + 1, Xend - 1, Yend - 1);
	SetColor(ITG_DARKGRAY);
	DrawScreenLine( Xstart + 1, Ystart + 1, Xstart + 1, Yend - 1);
	DrawScreenLine( Xstart + 1, Ystart + 1, Xend - 1, Ystart + 1);
	}
SetColor(ITG_DARKGRAY);
DrawScreenLine( Xstart, Ystart, Xend, Ystart);
DrawScreenLine( Xstart, Ystart, Xstart, Yend);
SetColor(ITG_BLACK);
}


/*
	draw a hollow 3D-box on the screen from screen coords
*/

void DrawScreenBoxHollow( int Xstart, int Ystart, int Xend, int Yend)
{
setfillstyle( 1,ITG_BLACK);
DrawScreenBox( Xstart + 1, Ystart + 1, Xend - 1, Yend - 1);
SetColor(ITG_WHITE);
DrawScreenLine( Xstart, Yend, Xend, Yend);
DrawScreenLine( Xend, Ystart, Xend, Yend);
if (Xend - Xstart > 20 && Yend - Ystart > 20)	{
	DrawScreenLine( Xstart + 1, Yend - 1, Xend - 1, Yend - 1);
	DrawScreenLine( Xend - 1, Ystart + 1, Xend - 1, Yend - 1);
	SetColor(ITG_DARKGRAY);
	DrawScreenLine( Xstart + 1, Ystart + 1, Xstart + 1, Yend - 1);
	DrawScreenLine( Xstart + 1, Ystart + 1, Xend - 1, Ystart + 1);
	}
SetColor(ITG_DARKGRAY);
DrawScreenLine( Xstart, Ystart, Xend, Ystart);
DrawScreenLine( Xstart, Ystart, Xstart, Yend);
SetColor(ITG_WHITE);
}


/*
	write text to the screen
*/

void XDrawScreenChar( int Xstart, int Ystart, char msg)
{
static int lastX;
static int lastY;
char temp[]={0,0};

if (Xstart < 0)
	Xstart = lastX;
if (Ystart < 0)
	Ystart = lastY;

temp[0]=msg;
//textprintf_ex((BITMAP *)IRE_GetBitmap(swapscreen),font,Xstart,Ystart,CURCOL.packed,-1,"%s",temp);
DefaultFont->Draw(swapscreen,Xstart,Ystart,&CURCOL,NULL,temp);

lastX = Xstart;
lastY = Ystart + 10;  /* or textheight("W") ? */
}

/*
	write text to the screen
*/

void DrawScreenText( int Xstart, int Ystart, char *msg, ...)
{
static int lastX;
static int lastY;
char temp[ 2048];
va_list args;

va_start( args, msg);
vsprintf( temp, msg, args);
va_end( args);
if (Xstart < 0)
	Xstart = lastX;
if (Ystart < 0)
	Ystart = lastY;
DefaultFont->Draw(swapscreen,Xstart,Ystart,&CURCOL,NULL,temp);

lastX = Xstart;
lastY = Ystart + 10;  /* or textheight("W") ? */
}


/*
*/


static char *InputNameFromList_getter(int index, int *list_size)
{
if(index < 0)	{
	if(list_size)
		*list_size = UserListLen;
	return NULL;
	}

return UserList[index];
}


void InputNameFromList( int x0, int y0, char *prompt, int listsize, char **list, char *name)
{
InputNameFromListWithFunc( x0, y0, prompt, listsize, list, 15, name, 31, 256, NULL);
}


// Picklist is 32 characters wide

void InputNameFromList32( int x0, int y0, char *prompt, int listsize, char **list, char *name)
{
InputNameFromListWithFunc( x0, y0, prompt, listsize, list, 15, name, 31, 256, NULL);
}


void InputNameFromListWithAdd( int x0, int y0, char *prompt, int listsize, char **list, char *name)
{
int ctr;
char **newlist;
char addnew[16];
newlist = (char **)M_get(listsize+1,sizeof(char *));

for(ctr=0;ctr<listsize;ctr++)
	newlist[ctr+1]=list[ctr];
strcpy(addnew,"(Add New)");
newlist[0]=addnew;

InputNameFromListWithFunc( x0, y0, prompt, listsize+1, newlist, 5, name, 0, 0, NULL);
M_free(newlist);

if(!istricmp(name,"(Add New)"))	{
	InputIString(-1,-1,prompt,31,name);
	return;
	}
}

/*
   ask for a name in a given list and call a function (for displaying objects, etc.)
*/

void InputNameFromListWithFunc( int x0, int y0, char *prompt, int listsize, char **list, int listdisp, char *name, int width, int height, void (*hookfunc)(int x, int y, char *name))
{
int  key, n, l;
int  x1, y1, x2, y2;
int  maxlen;
int ok, firstkey;
IRECOLOUR bodycolour;


int textwidth=24l;
//if(width>250)
      textwidth=300;
   
bodycolour.Set(mainface.packed);


maxlen=0;

if(!name)
	return; // Uh

if(listsize<=0)
	{
	Notify(-1,-1,"No items in list!","Ooops, my bad");
	name[ 0] = '\0'; /* return an empty string */
	return;
	}

for (n = 0; n < listsize; n++)
	if(!list[n])
		{
		Notify(-1,-1,"Empty item in list!","Internal error");
		name[ 0] = '\0'; /* return an empty string */
		return;
		}
	  
	
do IRE_ClearKeyBuf(); while (IRE_KeyPressed());

//	listsize++;	// Why were we doing this?!
maxlen--;

/* compute maxlen */
maxlen = 1;
for (n = 0; n < listsize; n++)
	if (strlen( list[ n]) > maxlen)
		maxlen = strlen( list[ n]);
for (n = strlen(name) + 1; n <= maxlen; n++)
	name[ n] = '\0';
/* compute the minimum width of the dialog box */
l = maxlen;
if (strlen( prompt) > l + 13)
	l = strlen( prompt) - 13;
l = l * 8 + 158;
x1 = l + 3;
y1 = 10 + 1;
if (width > 0)
	l += 10 + width;
if (height > 65)
	n = height + 20;
else
	n = 112;
if (x0 < 0)
	x0 = (ScrMaxX - l) / 2;
if (y0 < 0)
	y0 = (ScrMaxY - n) / 2;
x1 += x0;
y1 += y0;
if (x1 + width - 1 < ScrMaxX)
	x2 = x1 + width - 1;
else
	x2 = ScrMaxX;
if (y1 + height - 1 < ScrMaxY)
	y2 = y1 + height - 1;
else
	y2 = ScrMaxY;

Preserve();

DrawScreenBox3D( x0, y0, x0 + l, y0 + n);
DrawScreenBoxHollow( x0 + 10, y0 + 28, x0 + textwidth, y0 + 41);
DrawScreenText( x0 + 10, y0 + 8, prompt);
if (width > 0)
	{
	DrawScreenBox( x1, y1, x2 + 1, y2 + 1);
	SetColor(ITG_LIGHTGRAY);
	DrawScreenBox( x1 - 1, y1 - 1, x2, y2);
	}
firstkey = true;
for (;;)	{
	/* test if "name" is in the list */
	for (n = 0; n < listsize; n++)
		if (strcmp( name, list[ n]) <= 0)
			break;
	ok = n < listsize ? !strcmp( name, list[ n]) : false;
	if (n > listsize - 1)
		n = listsize - 1;
	/* display the "listdisp" next items in the list */
	SetColor(&bodycolour);
	DrawScreenBox( x0 + 10, y0 + 48, x0 + 10 + 8 * maxlen, y0 + 48 + 10 * listdisp);
	SetColor(ITG_BLACK);
	for (l = 0; l < listdisp && n + l < listsize; l++)
		DrawScreenText( x0 + 10, y0 + 48 + l * 10, list[ n + l]);
	l = strlen( name);
	DrawScreenBox( x0 + 11, y0 + 29, x0 + textwidth-1, y0 + 40);
	if (ok)
		SetColor(ITG_WHITE);
	else
		SetColor(ITG_LIGHTGRAY);
	DrawScreenText( x0 + 13, y0 + 31, name);
	if (hookfunc)
	    hookfunc( x1, y1, name);
	Show();
	key = GfxReadkey();
	if (firstkey && (key & 0x00FF) >= ' ')	{
		for (l = 0; l <= maxlen; l++)
			name[ l] = '\0';
		l = 0;
		}
	firstkey = false;
	if (l < maxlen && (key & 0x00FF) >= 'a' && (key & 0x00FF) <= 'z')	{
		name[ l] = (key & 0x00FF) + ('A' - 'a');
		name[ l + 1] = '\0';
		}
	else if (l < maxlen && (key & 0x00FF) >= ' ')	{
		name[ l] = key & 0x00FF;
		name[ l + 1] = '\0';
		}
	else if (l > 0 && (key >> 8) == IREKEY_BACKSPACE)
		name[ l - 1] = '\0';
	else if (n < listsize - 1 && (key >> 8) == IREKEY_DOWN)
		strcpy(name, list[ n + 1]);
	else if (n > 0 && (key >> 8) == IREKEY_UP)
		strcpy(name, list[ n - 1]);
	else if (n < listsize - listdisp && (key >> 8) == IREKEY_PGDN)
		strcpy(name, list[ n + listdisp]);
	else if (n > 0 && (key >> 8) == IREKEY_PGUP)	{
		if (n > listdisp)
			strcpy(name, list[ n - listdisp]);
		else
			strcpy(name, list[ 0]);
		}
	else if ((key >> 8) == IREKEY_END)
		strcpy(name, list[ listsize - 1]);
	else if ((key >> 8) == IREKEY_HOME)
		strcpy(name, list[ 0]);
	else if ((key >> 8) == IREKEY_TAB)
		strcpy(name, list[ n]);
	else if (ok && (key & 0x00FF) == 0x000D)
		break; /* return "name" */
	else if ((key >> 8) == IREKEY_ESC)	{
		name[ 0] = '\0'; /* return an empty string */
		break;
		}
	}
Restore();
Show();

IRE_ClearKeyBuf();
}



// InputFileName - Checking disabled, in case Long Filenames are desired

void InputFileName( int x0, int y0, char *prompt, int maxlen, char *filename)
{
char newfile[1024];
int ret;

strcpy(newfile,projectname);
strcat(newfile,"/");
strcat(newfile,filename);

IRE_ShowMouse();

do	{
	ret=GetFileList(prompt,newfile);
	if(ret == 0)	{
		printf("GFS Aborted\n");
		filename[0]='\0';
		break;
		}
	if(ret > 0)	{
//		printf("old filename = %s\n",newfile);
		clean_path(newfile);
//		printf("new filename = %s\n",filename);
		strncpy(filename,newfile,maxlen);
		filename[maxlen]='\0';
		}
	} while(ret <0); // Change dir, go around again

swapscreen->ShowMouse();
}


int GetFileList(char *prompt, char *filename)
{
char **filelist;
int listlen;
char *p;
char pickfile[1024];

filelist = igetdir(filename,&listlen,IGETDIR_SHOWDIRS|IGETDIR_DOTS);
if(!filelist)	{
	p=strrchr(filename,'/');
	if(p)
		*p=0;
	filelist = igetdir(filename,&listlen,IGETDIR_SHOWDIRS|IGETDIR_DOTS);
	if(!filelist)	{
		Notify(-1,-1,"Directory scan failed","");
		return 0;
		}
	}

qsort(filelist,listlen,sizeof(char*),CMP);

//extern char **igetdir(const char *dir, int *items, int showdirs);
//extern void ifreedir(char **list, int items);
Preserve();
strcpy(pickfile,filelist[0]);
InputNameFromList(-1,-1,prompt,listlen,filelist,pickfile);
Restore();

ifreedir(filelist,listlen);

if(!pickfile[0])
	return 0;	// Quit
if(pickfile[0] == '[')	{
	// Deal with directory change
	if(!stricmp(pickfile,"[.]"))	{
		// Just do a rescan
		return -1;
		}


	if(!stricmp(pickfile,"[..]"))	{
		// Knock the last directory off the path
		printf("doing .. on '%s'\n",filename);
		p=strrchr(filename,'/');
		if(p)
			*p=0;
		printf("now '%s'\n",filename);
		return -1; // Go around again
		}
	p=strchr(pickfile,']');
	if(p)
		*p=0;
	p=&pickfile[1];
	strcat(filename,"/");
	strcat(filename,p);
	return -1; // Go around again
	}

strcat(filename,"/");
strcat(filename,pickfile);
return 1; // Got a filename
}


// InputString - Adapted from InputFilename, but with checking removed

void InputString( int x0, int y0, char *prompt, int maxlen, char *filename)
{
	int   key, l, boxlen,l2;
   int  ok=1, firstkey;
   char evil[1024];
   maxlen--;

   do IRE_ClearKeyBuf(); while (IRE_KeyPressed());

Preserve();

	strcpy(evil,filename);
   SetColor(ITG_WHITE);
   for (l = strlen(filename) + 1; l <= maxlen; l++)
      filename[ l] = '\0';
   /* compute the width of the input box */
	if (maxlen > 20)
      boxlen = 20;
	else
      boxlen = maxlen;
	/* compute the width of the dialog box */
	if (strlen( prompt) > boxlen)
      l = strlen( prompt);
	else
      l = boxlen;
   if (x0 < 0)
      x0 = (ScrMaxX - 26 - 8 * l) / 2;
   if (y0 < 0)
      y0 = (ScrMaxY - 50) / 2;
   /* draw the dialog box */
   l2=l;
   DrawScreenBox3D( x0, y0, x0 + 26 + 8 * l, y0 + 50);
   DrawScreenBoxHollow( x0 + 10, y0 + 28, x0 + 15 + 8 * boxlen, y0 + 41);
   DrawScreenText( x0 + 10, y0 + 8, prompt);
   firstkey = 1;
	for (;;)
	{
		l = strlen( filename);
		SetColor(ITG_BLACK);
		DrawScreenBox( x0 + 11, y0 + 29, x0 + 14 + 8 * boxlen, y0 + 40);
		if (ok)
	 SetColor(ITG_WHITE);
		else
		{
		SetColor(&mainface);
		}
		if (l > boxlen)
		{
	 DrawScreenText( x0 + 11, y0 + 31, "<");
	 DrawScreenText( x0 + 13, y0 + 31, "<%s", filename + (l - boxlen + 1));
		}
		else
	 DrawScreenText( x0 + 13, y0 + 31, filename);
         Show();
		key = GfxReadkey();
		if (firstkey && (key & 0x00FF) >= ' ')
		{
	 for (l = 0; l <= maxlen; l++)
		 filename[ l] = '\0';
	 l = 0;
		}
		firstkey = 0;
		if (l < maxlen && (key & 0x00FF) >= 'a' && (key & 0x00FF) <= 'z')
		{
	 filename[ l] = (key & 0x00FF) + ('A' - 'a');
	 filename[ l + 1] = '\0';
		}
		else if (l < maxlen && (key & 0x00FF) >= ' ')
      {
	 filename[ l] = key & 0x00FF;
         if(filename[ l] == ' ')     // No spaces allowed
             filename[ l]='_';
	 filename[ l + 1] = '\0';
      }
      else if (l > 0 && (key & 0x00FF) == 0x0008)
	 filename[ l - 1] = '\0';
		else if (ok && (key & 0x00FF) == 0x000D)
	 break; /* return "filename" */
      else if ((key & 0x00FF) == 0x001B)
      {
	 strcpy(filename,evil);
	 break;
      }
   }
Restore();
Show();
IRE_ClearKeyBuf();
}


void InputIString( int x0, int y0, char *prompt, int maxlen, char *filename)
{
	int   key, l, boxlen,l2;
   int  ok=1, firstkey;
   char evil[2048];
   maxlen--;

   do IRE_ClearKeyBuf(); while (IRE_KeyPressed());
Preserve();

	strcpy(evil,filename);
   SetColor(ITG_WHITE);
   for (l = strlen(filename) + 1; l <= maxlen; l++)
      filename[ l] = '\0';
   /* compute the width of the input box */
	if (maxlen > 70)
      boxlen = 70;
	else
      boxlen = maxlen;
	/* compute the width of the dialog box */
	if (strlen( prompt) > boxlen)
      l = strlen( prompt);
	else
      l = boxlen;
   if (x0 < 0)
      x0 = (ScrMaxX - 26 - 8 * l) / 2;
   if (y0 < 0)
      y0 = (ScrMaxY - 50) / 2;
   /* draw the dialog box */
   l2=l;

   DrawScreenBox3D( x0, y0, x0 + 26 + 8 * l, y0 + 50);
   DrawScreenBoxHollow( x0 + 10, y0 + 28, x0 + 15 + 8 * boxlen, y0 + 41);
   DrawScreenText( x0 + 10, y0 + 8, prompt);
   firstkey = 1;
	for (;;)
	{

		l = strlen( filename);
		SetColor(ITG_BLACK);
		DrawScreenBox( x0 + 11, y0 + 29, x0 + 14 + 8 * boxlen, y0 + 40);
		if (ok)
	 SetColor(ITG_WHITE);
		else
		{
		SetColor(&mainface);
		}
		if (l > boxlen)
		{
	 DrawScreenText( x0 + 11, y0 + 31, "<");
	 DrawScreenText( x0 + 13, y0 + 31, "<%s", filename + (l - boxlen + 1));
		}
		else
	 DrawScreenText( x0 + 13, y0 + 31, filename);
         Show();
		key = GfxReadkey();
		if (firstkey && (key & 0x00FF) >= ' ')
		{
	 for (l = 0; l <= maxlen; l++)
		 filename[ l] = '\0';
	 l = 0;
		}
		firstkey = 0;

      if (l < maxlen && (key & 0x00FF) >= ' ')
      {
	 filename[ l] = key & 0x00FF;
	 filename[ l + 1] = '\0';
      }
      else if (l > 0 && (key & 0x00FF) == 0x0008)
	 filename[ l - 1] = '\0';
		else if (ok && (key & 0x00FF) == 0x000D)
	 break; // return "filename"

      else
      if ((key & 0x00FF) == 0x001B)
      {
	 strcpy(filename,evil);
	 break;
      }
   }
Restore();
Show();
IRE_ClearKeyBuf();
}


/*
 *  ask for confirmation (prompt2 may be NULL)
 */

int Confirm( int x0, int y0, char *prompt1, char *prompt2)
{
	int key;
	int maxlen = 46;

   do IRE_ClearKeyBuf(); while (IRE_KeyPressed());

	if (strlen( prompt1) > maxlen)
		maxlen = strlen( prompt1);
	if (prompt2 != NULL && strlen( prompt2) > maxlen)
		maxlen = strlen( prompt2);
	if (x0 < 0)
		x0 = (ScrMaxX - 22 - 8 * maxlen) / 2;
	if (y0 < 0)
		y0 = (ScrMaxY - (prompt2 ? 53 : 43)) / 2;

	Preserve();

	DrawScreenBox3D( x0, y0, x0 + 22 + 8 * maxlen, y0 + (prompt2 ? 53 : 43));
	SetColor(ITG_WHITE);
	DrawScreenText( x0 + 10, y0 + 8, prompt1);
	if (prompt2 != NULL)
		DrawScreenText( x0 + 10, y0 + 18, prompt2);
	SetColor( ITG_WHITE);
	DrawScreenText( x0 + 10, y0 + (prompt2 ? 38 : 28), "Press Y to confirm, or any other key to cancel");
	Show();
	key = GfxReadkey();
	Restore();
	Show();

	IRE_ClearKeyBuf();
	
	return ((key & 0x00FF) == 'Y' || (key & 0x00FF) == 'y');
}


/*
 *  display a notification and wait for a key (prompt2 may be NULL)
 */

int Notify( int x0, int y0, char *prompt1, char *prompt2)
{
int maxlen = 30;

   do IRE_ClearKeyBuf(); while (IRE_KeyPressed());

	if (strlen( prompt1) > maxlen)
		maxlen = strlen( prompt1);
	if (prompt2 != NULL && strlen( prompt2) > maxlen)
		maxlen = strlen( prompt2);
	if (x0 < 0)
		x0 = (ScrMaxX - 22 - 8 * maxlen) / 2;
	if (y0 < 0)
		y0 = (ScrMaxY - (prompt2 ? 53 : 43)) / 2;

	Preserve();
	DrawScreenBox3D( x0, y0, x0 + 22 + 8 * maxlen, y0 + (prompt2 ? 53 : 43));
	SetColor(ITG_WHITE);
	DrawScreenText( x0 + 10, y0 + 8, prompt1);
	if (prompt2 != NULL)
		DrawScreenText( x0 + 10, y0 + 18, prompt2);
	SetColor(ITG_YELLOW);
	DrawScreenText( x0 + 10, y0 + (prompt2 ? 38 : 28), "Press any key to continue...");
	Show();
	if((GfxReadkey()&0x00ff) ==0x001b)
		{
		IRE_ClearKeyBuf();
		Restore();
		return 0;
		}
	IRE_ClearKeyBuf();
	Restore();
return 1;
}


/*
	display the integer input box
*/

int InputInteger( int x0, int y0, int *valp, int minv, int maxv)
{
   int  key, val;
   int neg, ok, firstkey;

   do IRE_ClearKeyBuf(); while (IRE_KeyPressed());

   DrawScreenBoxHollow( x0, y0, x0 + 121, y0 + 13);
	neg = (*valp < 0);
	val = neg ? -(*valp) : *valp;
	firstkey = 1;
	for (;;)
	{
		ok = (neg ? -val : val) >= minv && (neg ? -val : val) <= maxv;
		SetColor(ITG_BLACK);
		DrawScreenBox( x0 + 1, y0 + 1, x0 + 120, y0 + 12);
		if (ok)
	 SetColor(ITG_WHITE);
		else
		      {
		      SetColor(&mainface);
		      }
		if (neg)
	 DrawScreenText( x0 + 3, y0 + 3, "-%d", val);
		else
	 DrawScreenText( x0 + 3, y0 + 3, "%d", val);
		Show();
		key = GfxReadkey();
		if (firstkey && (key & 0x00FF) > ' ')
		{
	 val = 0;
	 neg = 0;
		}
		firstkey = 0;
//		if (val < 3275 && (key & 0x00FF) >= '0' && (key & 0x00FF) <= '9')

		if (val < MAXDIV10 && (key & 0x00FF) >= '0' && (key & 0x00FF) <= '9')
	 val = val * 10 + (key & 0x00FF) - '0';
		else if (val > 0 && (key & 0x00FF) == 0x0008)
	 val = val / 10;
		else if (neg && (key & 0x00FF) == 0x0008)
	 neg = 0;
		else if ((key & 0x00FF) == '-')
	 neg = !neg;
		else if (ok && (key & 0x00FF) == 0x000D)
	 break; /* return "val" */
		else if ((key & 0xFF00) == 0x4800 || (key & 0xFF00) == 0x5000 ||
			 (key & 0xFF00) == 0x4B00 || (key & 0xFF00) == 0x4D00 ||
			 (key & 0x00FF) == 0x0009 || (key & 0xFF00) == 0x0F00)
	 break; /* return "val", even if not valid */
		else if ((key & 0x00FF) == 0x001B)
		{
	 val = INVALID; /* return a value out of range */
	 neg = 0;	// JPM: Do not negate this!
	 break;
		}
	}
	if (neg)
		*valp = -val;
	else
		*valp = val;

IRE_ClearKeyBuf();
return key;
}



/*
	ask for an integer value and check for minimum and maximum
*/

int InputIntegerValue( int x0, int y0, int minv, int maxv, int defv)
{
	int  val, key;
   char prompt[ 80];

   do IRE_ClearKeyBuf(); while (IRE_KeyPressed());

   sprintf( prompt, "Enter a number (%d-%d):", minv, maxv);
	if (x0 < 0)
      x0 = (ScrMaxX - 25 - 8 * strlen( prompt)) / 2;
   if (y0 < 0)
      y0 = (ScrMaxY - 55) / 2;

	Preserve();
	DrawScreenBox3D( x0, y0, x0 + 25 + 8 * strlen( prompt), y0 + 55);
	SetColor(ITG_WHITE);
	DrawScreenText( x0 + 10, y0 + 8, prompt);
	val = defv;
	while (((key = InputInteger( x0 + 10, y0 + 28, &val, minv, maxv)) & 0x00FF) != 0x000D && (key & 0x00FF) != 0x001B) Show();
	if(val==INVALID)
		val=defv;
	Restore();
	IRE_ClearKeyBuf();
	return val;
}


int InputIntegerValueP( int x0, int y0, int minv, int maxv, int defv, char *promptstr)
{
	int  val, key;
   char prompt[ 80];

   do IRE_ClearKeyBuf(); while (IRE_KeyPressed());

   sprintf( prompt, "%s (%d-%d):", promptstr, minv, maxv);
	if (x0 < 0)
      x0 = (ScrMaxX - 25 - 8 * strlen( prompt)) / 2;
   if (y0 < 0)
      y0 = (ScrMaxY - 55) / 2;

	Preserve();
	DrawScreenBox3D( x0, y0, x0 + 25 + 8 * strlen( prompt), y0 + 55);
	SetColor(ITG_WHITE);
	DrawScreenText( x0 + 10, y0 + 8, prompt);
	val = defv;
	while (((key = InputInteger( x0 + 10, y0 + 28, &val, minv, maxv)) & 0x00FF) != 0x000D && (key & 0x00FF) != 0x001B) Show();
	if(val==INVALID)
		val=-1;
	Restore();
	IRE_ClearKeyBuf();
	return val;
}


int InputIntegerValuePI( int x0, int y0, int minv, int maxv, int defv, char *promptstr)
{
	int  val, key;
   char prompt[ 80];

   do IRE_ClearKeyBuf(); while (IRE_KeyPressed());

   sprintf( prompt, "%s (%d-%d):", promptstr, minv, maxv);
	if (x0 < 0)
      x0 = (ScrMaxX - 25 - 8 * strlen( prompt)) / 2;
   if (y0 < 0)
      y0 = (ScrMaxY - 55) / 2;

	Preserve();
	DrawScreenBox3D( x0, y0, x0 + 25 + 8 * strlen( prompt), y0 + 55);
	SetColor(ITG_WHITE);
	DrawScreenText( x0 + 10, y0 + 8, prompt);
	val = defv;
	while (((key = InputInteger( x0 + 10, y0 + 28, &val, minv, maxv)) & 0x00FF) != 0x000D && (key & 0x00FF) != 0x001B) Show();
	Restore();
	IRE_ClearKeyBuf();
	return val;
}





void Box(int a,int b,int c,int d)
{
DrawScreenLine(a,b,c,b);
DrawScreenLine(a,d,c,d);
DrawScreenLine(a,b,a,d);
DrawScreenLine(c,b,c,d);
}


void ClipBox(int x,int y,int w,int h,IRECOLOUR *col,IREBITMAP *screen)
{
    int max,may,mix,miy;        // X,Y pairs, major, minor

    mix = x;
    if(mix < 0)
        mix = 0;
    if(mix > ScrMaxX)
        mix = ScrMaxX;

    max = x + w;
    if(max < 0)
        max = 0;
    if(max > ScrMaxX)
        max = ScrMaxX;

    miy = y;
    if(miy < 0)
        miy = 0;
    if(miy > ScrMaxY)
        miy = ScrMaxY;

    may = y + h;
    if(may < 0)
        may = 0;
    if(may > ScrMaxY)
        may = ScrMaxY;

    screen->Line(mix,miy,max,miy,col);
    screen->Line(mix,may,max,may,col);
    screen->Line(max,miy,max,may,col);
    screen->Line(mix,miy,mix,may,col);
//    rect(screen,mix,miy,max,may,col);
}

//
//	Bresenham circle algorithm, based on the optimised code from Wikipedia
//

void ClipCircle(int cx,int cy,int r,IRECOLOUR *col,IREBITMAP *screen)
{
int error = -r;
int x = r;
int y = 0;
while(x >= y)
	{
	screen->PutPixel(cx+x,cy+y,col);
	screen->PutPixel(cx-x,cy+y,col);
	screen->PutPixel(cx+x,cy-y,col);
	screen->PutPixel(cx-x,cy-y,col);
	screen->PutPixel(cx+y,cy+x,col);
	screen->PutPixel(cx-y,cy+x,col);
	screen->PutPixel(cx+y,cy-x,col);
	screen->PutPixel(cx-y,cy-x,col);
	  
	error += (y*2)+1;
	y++;
	if(error >= 0)
		{
		x--;
		error -= x*2;
		}
	}
}


/*
   draw a meter bar on the screen from screen coords (in a hollow box); max. value = 1.0
*/

void DrawScreenMeter( int Xstart, int Ystart, int Xend, int Yend, float value)
{
static int lx=0;

if (value < 0.0)
	value = 0.0;
if (value > 1.0)
	value = 1.0;

swapscreen->FillRect(Xstart + 1 + (int) ((Xend - Xstart - 2) * value), Ystart + 1, (Xend-Xstart) - 1, (Yend-Ystart) - 1,ITG_BLACK);
swapscreen->FillRect(Xstart + 1, Ystart + 1, 1 + (int) ((Xend - Xstart - 2) * value), (Yend-Ystart) - 1,ITG_LIGHTGREEN);
//rectfill(swapscreen, Xstart + 1 + (int) ((Xend - Xstart - 2) * value), Ystart + 1, Xend - 1, Yend - 1,ITG_BLACK);
//rectfill(swapscreen, Xstart + 1, Ystart + 1, Xstart + 1 + (int) ((Xend - Xstart - 2) * value), Yend - 1,ITG_LIGHTGREEN);

//Only draw on update

if((int) ((Xend - Xstart - 2) * value) != lx)
	{
	lx=(int) ((Xend - Xstart - 2) * value);
	Show();
	}
}




/* end of file */
