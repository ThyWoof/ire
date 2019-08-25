//
//      loadsave.cc - Map IO for starting the game, saving and loading
//

// Size of rows to split map into when saving in ASCII format
#define MAPFILEBLOCK 128	// 128 seems to be optimal for RLE


#include <stdio.h>
#include <string.h>

#include "ithelib.h"
#include "media.hpp"
#include "console.hpp"
#include "core.hpp"
#include "gamedata.hpp"
#include "loadsave.hpp"
#include "oscli.hpp"
#include "cookies.h"
#include "object.hpp"
#include "nuspeech.hpp"
#include "loadfile.hpp"
#include "linklist.hpp"
#include "resource.hpp"
#include "init.hpp"
#include "project.hpp"
#include "textfile.h"
#include "pe/pe_api.hpp"

// Defines

//#define DEBUG_SAVE
//#define LOG_RW       // If you want to debug object load/save

// Variables

static char stemp[256];
static char sigtemp[40];       // Signature buffer
static IFILE *ofp;
static unsigned int fastfind_id_num=0;
static OBJECT **fastfind_id_list=NULL;
static USEDATA empty;
static int save_version=1;

// Functions

void load_map();
void load_z1(char *filename);
void save_z1(char *filename);
void wipe_z1();
void load_z2();
static void load_map_r01(char *fname);
static void load_z2_r01(char *fname);
static void load_z3_r01(char *fname);


static void WriteContainers(OBJECT *cont, FILE *fp);
//static void WriteContainerSchedules(OBJECT *cont);
//static char *LoadSaveList_getter(int index, int *list_size);
static int ProcessBadObject(OBJECT **objptr, char *name, int maxlen);
static void DeleteProtoObject(OBJECT *o);

static void MoveToPocketEnd(OBJECT *object, OBJECT *container);
int SumObjects(OBJECT *cont, char *name, int total);
OBJECT *GetFirstObject(OBJECT *cont, char *name);
extern void Init_Areas(OBJECT *objsel);

extern OBJECT *find_id(unsigned int id);
static void fastfind_id_start();
static void fastfind_id_stop();
static OBJECT *fastfind_id(unsigned int id);
static void assign_objects(int mapnum);

static int CMP_sort(const void *a, const void *b);
static int CMP_search(const void *a, const void *b);
static int UsedataChanged(OBJECT *o);

extern void CallVMnum(int pos);
extern char *BestName(OBJECT *o);

extern int isDecor(int x,int y);

extern void read_OBJECT(OBJECT *o,IFILE *f);
extern void write_OBJECT(OBJECT *o,IFILE *f);
extern void read_STATS(STATS *s,IFILE *f);
extern void read_OLDSTATS(STATS *s,IFILE *f);
extern void write_STATS(STATS *s,IFILE *f);
extern void read_FUNCS(FUNCS *F,IFILE *f);
extern void write_FUNCS(FUNCS *F,IFILE *f);
extern void read_WORLD(WORLD *w,IFILE *f);
extern void write_WORLD(WORLD *w,IFILE *f);
void read_USEDATA(USEDATA *u,IFILE *f);
void write_USEDATA(USEDATA *u,IFILE *f);
extern void TWriteObject(FILE *fp,OBJECT *o);

// Code


/*
 *      LoadMap- Load the whole map from disk, for first-time startup
 */

void LoadMap(int mapnum)
{
char *fname;
OBJLIST	*ptr;

ilog_printf("  base\n");
load_map(mapnum);

fname=makemapname(mapnum,0,".mz1");
if(!fname)
	ithe_panic("LoadMap: Cannot open map file",NULL);
SAFE_STRCPY(stemp,fname);
ilog_printf("  objects: ");
load_z1(stemp);

ilog_printf("  roof: ");
load_z2(mapnum);
ilog_printf("Done\n");

ilog_printf("  lights: ");
load_z3(mapnum);
ilog_printf("Done\n");

ilog_printf("\n");
ilog_printf("Run Object Inits\n");

// Ensure the system pocket is ready
syspocket = curmap->object;

if(!in_editor)
	for(ptr=MasterList;ptr;ptr=ptr->next)
		if(!(ptr->ptr->flags&DID_INIT))
			{
			if(ptr->ptr->funcs->icache != -1)
				{
				current_object = ptr->ptr;
				person = ptr->ptr;
				new_x = person->x;
				new_y = person->y;
				CallVMnum(ptr->ptr->funcs->icache);
				}
			ptr->ptr->flags |= DID_INIT;
			}
ilog_printf("\n");
}

//
//      Functions for the tilemap IO
//

/*
 *      load_map - Load the tiles from disk for startup
 */

void load_map(int mapnum)
{
int yt,x,y,b,do_delete=0;
long mem;
char key;
char filename[1024];
char number[16];
char *fname;
IFILE *fp;

curmap->w = map_W;
curmap->h = map_H;

ilog_printf("Opening map: %04d\n",mapnum);
fname=makemapname(mapnum,0,".map");
if(!fname)
	ithe_panic("load_map: Cannot open map",NULL);
SAFE_STRCPY(stemp,fname);

if(!loadfile(fname,filename))
	{
	ilog_printf("Could not find file '%s'\n",stemp);
	ilog_printf("Hit Y to create a new file, or any other key to exit.\n");
        key = WaitForAscii();
	if(key!='y' && key!='Y')
		exit(1);

try_again:
	mem = map_W * map_H * (sizeof(short)+sizeof(char)+sizeof(char)+sizeof(char)+sizeof(OBJECT	*))/1024;
	ilog_printf("\n\n\n");
	ilog_printf("The new map will be %dx%d, and require %ldKB of memory.\n",map_W,map_H,mem);
	ilog_printf("Is this okay? (Y/N)\n");
	key = WaitForAscii();
	if(key!='y' && key!='Y')
		{
		ilog_printf("No.\n");
		ilog_printf("\n");
		do {
            number[0]=0;
			ilog_printf("Enter the new width of the map:\n");
			if(!irecon_getinput(number,15))
				exit(1);
			map_W = atoi(number);
            } while(map_W<1);

		do {
            number[0]=0;
			ilog_printf("Enter the new height of the map:\n");
			if(!irecon_getinput(number,15))
				exit(1);
			map_H = atoi(number);
            } while(map_H<1);

		curmap->w = map_W;
		curmap->h = map_H;
		goto try_again;
		}

	curmap->sig=MAPSIG;
	if(!curmap->objmap)
		{
		curmap->object = (struct OBJECT *)M_get(1,sizeof(OBJECT));
		curmap->object->next=NULL;
		}
	
	if(!curmap->physmap)
		curmap->physmap = (unsigned short *)M_get(curmap->w * curmap->h,sizeof(short));
	if(!curmap->roof)
		curmap->roof = (unsigned char *)M_get(curmap->w * curmap->h,sizeof(char));
	if(!curmap->objmap)
		curmap->objmap = (OBJECT **)M_get(curmap->w * curmap->h, sizeof(OBJECT *));
	if(!curmap->light)
		curmap->light = (unsigned char *)M_get(curmap->w * curmap->h,sizeof(char));
	if(!curmap->lightst)
		curmap->lightst = (unsigned char *)M_get(curmap->w * curmap->h,sizeof(char));

	// Build quick-access table for object layer
	for(yt=0;yt<curmap->h;yt++)
		ytab[yt]=curmap->w * yt;
	}
else
	{
	fp = iopen(filename);
	if(!fp)
		ithe_panic("load_map: cannot open file",stemp);

	// New signatures are an 8-byte header, with a 4-byte cookie
	iread((unsigned char *)sigtemp,8,fp);
	if(memcmp(sigtemp,COOKIE_MAPFILE,4)) // Skip last byte (either CR or LF)
		{
		if(memcmp(sigtemp,OLDCOOKIE_MAP,4)) // Skip last byte (either CR or LF)
			ithe_panic("load_map: file does not contain the right cookie",stemp);
		// It is the old 40-byte cookie, skip it all
		iseek(fp,40,SEEK_SET);
		read_WORLD(curmap,fp);
	
		// JM: 08/01/2006 : Free the maps first, to force reallocation
		// since they may be a different size to the original map and reusing
		// it won't work.
		erase_curmap();
		if(!curmap->physmap)
			curmap->physmap = (unsigned short *)M_get(curmap->w * curmap->h,sizeof(short));
		iread((unsigned char *)curmap->physmap,curmap->w*curmap->h*sizeof(short),fp);
		iclose(fp);
		}
	else
		{
		// New version, check rev
		if(!strncmp(&sigtemp[4],TEXTCOOKIE_R00,3))
			{
			// Binary version
			read_WORLD(curmap,fp);
			erase_curmap();		// See above for why we do this
			if(!curmap->physmap)
				curmap->physmap = (unsigned short *)M_get(curmap->w * curmap->h,sizeof(short));
			iread((unsigned char *)curmap->physmap,curmap->w*curmap->h*sizeof(short),fp);
			iclose(fp);
			}

		// It the ASCII version?
		if(!strncmp(&sigtemp[4],TEXTCOOKIE_R01,3))
			{
			iclose(fp);
			load_map_r01(filename);
			}
		}

	if(!curmap->object)
		curmap->object =	(struct	OBJECT *)M_get(1,sizeof(OBJECT));
	curmap->object->name="Linkedlist entrypoint object";
	curmap->object->next=NULL;
	curmap->object->flags |= IS_ON; // The Syspocket MUST BE ALIVE!

	// Set up matrix of linked lists

	if(!curmap->objmap)
		curmap->objmap =	(OBJECT	**)M_get(curmap->w * curmap->h, sizeof(OBJECT *));

	// Build quick-access table for the matrix

	for(yt=0;yt<curmap->h;yt++)
		ytab[yt]=curmap->w * yt;

	if(!curmap->roof)
		curmap->roof = (unsigned char *)M_get(curmap->w * curmap->h,sizeof(char));
	if(!curmap->light)
		curmap->light= (unsigned char *)M_get(curmap->w * curmap->h,sizeof(char));
	if(!curmap->lightst)
		curmap->lightst= (unsigned char *)M_get(curmap->w * curmap->h,sizeof(char));
	}

// Store current map size for script language
map_W = curmap->w;
map_H = curmap->h;

ilog_quiet("Check map integrity\n");

// Check map state and correct if necessary

for(y=0;y<curmap->h;y++)
	for(x=0;x<curmap->w;x++)
		{
/*
		ilog_quiet("curmap = %x\n",curmap);
		ilog_quiet("curmap->physmap = %x\n",curmap->physmap);
		ilog_quiet("pos = %d\n",MAP_POS(x,y));
		ilog_quiet("curmap->physmap[%d] = %x\n",MAP_POS(x,y),curmap->physmap[MAP_POS(x,y)]);
*/
		
		b=curmap->physmap[MAP_POS(x,y)];
		if(b>=TItot && b != 0xffff)	// 0xffff is random and allowed
			{
			if(in_editor)
				{
				if(!do_delete)
					{
					ilog_printf("Invalid tiles detected in the map.\n");
					ilog_printf("Do you want to:\n");
					ilog_printf("1. Set all unknown tiles to 0\n");
					ilog_printf("2. Quit so you can add the tile to resource.txt\n");
					do
						{
						key=WaitForAscii();
						if(key == '1')
						    do_delete=1;
						if(key == '2')
						    exit(1);
						} while(key != '1');
					}
				}
			else
				{
				curmap->physmap[MAP_POS(x,y)] = 0xffff; // surprise!!
				do_delete = 2;
				}
			if(do_delete == 1)
				curmap->physmap[MAP_POS(x,y)]=0;
			}
		}
if(do_delete ==	2)
	Bug("Invalid map tiles detected\n");
}


