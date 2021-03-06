
/*
 *  - Background menu tab
 */


#include <stdio.h>
#include "igui.hpp"
#include "../ithelib.h"
#include "../console.hpp"
#include "../core.hpp"
#include "../gamedata.hpp"
#include "menusys.h"

// defines

// variables

extern int focus,TL_Id;
extern int MapXpos_Id,MapYpos_Id;       // Map X/Y Coordinate boxes
extern char mapxstr[],mapystr[];

extern long M_core;     // Total amount of core, from memory.cc
extern int s_proj;      // Flag, are sprites being displayed?
extern int st_proj;      // Flag, are standunder tiles being highlighted?

extern long mapx,mapy;   // Current map position

static int curoff=0;    // Current position on the tile list at the bottom
static int L=0,R=0;     // Current tile for each button, Lclick and Rclick
int L_rooftile=0;
static int cycleflag=0,cycleptr=0;
static int lastx=-666,lasty=-666;

// functions

extern void Toolbar();
extern void DrawMap(int x,int y,char s_proj,char l_proj,char f_proj);
extern void WriteRoof(int x,int y,int tile);
extern int ReadRoof(int x,int y);
extern void (*EdUpdateFunc)(void);
extern int GetClickXY(int mx, int my, int *xout, int *yout);

static void Nothing();
static void clipmap();

void TLBl();
void TLBll();
void TLBr();
void TLBrr();
void TL_bar();
void TLBend();
void TLBstart();

void TL_up();
void TL_down();
void TL_left();
void TL_right();

static void GetL();                     // Get a tile for the left click
static void GetR();                     // Get a tile for the right click
static void SetR();                     // Write a right-click tile to map
static void SetL();                     // Write a left-click tile to map

static void Get_Prefab();               // Copy to clipboard
static void Put_Prefab();               // Copy to map
static void ClearL();
static void ClearR();

static void Set_Roof(int tile);   // Write a tile to the map, helper
static int Get_Roof();   // Get a tile from the map, helper

static void GetMapX();                  // Get map position
static void GetMapY();
static unsigned short prefab[8][8];     //
//static unsigned short blkundo[8][8];    // Undo block op

// GoFocal - The menu tab function.  This is called from the toolbar

void TL_GoFocal()
{
int temp;               // Used to get button handles

if(focus==3)            // Make sure we don't redraw if the user clicks again
	return;

focus=3;                // This is now the focus
Toolbar();              // Build up the menu system again from scratch
IG_SetFocus(TL_Id);     // And make the button stick inwards

// Set up Map window

IG_BlackPanel(VIEWX-1,VIEWY-1,258,258);
IG_Region(VIEWX,VIEWY,256,256,SetL,NULL,SetR);

// Write sprite drawing status

temp = IG_ToggleButton(486,72,"Sprites OFF",Nothing,NULL,NULL,&s_proj);
IG_SetInText(temp,"Sprites ON ");

temp = IG_ToggleButton(486,104,"Normal View",Nothing,NULL,NULL,&st_proj);
IG_SetInText(temp,"Stand Under");

temp = IG_ToggleButton(96,348,"Cycling off",Nothing,NULL,NULL,&cycleflag);
IG_SetInText(temp,"Cycling ON ");

// Draw the VCR buttons on the Tile selector

IG_TextButton(8,410,__left,TLBl,NULL,NULL);     // Normal speed
IG_TextButton(616,410,__right,TLBr,NULL,NULL);

IG_TextButton(8,440,__left2,TLBll,NULL,TLBstart);   // Fast Forward
IG_TextButton(608,440,__right2,TLBrr,NULL,TLBend);

// Draw the contents of Left and Right buttons

IG_BlackPanel(400,192,32,32);
DrawScreenText(400,182," L");
if(L)
     RTlist[L].image->Draw(swapscreen,400,192);

IG_BlackPanel(448,192,32,32);
DrawScreenText(448,182," R");
if(R)
     RTlist[R].image->Draw(swapscreen,448,192);

TL_bar();                                       // Draw the Tile bar
IG_Region(32,400,576,32,GetL,NULL,GetR);        // Add clickable region

// Set up the X and Y map position counter at the top

MapXpos_Id = IG_InputButton(24,40,mapxstr,GetMapX,NULL,NULL);
MapYpos_Id = IG_InputButton(112,40,mapystr,GetMapY,NULL,NULL);

// Get and Put prefab (clipboard)

IG_TextButton(300,300,"Cut screen",Get_Prefab,NULL,NULL);   // Fast Forward
IG_TextButton(400,300,"Paste screen",Put_Prefab,NULL,NULL);   // Fast Forward

IG_TextButton(300,348,"Fill L tile",ClearL,NULL,NULL);   // Fast Forward
IG_TextButton(400,348,"Fill R tile",ClearR,NULL,NULL);   // Fast Forward

DrawMap(mapx,mapy,s_proj,1,0);             // Draw the map


IG_TextButton(416,104,__up,TL_up,NULL,NULL);         // Pan up button
IG_TextButton(400,128,__left,TL_left,NULL,NULL);     // Pan left button
IG_TextButton(432,128,__right,TL_right,NULL,NULL);   // Pan right button
IG_TextButton(416,150,__down,TL_down,NULL,NULL);     // Pan down button

IG_AddKey(IREKEY_UP,TL_up);                             // Pan up key binding
IG_AddKey(IREKEY_DOWN,TL_down);                         // Pan down key binding
IG_AddKey(IREKEY_LEFT,TL_left);                         // Pan left key binding
IG_AddKey(IREKEY_RIGHT,TL_right);                       // Pan right key binding

IG_AddKey(IREKEY_C,Get_Prefab);                         // get prefab key binding
IG_AddKey(IREKEY_V,Put_Prefab);                         // put prefab key binding

EdUpdateFunc=NULL;
}