void load_map_r01(char *fname)
{
struct TF_S zm;
int ctr,x,y,dmapno,runctr;
unsigned short tile,*ptr;
char dir;
const char *str;
char first[1024];
char *l;
int didinit=0;
int tiles;

if(!curmap)
	return; // Ouch

TF_init(&zm);
TF_load(&zm,fname);

if(strncmp(zm.line[0],COOKIE_MAPFILE,4))
	if(strncmp(&zm.line[0][4],TEXTCOOKIE_R01,3))
		{
		ilog_quiet("[%s]\n",zm.line[0]);
		ilog_quiet("[%s]\n",zm.line[1]);
		ithe_panic("load_map: file does not contain the right cookie",zm.line[0]);
		}

for(ctr=1;ctr<zm.lines;ctr++)
	{
	l=zm.line[ctr];
	SAFE_STRCPY(first,strfirst(l));
	strstrip(first);

	// map <w> <h>
	if(!istricmp(first,"map"))
		{
		if(didinit)
			{
			sprintf(first,"at line %d",ctr+1);
			ithe_panic("load_map: reset map size after Mapdata:",first);
			}
		curmap->w = atoi(strfirst(strrest(l)));
		curmap->h = atoi(strfirst(strrest(strrest(l))));
		continue;
		}

	// connection <DIR> <mapno>
	if(!istricmp(first,"connection"))
		{
		dir = *(strfirst(strrest(l)));
		dmapno = atoi(strfirst(strrest(strrest(l))));
	
		switch(dir)
			{
			case 'n':
			case 'N':
				curmap->con_n=dmapno;
				break;
			case 's':
			case 'S':
				curmap->con_s=dmapno;
				break;
			case 'e':
			case 'E':
				curmap->con_e=dmapno;
				break;
			case 'w':
			case 'W':
				curmap->con_w=dmapno;
				break;
				
			default:
				sprintf(first,"at line %d",ctr+1);
				ithe_panic("load_map: unknown connection direction",first);
				break;
			}
		continue;
		}

	// This initialises the map and allocates the space for the tiles
	if(!istricmp(first,"Mapdata:"))
		{
		erase_curmap();		// See above for why we do this
		if(!curmap->physmap)
			curmap->physmap = (unsigned short *)M_get(curmap->w * curmap->h,sizeof(short));
		didinit=1;
		}

	// row <tilecount> at <xnum> <ynum> = <B64_datablock>
	if(!istricmp(first,"row"))
		{
		if(!didinit || !curmap->physmap)
			{
			sprintf(first,"at line %d",ctr+1);
			ithe_panic("load_map: row tag before Mapdata:",first);
			}
		tiles = atoi(strgetword(l,2));
		x = atoi(strgetword(l,4));
		y = atoi(strgetword(l,5));
	
		if(x+tiles > curmap->w)
			{
			sprintf(first,"at line %d (tried to write %d tiles at %d when width = %d)",ctr+1,tiles,x,curmap->w);
			ithe_panic("load_map: block won't fit on map",first);
			}
	
		// Do it this ugly way because it won't create a copy
		str = strrest(strrest(strrest(strrest(strrest(strrest(l))))));
		if(!str || str == NOTHING)
			{
			sprintf(first,"at line %d",ctr+1);
			ithe_panic("load_map: block missing",first);
			}
		strstrip((char *)str); // The original source was char* so this should be safe
		
		// Convert it directly into the map
		ImportB64(str,&curmap->physmap[(y*curmap->w)+x],tiles);
		continue;
		}

	// repeat <tilecount> at <xnum> <ynum> = <tileno>
	if(!istricmp(first,"repeat"))
		{
		if(!didinit || !curmap->physmap)
			{
			sprintf(first,"at line %d",ctr+1);
			ithe_panic("load_map: row tag before Mapdata:",first);
			}
		tiles = atoi(strgetword(l,2));
		x = atoi(strgetword(l,4));
		y = atoi(strgetword(l,5));
		tile = atoi(strgetword(l,7));
	
		if(x+tiles > curmap->w)
			{
			sprintf(first,"at line %d (tried to write %d tiles at %d when width = %d)",ctr+1,tiles,x,curmap->w);
			ithe_panic("load_map: block won't fit on map",first);
			}
	
		ptr=&curmap->physmap[(y*curmap->w)+x];
		for(runctr=0;runctr<tiles;runctr++)
			*ptr++=tile;
		continue;
		}
	}

// Don't need text file open anymore
TF_term(&zm);
}





/*
 *  save_map - Save the tiles to disk in B64 format
 */

void save_map(int mapnum)
{
char *fname;
ofp=NULL;
int blocksize,rowleft;
int x,y,ret,runctr,do_run;
unsigned short *ptr,tile;
char outbuffer[(MAPFILEBLOCK*3)+1];
FILE *fp;

if(!curmap)
	return;

fname=makemapname(mapnum,0,".map");
SAFE_STRCPY(stemp,fname);
fp=fopen(stemp,"w");
if(!fp)
	return;

fprintf(fp,"%s%s\n",COOKIE_MAPFILE,TEXTCOOKIE_R01);

fprintf(fp,"\tmap %d %d\n",curmap->w,curmap->h);
fprintf(fp,"\tconnection N %d\n",curmap->con_n);
fprintf(fp,"\tconnection S %d\n",curmap->con_s);
fprintf(fp,"\tconnection E %d\n",curmap->con_e);
fprintf(fp,"\tconnection W %d\n",curmap->con_w);

fprintf(fp,"\nMapdata:\n");

for(y=0;y<curmap->h;y++)
	{
	x=0;
	ptr=&curmap->physmap[(y*curmap->w)];
	rowleft = curmap->w;
	blocksize = MAPFILEBLOCK;
	while(rowleft > 0)
		{
		if(rowleft < blocksize)
			blocksize=rowleft;
		  
		// First, are they all the same?
		tile=*ptr;
		do_run=1; // Assume so
		for(runctr=0;runctr<blocksize;runctr++)
			if(ptr[runctr] != tile)
				{
				do_run=0; // No
				break;
				}
				
		if(do_run)
			 fprintf(fp,"\trepeat %d at %d %d = %d\n",blocksize,x,y,tile);
		else
			{
			ret=ExportB64(ptr,blocksize,outbuffer,sizeof(outbuffer));
			if(ret)
				fprintf(fp,"\trow %d at %d %d = %s\n",blocksize,x,y,outbuffer);
			}
		x += blocksize;
		ptr += blocksize;
		rowleft -= blocksize;
		}
	fprintf(fp,"\n");
	}

fclose(fp);
}

/*
void save_map(int mapnum)
{
char *fname;
ofp=NULL;

fname=makemapname(mapnum,0,".map");
SAFE_STRCPY(stemp,fname);
ofp = iopen_write(stemp);

// Since error checking will have been done previously it is safe to explode
// if something has gone wrong.

iwrite((unsigned char *)COOKIE_MAPFILE,4,ofp);
iwrite((unsigned char *)BINCOOKIE_R00,4,ofp);
curmap->sig=MAPSIG; // Just to make sure
write_WORLD(curmap,ofp);
iwrite((unsigned char *)curmap->physmap,curmap->w*curmap->h*sizeof(short),ofp);
iclose(ofp);
}
*/

/*
 *  Free the memory used by a map
 */

void erase_curmap()
{
// JM: 31/10/2006 - I was a total moron and only tried to free them if
// they were ALREADY null.  The devastation THAT caused was horrific.. it's
// amazing the game worked at all.

if(curmap->object)
	{
	FreePockets(curmap->object);
//	M_free(curmap->object);
	curmap->object = NULL;
	}

if(curmap->physmap)
	{
	M_free(curmap->physmap);
	curmap->physmap = NULL;
	}

if(curmap->roof)
	{
	M_free(curmap->roof);
	curmap->roof = NULL;
	}

if(curmap->objmap)
	{
	M_free(curmap->objmap);
	curmap->objmap = NULL;
	}

if(curmap->light)
	{
	M_free(curmap->light);
	curmap->light = NULL;
	}

if(curmap->lightst)
	{
	M_free(curmap->lightst);
	curmap->lightst = NULL;
	}
}


//
//      Z1 functions for the object layer IO
//

/*
 *      save_z1 - Write the object layer, or a savegame
 */

void save_z1(char *filename)
{
OBJECT *temp;
int vx,vy;
FILE *fp;

// Open the file.
fp=fopen(filename,"w");
if(!fp)
	return;

// Write the header
fprintf(fp,"%s%s\n",COOKIE_Z1,TEXTCOOKIE_R00);

// Set the object IDs
assign_objects(mapnumber);

// Scan entire map line by line, row by row.
// This may seem inefficient, but it is the only way to record the decoratives
// and ensure the objects are in the right places.

for(vy=0;vy<curmap->h;vy++)
	for(vx=0;vx<curmap->w;vx++)
		{
		temp = GetRawObjectBase(vx,vy);
		for(;temp;temp=temp->next)
			if(temp->user->edecor) // If it's decorative, write special
				fprintf(fp,"\tdecorative %s at %d %d\n\n",temp->name,vx,vy);
			else
				{
				if(temp->pocket.objptr)
					WriteContainers(temp->pocket.objptr,fp);
				TWriteObject(fp,temp);
				}
		}

fclose(fp);
}


/*
 *      load_z1 - Load the object layer, or a savegame
 */


void load_z1(char *fname)
{
OBJECT *temp,*parent,*next;
OBJLIST *ptr;
struct TF_S z1;
int ctr,x,y,ke,ctr2;
unsigned int saveid,num;
char *str,bad,ok;
char first[1024],filename[1024];
char badobject[256];
char *l;

if(!loadfile(fname,filename))
	{
	irecon_cls();
	ilog_printf("Could not find file '%s'\n",fname);
	ilog_printf("Hit Y to create a new file, or any other key to exit.\n");
	ke=WaitForAscii();
	if(ke!='y' && ke!='Y')
		exit(1);
	return;
	}

MZ1_LoadingGame=1;

TF_init(&z1);
TF_load(&z1,filename);

if(strncmp(z1.line[0],COOKIE_Z1,4))
	{
	if(strncmp(z1.line[0],OLDCOOKIE_MZ1,4))
		{
		ilog_quiet("[%s]\n",z1.line[0]);
		ilog_quiet("[%s]\n",z1.line[1]);
		ithe_panic("load_mz1: file does not contain the right cookie",z1.line[0]);
		}
	}

temp=NULL;

Plot(z1.lines);
for(ctr=1;ctr<z1.lines;ctr++)
	{
	Plot(0);
	l=z1.line[ctr];
	SAFE_STRCPY(first,strfirst(l));
	strstrip(first);

	// decorative <type> at <xnum> <ynum>
	if(!istricmp(first,"decorative"))
		{
		x = atoi(strfirst(strrest(strrest(strrest(l)))));
		y = atoi(strfirst(strrest(strrest(strrest(strrest(l))))));
		str = strfirst(strrest(l));

		// If the coordinates are 65535, the object has been deleted
		// by the map utilities
		if(x == 65535 || y == 65535)
			{
			temp=NULL;
			continue;
			}

		temp = OB_Decor(str); // Is it a decorative object?
		if(temp)
			{
			temp->save_id=SAVEID_INVALID;
			ForceDropObject(x,y,temp);
			}
		else
			// The editor treats Decoratives as regular objects for simplicity
			{
			temp = OB_Alloc();
			temp->save_id=0;
			temp->flags |= IS_ON; // Activate, or things will go wrong later
			OB_Init(temp,str);
			ShrinkDecor(temp); // Save memory if we can
			MoveToMap(x,y,temp);
			}
//			Bug("load_mz1: decorative '%s' isn't decorative in file %s at line %d\n",str,filename,ctr);
		temp=NULL;
		continue;
		}

	// object <idnum>
	if(!istricmp(first,"object"))
		{
		temp = OB_Alloc();

		saveid = atoi(strfirst(strrest(l)));
		if(!saveid)
			Bug("load_mz1: object saveid = 0 in file %s at line %d\n",filename,ctr);

		temp->save_id = saveid;
		temp->flags |= IS_ON; // Activate, or things will go wrong later
		continue;
		}

	// If we don't have an object, skip until the next create command
	if(!temp)
		continue;

	// type <name>
	if(!istricmp(first,"type"))
		{
		str = strfirst(strrest(l));
		do {
			bad=0;
			if(!OB_Init(temp,str))
				{
				if(!in_editor)
					ithe_panic("Load_Z1: Could not find this object in 'section: character'!",str);
				else
					{
					bad=ProcessBadObject(&temp,badobject,255);
					badobject[255]=0;
					str=badobject;
					}
				}
			else
				{
				// Activate it
				if(!ML_InList(&ActiveList,temp))
					AL_Add(&ActiveList,temp);
				}
			} while(bad); // Todo: error handling not just panic
		continue;
		}

	// inside <idnum>
	if(!istricmp(first,"inside"))
		{
		num = atoi(strfirst(strrest(l)));
		if(!num)
			Bug("load_mz1: object inside Null in file %s at line %d\n",filename,ctr);

		// Set the parent as an ID, not a pointer
		temp->parent.saveid = num;
		continue;
		}

	// at <x> <y>
	if(!istricmp(first,"at"))
		{
		temp->x = atoi(strfirst(strrest(l)));
		temp->y = atoi(strfirst(strrest(strrest(l))));

		// If the coordinates are 65535, the object has been deleted
		// by the map utilities
		if(temp->x == 65535 || temp->y == 65535)
			{
			DeleteProtoObject(temp);
			temp=NULL;
			continue;
			}

		// Move the object to its destination
		MoveToMap(temp->x,temp->y,temp);
//		Init_Areas(temp);
		continue;
		}

	// form <formstring>
	if(!istricmp(first,"form"))
		{
		str = strfirst(strrest(l));
		OB_SetSeq(temp,str);
		continue;
		}

	// sequence <sptr_num> <slen_num> <sdir_num>
	if(!istricmp(first,"sequence"))
		{
		x = atoi(strfirst(strrest(l)));
		y = atoi(strfirst(strrest(strrest(l))));
		num = atoi(strfirst(strrest(strrest(strrest(l)))));

		temp->sptr = x;
//		temp->slen = y;
		temp->sdir = num;
		continue;
		}

	// curdir <dir_num>
	if(!istricmp(first,"curdir"))
		{
		num = atoi(strfirst(strrest(l)));
		if(num < 0 || num > 3)
			Bug("load_mz1: bad direction in file %s at line %d\n",filename,ctr);

		OB_SetDir(temp,num,FORCE_SHAPE|FORCE_FRAME);
//		Init_Areas(temp);
		CalcSize(temp);
//		ilog_quiet("object %s : w:%d h:%d\n",temp->name,temp->w,temp->h);
		continue;
		}

	// flags <num>
	if(!istricmp(first,"flags")) {
		sscanf(strfirst(strrest(l)),"%x",&num);
		if(!num)
			Bug("load_mz1: flags = 0 in file %s at line %d\n",filename,ctr);
		// Cram the flags in place

		// Do we want to load the flags in? (Are we doing a total restore?)
		if(fullrestore)	{
			if(save_version == 0) {
				if(num & IS_SEMIVISIBLE) {
					ilog_quiet("load_mz1: Old-format savegame, workaround for flag loading\n");
				}
				// Strip out the semi-visible flag (was IS_PARTY)
				num &= ~IS_SEMIVISIBLE;
			}
			temp->flags = num;
			// Make it active
			if(!ML_InList(&ActiveList,temp)) {
				AL_Add(&ActiveList,temp);
			}
		}
		continue;
	}

	// Do we care about Width and Height? (Only if restoring a savegame)
	if(fullrestore)
		{
		// width <num>
		if(!istricmp(first,"width"))
			{
			temp->w = atoi(strfirst(strrest(l)));
			continue;
			}

		// height <num>
		if(!istricmp(first,"height"))
			{
			temp->h = atoi(strfirst(strrest(l)));
			continue;
			}

		// mwidth <num>
		if(!istricmp(first,"mwidth"))
			{
			temp->mw = atoi(strfirst(strrest(l)));
			continue;
			}

		// mheight <num>
		if(!istricmp(first,"mheight"))
			{
			temp->mh = atoi(strfirst(strrest(l)));
			continue;
			}
		}

	// z <num>
	if(!istricmp(first,"z"))
		{
		temp->z = atoi(strfirst(strrest(l)));
		continue;
		}

	// name <personalname>
	if(!istricmp(first,"name"))
		{
		strncpy(temp->personalname,strrest(l),31);
		temp->personalname[31]=0;
		continue;
		}

	// target <idnum>
	if(!istricmp(first,"target"))
		{
		num = atoi(strfirst(strrest(l)));
		if(!num)
			Bug("load_mz1: object target = Null in file %s at line %d\n",filename,ctr);

		// Set the field as an ID, not a pointer
		temp->target.saveid = num;
		continue;
		}

	// enemy <idnum>
	if(!istricmp(first,"enemy"))
		{
		num = atoi(strfirst(strrest(l)));
		if(!num)
			Bug("load_mz1: object enemy = Null in file %s at line %d\n",filename,ctr);

		// Set the field as an ID, not a pointer
		temp->enemy.saveid = num;
		continue;
		}

	// tag <num>
	if(!istricmp(first,"tag"))
		{
		temp->tag = atoi(strfirst(strrest(l)));
		continue;
		}

	// light <num>
	if(!istricmp(first,"light"))
		{
		temp->light = atoi(strfirst(strrest(l)));
		continue;
		}

	// activity <funcname>
	if(!istricmp(first,"activity"))
		{
		temp->activity = getnum4PE(strfirst(strrest(l)));

		// Did the function exist?
		if(temp->activity < 0)
			continue;  // No

		// Make it active
		if(!ML_InList(&ActiveList,temp))
			AL_Add(&ActiveList,temp);
		continue;
		}

	// labels->location <string>
	if(!istricmp(first,"labels->location"))
		{
		SetLocation(temp,strfirst(strrest(l)));
		continue;
		}

//
// Stats
//

	// stats->hp <num>
	if(!istricmp(first,"stats->hp"))
		{
		temp->stats->hp = atoi(strfirst(strrest(l)));
		temp->user->oldhp = temp->stats->hp; // Keep it in sync
		continue;
		}

	// stats->dex <num>
	if(!istricmp(first,"stats->dex"))
		{
		temp->stats->dex = atoi(strfirst(strrest(l)));
		continue;
		}

	// stats->str <num>
	if(!istricmp(first,"stats->str"))
		{
		temp->stats->str = atoi(strfirst(strrest(l)));
		continue;
		}

	// stats->intel <num>
	if(!istricmp(first,"stats->intel"))
		{
		temp->stats->intel = atoi(strfirst(strrest(l)));
		continue;
		}

	// stats->weight <num>
	if(!istricmp(first,"stats->weight"))
		{
		temp->stats->weight = atoi(strfirst(strrest(l)));
		continue;
		}

	// maxstats->weight <num>
	if(!istricmp(first,"maxstats->weight"))
		{
		temp->maxstats->weight = atoi(strfirst(strrest(l)));
		continue;
		}

	// stats->quantity <num>
	if(!istricmp(first,"stats->quantity"))
		{
		temp->stats->quantity = atoi(strfirst(strrest(l)));
		continue;
		}

	// stats->armour <num>
	if(!istricmp(first,"stats->armour"))
		{
		temp->stats->armour = atoi(strfirst(strrest(l)));
		continue;
		}

	// stats->damage <num>
	if(!istricmp(first,"stats->damage"))
		{
		temp->stats->damage = atoi(strfirst(strrest(l)));
		continue;
		}

	// stats->tick <num>
	if(!istricmp(first,"stats->tick"))
		{
		temp->stats->tick = atoi(strfirst(strrest(l)));
		continue;
		}

	// stats->owner <num>
	if(!istricmp(first,"stats->owner"))
		{
		temp->stats->owner.saveid = atoi(strfirst(strrest(l)));
		continue;
		}

	// stats->karma <num>
	if(!istricmp(first,"stats->karma"))
		{
		temp->stats->karma = atoi(strfirst(strrest(l)));
		continue;
		}

	// stats->bulk <num>
	if(!istricmp(first,"stats->bulk"))
		{
		temp->stats->bulk = atoi(strfirst(strrest(l)));
		continue;
		}

	// stats->range <num>
	if(!istricmp(first,"stats->range"))
		{
		temp->stats->range = atoi(strfirst(strrest(l)));
		continue;
		}

	// stats->speed <num>
	if(!istricmp(first,"stats->speed"))
		{
		temp->maxstats->speed = atoi(strfirst(strrest(l)));
		continue;
		}

	// stats->level <num>
	if(!istricmp(first,"stats->level"))
		{
		temp->stats->level = atoi(strfirst(strrest(l)));
		continue;
		}

	// stats->radius <num>
	if(!istricmp(first,"stats->radius"))
		{
		temp->stats->radius = atoi(strfirst(strrest(l)));
		// Make it active
		if(!ML_InList(&ActiveList,temp))
			AL_Add(&ActiveList,temp);
		continue;
		}

	// stats->alignment <num>
	if(!istricmp(first,"stats->alignment"))
		{
		temp->stats->dex = atoi(strfirst(strrest(l)));
		continue;
		}

	// stats->npcflags <num>
	if(!istricmp(first,"stats->npcflags"))
		{
//		num = atoi(strfirst(strrest(l)));
		sscanf(strfirst(strrest(l)),"%x",&num);
		// Cram the flags in place, if we are doing a savegame restore
#ifdef DEBUG_SAVE
		ilog_quiet("%s: npcf = %x, fullrest=%d\n",temp->name,num,fullrestore);
#endif
		if(fullrestore)
			{
			temp->stats->npcflags = num;
			// Make it active
			if(!ML_InList(&ActiveList,temp))
				AL_Add(&ActiveList,temp);
			}
		continue;
		}

//
// Funcs
//

	// funcs->use <string>
	if(!istricmp(first,"funcs->use"))
		{
		SAFE_STRCPY(temp->funcs->use,strfirst(strrest(l)));
		continue;
		}

	// funcs->talk <string>
	if(!istricmp(first,"funcs->talk"))
		{
		SAFE_STRCPY(temp->funcs->talk,strfirst(strrest(l)));
		temp->funcs->tcache=1;
		continue;
		}

	// funcs->kill <string>
	if(!istricmp(first,"funcs->kill"))
		{
		SAFE_STRCPY(temp->funcs->kill,strfirst(strrest(l)));
		continue;
		}

	// funcs->look <string>
	if(!istricmp(first,"funcs->look"))
		{
		SAFE_STRCPY(temp->funcs->look,strfirst(strrest(l)));
		continue;
		}

	// funcs->stand <string>
	if(!istricmp(first,"funcs->stand"))
		{
		SAFE_STRCPY(temp->funcs->stand,strfirst(strrest(l)));
		// Make it active
		if(!ML_InList(&ActiveList,temp))
			AL_Add(&ActiveList,temp);
		continue;
		}

	// funcs->hurt <string>
	if(!istricmp(first,"funcs->hurt"))
		{
		SAFE_STRCPY(temp->funcs->hurt,strfirst(strrest(l)));
		continue;
		}

	// funcs->init <string>
	if(!istricmp(first,"funcs->init"))
		{
		SAFE_STRCPY(temp->funcs->init,strfirst(strrest(l)));
		continue;
		}

	// funcs->resurrect <string>
	if(!istricmp(first,"funcs->resurrect"))
		{
		SAFE_STRCPY(temp->funcs->resurrect,strfirst(strrest(l)));
		continue;
		}

	// funcs->wield <string>
	if(!istricmp(first,"funcs->wield"))
		{
		SAFE_STRCPY(temp->funcs->wield,strfirst(strrest(l)));
		continue;
		}

	// funcs->horror <string>
	if(!istricmp(first,"funcs->horror"))
		{
		SAFE_STRCPY(temp->funcs->horror,strfirst(strrest(l)));
		continue;
		}

	// funcs->attack <string>
	if(!istricmp(first,"funcs->attack"))
		{
		SAFE_STRCPY(temp->funcs->attack,strfirst(strrest(l)));
		continue;
		}

	// funcs->attack <string>
	if(!istricmp(first,"funcs->user1"))
		{
		SAFE_STRCPY(temp->funcs->user1,strrest(l));
		continue;
		}

	// funcs->attack <string>
	if(!istricmp(first,"funcs->user2"))
		{
		SAFE_STRCPY(temp->funcs->user2,strrest(l));
		continue;
		}

	// funcs->stand <string>
	if(!istricmp(first,"funcs->quantity"))
		{
		SAFE_STRCPY(temp->funcs->quantity,strfirst(strrest(l)));
		continue;
		}

//
//  Schedule
//

	if(!istricmp(first,"schedule"))
		{
		x = atoi(strfirst(strrest(l)));
		y = atoi(strfirst(strrest(strrest(l))));
		num = atoi(strfirst(strrest(strrest(strrest(strrest(l))))));
		str = strfirst(strrest(strrest(strrest(l))));

		// Initialise schedule if none present
		if(!temp->schedule)
			{
			temp->schedule = (SCHEDULE *)M_get(24,sizeof(SCHEDULE));
			for(ctr2=0;ctr2<24;ctr2++)
				temp->schedule[ctr2].active = 0;
			}

		// Find a spare slot

		ke=-1;
		for(ctr2=0;ctr2<24;ctr2++)
			if(!temp->schedule[ctr2].active)
				{
				ke=ctr2;
				break;
				}

		// Fill the slot

		if(ke>-1)
			{
			temp->schedule[ke].active = 1;
			temp->schedule[ke].hour = x;
			temp->schedule[ke].minute = y;
			temp->schedule[ke].target.saveid = num; // Fix up later
			SAFE_STRCPY(temp->schedule[ke].vrm,str);
			temp->schedule[ke].call = getnum4PE(str);
			}
		continue;
		}
	}

// Don't need text file open anymore
TF_term(&z1);

ilog_printf("\n");

// Sort out nested objects

ilog_printf("    Nested Objects\n");

fastfind_id_start();

temp = curmap->object->next;
while(temp)
	{
	next = temp->next;
	parent = fastfind_id(temp->parent.saveid);
	if(parent)
		{
//		ilog_quiet("putting %s (%d) into Parent %s (%d)\n",temp->name,temp.save_id,parent->name,parent.save_id);
		MoveToPocketEnd(temp,parent);
		// Objects inside eggs should become dormant to save CPU
		if(temp->stats->npcflags & IS_BIOLOGICAL)
			ML_Del(&ActiveList,temp);
		}
	else
		{
		ilog_quiet("Oh shit!  %s is orphaned.  ID = %d\n",temp->name,temp->save_id);
//		Bug("%s [%d] has an identity crisis\n",temp->name,temp->save_id);
//		DestroyObject(temp);
		}
	temp = next;
	};

// Convert remaining integers to pointers and set up functions

for(ptr=MasterList;ptr;ptr=ptr->next)
	if(ptr->ptr)
		{
		if(ptr->ptr->target.saveid)
			ptr->ptr->target.objptr = fastfind_id(ptr->ptr->target.saveid);
		if(ptr->ptr->enemy.saveid)
			ptr->ptr->enemy.objptr = fastfind_id(ptr->ptr->enemy.saveid);
		if(ptr->ptr->stats->owner.saveid)
			ptr->ptr->stats->owner.objptr = fastfind_id(ptr->ptr->stats->owner.saveid);

		// Convert any targets
		if(ptr->ptr->schedule)
			{
			for(ctr2=0;ctr2<24;ctr2++)
				if(ptr->ptr->schedule[ctr2].active)
					ptr->ptr->schedule[ctr2].target.objptr = fastfind_id(ptr->ptr->schedule[ctr2].target.saveid);
			}

		InitFuncsFor(ptr->ptr,0);
		OB_Funcs(ptr->ptr);
		}

fastfind_id_stop();

// Now we're safe to have the script engine run
MZ1_LoadingGame=0;


// If we're in the game, deck the empty schedule pointers so we can quickly
// see if the object has any events or not

//OBJLIST	*ptr;
//char ok=0;

if(!in_editor)
	for(ptr=MasterList;ptr;ptr=ptr->next)
		{
		if(ptr->ptr->schedule)
			{
			ok=0;
			for(ctr=0;ctr<24;ctr++)
				if(ptr->ptr->schedule[ctr].active)
					ok=1;
			if(!ok)
				{
				M_free(ptr->ptr->schedule);
				ptr->ptr->schedule = NULL;
				}
			}
		}

}