void Nothing()
{
DrawMap(mapx,mapy,s_proj,1,0);     // Do something anyway
return;
}

void TLBr()
{
curoff++;
if(curoff>(RTtot-17))
    curoff=(RTtot-17);
if(curoff<0) curoff=0;
TL_bar();
}

void TLBl()
{
curoff--;
if(curoff<0) curoff=0;
TL_bar();
}

void TLBrr()
{
curoff+=10;
if(curoff>(RTtot-17))
    curoff=(RTtot-17);
if(curoff<0) curoff=0;
TL_bar();
}

void TLBll()
{
curoff-=10;
if(curoff<0) curoff=0;
TL_bar();
}

void TLBend()
{
curoff=(RTtot-17);
if(curoff<0) curoff=0;
TL_bar();
}

void TLBstart()
{
curoff=0;
TL_bar();
}


void TL_bar()
{
int co;
co=17;
if(RTtot<17)
    co=RTtot;
IG_BlackPanel(31,399,578,34);
for(int ctr=0;ctr<co;ctr++)
   if(ctr+curoff < 1)
        fbox2(33,401,32,32,0,swapscreen);
   else
   		{
		RTlist[ctr+curoff].image->Draw(swapscreen,33+(34*ctr),401);
		if(!(RTlist[ctr+curoff].flags & KILLROOF))
			{
			//textout_ex((BITMAP *)IRE_GetBitmap(swapscreen),font,"S",33+(34*ctr),401,ITG_BLUE->packed,-1);
			DefaultFont->Draw(swapscreen,33+(34*ctr),401,ITG_BLUE,NULL,"S");
			}
		}
}

void GetL()
{
int co;
co=17;
if(RTtot<17)
	co=RTtot;
for(int ctr=0;ctr<co;ctr++)
	if(x>33+(34*ctr) && x<65+(34*ctr))
		{
		L=ctr+curoff;
		L_rooftile=L; // For big-map painting
		IG_BlackPanel(400,192,32,32);
		if(L)
			RTlist[L].image->Draw(swapscreen,400,192);

		// If we're tile-cycling, reset and ensure sanity
		cycleptr=L;
		lastx=-666; // force a lastx,y reset
			
		return;
		}
}

void GetR()
{
int co;
co=17;
if(RTtot<17)
	co=RTtot;
for(int ctr=0;ctr<co;ctr++)
	if(x>33+(34*ctr) && x<65+(34*ctr))
		{
		R=ctr+curoff;
		IG_BlackPanel(448,192,32,32);
		if(R)
			RTlist[R].image->Draw(swapscreen,448,192);

		// If we're tile-cycling, ensure sanity
		if(cycleflag)
			if(cycleptr>R)
				cycleptr=R;
		return;
		}
}

void SetL()
{
int l;

// If shift is pressed, grab tile instead of drawing
if(IRE_TestShift(IRESHIFT_SHIFT))
	{
	l=Get_Roof();
	if(l>=RTtot)
		l=RTtot-1;
	if(l != -1)
		L=l;
	L_rooftile=L; // For big-map painting
	cycleptr=L;
	IG_BlackPanel(400,192,32,32);
	if(L)
		RTlist[L].image->Draw(swapscreen,400,192);
	return;
	}


Set_Roof(L);
}