/*
 *      wipe_z1 - deallocate the z1 layer entirely
 *                called prior to load_z1 in game, not in editor
 */

void wipe_z1()
{
OBJECT *temp,*anchor;
int vx,vy;


ilog_printf("  wiping object matrix\n");
//ilog_quiet("player at %d,%d\n",player->x,player->y);

for(vy=0;vy<curmap->h;vy++)
	for(vx=0;vx<curmap->w;vx++)
		{
		anchor = GetRawObjectBase(vx,vy); // RAWobjectbase ignores large objs
		if(anchor)
			{
			if(anchor->flags & IS_DECOR)
				{
				curmap->objmap[ytab[vy]+vx]=NULL; // Kill the decorative!
				if(isDecor(vx,vy))
					Bug("Something WENT WRONG deleting at %d,%d\n",vx,vy);
				}
			else
				for(temp = anchor;temp;temp=temp->next)
					{
					if(temp->flags & IS_DECOR)
						{
						if(temp->next)
							ithe_panic("Oh shit",NULL);
						DelDecor(vx,vy, temp);
						if(isDecor(vx,vy))
							Bug("Something WENT WRONG deleting at %d,%d\n",vx,vy);
						}
					else
						{
						temp->flags &= ~IS_ON;
//						ilog_quiet("del %s %x at %d,%d\n",temp->name,temp,vx,vy);
						}
					}
			}
		}

ilog_printf("  Emptying Syspocket\n");
FreePockets(curmap->object);

ilog_printf("  Delete Everything1\n");
MassCheckDepend();

// Skip internal dependency checks (because we just did a bulk scan)
NoDeps=1;

ilog_printf("  Delete Everything2\n");
pending_delete=1;	// Force garbage collection
DeletePending();
pending_delete=1;	// Force garbage collection
DeletePending();

// Careful with that axe, Eugene
NoDeps=0;

if(!change_map)
	{
	player=NULL;
	}

//ilog_quiet("Making Sure\n\n");
memset(curmap->objmap,0,curmap->w*curmap->h*sizeof(OBJECT *));
ilog_printf("  Finished\n");

if(!change_map)
	{
	ilog_quiet("\nwiping party members\n\n");
	for(vx=0;vx<MAX_MEMBERS;vx++)
		party[vx]=NULL;
	}
}



//
//      Z2 rooftops layer map IO
//

/*
 *      load_z2 - Load the rooftops from disk
 */

void load_z2(int mapnum)
{
IFILE *fp;
char filename[1024];
char *fname;
int key;

ilog_printf("  Rooftops\n");

//      Objects layer 2 - roof

fname=makemapname(mapnum,0,".mz2");
SAFE_STRCPY(stemp,fname);

if(!loadfile(stemp,filename))
	{
	irecon_cls();
	ilog_printf("Could not find file '%s'\n",stemp);
	irecon_printf("Hit Y to create a new file, or any other key to exit.\n");
	key=WaitForAscii();
	if(key!='Y'&&key!='y')
		exit(1);
	}
else
	{
	fp=iopen(filename);
	memset(sigtemp,0,sizeof(sigtemp));
	iread((unsigned char *)sigtemp,8,fp);
	if(strncmp(sigtemp,COOKIE_Z2,4))
		{
		if(strncmp(sigtemp,OLDCOOKIE_MZ2,4))
			{
			ilog_printf("File '%s' is not a z2 map.. it does not contain the right cookie\n",stemp);
			exit(1);
			}
		// It is the old version, skip 40 bytes
		iseek(fp,40,SEEK_SET);
		}
	else
		{
		// New version, check rev
		if(!strncmp(&sigtemp[4],TEXTCOOKIE_R01,3))
			{
			iclose(fp);
			load_z2_r01(filename);
			return;
			}
		}

	// Read the original binary format

	iread((unsigned char *)curmap->roof,curmap->w*curmap->h*sizeof(unsigned char),fp);
	iclose(fp);
	}
}

//
//	Load the new format file
//

void load_z2_r01(char *fname)
{
struct TF_S z2;
int ctr,x,y,w,h,t;
char first[1024];
char *l;
unsigned char *map=curmap->roof;
w=curmap->w;
h=curmap->h;

TF_init(&z2);
TF_load(&z2,fname);

if(strncmp(z2.line[0],COOKIE_Z2,4))
	if(strncmp(&z2.line[0][4],TEXTCOOKIE_R01,3))
		{
		ilog_quiet("[%s]\n",z2.line[0]);
		ilog_quiet("[%s]\n",z2.line[1]);
		ithe_panic("load_mz2: file does not contain the right cookie",z2.line[0]);
		}

for(ctr=1;ctr<z2.lines;ctr++)
	{
	l=z2.line[ctr];
	SAFE_STRCPY(first,strfirst(l));
	strstrip(first);

	// roof <type> at <xnum> <ynum>
	if(!istricmp(first,"roof"))
		{
		x = atoi(strgetword(l,4));
		y = atoi(strgetword(l,5));
		t = atoi(strgetword(l,2));	// Tile no

		// If the coordinates are 65535, the object has been deleted
		// by the map utilities
		if(x <0 || x >= w)
			continue;
		if(y <0 || y >= h)
			continue;
		if(t>0)
			map[(y*w)+x]=(unsigned char)t;
		continue;
		}
	}

// Don't need text file open anymore
TF_term(&z2);
}


void save_z2(int mapnum)
{
char *fname;
int vx,vy,r;
unsigned char *roofptr;
FILE *fp;
//      Objects layer 2 - rooftops

fname=makemapname(mapnum,0,".mz2");
SAFE_STRCPY(stemp,fname);

// Open the file.
fp=fopen(stemp,"w");
if(!fp)
	return;

// Write the header
fprintf(fp,"%s%s\n",COOKIE_Z2,TEXTCOOKIE_R01);

// Scan entire map line by line, row by row and record the coordinates

roofptr=curmap->roof;
for(vy=0;vy<curmap->h;vy++)
	for(vx=0;vx<curmap->w;vx++)
		{
		r=*roofptr++;
		if(r>0)
			fprintf(fp,"\troof %d at %d %d\n",r,vx,vy);
		}

fclose(fp);
}






//
//      Z3 static light layer map IO
//

/*
 *      load_z3 - Load the static lighting from disk
 */

void load_z3(int mapnum)
{
IFILE *fp;
char filename[1024];
char *fname;
int key;

//      Objects layer 3 - static lights

fname=makemapname(mapnum,0,".mz3");
SAFE_STRCPY(stemp,fname);
if(!loadfile(stemp,filename))
	{
	irecon_cls();
	ilog_printf("Could not find file '%s'\n",stemp);
	irecon_printf("Hit Y to create a new file, or any other key to exit.\n");
	key=WaitForAscii();
	if(key!='Y'&&key!='y')
		exit(1);
	}
else
	{
	fp=iopen(filename);
	iread((unsigned char *)sigtemp,8,fp);
	if(strncmp(sigtemp,COOKIE_Z3,4))
		{
		if(strncmp(sigtemp,OLDCOOKIE_MZ3,4))
			{
			ilog_printf("File '%s' is not a z3 map.. it does not contain the right cookie\n",stemp);
			exit(1);
			}
		// It is the old version, skip 40 bytes
		iseek(fp,40,SEEK_SET);
		}
	else
		{
		// New version, check rev
		if(!strncmp(&sigtemp[4],TEXTCOOKIE_R01,3))
			{
			iclose(fp);
			load_z3_r01(filename);
			return;
			}
		}

	// Read the original binary format

	iread((unsigned char *)curmap->light,curmap->w*curmap->h*sizeof(unsigned char),fp);
	iclose(fp);
	}
}

//
//	Load the new format file
//

void load_z3_r01(char *fname)
{
struct TF_S z3;
int ctr,x,y,w,h,t;
char first[1024];
char *l;
unsigned char *map=curmap->light;
w=curmap->w;
h=curmap->h;

TF_init(&z3);
TF_load(&z3,fname);

if(strncmp(z3.line[0],COOKIE_Z3,4))
	if(strncmp(&z3.line[0][4],TEXTCOOKIE_R01,3))
		{
		ilog_quiet("[%s]\n",z3.line[0]);
		ilog_quiet("[%s]\n",z3.line[1]);
		ithe_panic("load_mz3: file does not contain the right cookie",z3.line[0]);
		}

for(ctr=1;ctr<z3.lines;ctr++)
	{
	l=z3.line[ctr];
	SAFE_STRCPY(first,strfirst(l));
	strstrip(first);

	// light <no> at <xnum> <ynum>
	if(!istricmp(first,"light"))
		{
		x = atoi(strgetword(l,4));
		y = atoi(strgetword(l,5));
		t = atoi(strgetword(l,2));	// Tile no

		// If the coordinates are 65535, the object has been deleted
		// by the map utilities
		if(x <0 || x >= w)
			continue;
		if(y <0 || y >= h)
			continue;

		if(t>0)
			map[(y*w)+x]=(unsigned char)t;
		continue;
		}
	}
// Don't need text file open anymore
TF_term(&z3);
}



/*
 *      save_z3 - Save the static lighting to disk
 */

void save_z3(int mapnum)
{
char *fname;
int vx,vy,r;
unsigned char *ptr;
FILE *fp;
//      Objects layer 3 - statlc lights

fname=makemapname(mapnum,0,".mz3");
SAFE_STRCPY(stemp,fname);

// Open the file.
fp=fopen(stemp,"w");
if(!fp)
	return;

// Write the header
fprintf(fp,"%s%s\n",COOKIE_Z3,TEXTCOOKIE_R01);

// Scan entire map line by line, row by row and record the coordinates

ptr=curmap->light;
for(vy=0;vy<curmap->h;vy++)
	for(vx=0;vx<curmap->w;vx++)
		{
		r=*ptr++;
		if(r>0)
			fprintf(fp,"\tlight %d at %d %d\n",r,vx,vy);
		}

fclose(fp);
}

// Miscellaneous State

/*
 *      load_ms - Read miscellaneous savegame data
 */

void load_ms(char *filename)
{
IFILE *ifp;
char buf[256];
char name48[48];
int num,ctr,maxnum,slot,ok;
unsigned int ut,ctr2,ctr3,ctr4,newpos,saveid,dark;
OBJECT *o,*o2;
USEDATA usedata;
GLOBALINT *globalint;
GLOBALPTR *globalptr;
char *journalname;

int act;

// Open the file.

ifp=iopen(filename);

// Read the header
memset(buf,0,sizeof(buf));

newpos = itell(ifp);
iread((unsigned char *)buf,8,ifp);			 // Cookie
if(!strncmp(COOKIE_PWAD,buf,4))
	{
	// Good, wind back to start
	iseek(ifp,0,newpos);
	}
else
	{
	// Okay, it's not a straight PWAD, look for the original header instead
	iseek(ifp,0,newpos);
	iread((unsigned char *)buf,40,ifp);			 // Cookie
	if(strncmp(OLDCOOKIE_MS,buf,4))
		{
		// Wrong
		iclose(ifp);
		return;
		}
	}

if(MZ1_SavingGame)
	goto reload_game;	// Naughty Doug
	
if(!GetWadEntry(ifp,"PARTYBLK"))
	ithe_panic("Savegame corrupted","PARTYBLK not found");

saveid = igetl_i(ifp);
player=find_id(saveid);
#ifdef DEBUG_SAVE
ilog_quiet("player id = %d\n",saveid);
#endif
if(!player)
	ithe_panic("load_ms: Player AWOL in savegame",NULL);

// Erase existing party
slot=0;
for(ctr=0;ctr<MAX_MEMBERS-1;ctr++)
	{
	party[ctr]=0;
	partyname[ctr][0]='\0';
	}

// Load new party
for(ctr=0;ctr<MAX_MEMBERS;ctr++)
	{
	saveid = igetl_i(ifp);
	if(saveid)
		{
		o = find_id(saveid);      // ID numbers change to objptrs
		
		ok=1;
		for(ctr2=0;ctr2<MAX_MEMBERS;ctr2++)
			if(party[ctr2] == o)
				{
				ok=0;
				break;
				}
			  
		if(ok)
			{
			party[slot]=o;
			party[slot]->stats->npcflags |= IN_PARTY;
			slot++;
			}
		}
	}

if(!GetWadEntry(ifp,"FLAGSBLK"))
	ithe_panic("Savegame corrupted","FLAGSBLK not found");

// Load user flags

Wipe_tFlags();
ut=igetl_i(ifp);
for(ctr2=0;ctr2<ut;ctr2++)
	{
	num=igetb(ifp);
	iread((unsigned char *)buf,num,ifp);
//	ilog_quiet("flag: '%s'\n",buf);
	Set_tFlag(buf,1);
	}

// Load time/date

if(!GetWadEntry(ifp,"TIMEDATE"))
	ithe_panic("Savegame corrupted","TIMEDATE not found");

game_minute = igetl_i(ifp);
game_hour   = igetl_i(ifp);
game_day    = igetl_i(ifp);
game_month  = igetl_i(ifp);
game_year   = igetl_i(ifp);
days_passed = igetl_i(ifp);
day_of_week = igetl_i(ifp);
dark = igetl_i(ifp);
if(dark >= 0 && dark < 256)	// If it's an old savegame, this will be junk
	SetDarkness(dark);

ilog_quiet("game time = %02d:%02d\n",game_hour,game_minute);

// Old format!
if(GetWadEntry(ifp,"JOURNAL "))	{
	int id,day;
	// Load journal entries
	J_Free();

	ut=igetl_i(ifp);
	for(ctr2=0;ctr2<ut;ctr2++) {
		id=igetl_i(ifp);	// ID
		day=igetl_i(ifp);	// day
		num=igetl_i(ifp);	
		iread((unsigned char *)buf,num,ifp); // Date
		buf[32]=0;
		JOURNALENTRY *j=J_Add(id,day,buf);
		if(!j)
			ithe_panic("Savegame error","Out of memory allocating journal entry");
		if(id != -1) {
			// Read title, then text
			num=igetl_i(ifp);
			j->title = (char *)M_get(1,num+1);
			iread((unsigned char *)j->title,num,ifp);
			num=igetl_i(ifp);
			j->text = (char *)M_get(1,num+1);
			iread((unsigned char *)j->text,num,ifp);
		}
	}
}

// New format
if(GetWadEntry(ifp,"JOURNAL2"))	{
	int id,day;
	// Load journal entries
	J_Free();

	ut=igetl_i(ifp);
	for(ctr2=0;ctr2<ut;ctr2++) {
		id=igetl_i(ifp);

		num=igetl_i(ifp);
		journalname = (char *)M_get(1,num+1);
		iread((unsigned char *)journalname,num,ifp);
		if(id != -1) {
			id=getnum4stringname(journalname);
			if(id == -1) {
				ithe_panic("Savegame corrupted, journal entry not found",journalname);
			}
		}

		day=igetl_i(ifp);	// day
		num=igetl_i(ifp);
		iread((unsigned char *)buf,num,ifp); // Date
		buf[32]=0;
		JOURNALENTRY *j=J_Add(id,day,buf);
		if(!j) {
			ithe_panic("Savegame error","Out of memory allocating journal entry");
		}
		if(id == -1) {
			// Read title, then text
			num=igetl_i(ifp);
			j->title = (char *)M_get(1,num+1);
			iread((unsigned char *)j->title,num,ifp);
			num=igetl_i(ifp);
			j->text = (char *)M_get(1,num+1);
			iread((unsigned char *)j->text,num,ifp);
			num=igetl_i(ifp);
			j->tag = (char *)M_get(1,num+1);
			iread((unsigned char *)j->tag,num,ifp);

			j->name = journalname;
		} else {
			M_free(journalname);
		}
		j->status = igetl_i(ifp); // status code
	}
}


// If we're doing a map switch, reload from here:

reload_game:

// Load current goal data
if(!GetWadEntry(ifp,"GOALDATA"))
	ithe_panic("Savegame corrupted","GOALDATA not found");

num = igetl_i(ifp);
#ifdef DEBUG_SAVE
ilog_quiet("%d goals\n",num);
#endif
for(ctr=0;ctr<num;ctr++)
	{
	saveid = igetl_i(ifp);
	o = find_id(saveid);
	if(!o)
		{
		Bug("Goal: Cannot find object number %d\n",saveid);
//		ithe_panic("Oh well whatever","Never mind");
		}
	iread((unsigned char *)buf,32,ifp);
	saveid = igetl_i(ifp);
	o2=NULL;
	if(saveid != SAVEID_INVALID)
		{
		o2 = find_id(saveid);
		if(!o2)
			{
			Bug("Goal: Cannot find target object number %d\n",saveid);
//			ithe_panic("Oh well whatever","Never mind");
			}
		}
	if(o)
		{
		SubAction_Wipe(o); // Erase any sub-tasks
		ActivityName(o,buf,o2);
		}
	}

// Load usedata
if(!GetWadEntry(ifp,"USEDATA "))
	ithe_panic("Savegame corrupted","USEDATA not found");

num = igetl_i(ifp);
ctr2 = igetl_i(ifp);
#ifdef DEBUG_SAVE
ilog_printf("Reading %d usedata entries:\n",num);
#endif
for(ctr=0;ctr<num;ctr++)
	{
	saveid = igetl_i(ifp);
	o = find_id(saveid);
	if(!o)
		{
		ilog_quiet("Load_miscellaneous_state (usedata): can't load object %d\n",saveid);
//		ithe_panic("Load_miscellaneous_state (usedata): Can't find object","Savegame corrupted");
		}

	if(ctr2 == USEDATA_VERSION)
		read_USEDATA(&usedata,ifp);
	else
		{
		// Not good, but try to do it anyway:
		newpos = itell(ifp)+ctr2;
		read_USEDATA(&usedata,ifp);
		iseek(ifp,newpos,SEEK_SET);
		}

	if(o)
		{
		NPC_RECORDING *oldn,*oldl; 	// These should be stable enough to survive

		oldn=o->user->npctalk;
		oldl=o->user->lFlags;

		memcpy(o->user,&usedata,sizeof(USEDATA));
		// Now we must clear all the pointers to prevent death
		o->user->arcpocket.objptr=NULL;
		o->user->pathgoal.objptr=NULL;
	
		o->user->npctalk=oldn;
		o->user->lFlags=oldl;

		// Delete activity lists too
		memset(o->user->actlist,0,sizeof(o->user->actlist));
		memset(o->user->acttarget,0,sizeof(o->user->acttarget)); // Delete list
		}
	}

// Load secondary goal data (sub-tasks or subactions)
if(!GetWadEntry(ifp,"SUBTASKS"))
	ithe_panic("Savegame corrupted","SUBTASKS not found");

num = igetl_i(ifp);
for(ctr=0;ctr<num;ctr++)
	{
	saveid = igetl_i(ifp);
	o = find_id(saveid);
	if(!o)
		Bug("SubTasks: Cannot find object number %d\n",saveid);

	for(ctr2=0;ctr2<ACT_STACK;ctr2++)
		{
		act = igetl_i(ifp);
		saveid = igetl_i(ifp);

		if(act != -1)
			{
			if(saveid == SAVEID_INVALID)
				{
				SubAction_Push(o,act,NULL);
//				ilog_quiet("restore queue: %s does %s\n",o->name,PElist[act].name);
				}
			else
				{
				o2 = find_id(saveid);
				if(!o2)
					Bug("Subtask: Cannot find target object number %d\n",saveid);
//				ilog_quiet("restore queue: %s does %s to %s\n",o->name,PElist[act].name,o2->name);
				SubAction_Push(o,act,o2);
				}
			}
		}
	}

// load record of what we've said to NPCs (OLD FORMAT!)
if(GetWadEntry(ifp,"NPCTALKS"))
	{
	num = igetl_i(ifp);
	#ifdef DEBUG_SAVE
	ilog_quiet("Reading %d npctalks\n",num);
	#endif
	for(ctr=0;ctr<num;ctr++)
		{
		saveid = igetl_i(ifp);
		o = find_id(saveid);
		if(!o)
			{
			ilog_quiet("looking for %d\n",saveid);
			ilog_quiet("WARNING: Did not find object in NPCtalk(1) while loading map \n");
			if(fullrestore)
				ithe_panic("Load_miscellaneous_state (NPCtalk): Can't find object","Savegame corrupted");
			}
		ut = igetl_i(ifp); // number of entries for this NPC
		for(ctr2=0;ctr2<ut;ctr2++)
			{
			ctr3=igetl_i(ifp);
			ctr4=igetb(ifp)&0xff; // Limit to 255 for safety
			iread((unsigned char *)buf,ctr4,ifp);
			o2 = find_id(ctr3);

	#ifdef DEBUG_SAVE
			ilog_quiet("npc %d spoke to %d\n",saveid,ctr3);
	#endif

			if(!o2)
				{
				ilog_quiet("looking for %d, player's id is %d\n",ctr3,player->save_id);
				ilog_quiet("WARNING: Did not find player in NPCtalk(2) while loading map \n");
				if(fullrestore)
					ithe_panic("Load_miscellaneous_state (NPCtalk): Can't find 'player'","Savegame corrupted");
				}
	#ifdef DEBUG_SAVE
		ilog_quiet("BeenRead: %x,%s,%x\n",o,buf,o2);
	#endif
			if(o && o2)
				{
				char *bufname = NPC_MakeName(o2);
				NPC_BeenRead(o,buf,bufname);
				M_free(bufname);
				}
			}
		}
	}

// load record of what we've said to NPCs
if(GetWadEntry(ifp,"NPCTALK2"))
	{
	num = igetl_i(ifp);
	for(ctr=0;ctr<num;ctr++)
		{
		saveid = igetl_i(ifp);
		o = find_id(saveid);
		if(!o)
			{
			ilog_quiet("looking for %d\n",saveid);
			ilog_quiet("WARNING: Did not find object in NPCtalk(1) while loading map \n");
			if(fullrestore)
				ithe_panic("Load_miscellaneous_state (NPCtalk): Can't find object","Savegame corrupted");
			}
		ut = igetl_i(ifp); // number of entries for this NPC
		for(ctr2=0;ctr2<ut;ctr2++)
			{
			ctr4=igetl_i(ifp);
			char *bufname = (char *)M_get(1,ctr4+1);
			iread((unsigned char *)bufname,ctr4,ifp);
		
			ctr4=(unsigned char)igetb(ifp);
			iread((unsigned char *)buf,ctr4,ifp);
			if(o)
				NPC_BeenRead(o,buf,bufname);
			M_free(bufname);
			}
		}
	}

// load record of local NPC flags (Legacy format)
if(GetWadEntry(ifp,"NPCLFLAG"))
	{
	num = igetl_i(ifp);
	for(ctr=0;ctr<num;ctr++)
		{
		saveid = igetl_i(ifp);
		o = find_id(saveid);
		if(!o)
			{
			ilog_quiet("can't find object %d (0x%x)\n",saveid,saveid);
			ilog_quiet("WARNING: Did not find object in lFlags(1) while loading map \n");
			if(fullrestore)
				ithe_panic("Load_miscellaneous_state (lFlags): Can't find object","Savegame corrupted");
			}
			
		ut = igetl_i(ifp); // number of entries for this NPC
		for(ctr2=0;ctr2<ut;ctr2++)
			{
			ctr3=igetl_i(ifp);
			ctr4=(unsigned char)igetb(ifp);
			iread((unsigned char *)buf,ctr4,ifp);
			o2 = find_id(ctr3);
			if(!o2)
				{
				ilog_quiet("can't find object %d (0x%x)\n",saveid,saveid);
				ilog_quiet("WARNING: Did not find object in lFlags(2) while loading map \n");
				if(fullrestore)
					ithe_panic("Load_miscellaneous_state (lFlags): Can't find 'player'","Savegame corrupted");
				}
			if(o && o2)
				{
				char *bufname = NPC_MakeName(o2);
				NPC_set_lFlag(o,buf,bufname,1);
				M_free(bufname);
				}
			}
		}
	}
	
// load record of local NPC flags
if(GetWadEntry(ifp,"NPCLFLG2"))
	{
	num = igetl_i(ifp);
	for(ctr=0;ctr<num;ctr++)
		{
		saveid = igetl_i(ifp);
		o = find_id(saveid);
		if(!o)
			{
			ilog_quiet("can't find object %d (0x%x)\n",saveid,saveid);
			ilog_quiet("WARNING: Did not find object in lFlags(1) while loading map \n");
			if(fullrestore)
				ithe_panic("Load_miscellaneous_state (lFlags): Can't find object","Savegame corrupted");
			}
			
		ut = igetl_i(ifp); // number of entries for this NPC
		for(ctr2=0;ctr2<ut;ctr2++)
			{
			ctr4=igetl_i(ifp);
			char *bufname = (char *)M_get(1,ctr4+1);
			iread((unsigned char *)bufname,ctr4,ifp);
		
			ctr4=(unsigned char)igetb(ifp);
			iread((unsigned char *)buf,ctr4,ifp);
			if(o)
				NPC_set_lFlag(o,buf,bufname,1);
			M_free(bufname);
			}
		}
	}
	

// Load wielded objects
if(!GetWadEntry(ifp,"NPCWIELD"))
	ithe_panic("Savegame corrupted","NPCWIELD not found");
	
num = igetl_i(ifp);
ctr2 = igetl_i(ifp);
for(ctr=0;ctr<num;ctr++) {
	saveid = igetl_i(ifp);
	o = find_id(saveid);
	if(!o)
		ithe_panic("Load_miscellaneous_state (wieldtab): Can't find object","Savegame corrupted");

	if(fullrestore || !((o->stats->npcflags&STRIPNPC) & IN_PARTY)) {
		memset(o->wield,0,sizeof(WIELD)); // Default to blank, unless a party member changing map
	}

	for(ctr3=0;ctr3<ctr2;ctr3++) {
		saveid = igetl_i(ifp);
		if(saveid != SAVEID_INVALID) {
			o2 = find_id(saveid);
			if(!o2) {
				ilog_quiet("Did not find id %ld\n",saveid);
				ilog_quiet("WARNING: Did not find object in wieldtab(2) while loading map \n");
				if(fullrestore)
					ithe_panic("Load_miscellaneous_state (wieldtab2): Can't find object","Savegame corrupted");
			}

			// Don't actually do it if the map is changing and it's a party member being done
			// Otherwise, the objects they're wielding will change from one map to the other
			if(fullrestore || !((o->stats->npcflags & STRIPNPC) & IN_PARTY))	{
				if(o2) {
					SetWielded(o,ctr3,o2);
				}
			}
		}
	}
}

// Load Global Integers (old format for backwards compatibility only)
if(fullrestore)	// Not for changing map
	{
	if(GetWadEntry(ifp,"GLOBALVI"))
		{
		globalint = GetGlobalIntlist(&maxnum);
		if(!globalint)
			maxnum=0;

		num= igetl_i(ifp);
		for(ctr=0;ctr<num;ctr++)
			{
			ctr2 = igetl_i(ifp);
			// Protect against array overflow while still reading in the data from disk
			if(ctr<maxnum)
				if(globalint[ctr].ptr)
					{
					//ilog_quiet("Globalvi: Setting %s to %d\n",globalint[ctr].name,ctr2);
					*globalint[ctr].ptr = ctr2;
					}
			}
		}
	}

// Load Global Objects (old format!)
if(GetWadEntry(ifp,"GLOBALVO"))
	{
	globalptr = GetGlobalPtrlist(&maxnum);
	if(!globalptr)
		maxnum=0;

	num = igetl_i(ifp);
	for(ctr=0;ctr<num;ctr++)
		{
		ctr2 = igetl_i(ifp);
		o = find_id(ctr2);

		if(ctr<maxnum)
			if(globalptr[ctr].ptr)
				{
//				ilog_quiet("Globalvo: Setting %s to %p\n",globalptr[ctr].name,o);
				*(OBJECT **)globalptr[ctr].ptr = o;
				}
		}
	}

// Now try the new formats

// Load Global Integers
if(fullrestore)	// Not for changing map
	{
	if(GetWadEntry(ifp,"GLOBALI2"))
		{
		num = igetl_i(ifp);
		for(ctr=0;ctr<num;ctr++)
			{
			iread((unsigned char *)name48,48,ifp);
			ctr2 = igetl_i(ifp);
		
			globalint = FindGlobalInt(name48);
			if(globalint && globalint->ptr)
				{
//				ilog_quiet("Globali2: Setting %s to %d\n",name48,ctr2);
				*globalint->ptr = ctr2;
				}
			}
		}
	}

// Load Global Objects (old format!)
if(GetWadEntry(ifp,"GLOBALO2"))
	{
	num = igetl_i(ifp);
	for(ctr=0;ctr<num;ctr++)
		{
		iread((unsigned char *)name48,48,ifp);
		ctr2 = igetl_i(ifp);
		o = find_id(ctr2);

		globalptr = FindGlobalPtr(name48);
		if(globalptr && globalptr->ptr)
			{
//			ilog_quiet("Globalo2: Setting %s to %p\n",name48,o);
			*(OBJECT **)(globalptr->ptr) = o;
			}
		}
	}

// Load goal destinations
if(GetWadEntry(ifp,"PATHGOAL"))
	{
	num = igetl_i(ifp);
	for(ctr=0;ctr<num;ctr++)
		{
		// Get object to set
		ctr2 = igetl_i(ifp);
		o = find_id(ctr2);

		// Get object in path goal (if any)
		ctr2 = igetl_i(ifp);
		o2 = find_id(ctr2);

		// Set it up
		if(o && o->user)
			o->user->pathgoal.objptr = o2;
		}
	}

// Load tile animation states
if(GetWadEntry(ifp,"TILEANIM"))
	{
	num = igetl_i(ifp);
	if(num > TItot)
		num=TItot;
	for(ctr=0;ctr<num;ctr++)
		{
		TIlist[ctr].sptr=igetl_i(ifp);
		TIlist[ctr].sdir=igetl_i(ifp);
		TIlist[ctr].tick=igetl_i(ifp);
		TIlist[ctr].sdx=igetl_i(ifp);
		TIlist[ctr].sdy=igetl_i(ifp);
		TIlist[ctr].sx=igetl_i(ifp);
		TIlist[ctr].sy=igetl_i(ifp);
		}
	}

iclose(ifp);
}