void SetR()
{
int r;

// If shift is pressed, grab tile instead of drawing
if(IRE_TestShift(IRESHIFT_SHIFT))
	{
	r=Get_Roof();
	if(r>=RTtot)
		r=RTtot-1;
	if(r != -1)
		R=r;
	IG_BlackPanel(448,192,32,32);
	if(R)
		RTlist[R].image->Draw(swapscreen,448,192);
	return;
	}


Set_Roof(R);
}

void Set_Roof(int tile)
{
int t,xx,yy,yyy,xxx;

yyy=-1;
xxx=-1;
for(xx=0;xx<VSW;xx++)
	{
	t=32*xx+(VIEWX-1);
	if(x>=t &&x<(t+32))
		xxx=xx;
	}

for(yy=0;yy<VSH;yy++)
	{
	t=32*yy+(VIEWY-1);
	if(y>=t && y<(t+32))
		yyy=yy;
	}

if(xxx!=-1 && yyy!=-1)
	{
	// Convert to map coordinates
	xxx+=mapx;
	yyy+=mapy;

	// If we are tile-cycling, only write if tile isn't same as last
	if(cycleflag)
		if(xxx == lastx && yyy == lasty)
			return;

	// Handle any tile-cycling
	if(cycleflag)
		{
		if(L>R)
			return;
		tile=cycleptr++;
		if(cycleptr>R)
			cycleptr=L;
		}

	WriteRoof(xxx,yyy,tile);
	lastx=xxx;
	lasty=yyy;
	}
DrawMap(mapx,mapy,s_proj,1,0);
}


int Get_Roof()
{
int xpos,ypos;
if(GetClickXY(x,y,&xpos,&ypos)) {
	return ReadRoof(xpos,ypos);
}
return -1;
}




void TL_up()
{
if(IRE_TestShift(IRESHIFT_SHIFT))
	mapy-=8;
else
	mapy--;
clipmap();
DrawMap(mapx,mapy,s_proj,1,0);
}

void TL_down()
{
if(IRE_TestShift(IRESHIFT_SHIFT))
	mapy+=8;
else
	mapy++;
clipmap();
DrawMap(mapx,mapy,s_proj,1,0);
}


void TL_left()
{
if(IRE_TestShift(IRESHIFT_SHIFT))
	mapx-=8;
else
	mapx--;
clipmap();
DrawMap(mapx,mapy,s_proj,1,0);
}

void TL_right()
{
if(IRE_TestShift(IRESHIFT_SHIFT))
	mapx+=8;
else
	mapx++;
clipmap();
DrawMap(mapx,mapy,s_proj,1,0);
}

void ClearL()
{
for(int cy=0;cy<VSH;cy++)
        for(int cx=0;cx<VSW;cx++)
                WriteRoof(mapx+cx,mapy+cy,L);
DrawMap(mapx,mapy,s_proj,1,0);
}

void ClearR()
{
for(int cy=0;cy<VSH;cy++)
        for(int cx=0;cx<VSW;cx++)
                WriteRoof(mapx+cx,mapy+cy,R);
DrawMap(mapx,mapy,s_proj,1,0);
}

void Get_Prefab()
{
for(int cy=0;cy<VSH;cy++)
        for(int cx=0;cx<VSW;cx++)
                prefab[cx][cy]=ReadRoof(mapx+cx,mapy+cy);
//DrawMap(mapx,mapy,s_proj,l_proj,0);
}

void Put_Prefab()
{
for(int cy=0;cy<VSH;cy++)
        for(int cx=0;cx<VSW;cx++)
                WriteRoof(mapx+cx,mapy+cy,prefab[cx][cy]);
DrawMap(mapx,mapy,s_proj,1,0);
}

/*
 *   clipmap - clip the map's coordinates to sensible values
 */

void clipmap()
{
if(mapx<0) mapx=0;
if(mapx>curmap->w-VSW) mapx=curmap->w-VSW;
if(mapy>curmap->h-VSH) mapy=curmap->h-VSH;
if(mapy<0) mapy=0;
}


/*
 *    GetMapX - Ask the user where on the map they want to be
 */

void GetMapX()
{
mapx=InputIntegerValue(-1,-1,0,curmap->w-VSW,mapx);
DrawMap(mapx,mapy,s_proj,1,0);
}

/*
 *    GetMapY - Ask the user where on the map they want to be
 */

void GetMapY()
{
mapy=InputIntegerValue(-1,-1,0,curmap->h-VSH,mapy);
DrawMap(mapx,mapy,s_proj,1,0);
}