/*
 *      save_ms - Write miscellaneous savegame data
 */

void save_ms(char *filename)
{
long i;
unsigned int ctr2,ctr3,ok;
WadEntry entry[32];
int Entry=0;

OBJLIST	*o;
NPC_RECORDING *n;
IFILE *ofp;
GLOBALINT *globalint;
GLOBALPTR *globalptr;

// For saving globals:
int ctr,num;
OBJECT **temp,*objtmp;

ofp=iopen_write(filename);
if(!ofp)
	{
	if(!in_editor)
		{
		ilog_printf("Oh no!\n");
		ilog_printf("Could not create file '%s'\n",filename);
		}
	return;
	}

// Just write the PWAD part now, the header is included

StartWad(ofp);

// Block header
entry[Entry].name="PARTYBLK";
entry[Entry++].start = itell(ofp);

iputl_i(player->save_id,ofp);			// The Player
#ifdef DEBUG_SAVE
ilog_quiet("player id = %d\n",player->save_id);
#endif

for(ctr=0;ctr<MAX_MEMBERS;ctr++)
	{
	i=0;			// Get the member's ID without blowing up
	if(party[ctr])		// if the member does not exist.
		i=party[ctr]->save_id;
	iputl_i(i,ofp);		// Write the ID number, or 0 for N/A
	}


// Block header
entry[Entry].name="FLAGSBLK";
entry[Entry++].start = itell(ofp);

// Flags: find all flags which are TRUE

i=0;
for(ctr=0;ctr<MAXFLAGS;ctr++)
	if(tFlag[ctr])
		{
		if(!tFlagIdentifier[ctr])
			{
			Bug("Flag is TRUE but has no identifier!\n");
			continue;
			}
		i++;
		}
iputl_i(i,ofp);

// Now write the names to disk

for(ctr=0;ctr<MAXFLAGS;ctr++)
	if(tFlag[ctr])
		{
		i = strlen(tFlagIdentifier[ctr])+1;
		iputb((unsigned char)i,ofp);
		iwrite((unsigned char *)tFlagIdentifier[ctr],i,ofp);
		}


// Block header
entry[Entry].name="TIMEDATE";
entry[Entry++].start = itell(ofp);

iputl_i(game_minute,ofp);
iputl_i(game_hour,ofp);
iputl_i(game_day,ofp);
iputl_i(game_month,ofp);
iputl_i(game_year,ofp);
iputl_i(days_passed,ofp);
iputl_i(day_of_week,ofp);
iputl_i(GetDarkness(),ofp);


// Block header
entry[Entry].name="JOURNAL2";
entry[Entry++].start = itell(ofp);

num=0;
JOURNALENTRY *ptr;
for(ptr=Journal;ptr;ptr=ptr->next)
	if(ptr)
		num++;

iputl_i(num,ofp);	// Number of entries

for(ptr=Journal;ptr;ptr=ptr->next)
	if(ptr)	{
		iputl_i(ptr->id,ofp);

		num=strlen(ptr->name)+1;
		iputl_i(num,ofp);
		iwrite((unsigned char *)ptr->name,num,ofp);

		iputl_i(ptr->day,ofp);

		num=strlen(ptr->date)+1;
		iputl_i(num,ofp);
		iwrite((unsigned char *)ptr->date,num,ofp);

		if(ptr->id == -1) {
			if(ptr->title) {
				num=strlen(ptr->title)+1;
				iputl_i(num,ofp);
				iwrite((unsigned char *)ptr->title,num,ofp);
			} else {
				iputl_i(0,ofp);
			}

			if(ptr->text) {
				num=strlen(ptr->text)+1;
				iputl_i(num,ofp);
				iwrite((unsigned char *)ptr->text,num,ofp);
			} else {
				iputl_i(0,ofp);
			}

			if(ptr->tag) {
				num=strlen(ptr->tag)+1;
				iputl_i(num,ofp);
				iwrite((unsigned char *)ptr->tag,num,ofp);
			} else {
				iputl_i(ptr->status,ofp);
			}
		}
		iputl_i(0,ofp);	// Task completion state, future use
	}


// Block header
entry[Entry].name="GOALDATA";
entry[Entry++].start = itell(ofp);

i=0;
for(o=ActiveList;o;o=o->next)
	if(o->ptr->activity >= 0)
		if(o->ptr->save_id > 0)
			i++;

iputl_i(i,ofp);
for(o=ActiveList;o;o=o->next)
	if(o->ptr->activity >= 0)
		if(o->ptr->save_id > 0)
			{
			// Write the main goal
#ifdef DEBUG_SAVE
			ilog_quiet("Storing goal for ID %d 0x%x [%s at %d,%d]\n",o->ptr->save_id,o->ptr,o->ptr->name,o->ptr->x,o->ptr->y);
			if(o->ptr->parent.objptr)
				ilog_quiet("%d is inside %s at %d,%d",o->ptr->save_id,o->ptr->parent.objptr->name,o->ptr->parent.objptr->x,o->ptr->parent.objptr->y);
#endif
			iputl_i(o->ptr->save_id,ofp);
			iwrite((unsigned char *)PElist[o->ptr->activity].name,32,ofp);
			if(!o->ptr->target.objptr)
				iputl_i(SAVEID_INVALID,ofp);
			else
				iputl_i(o->ptr->target.objptr->save_id,ofp);
			}

// Save usedata

// Block header
entry[Entry].name="USEDATA ";
entry[Entry++].start = itell(ofp);

// count blank ones

memset(&empty,0,sizeof(empty));

i=0;
for(o=MasterList;o;o=o->next)
	if(o->ptr->save_id > 0)
		if(UsedataChanged(o->ptr))
			i++;

iputl_i(i,ofp);
iputl_i(USEDATA_VERSION,ofp);

for(o=MasterList;o;o=o->next)
	if(o->ptr->save_id > 0)
		if(UsedataChanged(o->ptr))
			{
#ifdef DEBUG_SAVE
			ilog_quiet("Save %d : %s\n",o->ptr->save_id,o->ptr->name);
#endif
			iputl_i(o->ptr->save_id,ofp);
			
			//iwrite((unsigned char *)o->ptr->user,sizeof(USEDATA),ofp);
			write_USEDATA(o->ptr->user,ofp);
			}


// Save the sub-tasks
entry[Entry].name="SUBTASKS";
entry[Entry++].start = itell(ofp);

// First count how many objects have subtasks
i=0;
for(o=ActiveList;o;o=o->next)
	if(o->ptr->activity >= 0)
		if(o->ptr->save_id > 0)
			{
			// Does it have any tasks?
			ok=0;
			for(ctr=0;ctr<ACT_STACK;ctr++)
				if(o->ptr->user->actlist[ctr]>0)
					ok=1;
			// Yes
			if(ok)
				i++;
			}

iputl_i(i,ofp);
for(o=ActiveList;o;o=o->next)
	if(o->ptr->activity >= 0)
		if(o->ptr->save_id > 0)
			{
			ok=0;
			for(ctr=0;ctr<ACT_STACK;ctr++)
				if(o->ptr->user->actlist[ctr]>0)
					ok=1;
			if(ok)
				{
				// Write the object ID
				iputl_i(o->ptr->save_id,ofp);
				// Write the tasks
				for(ctr=0;ctr<ACT_STACK;ctr++)
					{
					if(o->ptr->user->actlist[ctr]<=0)
						{
						iputl_i(-1,ofp);
						iputl_i(SAVEID_INVALID,ofp);
						}
					else
						{
#ifdef DEBUG_SAVE
						if(o->ptr->target.objptr)
							ilog_quiet("subtask %d: %s '%s' does %s to %s\n",ctr,o->ptr->name,o->ptr->personalname,PElist[o->ptr->activity].name,o->ptr->target.objptr->name);
						else
							ilog_quiet("subtask %d: %s '%s' does %s\n",ctr,o->ptr->name,o->ptr->personalname,PElist[o->ptr->activity].name);
#endif
						iputl_i(o->ptr->user->actlist[ctr],ofp);
						if(o->ptr->user->acttarget[ctr])
							iputl_i(o->ptr->user->acttarget[ctr]->save_id,ofp);
						else
							iputl_i(SAVEID_INVALID,ofp); // No target
						}
					}
				}
			}


// Write NPC conversation log
entry[Entry].name="NPCTALK2";
entry[Entry++].start = itell(ofp);

// Count number of NPC's we've spoken to.  This can become fragmented with
// blanks so the detection routine is a little strange

i=0;
for(o=MasterList;o;o=o->next)
	if(o->ptr->user->npctalk)
		{
		ok=0; // Did we speak with words, or silence?
		for(n=o->ptr->user->npctalk;n;n=n->next)
			if(n->readername != NULL && n->page[0] != '\0')
				ok=1;
		if(ok)
			i++;
		}

iputl_i(i,ofp);    // Store the count
#ifdef DEBUG_SAVE
ilog_quiet("Writing %d npctalks\n",i);
#endif

// Write all the pages we've seen for each NPC

for(o=MasterList;o;o=o->next)
	if(o->ptr->user->npctalk)
		{
		i=0;
		for(n=o->ptr->user->npctalk;n;n=n->next)
			if(n->readername != NULL && n->page[0] != '\0')
				i++;
#ifdef DEBUG_SAVE
		ilog_quiet("writing %d entries for NPC %d\n",i,o->ptr->save_id);
#endif
		if(i>0)
			{
			iputl_i(o->ptr->save_id,ofp);
			iputl_i(i,ofp);
			for(n=o->ptr->user->npctalk;n;n=n->next)
				if(n->readername != NULL && n->page[0] != '\0')
					{
#ifdef DEBUG_SAVE
					ilog_quiet("Save_NPCtalk: %s.%s\n",n->readername);
#endif
					i=strlen(n->readername)+1;
					iputl_i(i,ofp);
					iwrite((unsigned char *)n->readername,i,ofp);					
					i=strlen(n->page)+1;
					iputb((unsigned char)i,ofp);
					iwrite((unsigned char *)n->page,i,ofp);
					}
			}
		}

// Write NPC conversation log
entry[Entry].name="NPCLFLG2";
entry[Entry++].start = itell(ofp);

// Count number of NPC's with lFlags
// Again this may be fragmented.

i=0;
for(o=MasterList;o;o=o->next)
	if(o->ptr->user->lFlags)
		{
		ok=0; // Did we speak with words, or silence?
		for(n=o->ptr->user->lFlags;n;n=n->next)
			if(n->readername != NULL && n->page[0] != '\0')
				ok=1;
		if(ok)
			i++;
		}

iputl_i(i,ofp);    // Store the count

// Write all the lFlags for each NPC

for(o=MasterList;o;o=o->next)
	if(o->ptr->user->lFlags)
		{
		i=0;
		for(n=o->ptr->user->lFlags;n;n=n->next)
			if(n->readername != NULL && n->page[0] != '\0')
				i++;
		if(i>0)
			{
			iputl_i(o->ptr->save_id,ofp);
			iputl_i(i,ofp);
			for(n=o->ptr->user->lFlags;n;n=n->next)
				if(n->readername != NULL && n->page[0] != '\0')
					{
					i=strlen(n->readername)+1;
					iputl_i(i,ofp);
					iwrite((unsigned char *)n->readername,i,ofp);					
					i=strlen(n->page)+1;
					iputb((unsigned char)i,ofp);
					iwrite((unsigned char *)n->page,i,ofp);
					}
			}
		}

// Write NPC wield table
entry[Entry].name="NPCWIELD";
entry[Entry++].start = itell(ofp);

// Count number of characters wielding objects

i=0;
for(o=MasterList;o;o=o->next)
	{
	ctr2=0;
	for(ctr=0;ctr<WIELDMAX;ctr++)
		if(GetWielded(o->ptr,ctr))
			ctr2=1;
	if(ctr2)
		i++;
	}

iputl_i(i,ofp);    // Store the count
iputl_i(WIELDMAX,ofp);    // 9 objects in the wieldtab in this version, maybe more later

// Store the wielded objects

for(o=MasterList;o;o=o->next)
	{
	ctr2=0;
	for(ctr=0;ctr<WIELDMAX;ctr++)
		if(GetWielded(o->ptr,ctr))
			ctr2=1;
	if(ctr2)
		{
		iputl_i(o->ptr->save_id,ofp);
		for(ctr3=0;ctr3<WIELDMAX;ctr3++)
			{
			objtmp = GetWielded(o->ptr,ctr3);
			if(objtmp)
				iputl_i(objtmp->save_id,ofp);  // Store ID number
			else
				iputl_i(-1,ofp);    // No object here
			}
		}
	}


// Save the global variables in the script engine

// Write Global Integers
entry[Entry].name="GLOBALI2";
entry[Entry++].start = itell(ofp);

num=0;
globalint = GetGlobalIntlist(&num);
if(!globalint)
	num=0;

iputl_i(num,ofp);
for(ctr=0;ctr<num;ctr++)
	{
	iwrite((unsigned char *)globalint[ctr].name,48,ofp);
	if(globalint[ctr].ptr)
		iputl_i(*globalint[ctr].ptr,ofp);
	else
		iputl_i(-1,ofp);
	}

// Write Global Integers
entry[Entry].name="GLOBALO2";
entry[Entry++].start = itell(ofp);

num=0;
globalptr = GetGlobalPtrlist(&num);
iputl_i(num,ofp);
for(ctr=0;ctr<num;ctr++)
	{
	iwrite((unsigned char *)globalptr[ctr].name,48,ofp);
	temp=NULL;
	if(globalptr[ctr].ptr)
		temp = (OBJECT **)globalptr[ctr].ptr;

	if(temp && *temp)
		iputl_i((*temp)->save_id,ofp);
	else
		iputl_i(-1,ofp);
	}

/*
// Write Global Integers
entry[Entry].name="GLOBALVI";
entry[Entry++].start = itell(ofp);

GetGlobalInts(&intlist,&num);
iputl_i(num,ofp);

for(ctr=0;ctr<num;ctr++)
	{
	if(intlist[ctr])
		iputl_i(*(int *)intlist[ctr],ofp);
	else
		iputl_i(-1,ofp);
	}

// Write Global Objects
entry[Entry].name="GLOBALVO";
entry[Entry++].start = itell(ofp);

GetGlobalPtrs(&objlist,&num);
iputl_i(num,ofp);

for(ctr=0;ctr<num;ctr++)
	{
	temp = (OBJECT **)objlist[ctr];
	if(temp && *temp)
		{
		iputl_i((*temp)->save_id,ofp);
//		ilog_quiet("Stored item %d, which was %s\n",ctr,(*temp)->name);
		}
	else
		iputl_i(-1,ofp);
	}
*/

// Write NPC path goals
entry[Entry].name="PATHGOAL";
entry[Entry++].start = itell(ofp);

// Count number of characters with pathgoals

i=0;
for(o=MasterList;o;o=o->next)
	if(o->ptr->user && o->ptr->user->pathgoal.objptr)
		i++;

iputl_i(i,ofp);    // Store the count

// Store the pathgoals

for(o=MasterList;o;o=o->next)
	if(o->ptr->user && o->ptr->user->pathgoal.objptr)
		iputl_i(o->ptr->user->pathgoal.objptr->save_id,ofp);  // Store ID number


// Write tile animation state
entry[Entry].name="TILEANIM";
entry[Entry++].start = itell(ofp);

iputl_i(TItot,ofp);    // Store the count
for(ctr=0;ctr<TItot;ctr++)
	{
	iputl_i(TIlist[ctr].sptr,ofp);
	iputl_i(TIlist[ctr].sdir,ofp);
	iputl_i(TIlist[ctr].tick,ofp);
	iputl_i(TIlist[ctr].sdx,ofp);
	iputl_i(TIlist[ctr].sdy,ofp);
	iputl_i(TIlist[ctr].sx,ofp);
	iputl_i(TIlist[ctr].sy,ofp);
	}



// Finish the WAD
entry[Entry].name=NULL;
entry[Entry].start = itell(ofp);
FinishWad(ofp,entry); // This closes the file stream
}



/*
 *      load_lightstate - load the map's current light states
 */

void load_lightstate(int mapnum, int sgnum)
{
IFILE *fp;
char filename[1024];
char *fname;

fname=makemapname(mapnum,sgnum,".lsd");
SAFE_STRCPY(stemp,fname);
if(!loadfile(stemp,filename))
	{
	// Can't open file, just blank the lights
	memset((unsigned char *)curmap->lightst,0,curmap->w*curmap->h*sizeof(unsigned char));
	return;
	}

fp=iopen(filename);
iread((unsigned char *)sigtemp,8,fp);
if(strncmp(sigtemp,COOKIE_LSDFILE,4))
	{
	if(strncmp(sigtemp,OLDCOOKIE_LSD,4))
		{
		ilog_printf("File '%s' is not light state data.. it does not contain the right cookie\n",stemp);
		exit(1);
		}
	// It is the old version, skip 40 bytes
	iseek(fp,40,SEEK_SET);
	}

iread((unsigned char *)curmap->lightst,curmap->w*curmap->h*sizeof(unsigned char),fp);
iclose(fp);
ilog_printf("  Done\n");
}

/*
 *      save_lightstate - Save the static lighting to disk
 */

void save_lightstate(int mapnum, int sgnum)
{
char *fname;
//      Objects layer 3 - static lights

fname=makemapname(mapnum,sgnum,".lsd");
SAFE_STRCPY(stemp,fname);
ofp = iopen_write(stemp);

if(!ofp)
	{
	if(!in_editor)
		{
		ilog_printf("Oh no!\n");
		ilog_printf("Could not create file '%s'\n",stemp);
		}
	return;
	}

iwrite((unsigned char *)COOKIE_LSDFILE,4,ofp);
iwrite((unsigned char *)BINCOOKIE_R00,4,ofp);
iwrite((unsigned char *)curmap->lightst,curmap->w*curmap->h,ofp);
iclose(ofp);
return;
}


/*
 *      Read savegame header
 */

extern char *read_sgheader(int sgnum, int *world)
{
IFILE *fp;
char filename[1024];
char *fname;
char *title;

if(sgnum < 0)	{
	sprintf(filename,"%d",sgnum);
	ithe_panic("read_sgheader called with bad savegame number",filename);
}

fname=makemapname(0,sgnum,".sav");
SAFE_STRCPY(stemp,fname);
if(!loadfile(stemp,filename))	{
	//ilog_quiet("read_sgheader failed attempting to open '%s', '%s'\n",fname,filename);
	return NULL;
}

fp=iopen(filename);
iread((unsigned char *)sigtemp,8,fp);
if(strncmp(sigtemp,COOKIE_SAVEFILE,4))	{
	if(strncmp(sigtemp,OLDCOOKIE_SAV,4))	{
		ilog_quiet("read_sgheader failed because savegame cookie was invalid\n");
		iclose(fp);
		return NULL;
	}
	// It is the old version, skip 40 bytes
	iseek(fp,40,SEEK_SET);
}

save_version = 1;
if(!strncmp(&sigtemp[4],BINCOOKIE_R00,4)) {
	save_version = 0; // Workaround for party flag change
}

title=strLocalBuf(); // Allocate a 'local' buffer

// Get world number
*world=igetl_i(fp);

// Get savegame title
iread((unsigned char *)title,40,fp);

// Finish
iclose(fp);

return title;
}

/*
 *      Write savegame header
 */

void write_sgheader(int sgnum, int world, char *title)
{
IFILE *fp;
char *fname;

fname=makemapname(0,sgnum,".sav");
SAFE_STRCPY(stemp,fname);

fp=iopen_write(fname);
// Write magic cookie
iwrite((unsigned char *)COOKIE_SAVEFILE,4,fp);
iwrite((unsigned char *)BINCOOKIE_R01,4,fp);

// Write current map number
iputl_i(world,fp);

// Write savegame title
iwrite((unsigned char *)title,40,fp);

// Finish
iclose(fp);

return;
}



//======================================================================
//===========  Auxiliary functions for map IO systems ==================
//======================================================================

/*
 *    MoveToPocketEnd()  -  Move an object into the end of someone's pocket
 *                          Only for objects in the main objlist
 */

void MoveToPocketEnd(OBJECT *object, OBJECT *container)
{
OBJECT *temp;

// Check for silliness.  This error has never happened to me, but if it
// does I thought that you should know.

if(!object || !container)
	{
	Bug("MoveToPocketEnd(%x,%x) attempted\n",object,container);
	return;
	}

// Look for the object in the list

for(temp=curmap->object;temp->next;temp=temp->next)
	if(temp->next==object)
		{
		LL_Remove(&curmap->object->next,object);
		LL_Add(&container->pocket.objptr,object);	// Add to end
		object->parent.objptr = container;
		object->flags |= IS_FIXED;
		return;
		}
ithe_panic("MoveToPocketEnd: This object was not in the list:",object->name);
}

/*
 *      WriteContainers - Write contents containers to disk.
 *                        Recursive, called by save_z1
 */

void WriteContainers(OBJECT *cont, FILE *fp)
{
OBJECT *temp;

for(temp = cont;temp;temp=temp->next)
	{
	TWriteObject(fp,temp);
	if(temp->pocket.objptr)
		WriteContainers(temp->pocket.objptr,fp);
	}

return;
}

/*
 *  When changing maps, objects that we wish to keep must be moved out
 *  of the current world and into limbo.
 */

void save_objects(int mapnum)
{
int ctr;
OBJECT *temp;

assign_objects(mapnum); // Ensure even the limbo objects have SaveIDs

// Get the party, handling the case where someone is in a boat etc

for(ctr=0;ctr<MAX_MEMBERS;ctr++)
	if(party[ctr])
		{
		if(party[ctr]->parent.objptr)
			{
#ifdef DEBUG_SAVE
ilog_quiet("%s is in pocket\n",party[ctr]->name);
#endif
			temp = party[ctr]->parent.objptr;
			if(temp)
				{
#ifdef DEBUG_SAVE
ilog_quiet("pocket is called %s\n",temp->name);
#endif
				party[ctr]->user->archive = UARC_INPOCKET;
				party[ctr]->user->arcpocket.objptr = temp;
				TransferToPocket(party[ctr],&limbo);

				// And transfer the container too, but only once
				if(temp->user && temp->user->archive != UARC_ONCEONLY)
					{
					temp->user->archive = UARC_ONCEONLY;
					TransferToPocket(temp,&limbo);
					}
				}
			}
		else
			{
			party[ctr]->user->archive = UARC_NORMAL;
//			ilog_quiet("dump member %s\n",party[ctr]->name);
			TransferToPocket(party[ctr],&limbo);
			}
		}

// Get the stuff in Nowhereland

while(curmap->object->next)
	{
	(curmap->object->next)->user->archive = UARC_SYSLIST;
#ifdef DEBUG_SAVE
	ilog_quiet("%s is in syslist\n",curmap->object->next->name);
#endif
	TransferToPocket(curmap->object->next,&limbo);
	}

// Get the stuff in the Syspocket

while(curmap->object->pocket.objptr)
	{
	(curmap->object->pocket.objptr)->user->archive = UARC_SYSPOCKET;
#ifdef DEBUG_SAVE
	ilog_quiet("%s is in syspocket\n",curmap->object->pocket.objptr->name);
#endif
	TransferToPocket(curmap->object->pocket.objptr,&limbo);
	}
}

/*
 *  Move objects out of limbo and into the Real World
 */

void restore_objects()
{
OBJECT *temp,*t2;

// Find new location, if using a tag
if(change_map_tag)
	{
	// Find the destination object
	temp=FindTag(change_map_tag,"target");
	if(!temp)
		temp=FindTag(change_map_tag,NULL);
	if(temp)
		{
		// We will land here
		change_map_x = temp->x;
		change_map_y = temp->y;
		}
	}

change_map_tag=0; // reset tag

// Decode everything

while(limbo.pocket.objptr)
	{
	t2=limbo.pocket.objptr;

#ifdef DEBUG_SAVE
	ilog_quiet("unpack %s: archive flags = %x (%d)\n",limbo.pocket.objptr->name,limbo.pocket.objptr->user->archive,limbo.pocket.objptr->user->archive);
#endif
	
	switch(limbo.pocket.objptr->user->archive)
		{
		// Move to the linked list buffer
		case UARC_SYSPOCKET:
		temp=limbo.pocket.objptr->next;
		TransferToPocket(limbo.pocket.objptr,curmap->object);
		limbo.pocket.objptr=temp;
		break;

		// Move to the linked list buffer
		case UARC_SYSLIST:
		temp=limbo.pocket.objptr->next;
		limbo.pocket.objptr->parent.objptr = NULL;
		LL_Add(&curmap->object,limbo.pocket.objptr);
		limbo.pocket.objptr=temp;
		break;

		// Move to another object's pocket
		case UARC_INPOCKET:
		temp=limbo.pocket.objptr->next;
		TransferToPocket(limbo.pocket.objptr,limbo.pocket.objptr->user->arcpocket.objptr);
//		LL_Add(&limbo.pocket->user->arcpocket,limbo.pocket);
		limbo.pocket.objptr=temp;
		break;

		// Move the object to its appropriate map position
		case UARC_NORMAL:
		case UARC_ONCEONLY:
		default:
		limbo.pocket.objptr->x=change_map_x;
		limbo.pocket.objptr->y=change_map_y;
		if(!ForceFromPocket(limbo.pocket.objptr,&limbo,limbo.pocket.objptr->x,limbo.pocket.objptr->y))
			{
			Bug("Problem restoring '%s' from Limbo!\n",limbo.pocket.objptr->name);
			// Force us to the next one
			limbo.pocket.objptr = limbo.pocket.objptr->next;
			}
		break;
		}
	// Reset Archive state for object
	t2->user->archive=0;
	}
}


// Set object IDs

static void assign_objects(int mapnum)
{
int objtot,objctr;
unsigned int newid;
OBJLIST *ptr;
// First, set each object so it has an ID of its own.
objctr=1; // 1-based, so 0 means 'nothing'
objtot=0;

for(ptr=MasterList;ptr;ptr=ptr->next)
	if(in_editor || ptr->ptr->save_id < 1) // Only if it doesn't have one already
		{
		if(!in_editor)
			{
			newid=0;
			do
				{
				newid=MakeSaveID(mapnum,objctr++);
				} while(find_id(newid));
			}
		else
			newid=MakeSaveID(mapnum,objctr++);
		ptr->ptr->save_id = newid;
#ifdef DEBUG_SAVE
		ilog_quiet("Assigned object '%s' the ID %d, from map %d\n",ptr->ptr->name,ptr->ptr->save_id,mapnumber);
#endif
		objtot++;
		}
}


// Fast Find object ID

static void fastfind_id_start()
{
OBJLIST *t;
int ctr;

if(fastfind_id_list)
	return;

ctr=0;
for(t=MasterList;t;t=t->next)
	if(t->ptr)
		ctr++;

if(!ctr)
	ithe_panic("fastfind_id_start: No objects in the world!",NULL);

fastfind_id_num = ctr;
fastfind_id_list = (OBJECT **)M_get(ctr,sizeof(OBJECT *));

ctr=0;
for(t=MasterList;t;t=t->next)
	if(t->ptr)
		fastfind_id_list[ctr++]=t->ptr;

qsort(fastfind_id_list,fastfind_id_num,sizeof(OBJECT *),CMP_sort);
}


static void fastfind_id_stop()
{
if(!fastfind_id_list)
	return;
M_free(fastfind_id_list);
fastfind_id_list = NULL;
}


static OBJECT *fastfind_id(unsigned int id)
{
OBJECT **xx;

if(!id)
	return NULL;

if(!fastfind_id_list)
	ithe_panic("FastFind_ID called before FastFind_ID_start",NULL);

xx=(OBJECT **)bsearch((const void *)id,fastfind_id_list,fastfind_id_num,sizeof(OBJECT *),CMP_search);
if(xx)
	return *xx;
return NULL;
}

static int CMP_sort(const void *a, const void *b)
{
return (*(OBJECT **)a)->save_id - (*(OBJECT **)b)->save_id;
}

static int CMP_search(const void *a, const void *b)
{
return (int)((long)a - (*(OBJECT **)b)->save_id);
}

//
//  Has the usedata block changed to be worth saving?
//

int UsedataChanged(OBJECT *o)
{
int hp;

// Wish away the oldHP value because it gets regenerated during reload
// and otherwise every object in the world gets saved (~10k objects)

hp = o->user->oldhp;
o->user->oldhp=0;

// Now see if it's all blank
if(memcmp(o->user,&empty,sizeof(USEDATA)))
	{
	// No, store it
	o->user->oldhp=hp;
	return 1;
	}

// Yes, don't bother with this one
o->user->oldhp=hp;
return 0;
}

//
//  In the editor, process objects that we don't understand when loading
//

int ProcessBadObject(OBJECT **objptr, char *name, int maxlen)
{
static int DeleteAll=0;
static int ReplaceAll=0;
int key;

// If we're deleteing all unknown objects, do it automatically
if(DeleteAll)
	{
	DeleteProtoObject(*objptr);
	*objptr=NULL;
	return 0; // Dealt with
	}

if(ReplaceAll)
	{
	strncpy(name,CHlist[0].name,maxlen);
	return 1; // Retry it, this will cause it to initialise as this
	}

// Ask the user what to do
irecon_printf("Could not find definition of object '%s' in the scriptfile.\n",name);
irecon_printf("Do you want to..\n");
irecon_printf("1.  Change this object to '%s'\n",CHlist[0].name);
irecon_printf("2.  Change ALL unknown objects to '%s'\n",CHlist[0].name);
irecon_printf("3.  Delete this object\n");
irecon_printf("4.  Delete ALL unknown objects\n");
//irecon_printf("5.  Enter a name to change this object to\n");
irecon_printf("5.  Quit (so you can add the object to resource.txt)\n");

do
	{
	key=WaitForAscii();
	switch(key)
		{
		case '1':
			strncpy(name,CHlist[0].name,maxlen);  // Turn to unknown object
			return 1; // Re-evaluate the new string

		case '2':
			ReplaceAll=1; // Do this in future
			strncpy(name,CHlist[0].name,maxlen);  // Turn to unknown object
			return 1; // Re-evaluate

		case '3':
			// Kill it
			DeleteProtoObject(*objptr);
			*objptr=NULL;
			return 0; // Dealt with

		case '4':
			DeleteAll=1; // Do this in future
			DeleteProtoObject(*objptr);
			*objptr=NULL;
			return 0; // Dealt with

		case '5':
			ithe_panic("Could not find this character in resource.txt:",name);
			return 0; // Likely

		default:
			break;
		}
	} while(1);
/*
							if(key == '5')
								{
								do  {
									irecon_printf("Enter the object name:\n");
									if(!GetStringInput(objname,64))
										exit(1);
//								scanf("%s",stemp);
									temp_int = getnum4char(objname);  // Find character
									if(temp_int == -1)
										irecon_printf("Not found in main.txt\n");
									} while(temp_int == -1);
								}
*/
return 0;
}

//
//  Delete a not-yet-fully-formed object
//

void DeleteProtoObject(OBJECT *o)
{
LL_Remove(&curmap->object,o);
ML_Del(&MasterList,o);
M_free(o);
}

//
//  SaveID coder/decoder
//

unsigned int inline MakeSaveID(unsigned int map, unsigned int id)
{
return ((map<<SAVEID_IDBITS) & SAVEID_MAPMASK) | (id & SAVEID_IDMASK);
}

unsigned int inline GetSaveID_ID(unsigned int saveid)
{
return (saveid & SAVEID_IDMASK);
}

unsigned int inline GetSaveID_Map(unsigned int saveid)
{
return (saveid & SAVEID_MAPMASK)>>SAVEID_IDBITS;
}
