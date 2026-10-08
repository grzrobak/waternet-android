#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fstream>
#include <time.h>
#include "port.h"   // SDL2 port: replaces the SDL 1.2, SDL_gfx and SDL_rotozoom headers
#include <math.h>
#include "gp2x.h"
#include <vector>

// SDL2 port: always render to a 32 bit surface
#define SCREEN_BPP 32




#if (SCREEN_BPP == 16)
 typedef Uint16 pixel;
#else
#if (SCREEN_BPP == 8)
 typedef Uint8 pixel;
#else
 typedef Uint32 pixel;
#endif
#endif

#define SCREEN_BYTE sizeof(pixel)

using namespace std;

#ifdef GP2X
    int  SCREENPOSMULTIPLY = 1;
#else
    int SCREENPOSMULTIPLY = 2;
#endif
int SMOOTHING = 0,
    WINDOW_WIDTH = 320 *SCREENPOSMULTIPLY,
    WINDOW_HEIGHT = 240*SCREENPOSMULTIPLY,
    TileWidth = 24*SCREENPOSMULTIPLY,
    TileHeight = 24*SCREENPOSMULTIPLY;

const int   NumRows = 10,
            NumCols = 10,
            NrOfSounds=6,
            SND_SELECT=0,
            SND_WINNER=1,
            SND_MENU=2,
            SND_LEVELSFINISHED=3,
            SND_ROTATE = 4,
            SND_ERROR = 5,
            NrOfMusic = 2,
            MUS_TITLE = 0,
            MUS_GAME = 1,
            MaxMazeWidth = 10,
            MaxMazeHeight = 10,
            NrOfBlocks = 27,
            MaxAlpha = 150,
            MaxSkins = 100,
            #ifdef GP2X
            AlphaIncrease = 20;
            #else
            AlphaIncrease = 1;
            #endif

enum GameStates {GSQuit,GSIntro,GSGame,GSTitleScreen,GSLevelCompleted,GSLevelSelector,GSLevelsFinished,GSCredits,GSLevelTypeMenu,GSGameTypeMenu,GSNoLevelsFound};
enum GameTypes {Rotating=0,Sliding=1,RotatingSliding=2};
enum SortOfLevels{Tutorial=0,Easy=1,Medium=2,Hard=3,Custom=4,RandomEasy=5,RandomMedium=6,RandomHard=7};
Mix_Chunk *Sounds[NrOfSounds];
bool GlobalSoundEnabled = true;



SDL_Surface *IMGSkinTitleScreen,*IMGSkinQuit1,*IMGSkinQuit2,*IMGSkinCredits1,*IMGSkinCredits2,*IMGSkinPlay1,*IMGSkinPlay2,*IMGSkinHard,*IMGSkinEasy,*IMGSkinMedium,
            *IMGSkinCustom,*IMGSkinTutorial,*IMGSkinSelect,*IMGSkinRandomHard,*IMGSkinRandomMedium,*IMGSkinRandomEasy,*IMGSkinSelect2,*IMGSkinRotating,*IMGSkinSliding,
            *IMGSkinSlidingRotating,*IMGSkinArrows,*IMGSkinTubes,*IMGSkinBackGround,*IMGSkinL1,*IMGSkinL2,*IMGSkinE,*IMGSkinV,*IMGSkinC,*IMGSkinO,*IMGSkinM,*IMGSkinP,
            *IMGSkinT,*IMGSkinD,*IMGSkinNoLevels,*IMGSkinCongrats,*IMGSkinYouHave,*IMGSkinEasyLevels,*IMGSkinMediumLevels,*IMGSkinHardLevels,*IMGSkinTutorialLevels,
            *IMGSkinCustomLevels,*IMGSkinCreditsInfo1,*IMGSkinCreditsInfo2,*IMGSkinCreditsInfo3,*IMGSkinIntro1,*IMGSkinIntro2,*IMGSkinIntro3,*IMGSkinTubesConnected,
            *IMGSkinCursor;

struct SPoint
{
    int X,Y;
};

struct SBlock{
   bool up,left,down,right ;
   int blocknr;
};

struct SMazeRoom{
    bool NorthWall,SouthWall,WestWall,EastWall,visited;
};

class CWorld
{
    private:
        int PlayField[NumRows][NumCols],PlayFieldConnected[NumRows][NumCols];
        SBlock Blocks[NrOfBlocks];
        int StartX,StartY,EndX,EndY,StartPointX,StartPointY;
        void DrawConnectedRecurse( SDL_Surface *Surface,int X,int Y);
    public:
        CWorld();
        ~CWorld();
        void Draw( SDL_Surface *Surface);
        void DrawConnected( SDL_Surface *Surface);
        bool Rotate1(int PlayFieldX, int PlayFieldY);
        bool Rotate2(int PlayFieldX, int PlayFieldY);
        bool LoadLevel(GameTypes SortOfGame,SortOfLevels Sort,int Number);
        bool GenerateLevel(int Width,int Height);
        bool LevelCompleted(); //has to be called after DrawConnected so that we know which tiles are connected and which arent !
        void InitBlocks();
        void ShuffleGame1();
        void ShuffleGame2();
        void DrawArrows(SDL_Surface *Surface,int SelectedX,int SelectedY);
        bool Slide(int SelectedX,int SelectedY);
        void Clear();
        void CenterLevel();
        int GetStartPointX(){ return StartPointX;}
        int GetStartPointY(){ return StartPointY;}
};


class CSelector
{
    private:
        SPoint SelectedPoint,CurrentPoint; //variables to hold the current position and the selected position

    public:
        bool HasSelection;
        // constructor, parameters define the starting position
        CSelector(const int PlayFieldXin,const int PlayFieldYin)
        {
            SelectedPoint.X = 0; // there's no selection set it 0
            SelectedPoint.Y = 0;
            CurrentPoint.X = PlayFieldXin; // set current position
            CurrentPoint.Y = PlayFieldYin;
            HasSelection = false; // No selection when it's created
        }
        // Will set the position only if it layes within the board boundary
        void SetPosition(const int PlayFieldXin,const int PlayFieldYin)
        {
            // check if the new position is inside the board, if so set the new position as the currentposition
            if ((PlayFieldYin>=0) && (PlayFieldYin < NumRows) && (PlayFieldXin >= 0) && (PlayFieldXin <NumCols))
            {
                CurrentPoint.X = PlayFieldXin;
                CurrentPoint.Y = PlayFieldYin;
            }
            else
            {
                if(PlayFieldYin < 0)
                {
                    CurrentPoint.X = PlayFieldXin;
                    CurrentPoint.Y = NumRows-1;
                }

                if(PlayFieldYin >= NumRows)
                {
                    CurrentPoint.X = PlayFieldXin;
                    CurrentPoint.Y = 0;
                }

                if(PlayFieldXin < 0)
                {
                    CurrentPoint.X = NumCols-1;
                    CurrentPoint.Y = PlayFieldYin;
                }

                if(PlayFieldXin >= NumCols)
                {
                    CurrentPoint.X = 0;
                    CurrentPoint.Y = PlayFieldYin;
                }



            }
        }
        // Select will set the current position as the selected position
        void Select()
        {
            SelectedPoint.X = CurrentPoint.X;
            SelectedPoint.Y = CurrentPoint.Y;
            HasSelection = true;
        }
        // return the selected position
        SPoint GetSelection()
        {
            return SelectedPoint;
        }
        // return the current position
        SPoint GetPosition()
        {
            return CurrentPoint;
        }
        void DeSelect()
        {
            HasSelection = false;
        }
        // Draw the blue box on the current position, with the offsets in mind
        void Draw(SDL_Surface *Surface)
        {
            SDL_Rect DstRect;
            DstRect.x = CurrentPoint.X * TileWidth;
            DstRect.y = CurrentPoint.Y * TileHeight;
            DstRect.w = TileWidth;
            DstRect.h = TileHeight;
            SDL_BlitSurface(IMGSkinCursor,NULL,Surface,&DstRect);
        }
        ~CSelector()
        {
        }
};

// Class for the main menu
class CMainMenu
{
    private:
        int Selection; //Holds what we currently selected in the main menu, 1 = new game, 2= credits, 3 = quit
    public:
        CMainMenu(); // constructor
        void Draw(SDL_Surface *Surface); //draw to a certain surface
        void NextItem(); //Select the next menuitem
        void PreviousItem(); //select the previous menuitem
        int GetSelection() { return Selection; } // return the current selection
        void SetSelection(int s) { Selection = s; } // android port: tap on a menu item
        ~CMainMenu(); // destructor
};
CMainMenu *CurrentMainMenu = NULL; // android port: menu shown on the title screen
CSelector *CurrentSelector = NULL; // android port: cursor of the running game
// constructor of main menu will Load the graphics and set the current selection to 1 (io newgame)
CMainMenu::CMainMenu()
{
    Selection = 1;
}

CMainMenu::~CMainMenu()
{
}

// Increase the selection if it goes to far set i to the first selection
void CMainMenu::NextItem()
{
    Selection++;
    if (Selection == 4)
        Selection = 1;
    if (GlobalSoundEnabled)
        Mix_PlayChannel(-1,Sounds[SND_MENU],0);
}

// decrease the selection if it goes to low set it to the last selection
void CMainMenu::PreviousItem()
{
    Selection--;
    if (Selection == 0)
        Selection = 3;
    if (GlobalSoundEnabled)
        Mix_PlayChannel(-1,Sounds[SND_MENU],0);
}

// Draw the main menu
void CMainMenu::Draw(SDL_Surface *Surface)
{
    SDL_Rect Dest;

    // draw the title screen background
    SDL_BlitSurface(IMGSkinTitleScreen,NULL,Surface,NULL);
    if (Selection == 1)
    {
        Dest.x = 79 *SCREENPOSMULTIPLY ;
        Dest.y = 73 *SCREENPOSMULTIPLY;
        Dest.w = IMGSkinPlay2->w;
        Dest.h = IMGSkinPlay2->h;
        SDL_BlitSurface(IMGSkinPlay1,NULL,Surface,&Dest);
    }
    else
    {
        Dest.x = 79 *SCREENPOSMULTIPLY;
        Dest.y = 73 *SCREENPOSMULTIPLY;
        Dest.w = IMGSkinPlay1->w;
        Dest.h = IMGSkinPlay1->h;
        SDL_BlitSurface(IMGSkinPlay2,NULL,Surface,&Dest);
    }

    if (Selection == 2)
    {
        Dest.x = 79 *SCREENPOSMULTIPLY;
        Dest.y = 121 *SCREENPOSMULTIPLY;
        Dest.w = IMGSkinCredits2->w;
        Dest.h = IMGSkinCredits2->h;
        SDL_BlitSurface(IMGSkinCredits1,NULL,Surface,&Dest);
    }
    else
    {
        Dest.x = 79 *SCREENPOSMULTIPLY;
        Dest.y = 121 *SCREENPOSMULTIPLY;
        Dest.w = IMGSkinCredits1->w;
        Dest.h = IMGSkinCredits1->h;
        SDL_BlitSurface(IMGSkinCredits2,NULL,Surface,&Dest);
    }


    if (Selection == 3) // = quit draw purple
    {
        Dest.x = 79 *SCREENPOSMULTIPLY;
        Dest.y = 167 *SCREENPOSMULTIPLY;
        Dest.w = IMGSkinQuit2->w;
        Dest.h = IMGSkinQuit2->h;
        SDL_BlitSurface(IMGSkinQuit1,NULL,Surface,&Dest);
    }
    else // draw blue
    {
        Dest.x = 79 *SCREENPOSMULTIPLY;
        Dest.y = 167 *SCREENPOSMULTIPLY;
        Dest.w = IMGSkinQuit1->w;
        Dest.h = IMGSkinQuit1->h;
        SDL_BlitSurface(IMGSkinQuit2,NULL,Surface,&Dest);
    }

}


// Class for the main menu
class CLevelTypeMenu
{
    private:
        int Selection; //Holds what we currently selected in the main menu, 1 = new game, 2= credits, 3 = quit
    public:
        CLevelTypeMenu(SortOfLevels Selected); // constructor
        void Draw(SDL_Surface *Surface); //draw to a certain surface
        void NextItem(); //Select the next menuitem
        void PreviousItem(); //select the previous menuitem
        int GetSelection() { return Selection; } // return the current selection
        ~CLevelTypeMenu(); // destructor
};
// constructor of main menu will Load the graphics and set the current selection to 1 (io newgame)
CLevelTypeMenu::CLevelTypeMenu(SortOfLevels Selected)
{
    Selection = Selected;
}

// Destructor will free the surface images
CLevelTypeMenu::~CLevelTypeMenu()
{

}

// Increase the selection if it goes to far set i to the first selection
void CLevelTypeMenu::NextItem()
{
    Selection++;
    if (Selection == 8)
        Selection = 0;
    if (GlobalSoundEnabled)
        Mix_PlayChannel(-1,Sounds[SND_MENU],0);
}

// decrease the selection if it goes to low set it to the last selection
void CLevelTypeMenu::PreviousItem()
{
    Selection--;
    if (Selection == -1)
        Selection = 7;
    if (GlobalSoundEnabled)
        Mix_PlayChannel(-1,Sounds[SND_MENU],0);
}

// Draw the main menu
void CLevelTypeMenu::Draw(SDL_Surface *Surface)
{
    SDL_Rect Dest;

    // draw the title screen background
    SDL_BlitSurface(IMGSkinTitleScreen,NULL,Surface,NULL);
    Dest.x = 57 *SCREENPOSMULTIPLY;
    Dest.y = 108 *SCREENPOSMULTIPLY;
    Dest.w = IMGSkinSelect->w;
    Dest.h = IMGSkinSelect->h;
    SDL_BlitSurface(IMGSkinSelect,NULL,Surface,&Dest);
    Dest.x = 111 *SCREENPOSMULTIPLY;
    Dest.y = 146 *SCREENPOSMULTIPLY;

    if (Selection == Easy)
    {
        Dest.w = IMGSkinEasy->w;
        Dest.h = IMGSkinEasy->h;
        SDL_BlitSurface(IMGSkinEasy,NULL,Surface,&Dest);
    }

    if (Selection == Medium)
    {
        Dest.w = IMGSkinMedium->w;
        Dest.h = IMGSkinMedium->h;
        SDL_BlitSurface(IMGSkinMedium,NULL,Surface,&Dest);
    }

    if (Selection == Hard)
    {
        Dest.w = IMGSkinHard->w;
        Dest.h = IMGSkinHard->h;
        SDL_BlitSurface(IMGSkinHard,NULL,Surface,&Dest);
    }

    if (Selection == Custom)
    {
        Dest.w = IMGSkinCustom->w;
        Dest.h = IMGSkinCustom->h;
        SDL_BlitSurface(IMGSkinCustom,NULL,Surface,&Dest);
    }

    if (Selection == Tutorial)
    {
        Dest.w = IMGSkinTutorial->w;
        Dest.h = IMGSkinTutorial->h;
        SDL_BlitSurface(IMGSkinTutorial,NULL,Surface,&Dest);
    }

    if (Selection == RandomEasy)
    {
        Dest.w = IMGSkinRandomEasy->w;
        Dest.h = IMGSkinRandomEasy->h;
        SDL_BlitSurface(IMGSkinRandomEasy,NULL,Surface,&Dest);
    }

    if (Selection == RandomMedium)
    {
        Dest.w = IMGSkinRandomMedium->w;
        Dest.h = IMGSkinRandomMedium->h;
        SDL_BlitSurface(IMGSkinRandomMedium,NULL,Surface,&Dest);
    }


    if (Selection == RandomHard)
    {
        Dest.w = IMGSkinRandomHard->w;
        Dest.h = IMGSkinRandomHard->h;
        SDL_BlitSurface(IMGSkinRandomHard,NULL,Surface,&Dest);
    }



}


// Class for the main menu
class CGameTypeMenu
{
    private:
        int Selection; //Holds what we currently selected in the main menu, 1 = new game, 2= credits, 3 = quit
    public:
        CGameTypeMenu(GameTypes Selected); // constructor
        void Draw(SDL_Surface *Surface); //draw to a certain surface
        void NextItem(); //Select the next menuitem
        void PreviousItem(); //select the previous menuitem
        int GetSelection() { return Selection; } // return the current selection
        ~CGameTypeMenu(); // destructor
};
// constructor of main menu will Load the graphics and set the current selection to 1 (io newgame)
CGameTypeMenu::CGameTypeMenu(GameTypes Selected)
{
    Selection = Selected;
}

// Destructor will free the surface images
CGameTypeMenu::~CGameTypeMenu()
{

}

// Increase the selection if it goes to far set i to the first selection
void CGameTypeMenu::NextItem()
{
    Selection++;
    if (Selection == 3)
        Selection = 0;
    if (GlobalSoundEnabled)
        Mix_PlayChannel(-1,Sounds[SND_MENU],0);
}

// decrease the selection if it goes to low set it to the last selection
void CGameTypeMenu::PreviousItem()
{
    Selection--;
    if (Selection == -1)
        Selection = 2;
    if (GlobalSoundEnabled)
        Mix_PlayChannel(-1,Sounds[SND_MENU],0);
}

// Draw the main menu
void CGameTypeMenu::Draw(SDL_Surface *Surface)
{
    SDL_Rect Dest;

    // draw the title screen background
    SDL_BlitSurface(IMGSkinTitleScreen,NULL,Surface,NULL);
    Dest.x = 57 *SCREENPOSMULTIPLY;
    Dest.y = 108 *SCREENPOSMULTIPLY;
    Dest.w = IMGSkinSelect2->w;
    Dest.h = IMGSkinSelect2->h;
    SDL_BlitSurface(IMGSkinSelect2,NULL,Surface,&Dest);
    Dest.x = 111 *SCREENPOSMULTIPLY;
    Dest.y = 146 *SCREENPOSMULTIPLY;

    if (Selection == Rotating)
    {
        Dest.w = IMGSkinRotating->w;
        Dest.h = IMGSkinRotating->h;
        SDL_BlitSurface(IMGSkinRotating,NULL,Surface,&Dest);
    }

    if (Selection == Sliding)
    {
        Dest.w = IMGSkinSliding->w;
        Dest.h = IMGSkinSliding->h;
        SDL_BlitSurface(IMGSkinSliding,NULL,Surface,&Dest);
    }

    if (Selection == RotatingSliding)
    {
        Dest.w = IMGSkinSlidingRotating->w;
        Dest.h = IMGSkinSlidingRotating->h;
        SDL_BlitSurface(IMGSkinSlidingRotating,NULL,Surface,&Dest);
    }

}



//Screen is het scherm, Buffer is de groote van het spelscherm (altijd 320x240)
//Buffer1 is de groote van het spel scherm of de groote de tv out mode
// er wordt dus altijd naar Buffer getekend, buffer wordt naar buffer1 geblit en buffer1 naar het scherm
SDL_Surface *Buffer,*Buffer1,*Screen,*ScreenBufferZoom,*ScreenBufferZoomTmp;
TTF_Font* font,*BigFont;
SDL_Joystick *Joystick;
bool TvOutMode=false;
GameStates GameState = GSIntro;
int Volume = 128,StartScreenX=0,StartScreenY=0,InitialStartScreenX=20,InitialStartScreenY=24,Level=1,InstalledSkinsCount=0,SelectedSkin=-1;
CWorld *World;
SortOfLevels LevelType = Tutorial;
int UnLockedLevels[3][4];
Mix_Music *Music[NrOfMusic];
GameTypes GameType = Rotating;
char InstalledSkins[MaxSkins][FILENAME_MAX];
char SkinName[FILENAME_MAX];
SDL_Color TextColor = {255,255,255,255};


static void SDL_PutPixel(SDL_Surface *screen,int x, int y,pixel color)
{
     pixel *BufP;
     BufP = (pixel *) screen->pixels + y * screen->pitch / SCREEN_BYTE + x ;
     *BufP = color;
//   pixelRGBA(screen, x, y, 0, 0, 255, 255);
}

static pixel* SDL_GetPixel(SDL_Surface *screen,int x, int y)
{
	return ((pixel*)screen->pixels + y*screen->pitch/SCREEN_BYTE + x);
}


SDL_Surface* ScaleSurface(SDL_Surface* Surface,int ScaleFactor)
{
    int X=0,Y=0,Xi=0,Yi=0,X2=0,X3=0,Y2=0;
    SDL_Surface *Tmp;
    pixel PixelColor;
    Tmp = SDL_CreateRGBSurface(SDL_HWSURFACE,Surface->w * ScaleFactor,Surface->h * ScaleFactor,Surface->format->BitsPerPixel,Surface->format->Rmask,Surface->format->Gmask,Surface->format->Bmask,Surface->format->Amask);
    if (SDL_MUSTLOCK(Surface))
        SDL_LockSurface(Surface);
    if (SDL_MUSTLOCK(Tmp))
        SDL_LockSurface(Tmp);



    for(Y=0;Y<Surface->h;Y++)
    {
        for(X=0;X<Surface->w;X++)
        {
            //copy single pixel
            for(Xi = 0;Xi<ScaleFactor;Xi++)
            {
                memcpy((pixel *)Tmp->pixels +(Y2)*Tmp->pitch/SCREEN_BYTE + X2,(pixel *)Surface->pixels + (Y)*Surface->pitch/SCREEN_BYTE+X,SCREEN_BYTE);
                X2++;
            }
        }
        X2 = 0;
        //copy previous line
        for (Yi=1;Yi<ScaleFactor;Yi++)
            memcpy((pixel *)Tmp->pixels + (Y2+Yi)*Tmp->pitch/SCREEN_BYTE,(pixel *)Tmp->pixels+(Y2)*Tmp->pitch/SCREEN_BYTE,Tmp->w*SCREEN_BYTE);

        Y2+=ScaleFactor;
    }

    if (SDL_MUSTLOCK(Tmp))
        SDL_UnlockSurface(Tmp);
    if (SDL_MUSTLOCK(Surface))
        SDL_UnlockSurface(Surface);
    return Tmp;

}

/*static Uint32 SDL_GetPixel(SDL_Surface *screen,int x, int y)
{
    if(SDL_MUSTLOCK(screen))
        SDL_LockSurface(screen);
    Uint32 color = 0;
	switch(screen->format->BytesPerPixel)
	{
        case 1:{
                Uint8 *bufp;
                bufp = (Uint8 *)screen->pixels + y * screen->pitch + x;
                color = *bufp;
                break;
        }

        case 2:{
                Uint16 *bufp;
                bufp = (Uint16 *)screen->pixels + y * screen->pitch / 2 + x;
                color = *bufp;
                break;
        }

        case 4:{
                Uint32 *bufp;
                bufp = (Uint32 *)screen->pixels + y * screen->pitch / 4 + x;
                color = *bufp;
                break;
        }
	}
    if(SDL_MUSTLOCK(screen))
        SDL_UnlockSurface(screen);
	return color;
}
*/



static void SDL_FloodFill(SDL_Surface *Surface,int x, int y,pixel Color,pixel ReplaceColor)
{
    vector<SPoint> Points;
    SPoint Point;
    int y1,X,Y;
    bool spanLeft, spanRight;

    Point.X = x;
    Point.Y = y;
    Points.push_back(Point);

    while(Points.size() > 0)
    {
        Point = Points.back();
        Points.pop_back();
        X = Point.X;
        Y = Point.Y;
        y1 = Y;
        while(*SDL_GetPixel(Surface,X,y1) == ReplaceColor && y1 >= 0) y1--;
        y1++;
        spanLeft = spanRight = false;
        while(*SDL_GetPixel(Surface,X,y1) == ReplaceColor && y1 < 240)
        {
            SDL_PutPixel(Surface,X,y1,Color);
            if((!spanLeft) && (X > 0 ) && (*SDL_GetPixel(Surface,X-1,y1) == ReplaceColor))
            {
                Point.X = X -1;
                Point.Y = y1;
                Points.push_back(Point);
                spanLeft = true;
            }
            else if(spanLeft && (X > 0) && (*SDL_GetPixel(Surface,X-1,y1) != ReplaceColor))
            {
                spanLeft = false;
            }

            if((!spanRight) && (X < 320 - 1) && (*SDL_GetPixel(Surface,X+1,y1) == ReplaceColor))
            {
                Point.X = X + 1;
                Point.Y = y1;
                Points.push_back(Point);
                spanRight = true;
            }
            else if(spanRight && (X < 320 - 1) &&(*SDL_GetPixel(Surface,X+1,y1) != ReplaceColor))
            {
                spanRight = false;
            }
            y1++;
        }
    }

}
/*
static void SDL_FloodFill1(SDL_Surface *Surface,int x, int y, int dx, int dy,pixel Color,pixel ReplaceColor)
{
    if ((x <0) && (y < 0) &&(x>=320) && (y>=240))
        return;
        if (*SDL_GetPixelFloodFill(Surface,x,y) != ReplaceColor)
        return;

            SDL_PutPixel(Surface,x,y,Color);
            if (dx == 1)
                SDL_FloodFill1(Surface,x+1,y,1,0,Color,ReplaceColor);
            else
                if(dx == -1)
                    SDL_FloodFill1(Surface,x-1,y,-1,0,Color,ReplaceColor);
                else
                {
                    SDL_FloodFill1(Surface,x+1,y,1,0,Color,ReplaceColor);
                    SDL_FloodFill1(Surface,x-1,y,-1,0,Color,ReplaceColor);
                }
            if (dy == 1)
                SDL_FloodFill1(Surface,x,y+1,0,1,Color,ReplaceColor);
            else
                if(dy == -1)
                    SDL_FloodFill1(Surface,x,y-1,0,-1,Color,ReplaceColor);
                else
                {
                    SDL_FloodFill1(Surface,x,y+1,0,1,Color,ReplaceColor);
                    SDL_FloodFill1(Surface,x,y-1,0,-1,Color,ReplaceColor);
                }


}
*/


CWorld::CWorld()
{
    SDL_Surface *Tmp;
    int X,Y;
    StartX=NumCols;
    StartY=NumRows;
    EndX=-1;
    EndY=-1;

    for (Y=0;Y<NumRows;Y++)
        for(X=0;X<NumCols;X++)
            PlayField[X][Y] =  0;
    InitBlocks();
}

CWorld::~CWorld()
{

}

void CWorld::Clear(){
    int X=0,Y=0;
    for (X=0;X<NumCols;X++)
        for (Y=0;Y<NumRows;Y++)
            PlayField[X][Y] = 0;
    StartPointX = -1;
    StartPointY = -1;
}

void CWorld::CenterLevel(){
    int Xi=0,Yi=0,X,Y;
    int Temp[NumCols][NumRows];
    if((StartX!=NumCols) && (StartY != NumRows) &&   (EndX != -1) && (EndY != -1))
    {
        for(Y=0;Y<NumRows;Y++)
            for(X=0;X<NumCols;X++)
                Temp[X][Y] = 0;
        Xi = ( 4 - trunc((EndX + StartX-1) / 2));
        Yi = (4 - trunc((EndY + StartY-1) / 2));

        for(Y=StartY;Y<=EndY;Y++)
            for(X=StartX;X<=EndX;X++)
                Temp[X+Xi][Y+Yi] = PlayField[X][Y];

        for(Y=0;Y<NumRows;Y++)
            for(X=0;X<NumCols;X++)
                PlayField[X][Y] = Temp[X][Y];

        StartX+=Xi;
        StartY+=Yi;
        EndX+=Xi;
        EndY+=Yi;
        StartPointX +=Xi;
        StartPointY +=Yi;
    }
}

void CWorld::InitBlocks()
{

 Blocks[0].up = false;
 Blocks[0].left = false;
 Blocks[0].right = false;
 Blocks[0].down = false;
 Blocks[0].blocknr = 0;

 Blocks[1].up = true;
 Blocks[1].left = false;
 Blocks[1].right = true;
 Blocks[1].down = true;
 Blocks[1].blocknr = 1;

 Blocks[2].up = true;
 Blocks[2].left = true;
 Blocks[2].right = true;
 Blocks[2].down = false;
 Blocks[2].blocknr = 2;

 Blocks[3].up = true;
 Blocks[3].left = true;
 Blocks[3].right = false;
 Blocks[3].down = true;
 Blocks[3].blocknr = 3;

 Blocks[4].up = false;
 Blocks[4].left = true;
 Blocks[4].right = true;
 Blocks[4].down = true;
 Blocks[4].blocknr = 4;


 Blocks[5].up = true;
 Blocks[5].left = false;
 Blocks[5].right = false;
 Blocks[5].down = true;
 Blocks[5].blocknr = 5;

 Blocks[6].up = false;
 Blocks[6].left = true;
 Blocks[6].right = true;
 Blocks[6].down = false;
 Blocks[6].blocknr = 6;



 Blocks[7].up = false;
 Blocks[7].left = false;
 Blocks[7].right = true;
 Blocks[7].down = true;
 Blocks[7].blocknr = 7;

 Blocks[8].up = true;
 Blocks[8].left = false;
 Blocks[8].right = true;
 Blocks[8].down = false;
 Blocks[8].blocknr = 8;

 Blocks[9].up = true;
 Blocks[9].left = true;
 Blocks[9].right = false;
 Blocks[9].down = false;
 Blocks[9].blocknr = 9;

 Blocks[10].up = false;
 Blocks[10].left = true;
 Blocks[10].right = false;
 Blocks[10].down = true;
 Blocks[10].blocknr = 10;

 Blocks[11].up = true;
 Blocks[11].left = false;
 Blocks[11].right = true;
 Blocks[11].down = true;
 Blocks[11].blocknr = 11;

 Blocks[12].up = true;
 Blocks[12].left = true;
 Blocks[12].right = true;
 Blocks[12].down = false;
 Blocks[12].blocknr = 12;

 Blocks[13].up = true;
 Blocks[13].left = true;
 Blocks[13].right = false;
 Blocks[13].down = true;
 Blocks[13].blocknr = 13;

 Blocks[14].up = false;
 Blocks[14].left = true;
 Blocks[14].right = true;
 Blocks[14].down = true;
 Blocks[14].blocknr = 14;


 Blocks[15].up = true;
 Blocks[15].left = false;
 Blocks[15].right = false;
 Blocks[15].down = true;
 Blocks[15].blocknr = 15;

 Blocks[16].up = false;
 Blocks[16].left = true;
 Blocks[16].right = true;
 Blocks[16].down = false;
 Blocks[16].blocknr = 16;

 Blocks[17].up = false;
 Blocks[17].left = false;
 Blocks[17].right = true;
 Blocks[17].down = true;
 Blocks[17].blocknr = 17;

 Blocks[18].up = true;
 Blocks[18].left = false;
 Blocks[18].right = true;
 Blocks[18].down = false;
 Blocks[18].blocknr = 18;

 Blocks[19].up = true;
 Blocks[19].left = true;
 Blocks[19].right = false;
 Blocks[19].down = false;
 Blocks[19].blocknr = 19;

 Blocks[20].up = false;
 Blocks[20].left = true;
 Blocks[20].right = false;
 Blocks[20].down = true;
 Blocks[20].blocknr = 20;

 Blocks[21].up = false;
 Blocks[21].left = false;
 Blocks[21].right = true;
 Blocks[21].down = false;
 Blocks[21].blocknr = 21;

 Blocks[22].up = true;
 Blocks[22].left = false;
 Blocks[22].right = false;
 Blocks[22].down = false;
 Blocks[22].blocknr = 22;

 Blocks[23].up = false;
 Blocks[23].left = true;
 Blocks[23].right = false;
 Blocks[23].down = false;
 Blocks[23].blocknr = 23;

 Blocks[24].up = false;
 Blocks[24].left = false;
 Blocks[24].right = false;
 Blocks[24].down = true;
 Blocks[24].blocknr = 24;

 Blocks[25].up = true;
 Blocks[25].left = true;
 Blocks[25].right = true;
 Blocks[25].down = true;
 Blocks[25].blocknr = 25;

 Blocks[26].up = true;
 Blocks[26].left = true;
 Blocks[26].right = true;
 Blocks[26].down = true;
 Blocks[26].blocknr = 26;
}

bool CWorld::Rotate1(int PlayFieldX, int PlayFieldY)
{
    bool result = false;
    switch(PlayField[PlayFieldX][PlayFieldY])
    {
        case 1: PlayField[PlayFieldX][PlayFieldY] = 4;
                result = true;
                break;
        case 2: PlayField[PlayFieldX][PlayFieldY] = 1;
                result = true;
                break;
        case 3: PlayField[PlayFieldX][PlayFieldY] = 2;
                result = true;
                break;
        case 4: PlayField[PlayFieldX][PlayFieldY] = 3;
                result = true;
                break;
        case 5: PlayField[PlayFieldX][PlayFieldY] = 6;
                result = true;
                break;
        case 6: PlayField[PlayFieldX][PlayFieldY] = 5;
                result = true;
                break;
        case 7: PlayField[PlayFieldX][PlayFieldY] = 10;
                result = true;
                break;
        case 8: PlayField[PlayFieldX][PlayFieldY] = 7;
                result = true;
                break;
        case 9: PlayField[PlayFieldX][PlayFieldY] = 8;
                result = true;
                break;
        case 10: PlayField[PlayFieldX][PlayFieldY] = 9;
                result = true;
                break;
        case 11: PlayField[PlayFieldX][PlayFieldY] = 14;
                result = true;
                break;
        case 12: PlayField[PlayFieldX][PlayFieldY] = 11;
                result = true;
                break;
        case 13: PlayField[PlayFieldX][PlayFieldY] = 12;
                result = true;
                break;
        case 14: PlayField[PlayFieldX][PlayFieldY] = 13;
                result = true;
                break;
        case 15: PlayField[PlayFieldX][PlayFieldY] = 16;
                result = true;
                break;
        case 16: PlayField[PlayFieldX][PlayFieldY] = 15;
                result = true;
                break;
        case 17: PlayField[PlayFieldX][PlayFieldY] = 20;
                result = true;
                break;
        case 18: PlayField[PlayFieldX][PlayFieldY] = 17;
                result = true;
                break;
        case 19: PlayField[PlayFieldX][PlayFieldY] = 18;
                result = true;
                break;
        case 20: PlayField[PlayFieldX][PlayFieldY] = 19;
                result = true;
                break;
        case 21: PlayField[PlayFieldX][PlayFieldY] = 24;
                result = true;
                break;
        case 22: PlayField[PlayFieldX][PlayFieldY] = 21;
                result = true;
                break;
        case 23: PlayField[PlayFieldX][PlayFieldY] = 22;
                result = true;
                break;
        case 24: PlayField[PlayFieldX][PlayFieldY] = 23;
                result = true;
                break;
        case 25:
        case 26:
                result = true;
                break;

    }
    return result;
}


bool CWorld::Rotate2(int PlayFieldX, int PlayFieldY)
{
    bool result = false;
    switch(PlayField[PlayFieldX][PlayFieldY])
    {
        case 1: PlayField[PlayFieldX][PlayFieldY] = 2;
                result = true;
                break;
        case 2: PlayField[PlayFieldX][PlayFieldY] = 3;
                result = true;
                break;
        case 3: PlayField[PlayFieldX][PlayFieldY] = 4;
                result = true;
                break;
        case 4: PlayField[PlayFieldX][PlayFieldY] = 1;
                result = true;
                break;
        case 5: PlayField[PlayFieldX][PlayFieldY] = 6;
                result = true;
                break;
        case 6: PlayField[PlayFieldX][PlayFieldY] = 5;
                result = true;
                break;
        case 7: PlayField[PlayFieldX][PlayFieldY] = 8;
                result = true;
                break;
        case 8: PlayField[PlayFieldX][PlayFieldY] = 9;
                result = true;
                break;
        case 9: PlayField[PlayFieldX][PlayFieldY] = 10;
                result = true;
                break;
        case 10: PlayField[PlayFieldX][PlayFieldY] = 7;
                result = true;
                break;
        case 11: PlayField[PlayFieldX][PlayFieldY] = 12;
                result = true;
                break;
        case 12: PlayField[PlayFieldX][PlayFieldY] = 13;
                result = true;
                break;
        case 13: PlayField[PlayFieldX][PlayFieldY] = 14;
                result = true;
                break;
        case 14: PlayField[PlayFieldX][PlayFieldY] = 11;
                result = true;
                break;
        case 15: PlayField[PlayFieldX][PlayFieldY] = 16;
                result = true;
                break;
        case 16: PlayField[PlayFieldX][PlayFieldY] = 15;
                result = true;
                break;
        case 17: PlayField[PlayFieldX][PlayFieldY] = 18;
                result = true;
                break;
        case 18: PlayField[PlayFieldX][PlayFieldY] = 19;
                result = true;
                break;
        case 19: PlayField[PlayFieldX][PlayFieldY] = 20;
                result = true;
                break;
        case 20: PlayField[PlayFieldX][PlayFieldY] = 17;
                result = true;
                break;
        case 21: PlayField[PlayFieldX][PlayFieldY] = 22;
                result = true;
                break;
        case 22: PlayField[PlayFieldX][PlayFieldY] = 23;
                result = true;
                break;
        case 23: PlayField[PlayFieldX][PlayFieldY] = 24;
                result = true;
                break;
        case 24: PlayField[PlayFieldX][PlayFieldY] = 21;
                result = true;
                break;
        case 25:
        case 26:
                result = true;
                break;

    }
    return result;
}

bool CWorld::Slide(int SelectedX,int SelectedY)
{
    int X,Y,temp;
    bool result = false;

    if((SelectedX == StartPointX) || (SelectedY == StartPointY))
        return result;

    if((SelectedY == StartY-1) && (SelectedX >=StartX) && (SelectedX <= EndX))
    {
        temp = PlayField[SelectedX][StartY];
        for(Y=StartY;Y<EndY;Y++)
            PlayField[SelectedX][Y] = PlayField[SelectedX][Y+1];
        PlayField[SelectedX][EndY] = temp;
        result = true;
    }

    if((SelectedY == EndY+1) && (SelectedX >=StartX) && (SelectedX <= EndX))
    {
        temp = PlayField[SelectedX][EndY];
        for(Y=EndY;Y>StartY;Y--)
            PlayField[SelectedX][Y] = PlayField[SelectedX][Y-1];
        PlayField[SelectedX][StartY] = temp;
        result = true;
    }

    if((SelectedX == StartX-1)&& (SelectedY >=StartY) && (SelectedY <= EndY))
    {
        temp = PlayField[StartX][SelectedY];
        for(X=StartX;X<EndX;X++)
            PlayField[X][SelectedY] = PlayField[X+1][SelectedY];
        PlayField[EndX][SelectedY] = temp;
        result = true;
    }

    if((SelectedX == EndX+1) && (SelectedY >=StartY) && (SelectedY <= EndY))
    {
        temp = PlayField[EndX][SelectedY];
        for(X=EndX;X>StartX;X--)
            PlayField[X][SelectedY] = PlayField[X-1][SelectedY];
        PlayField[StartX][SelectedY] = temp;
        result = true;
    }

    return result;

}

void CWorld::DrawArrows(SDL_Surface *Surface,int SelectedX, int SelectedY)
{

    int X,Y;
    SDL_Rect DstRect,SrcRect;
    if(StartY>0)
        for(X=StartX;X<=EndX;X++)
        {
            if (X !=StartPointX)
            {
                DstRect.x = X * TileWidth;
                DstRect.y = (StartY-1) * TileHeight;
                DstRect.w = TileWidth;
                DstRect.h = TileHeight;
                if((X==SelectedX) && (SelectedY==StartY-1) )
                {
                    SrcRect.x = 72*SCREENPOSMULTIPLY,
                    SrcRect.y = 0;
                    SrcRect.w = TileWidth;
                    SrcRect.h = TileHeight;
                    SDL_BlitSurface(IMGSkinArrows,&SrcRect,Surface,&DstRect);
                }
                else
                {
                    SrcRect.x = 72*SCREENPOSMULTIPLY,
                    SrcRect.y = 24*SCREENPOSMULTIPLY;
                    SrcRect.w = TileWidth;
                    SrcRect.h = TileHeight;
                    SDL_BlitSurface(IMGSkinArrows,&SrcRect,Surface,&DstRect);
                }
            }
        }

    if(StartY<NumRows-1)
        for(X=StartX;X<=EndX;X++)
        {
            if (X != StartPointX)
            {
                DstRect.x = X * TileWidth;
                DstRect.y = (EndY+1) * TileHeight;
                DstRect.w = TileWidth;
                DstRect.h = TileHeight;
                if((X==SelectedX) && (SelectedY==EndY+1))
                {
                    SrcRect.x = 24*SCREENPOSMULTIPLY,
                    SrcRect.y = 0;
                    SrcRect.w = TileWidth;
                    SrcRect.h = TileHeight;
                    SDL_BlitSurface(IMGSkinArrows,&SrcRect,Surface,&DstRect);
                }
                else
                {
                    SrcRect.x = 24*SCREENPOSMULTIPLY,
                    SrcRect.y = 24*SCREENPOSMULTIPLY;
                    SrcRect.w = TileWidth;
                    SrcRect.h = TileHeight;
                    SDL_BlitSurface(IMGSkinArrows,&SrcRect,Surface,&DstRect);
                }
            }
        }

    if(StartX>0)
        for(Y=StartY;Y<=EndY;Y++)
        {
            if(Y != StartPointY)
            {
                DstRect.x = (StartX-1) * TileWidth;
                DstRect.y = Y * TileHeight;
                DstRect.w = TileWidth;
                DstRect.h = TileHeight;
                if((Y==SelectedY) && (SelectedX==StartX-1)  )
                {
                    SrcRect.x = 0,
                    SrcRect.y = 0;
                    SrcRect.w = TileWidth;
                    SrcRect.h = TileHeight;
                    SDL_BlitSurface(IMGSkinArrows,&SrcRect,Surface,&DstRect);
                }
                else
                {
                    SrcRect.x = 0,
                    SrcRect.y = 24*SCREENPOSMULTIPLY;
                    SrcRect.w = TileWidth;
                    SrcRect.h = TileHeight;
                    SDL_BlitSurface(IMGSkinArrows,&SrcRect,Surface,&DstRect);
                }
            }
        }

    if(StartX<NumCols-1)
        for(Y=StartY;Y<=EndY;Y++)
        {
            if (Y != StartPointY)
            {
                DstRect.x = (EndX+1) * TileWidth;
                DstRect.y = Y * TileHeight;
                DstRect.w = TileWidth;
                DstRect.h = TileHeight;
                if((Y==SelectedY) && (SelectedX==EndX+1))
                {
                    SrcRect.x = 48*SCREENPOSMULTIPLY,
                    SrcRect.y = 0;
                    SrcRect.w = TileWidth;
                    SrcRect.h = TileHeight;
                    SDL_BlitSurface(IMGSkinArrows,&SrcRect,Surface,&DstRect);
                }
                else
                {
                    SrcRect.x = 48*SCREENPOSMULTIPLY,
                    SrcRect.y = 24*SCREENPOSMULTIPLY;
                    SrcRect.w = TileWidth;
                    SrcRect.h = TileHeight;
                    SDL_BlitSurface(IMGSkinArrows,&SrcRect,Surface,&DstRect);
                }
            }
        }

}

void CWorld::DrawConnectedRecurse( SDL_Surface *Surface,int X,int Y)
{
    SDL_Rect DstRect,SrcRect;
    if ((X >= StartX) && (X <= EndX) && (Y>=StartY) && (Y<=EndY) && (PlayFieldConnected[X][Y] == 0))
    {

        PlayFieldConnected[X][Y] = 1;
        DstRect.x = X * TileWidth;
        DstRect.y = Y * TileHeight;
        DstRect.w = TileWidth;
        DstRect.h = TileHeight;
        SrcRect.x = ((PlayField[X][Y] -1) & 3)*TileWidth;
        SrcRect.y = ((PlayField[X][Y] -1) / 4)*TileHeight;
        SrcRect.w = TileWidth;
        SrcRect.h = TileHeight;
        SDL_BlitSurface(IMGSkinTubesConnected,&SrcRect,Surface,&DstRect);
        if(Blocks[PlayField[X][Y]].left)
            if(X-1>= 0)
                if (PlayFieldConnected[X-1][Y] == 0)
                    if(Blocks[PlayField[X-1][Y]].right)
                    {
                        DrawConnectedRecurse(Surface,X-1,Y);
                    }
        if(Blocks[PlayField[X][Y]].right)
            if(X+1< NumCols)
                if (PlayFieldConnected[X+1][Y] == 0)
                    if(Blocks[PlayField[X+1][Y]].left)
                    {
                        DrawConnectedRecurse(Surface,X+1,Y);
                    }
        if(Blocks[PlayField[X][Y]].up)
            if(Y-1 >= 0)
                if (PlayFieldConnected[X][Y-1] == 0)
                    if(Blocks[PlayField[X][Y-1]].down)
                    {
                        DrawConnectedRecurse(Surface,X,Y-1);
                    }
        if(Blocks[PlayField[X][Y]].down)
            if(Y+1< NumRows)
            if (PlayFieldConnected[X][Y+1] == 0)
                if(Blocks[PlayField[X][Y+1]].up)
                {
                    DrawConnectedRecurse(Surface,X,Y+1);
                }

    }
}

void CWorld::DrawConnected( SDL_Surface *Surface)
{
    int X,Y;
    for(Y=0;Y<NumRows;Y++)
        for(X=0;X<NumCols;X++)
            PlayFieldConnected[X][Y] = 0;
    DrawConnectedRecurse(Surface,StartPointX,StartPointY);
}

void CWorld::Draw(SDL_Surface *Surface)
{
    int X,Y;
    SDL_Rect DstRect,SrcRect;
    for (Y = StartY;Y<=EndY;Y++)
        for (X=StartX;X<=EndX;X++)
        {

            if(PlayField[X][Y] !=0)
            {
                DstRect.x = X * (TileWidth);
                DstRect.y = Y * (TileHeight);
                DstRect.w = TileWidth;
                DstRect.h = TileHeight;
                SrcRect.x = ((PlayField[X][Y] -1) %4)*(TileWidth);
                SrcRect.y = ((PlayField[X][Y] -1) / 4)*(TileHeight);
                SrcRect.w = TileWidth;
                SrcRect.h = TileHeight;
                SDL_BlitSurface(IMGSkinTubes,&SrcRect,Surface,&DstRect);
            }
            /*switch(PlayField[X][Y])
            {
                case 0: SDL_BlitSurface(Empty,NULL,Surface,&DstRect);
                        break;
                case 1: SDL_BlitSurface(StartBlock11,NULL,Surface,&DstRect);
                        break;
                case 2: SDL_BlitSurface(StartBlock12,NULL,Surface,&DstRect);
                        break;
                case 3: SDL_BlitSurface(StartBlock13,NULL,Surface,&DstRect);
                        break;
                case 4: SDL_BlitSurface(StartBlock14,NULL,Surface,&DstRect);
                        break;
                case 5: SDL_BlitSurface(StartBlock21,NULL,Surface,&DstRect);
                        break;
                case 6: SDL_BlitSurface(StartBlock22,NULL,Surface,&DstRect);
                        break;
                case 7: SDL_BlitSurface(StartBlock31,NULL,Surface,&DstRect);
                        break;
                case 8: SDL_BlitSurface(StartBlock32,NULL,Surface,&DstRect);
                        break;
                case 9: SDL_BlitSurface(StartBlock33,NULL,Surface,&DstRect);
                        break;
                case 10: SDL_BlitSurface(StartBlock34,NULL,Surface,&DstRect);
                        break;
                case 11: SDL_BlitSurface(Block11,NULL,Surface,&DstRect);
                        break;
                case 12: SDL_BlitSurface(Block12,NULL,Surface,&DstRect);
                        break;
                case 13: SDL_BlitSurface(Block13,NULL,Surface,&DstRect);
                        break;
                case 14: SDL_BlitSurface(Block14,NULL,Surface,&DstRect);
                        break;
                case 15: SDL_BlitSurface(Block21,NULL,Surface,&DstRect);
                        break;
                case 16: SDL_BlitSurface(Block22,NULL,Surface,&DstRect);
                        break;
                case 17: SDL_BlitSurface(Block31,NULL,Surface,&DstRect);
                        break;
                case 18: SDL_BlitSurface(Block32,NULL,Surface,&DstRect);
                        break;
                case 19: SDL_BlitSurface(Block33,NULL,Surface,&DstRect);
                        break;
                case 20: SDL_BlitSurface(Block34,NULL,Surface,&DstRect);
                        break;
                case 21: SDL_BlitSurface(EndBlock1,NULL,Surface,&DstRect);
                        break;
                case 22: SDL_BlitSurface(EndBlock2,NULL,Surface,&DstRect);
                        break;
                case 23: SDL_BlitSurface(EndBlock3,NULL,Surface,&DstRect);
                        break;
                case 24: SDL_BlitSurface(EndBlock4,NULL,Surface,&DstRect);
                        break;
                case 25: SDL_BlitSurface(Block4,NULL,Surface,&DstRect);
                        break;
                case 26: SDL_BlitSurface(StartBlock4,NULL,Surface,&DstRect);
                        break;

            }*/
        }
  /*  for (Y = 0;Y<NumRows;Y++)
        for (X=0;X<NumCols;X++)
        {
            if((PlayField[X][Y] >= 1) && (PlayField[X][Y] <=10) || (PlayField[X][Y] == 26))
            {
                if (SDL_MUSTLOCK(Surface))
                    SDL_LockSurface(Surface);
                SDL_FloodFill(Surface,(X*TileWidth)+12,(Y*TileHeight)+12,SDL_MapRGB(Surface->format,87,130,251),SDL_MapRGB(Surface->format,255,255,255));
                if (SDL_MUSTLOCK(Surface) )
                    SDL_UnlockSurface(Surface);
            }
        }*/
      /*  if (SDL_MUSTLOCK(Surface))
            SDL_LockSurface(Surface);
        SDL_FloodFill(Surface,(StartPointX*TileWidth)+12,(StartPointY*TileHeight)+12,SDL_MapRGB(Surface->format,87,130,251),SDL_MapRGB(Surface->format,255,255,255));
        if (SDL_MUSTLOCK(Surface) )
            SDL_UnlockSurface(Surface);
    */
}

bool CWorld::GenerateLevel(int Width,int Height)
{
    int x,y,Teller,TotalRooms,VisitedRooms,NeighboursFound,SelectedNeighbour,MazeHeight=Height,MazeWidth=Width;
    bool StartPointSet = false;
    vector<SPoint> CellStack;
    SPoint CurrentPoint;
    SPoint Neighbours[4];
    SMazeRoom Maze[MaxMazeWidth][MaxMazeHeight];
    for (y=0;y <MaxMazeHeight;y++)
        for (x=0;x < MaxMazeWidth;x++)
        {
            Maze[x][y].NorthWall = true;
            Maze[x][y].SouthWall = true;
            Maze[x][y].EastWall = true;
            Maze[x][y].WestWall = true;
            Maze[x][y].visited = false;
        }

    TotalRooms = MazeWidth * MazeHeight;
    CurrentPoint.X =rand() % MazeWidth;
    CurrentPoint.Y =rand() % MazeHeight;
    VisitedRooms = 1;
    while (VisitedRooms < TotalRooms)
    {
        NeighboursFound = 0;
        //left of current room
        if (CurrentPoint.X +1 < MazeWidth)
            if (Maze[CurrentPoint.X +1][CurrentPoint.Y].NorthWall &&
                Maze[CurrentPoint.X +1][CurrentPoint.Y].SouthWall &&
                Maze[CurrentPoint.X +1][CurrentPoint.Y].WestWall &&
                Maze[CurrentPoint.X +1][CurrentPoint.Y].EastWall)
            {
                Neighbours[NeighboursFound].X = CurrentPoint.X +1;
                Neighbours[NeighboursFound].Y = CurrentPoint.Y  ;
                NeighboursFound++;
            }

        if (CurrentPoint.X -1 >=0)
            if (Maze[CurrentPoint.X -1][CurrentPoint.Y].NorthWall &&
                Maze[CurrentPoint.X -1][CurrentPoint.Y].SouthWall &&
                Maze[CurrentPoint.X -1][CurrentPoint.Y].WestWall &&
                Maze[CurrentPoint.X -1][CurrentPoint.Y].EastWall)
            {
                Neighbours[NeighboursFound].X = CurrentPoint.X -1;
                Neighbours[NeighboursFound].Y = CurrentPoint.Y  ;
                NeighboursFound++;
            }

        if (CurrentPoint.Y -1 >= 0)
            if (Maze[CurrentPoint.X ][CurrentPoint.Y-1].NorthWall &&
                Maze[CurrentPoint.X ][CurrentPoint.Y-1].SouthWall &&
                Maze[CurrentPoint.X ][CurrentPoint.Y-1].WestWall &&
                Maze[CurrentPoint.X ][CurrentPoint.Y-1].EastWall)
            {
                Neighbours[NeighboursFound].X = CurrentPoint.X ;
                Neighbours[NeighboursFound].Y = CurrentPoint.Y -1;
                NeighboursFound++;
            }

        if (CurrentPoint.Y + 1 < MazeHeight)
            if (Maze[CurrentPoint.X ][CurrentPoint.Y+1].NorthWall &&
                Maze[CurrentPoint.X ][CurrentPoint.Y+1].SouthWall &&
                Maze[CurrentPoint.X ][CurrentPoint.Y+1].WestWall &&
                Maze[CurrentPoint.X ][CurrentPoint.Y+1].EastWall)
            {
                Neighbours[NeighboursFound].X = CurrentPoint.X ;
                Neighbours[NeighboursFound].Y = CurrentPoint.Y +1 ;
                NeighboursFound++;
            }

        if (NeighboursFound > 0)
        {
            SelectedNeighbour = rand() % NeighboursFound;

            // neighbour to the east
            if( Neighbours[SelectedNeighbour].X > CurrentPoint.X)
            {
                Maze[Neighbours[SelectedNeighbour].X][Neighbours[SelectedNeighbour].Y].WestWall = false;
                Maze[CurrentPoint.X][CurrentPoint.Y].EastWall = false;
            }
            else // neighbour to the west
                if( Neighbours[SelectedNeighbour].X < CurrentPoint.X)
                {
                    Maze[Neighbours[SelectedNeighbour].X][Neighbours[SelectedNeighbour].Y].EastWall  = false;
                    Maze[CurrentPoint.X][CurrentPoint.Y].WestWall = false;
                }
                else // neighbour to the north
                    if(Neighbours[SelectedNeighbour].Y < CurrentPoint.Y)
                    {
                        Maze[Neighbours[SelectedNeighbour].X][Neighbours[SelectedNeighbour].Y].SouthWall  = false;
                        Maze[CurrentPoint.X][CurrentPoint.Y].NorthWall = false;
                    }
                    else // neighbour to the south
                        if (Neighbours[SelectedNeighbour].Y > CurrentPoint.Y)
                        {
                            Maze[Neighbours[SelectedNeighbour].X][Neighbours[SelectedNeighbour].Y].NorthWall  = false;
                            Maze[CurrentPoint.X][CurrentPoint.Y].SouthWall  = false;
                        }

            CellStack.push_back(CurrentPoint);
            CurrentPoint.X =Neighbours[SelectedNeighbour].X;
            CurrentPoint.Y =Neighbours[SelectedNeighbour].Y;
            VisitedRooms = VisitedRooms + 1;
        }
        else
        {
            CurrentPoint = CellStack.back();
            CellStack.pop_back();
        }
    }
    CellStack.clear();

    for (y =0; y < NumRows ;y++)
        for (x= 0 ;x < NumCols;x++)
            PlayField[x][y] = 0;

    StartX=NumCols;
    StartY=NumRows;
    EndX = -1;
    EndY = -1;
    for (y =0; y < MazeHeight ;y++)
        for (x= 0 ;x < MazeWidth;x++)
        {
            for(Teller = 11; Teller <26;Teller++)
            {
                if ((Blocks[Teller].up !=  Maze[x][y].NorthWall) &&
                    (Blocks[Teller].down != Maze[x][y].SouthWall) &&
                    (Blocks[Teller].left  != Maze[x][y].WestWall) &&
                    (Blocks[Teller].right != Maze[x][y].EastWall))
                {
                    PlayField[x][y] = Teller;
                    if (PlayField[x][y] != 0)
                    {
                        if (x < StartX)
                            StartX  =  x;
                        if (y < StartY)
                            StartY = y;
                        if (x > EndX)
                            EndX = x;
                        if (y > EndY)
                            EndY = y;
                    }
                    break;
                }
            }


            if ((x+(y*MazeHeight) >= (int)(MazeWidth /2)+(int)(MazeHeight/2)*MazeHeight ) && !StartPointSet)
            {
                switch (PlayField[x][y])
                {
                    case 11:
                    case 12:
                    case 13:
                    case 14:
                    case 15:
                    case 16:
                    case 17:
                    case 18:
                    case 19:
                    case 20:
                        PlayField[x][y]= PlayField[x][y] - 10;
                        StartPointSet = true;
                        StartPointX = x;
                        StartPointY = y;
                        break;
                    case 25:
                        PlayField[x][y]=26;
                        StartPointSet = true;
                        StartPointX = x;
                        StartPointY = y;
                        break;
                }
            }
        }
    return true;

}

void CWorld::ShuffleGame2()
{
    int ShuffleCounter = 0;
    while (ShuffleCounter < 50)
    {
        switch (rand()%4)
        {
            case 0 :
                if(Slide(StartX + rand()%(EndX-StartX+1),StartY-1))
                    ShuffleCounter++;
                break;
            case 1 :
                if(Slide(StartX + rand()%(EndX-StartX+1),EndY+1))
                    ShuffleCounter++;
                break;
            case 2 :
                if(Slide(StartX -1,StartY + rand()%(EndY-StartY+1)))
                    ShuffleCounter++;
                break;
            case 3:
                if(Slide(EndX +1,StartY + rand()%(EndY-StartY+1)))
                    ShuffleCounter++;
        }
    }
}

void CWorld::ShuffleGame1()
{
    int x,y;
    for(y=0;y<NumRows;y++)
        for(x=0;x<NumCols;x++)
            switch( PlayField[x][y])
                    {
                        case 1:
                        case 2:
                        case 3:
                        case 4:
                            PlayField[x][y] = 1+ rand() % 4;
                            break;
                        case 5:
                        case 6:
                            PlayField[x][y] = 5+ rand()%2;
                            break;
                        case 7:
                        case 8:
                        case 9:
                        case 10:
                            PlayField[x][y] = 7+ rand()%4;
                            break;
                        case 11:
                        case 12:
                        case 13:
                        case 14:
                            PlayField[x][y] = 11+ rand()%4;
                            break;
                        case 15:
                        case 16:
                            PlayField[x][y] = 15+ rand()%2;
                            break;
                        case 17:
                        case 18:
                        case 19:
                        case 20:
                            PlayField[x][y] = 17 + rand() %4;
                            break;
                        case 21:
                        case 22:
                        case 23:
                        case 24:
                            PlayField[x][y] = 21 + rand()%4;
                            break;
                        case 25 :
                            PlayField[x][y] = 25;
                            break;
                    }
}


bool CWorld::LoadLevel(GameTypes SortOfGame, SortOfLevels Sort,int Number)
{
	int X,Y, Value;
	FILE *PFile;
	char FileName[FILENAME_MAX],Text[50];
	switch(SortOfGame)
	{
	    case Rotating:
            sprintf(Text,"rotating");
            break;
        case Sliding:
            sprintf(Text,"sliding");
            break;
        case RotatingSliding:
            sprintf(Text,"slidingrotating");
            break;
	}
	switch (Sort)
	{
	    case Custom:
            sprintf(FileName,"./levels/%s/custom/level%d.lev",Text,Number);
            break;
	    case Tutorial:
            sprintf(FileName,"./levels/%s/tutorial/level%d.lev",Text,Number);
            break;
        case Easy:
            sprintf(FileName,"./levels/%s/easy/level%d.lev",Text,Number);
            break;
        case Medium:
            sprintf(FileName,"./levels/%s/medium/level%d.lev",Text,Number);
            break;
        case Hard:
            sprintf(FileName,"./levels/%s/hard/level%d.lev",Text,Number);
            break;
        default:
            break;
	}

	PFile = fopen(FileName,"r");
	if(PFile)
	{
	    StartX=NumCols;
        StartY=NumRows;
        EndX = -1;
        EndY = -1;
		for (Y=0;Y <NumRows;Y++)
		{ 	for (X=0;X<NumCols;X++)
	 		{
	 			fscanf(PFile,"%d,",&Value);
	 			PlayField[X][Y] = Value;
	 			if (Value !=0)
	 			{
                    if (X < StartX)
                        StartX  =  X;
                    if (Y < StartY)
                        StartY = Y;
                    if (X > EndX)
                        EndX = X;
                    if (Y > EndY)
                        EndY = Y;
	 			}
	 			if (((Value >= 1) && (Value<11))|| (Value == 26))
	 			{
	 			    StartPointX = X;
	 			    StartPointY = Y;
	 			}
	 		}
		}
		fclose(PFile);
	}
	else
        return false;

    return true;
}

bool CWorld::LevelCompleted()
{
     int X,Y,HorizontalFalse=0,VerticalFalse=0;
     bool result = true;

    //standard check for all connected
     for (Y = StartY;Y<=EndY;Y++)
     {
        for (X=StartX;X<=EndX;X++)
        {
            if(X ==StartX)
                result = result && (!(Blocks[PlayField[X][Y]].left));
            if(X < EndX)
                result = result && (Blocks[PlayField[X][Y]].right == Blocks[PlayField[X+1][Y]].left );
            if (X == EndX)
                result = result && (!(Blocks[PlayField[X][Y]].right));
            if(Y == StartY)
                result = result && (!(Blocks[PlayField[X][Y]].up));
            if(Y < EndY)
                result = result && (Blocks[PlayField[X][Y]].down == Blocks[PlayField[X][Y+1]].up );
            if(Y == EndY)
                result = result && (!(Blocks[PlayField[X][Y]].down));
            if (!result)
                break;

        }
        if(!result)
            break;
     }

     //check for a horizontal gap
     for (Y = StartY;Y<EndY;Y++)
     {
        HorizontalFalse = 0;
        for (X=StartX;X<=EndX;X++)
        {
            if ((!Blocks[PlayField[X][Y]].down) && (!Blocks[PlayField[X][Y+1]].up ))
                HorizontalFalse++;
        }
        if (HorizontalFalse == EndX-StartX+1)
        {
            result = false;
            break;
        }
     }

     //check for a vertical gap
     for (X = StartX;X<EndX;X++)
     {
        VerticalFalse = 0;
        for (Y=StartY;Y<=EndY;Y++)
        {
            if ((!Blocks[PlayField[X][Y]].right) && (!Blocks[PlayField[X+1][Y]].left ))
                VerticalFalse++;
        }
        if (VerticalFalse == EndY-StartY+1)
        {
            result = false;
            break;
        }
     }

     //check if all tiles are flooded connected
     for (Y = StartY;Y<=EndY;Y++)
     {
        for (X=StartX;X<=EndX;X++)
        {
            if(PlayField[X][Y] > 0)
                if(PlayFieldConnected[X][Y] == 0)
                {
                    result = false;
                    break;
                }
        }
     }
    return result;

}

// set the volume of the music and both channels (left/right)
void SetVolume(const int VolumeIn)
{
	Mix_Volume(0,VolumeIn);
	Mix_Volume(1,VolumeIn);
	Mix_VolumeMusic(VolumeIn);
}

// increase the volume with 8
void IncVolume()
{
	if (Volume < 128)
	{
		Volume += 8;
		SetVolume(Volume);
	}
}

// decrease the volume with 8
void DecVolume()
{
	if (Volume > 0)
	{
		Volume-= 8;
		SetVolume(Volume);
	}
}

bool FileExists(char * FileName)
{
	FILE *Fp;
	Fp = fopen(FileName,"rb");
	if (Fp)
	{
		fclose(Fp);
		return true;
	}
	else
		return false;
}

void SearchForSkins()
{
	struct dirent *Entry;
	DIR *Directory;
	struct stat Stats;
	int Teller=0;
	char FileName[FILENAME_MAX];
	Directory = opendir("./skins");
	if (Directory)
	{
		Entry=readdir(Directory);
		while(Entry)
		{
			sprintf(FileName,"./skins/%s",Entry->d_name);
			stat(FileName,&Stats);
			if(S_ISDIR(Stats.st_mode))
			{
				//printf("%s\n",Entry->d_name);
				if(strncmp(".", Entry->d_name, 1)  && (Teller< MaxSkins) && (strlen(Entry->d_name) < FILENAME_MAX-1))
				{
					sprintf(InstalledSkins[Teller],"%s",Entry->d_name);
					Teller++;
				}
			}
			Entry=readdir(Directory);
		}
		closedir(Directory);
	}
	InstalledSkinsCount = Teller;
	SelectedSkin=-1;
}

void LoadGraphics()
{
    SDL_Surface *Tmp,*Tmp2;
    FILE *ColorFile;
    int R,G,B;
    char FileName[FILENAME_MAX];

    sprintf(FileName,"./skins/%s/color.txt",SkinName);
    ColorFile = fopen(FileName,"r");
 	if(ColorFile)
 	{
		fscanf(ColorFile,"R=%d\n",&R);
		fscanf(ColorFile,"G=%d\n",&G);
		fscanf(ColorFile,"B=%d\n",&B);
		fclose(ColorFile);
        TextColor.r = R;
 	    TextColor.g = G;
 	    TextColor.b = B;
 	}
 	else
 	{
 	    TextColor.r = 255;
 	    TextColor.g = 255;
 	    TextColor.b = 255;
 	}
    if (IMGSkinCursor)
        SDL_FreeSurface(IMGSkinCursor);

    if(IMGSkinTubesConnected)
        SDL_FreeSurface(IMGSkinTubesConnected);
    if(IMGSkinTitleScreen)
        SDL_FreeSurface(IMGSkinTitleScreen);

    if(IMGSkinPlay1)
        SDL_FreeSurface(IMGSkinPlay1);
    if(IMGSkinPlay2)
        SDL_FreeSurface(IMGSkinPlay2);
    if(IMGSkinCredits1)
        SDL_FreeSurface(IMGSkinCredits1);
    if(IMGSkinCredits2)
        SDL_FreeSurface(IMGSkinCredits2);
    if(IMGSkinQuit1)
        SDL_FreeSurface(IMGSkinQuit1);
    if(IMGSkinQuit2)
        SDL_FreeSurface(IMGSkinQuit2);

    if(IMGSkinHard)
        SDL_FreeSurface(IMGSkinHard);
    if(IMGSkinEasy)
        SDL_FreeSurface(IMGSkinEasy);
    if(IMGSkinMedium)
         SDL_FreeSurface(IMGSkinMedium);
    if(IMGSkinCustom)
        SDL_FreeSurface(IMGSkinCustom);
    if(IMGSkinTutorial)
        SDL_FreeSurface(IMGSkinTutorial);
    if(IMGSkinSelect)
        SDL_FreeSurface(IMGSkinSelect);
    if(IMGSkinRandomHard)
        SDL_FreeSurface(IMGSkinRandomHard);
    if(IMGSkinRandomMedium)
        SDL_FreeSurface(IMGSkinRandomMedium);
    if(IMGSkinRandomEasy)
        SDL_FreeSurface(IMGSkinRandomEasy);


    if(IMGSkinSelect2)
        SDL_FreeSurface(IMGSkinSelect2);
    if(IMGSkinRotating)
        SDL_FreeSurface(IMGSkinRotating);
    if(IMGSkinSliding)
        SDL_FreeSurface(IMGSkinSliding);
    if(IMGSkinSlidingRotating)
        SDL_FreeSurface(IMGSkinSlidingRotating);

    if(IMGSkinTubes)
        SDL_FreeSurface(IMGSkinTubes);
    if(IMGSkinArrows)
        SDL_FreeSurface(IMGSkinArrows);

    if(IMGSkinBackGround)
        SDL_FreeSurface(IMGSkinBackGround);

    if(IMGSkinL1)
        SDL_FreeSurface(IMGSkinL1);
    if(IMGSkinL2)
        SDL_FreeSurface(IMGSkinL2);
    if(IMGSkinE)
        SDL_FreeSurface(IMGSkinE);
    if(IMGSkinV)
        SDL_FreeSurface(IMGSkinV);
    if(IMGSkinC)
        SDL_FreeSurface(IMGSkinC);
    if(IMGSkinO)
        SDL_FreeSurface(IMGSkinO);
    if(IMGSkinM)
        SDL_FreeSurface(IMGSkinM);
    if(IMGSkinP)
        SDL_FreeSurface(IMGSkinP);
    if(IMGSkinT)
        SDL_FreeSurface(IMGSkinT);
    if(IMGSkinD)
        SDL_FreeSurface(IMGSkinD);

    if(IMGSkinCongrats)
        SDL_FreeSurface(IMGSkinCongrats);
    if(IMGSkinYouHave)
        SDL_FreeSurface(IMGSkinYouHave);
    if(IMGSkinEasyLevels)
        SDL_FreeSurface(IMGSkinEasyLevels);
    if(IMGSkinMediumLevels)
        SDL_FreeSurface(IMGSkinMediumLevels);
    if(IMGSkinHardLevels)
        SDL_FreeSurface(IMGSkinHardLevels);
    if(IMGSkinTutorialLevels)
        SDL_FreeSurface(IMGSkinTutorialLevels);
    if(IMGSkinCustomLevels)
        SDL_FreeSurface(IMGSkinCustomLevels);

    if(IMGSkinCreditsInfo1)
        SDL_FreeSurface(IMGSkinCreditsInfo1);
    if(IMGSkinCreditsInfo2)
        SDL_FreeSurface(IMGSkinCreditsInfo2);
    if(IMGSkinCreditsInfo3)
        SDL_FreeSurface(IMGSkinCreditsInfo3);


    if(IMGSkinIntro1)
        SDL_FreeSurface(IMGSkinIntro1);
    if(IMGSkinIntro2)
        SDL_FreeSurface(IMGSkinIntro2);
    if(IMGSkinIntro3)
        SDL_FreeSurface(IMGSkinIntro3);


    sprintf(FileName,"./skins/%s/cursor.png",SkinName);
    if (FileExists(FileName))
        Tmp = IMG_Load(FileName);
    else
        Tmp = IMG_Load("./graphics/cursor.png");
    if(SCREENPOSMULTIPLY > 1)
    {
        Tmp2 = SDL_DisplayFormat(Tmp);
        SDL_FreeSurface(Tmp);
        IMGSkinCursor = ScaleSurface(Tmp2,SCREENPOSMULTIPLY);
        SDL_FreeSurface(Tmp2);
    }
    else
    {
        IMGSkinCursor = SDL_DisplayFormat(Tmp);
        SDL_FreeSurface(Tmp);
    }
    SDL_SetColorKey(IMGSkinCursor, (SDL_SRCCOLORKEY|SDL_RLEACCEL),SDL_MapRGB(IMGSkinCursor->format,255,0,255));

    sprintf(FileName,"./skins/%s/tubesconnected.png",SkinName);
    if (FileExists(FileName))
        Tmp = IMG_Load(FileName);
    else
        Tmp = IMG_Load("./graphics/tubesconnected.png");
    if(SCREENPOSMULTIPLY > 1)
    {
        Tmp2 = SDL_DisplayFormat(Tmp);
        SDL_FreeSurface(Tmp);
        IMGSkinTubesConnected = ScaleSurface(Tmp2,SCREENPOSMULTIPLY);
        SDL_FreeSurface(Tmp2);
    }
    else
    {
        IMGSkinTubesConnected = SDL_DisplayFormat(Tmp);
        SDL_FreeSurface(Tmp);
    }
    SDL_SetColorKey(IMGSkinTubesConnected, (SDL_SRCCOLORKEY|SDL_RLEACCEL),SDL_MapRGB(IMGSkinTubesConnected->format,255,0,255));

    sprintf(FileName,"./skins/%s/titlescreen.png",SkinName);
    if (FileExists(FileName))
        Tmp = IMG_Load(FileName);
    else
        Tmp = IMG_Load("./graphics/titlescreen.png");
    if(SCREENPOSMULTIPLY > 1)
    {
        Tmp2 = SDL_DisplayFormat(Tmp);
        SDL_FreeSurface(Tmp);
        IMGSkinTitleScreen = ScaleSurface(Tmp2,SCREENPOSMULTIPLY);
        SDL_FreeSurface(Tmp2);
    }
    else
    {
        IMGSkinTitleScreen = SDL_DisplayFormat(Tmp);
        SDL_FreeSurface(Tmp);
    }

    sprintf(FileName,"./skins/%s/play1.png",SkinName);
    if (FileExists(FileName))
        Tmp = IMG_Load(FileName);
    else
        Tmp = IMG_Load("./graphics/play1.png");
    if(SCREENPOSMULTIPLY > 1)
    {
        Tmp2 = SDL_DisplayFormat(Tmp);
        SDL_FreeSurface(Tmp);
        IMGSkinPlay1 = ScaleSurface(Tmp2,SCREENPOSMULTIPLY);
        SDL_FreeSurface(Tmp2);
    }
    else
    {
        IMGSkinPlay1 = SDL_DisplayFormat(Tmp);
        SDL_FreeSurface(Tmp);
    }
    SDL_SetColorKey(IMGSkinPlay1,(SDL_SRCCOLORKEY|SDL_RLEACCEL),SDL_MapRGB(IMGSkinPlay1->format,255,0,255));



    sprintf(FileName,"./skins/%s/play2.png",SkinName);
    if (FileExists(FileName))
        Tmp = IMG_Load(FileName);
    else
        Tmp = IMG_Load("./graphics/play2.png");
    if(SCREENPOSMULTIPLY > 1)
    {
        Tmp2 = SDL_DisplayFormat(Tmp);
        SDL_FreeSurface(Tmp);
        IMGSkinPlay2 = ScaleSurface(Tmp2,SCREENPOSMULTIPLY);
        SDL_FreeSurface(Tmp2);
    }
    else
    {
        IMGSkinPlay2 = SDL_DisplayFormat(Tmp);
        SDL_FreeSurface(Tmp);
    }
    SDL_SetColorKey(IMGSkinPlay2,(SDL_SRCCOLORKEY|SDL_RLEACCEL),SDL_MapRGB(IMGSkinPlay2->format,255,0,255));



    sprintf(FileName,"./skins/%s/credits1.png",SkinName);
    if (FileExists(FileName))
        Tmp = IMG_Load(FileName);
    else
        Tmp = IMG_Load("./graphics/credits1.png");
    if(SCREENPOSMULTIPLY > 1)
    {
        Tmp2 = SDL_DisplayFormat(Tmp);
        SDL_FreeSurface(Tmp);
        IMGSkinCredits1 = ScaleSurface(Tmp2,SCREENPOSMULTIPLY);
        SDL_FreeSurface(Tmp2);
    }
    else
    {
        IMGSkinCredits1 = SDL_DisplayFormat(Tmp);
        SDL_FreeSurface(Tmp);
    }
    SDL_SetColorKey(IMGSkinCredits1,(SDL_SRCCOLORKEY|SDL_RLEACCEL),SDL_MapRGB(IMGSkinCredits1->format,255,0,255));



    sprintf(FileName,"./skins/%s/credits2.png",SkinName);
    if (FileExists(FileName))
        Tmp = IMG_Load(FileName);
    else
        Tmp = IMG_Load("./graphics/credits2.png");
    if(SCREENPOSMULTIPLY > 1)
    {
        Tmp2 = SDL_DisplayFormat(Tmp);
        SDL_FreeSurface(Tmp);
        IMGSkinCredits2 = ScaleSurface(Tmp2,SCREENPOSMULTIPLY);
        SDL_FreeSurface(Tmp2);
    }
    else
    {
        IMGSkinCredits2 = SDL_DisplayFormat(Tmp);
        SDL_FreeSurface(Tmp);
    }
    SDL_SetColorKey(IMGSkinCredits2,(SDL_SRCCOLORKEY|SDL_RLEACCEL),SDL_MapRGB(IMGSkinCredits2->format,255,0,255));



    sprintf(FileName,"./skins/%s/quit1.png",SkinName);
    if (FileExists(FileName))
        Tmp = IMG_Load(FileName);
    else
        Tmp = IMG_Load("./graphics/quit1.png");
    if(SCREENPOSMULTIPLY > 1)
    {
        Tmp2 = SDL_DisplayFormat(Tmp);
        SDL_FreeSurface(Tmp);
        IMGSkinQuit1 = ScaleSurface(Tmp2,SCREENPOSMULTIPLY);
        SDL_FreeSurface(Tmp2);
    }
    else
    {
        IMGSkinQuit1 = SDL_DisplayFormat(Tmp);
        SDL_FreeSurface(Tmp);
    }
    SDL_SetColorKey(IMGSkinQuit1,(SDL_SRCCOLORKEY|SDL_RLEACCEL),SDL_MapRGB(IMGSkinQuit1->format,255,0,255));


    sprintf(FileName,"./skins/%s/quit2.png",SkinName);
    if (FileExists(FileName))
        Tmp = IMG_Load(FileName);
    else
        Tmp = IMG_Load("./graphics/quit2.png");
     if(SCREENPOSMULTIPLY > 1)
    {
        Tmp2 = SDL_DisplayFormat(Tmp);
        SDL_FreeSurface(Tmp);
        IMGSkinQuit2 = ScaleSurface(Tmp2,SCREENPOSMULTIPLY);
        SDL_FreeSurface(Tmp2);
    }
    else
    {
        IMGSkinQuit2 = SDL_DisplayFormat(Tmp);
        SDL_FreeSurface(Tmp);
    }
    SDL_SetColorKey(IMGSkinQuit2,(SDL_SRCCOLORKEY|SDL_RLEACCEL),SDL_MapRGB(IMGSkinQuit2->format,255,0,255));


    sprintf(FileName,"./skins/%s/select.png",SkinName);
    if (FileExists(FileName))
        Tmp = IMG_Load(FileName);
    else
        Tmp = IMG_Load("./graphics/select.png");
     if(SCREENPOSMULTIPLY > 1)
    {
        Tmp2 = SDL_DisplayFormat(Tmp);
        SDL_FreeSurface(Tmp);
        IMGSkinSelect = ScaleSurface(Tmp2,SCREENPOSMULTIPLY);
        SDL_FreeSurface(Tmp2);
    }
    else
    {
        IMGSkinSelect = SDL_DisplayFormat(Tmp);
        SDL_FreeSurface(Tmp);
    }
    SDL_SetColorKey(IMGSkinSelect,(SDL_SRCCOLORKEY|SDL_RLEACCEL),SDL_MapRGB(IMGSkinSelect->format,255,0,255));



    sprintf(FileName,"./skins/%s/easy.png",SkinName);
    if (FileExists(FileName))
        Tmp = IMG_Load(FileName);
    else
        Tmp = IMG_Load("./graphics/easy.png");
    if(SCREENPOSMULTIPLY > 1)
    {
        Tmp2 = SDL_DisplayFormat(Tmp);
        SDL_FreeSurface(Tmp);
        IMGSkinEasy = ScaleSurface(Tmp2,SCREENPOSMULTIPLY);
        SDL_FreeSurface(Tmp2);
    }
    else
    {
        IMGSkinEasy = SDL_DisplayFormat(Tmp);
        SDL_FreeSurface(Tmp);
    }
    SDL_SetColorKey(IMGSkinEasy,(SDL_SRCCOLORKEY|SDL_RLEACCEL),SDL_MapRGB(IMGSkinEasy->format,255,0,255));



    sprintf(FileName,"./skins/%s/medium.png",SkinName);
    if (FileExists(FileName))
        Tmp = IMG_Load(FileName);
    else
        Tmp = IMG_Load("./graphics/medium.png");
    if(SCREENPOSMULTIPLY > 1)
    {
        Tmp2 = SDL_DisplayFormat(Tmp);
        SDL_FreeSurface(Tmp);
        IMGSkinMedium = ScaleSurface(Tmp2,SCREENPOSMULTIPLY);
        SDL_FreeSurface(Tmp2);
    }
    else
    {
        IMGSkinMedium = SDL_DisplayFormat(Tmp);
        SDL_FreeSurface(Tmp);
    }
    SDL_SetColorKey(IMGSkinMedium,(SDL_SRCCOLORKEY|SDL_RLEACCEL),SDL_MapRGB(IMGSkinMedium->format,255,0,255));



    sprintf(FileName,"./skins/%s/hard.png",SkinName);
    if (FileExists(FileName))
        Tmp = IMG_Load(FileName);
    else
        Tmp = IMG_Load("./graphics/hard.png");
    if(SCREENPOSMULTIPLY > 1)
    {
        Tmp2 = SDL_DisplayFormat(Tmp);
        SDL_FreeSurface(Tmp);
        IMGSkinHard = ScaleSurface(Tmp2,SCREENPOSMULTIPLY);
        SDL_FreeSurface(Tmp2);
    }
    else
    {
        IMGSkinHard = SDL_DisplayFormat(Tmp);
        SDL_FreeSurface(Tmp);
    }
    SDL_SetColorKey(IMGSkinHard,(SDL_SRCCOLORKEY|SDL_RLEACCEL),SDL_MapRGB(IMGSkinHard->format,255,0,255));



    sprintf(FileName,"./skins/%s/tutorial.png",SkinName);
    if (FileExists(FileName))
        Tmp = IMG_Load(FileName);
    else
        Tmp = IMG_Load("./graphics/tutorial.png");
    if(SCREENPOSMULTIPLY > 1)
    {
        Tmp2 = SDL_DisplayFormat(Tmp);
        SDL_FreeSurface(Tmp);
        IMGSkinTutorial = ScaleSurface(Tmp2,SCREENPOSMULTIPLY);
        SDL_FreeSurface(Tmp2);
    }
    else
    {
        IMGSkinTutorial = SDL_DisplayFormat(Tmp);
        SDL_FreeSurface(Tmp);
    }
    SDL_SetColorKey(IMGSkinTutorial,(SDL_SRCCOLORKEY|SDL_RLEACCEL),SDL_MapRGB(IMGSkinTutorial->format,255,0,255));



    sprintf(FileName,"./skins/%s/custom.png",SkinName);
    if (FileExists(FileName))
        Tmp = IMG_Load(FileName);
    else
        Tmp = IMG_Load("./graphics/custom.png");
    if(SCREENPOSMULTIPLY > 1)
    {
        Tmp2 = SDL_DisplayFormat(Tmp);
        SDL_FreeSurface(Tmp);
        IMGSkinCustom = ScaleSurface(Tmp2,SCREENPOSMULTIPLY);
        SDL_FreeSurface(Tmp2);
    }
    else
    {
        IMGSkinCustom = SDL_DisplayFormat(Tmp);
        SDL_FreeSurface(Tmp);
    }
    SDL_SetColorKey(IMGSkinCustom,(SDL_SRCCOLORKEY|SDL_RLEACCEL),SDL_MapRGB(IMGSkinCustom->format,255,0,255));


    sprintf(FileName,"./skins/%s/randomeasy.png",SkinName);
    if (FileExists(FileName))
        Tmp = IMG_Load(FileName);
    else
        Tmp = IMG_Load("./graphics/randomeasy.png");
    if(SCREENPOSMULTIPLY > 1)
    {
        Tmp2 = SDL_DisplayFormat(Tmp);
        SDL_FreeSurface(Tmp);
        IMGSkinRandomEasy = ScaleSurface(Tmp2,SCREENPOSMULTIPLY);
        SDL_FreeSurface(Tmp2);
    }
    else
    {
        IMGSkinRandomEasy = SDL_DisplayFormat(Tmp);
        SDL_FreeSurface(Tmp);
    }
    SDL_SetColorKey(IMGSkinRandomEasy,(SDL_SRCCOLORKEY|SDL_RLEACCEL),SDL_MapRGB(IMGSkinRandomEasy->format,255,0,255));



    sprintf(FileName,"./skins/%s/randommedium.png",SkinName);
    if (FileExists(FileName))
        Tmp = IMG_Load(FileName);
    else
        Tmp = IMG_Load("./graphics/randommedium.png");
    if(SCREENPOSMULTIPLY > 1)
    {
        Tmp2 = SDL_DisplayFormat(Tmp);
        SDL_FreeSurface(Tmp);
        IMGSkinRandomMedium = ScaleSurface(Tmp2,SCREENPOSMULTIPLY);
        SDL_FreeSurface(Tmp2);
    }
    else
    {
        IMGSkinRandomMedium = SDL_DisplayFormat(Tmp);
        SDL_FreeSurface(Tmp);
    }
    SDL_SetColorKey(IMGSkinRandomMedium,(SDL_SRCCOLORKEY|SDL_RLEACCEL),SDL_MapRGB(IMGSkinRandomMedium->format,255,0,255));



    sprintf(FileName,"./skins/%s/randomhard.png",SkinName);
    if (FileExists(FileName))
        Tmp = IMG_Load(FileName);
    else
        Tmp = IMG_Load("./graphics/randomhard.png");
    if(SCREENPOSMULTIPLY > 1)
    {
        Tmp2 = SDL_DisplayFormat(Tmp);
        SDL_FreeSurface(Tmp);
        IMGSkinRandomHard = ScaleSurface(Tmp2,SCREENPOSMULTIPLY);
        SDL_FreeSurface(Tmp2);
    }
    else
    {
        IMGSkinRandomHard = SDL_DisplayFormat(Tmp);
        SDL_FreeSurface(Tmp);
    }
    SDL_SetColorKey(IMGSkinRandomHard,(SDL_SRCCOLORKEY|SDL_RLEACCEL),SDL_MapRGB(IMGSkinRandomHard->format,255,0,255));



    sprintf(FileName,"./skins/%s/select2.png",SkinName);
    if (FileExists(FileName))
        Tmp = IMG_Load(FileName);
    else
        Tmp = IMG_Load("./graphics/select2.png");
    if(SCREENPOSMULTIPLY > 1)
    {
        Tmp2 = SDL_DisplayFormat(Tmp);
        SDL_FreeSurface(Tmp);
        IMGSkinSelect2 = ScaleSurface(Tmp2,SCREENPOSMULTIPLY);
        SDL_FreeSurface(Tmp2);
    }
    else
    {
        IMGSkinSelect2 = SDL_DisplayFormat(Tmp);
        SDL_FreeSurface(Tmp);
    }
    SDL_SetColorKey(IMGSkinSelect2,(SDL_SRCCOLORKEY|SDL_RLEACCEL),SDL_MapRGB(IMGSkinSelect2->format,255,0,255));


    sprintf(FileName,"./skins/%s/rotating.png",SkinName);
    if (FileExists(FileName))
        Tmp = IMG_Load(FileName);
    else
        Tmp = IMG_Load("./graphics/rotating.png");
    if(SCREENPOSMULTIPLY > 1)
    {
        Tmp2 = SDL_DisplayFormat(Tmp);
        SDL_FreeSurface(Tmp);
        IMGSkinRotating = ScaleSurface(Tmp2,SCREENPOSMULTIPLY);
        SDL_FreeSurface(Tmp2);
    }
    else
    {
        IMGSkinRotating = SDL_DisplayFormat(Tmp);
        SDL_FreeSurface(Tmp);
    }
    SDL_SetColorKey(IMGSkinRotating,(SDL_SRCCOLORKEY|SDL_RLEACCEL),SDL_MapRGB(IMGSkinRotating->format,255,0,255));



    sprintf(FileName,"./skins/%s/sliding.png",SkinName);
    if (FileExists(FileName))
        Tmp = IMG_Load(FileName);
    else
        Tmp = IMG_Load("./graphics/sliding.png");
    if(SCREENPOSMULTIPLY > 1)
    {
        Tmp2 = SDL_DisplayFormat(Tmp);
        SDL_FreeSurface(Tmp);
        IMGSkinSliding = ScaleSurface(Tmp2,SCREENPOSMULTIPLY);
        SDL_FreeSurface(Tmp2);
    }
    else
    {
        IMGSkinSliding = SDL_DisplayFormat(Tmp);
        SDL_FreeSurface(Tmp);
    }
    SDL_SetColorKey(IMGSkinSliding,(SDL_SRCCOLORKEY|SDL_RLEACCEL),SDL_MapRGB(IMGSkinSliding->format,255,0,255));



    sprintf(FileName,"./skins/%s/slidingrotating.png",SkinName);
    if (FileExists(FileName))
        Tmp = IMG_Load(FileName);
    else
        Tmp = IMG_Load("./graphics/slidingrotating.png");
    if(SCREENPOSMULTIPLY > 1)
    {
        Tmp2 = SDL_DisplayFormat(Tmp);
        SDL_FreeSurface(Tmp);
        IMGSkinSlidingRotating = ScaleSurface(Tmp2,SCREENPOSMULTIPLY);
        SDL_FreeSurface(Tmp2);
    }
    else
    {
        IMGSkinSlidingRotating = SDL_DisplayFormat(Tmp);
        SDL_FreeSurface(Tmp);
    }
    SDL_SetColorKey(IMGSkinSlidingRotating,(SDL_SRCCOLORKEY|SDL_RLEACCEL),SDL_MapRGB(IMGSkinSlidingRotating->format,255,0,255));



    sprintf(FileName,"./skins/%s/tubes.png",SkinName);
    if (FileExists(FileName))
        Tmp = IMG_Load(FileName);
    else
        Tmp = IMG_Load("./graphics/tubes.png");
    if(SCREENPOSMULTIPLY > 1)
    {
        Tmp2 = SDL_DisplayFormat(Tmp);
        SDL_FreeSurface(Tmp);
        IMGSkinTubes = ScaleSurface(Tmp2,SCREENPOSMULTIPLY);
        SDL_FreeSurface(Tmp2);
    }
    else
    {
        IMGSkinTubes = SDL_DisplayFormat(Tmp);
        SDL_FreeSurface(Tmp);
    }
    SDL_SetColorKey(IMGSkinTubes,SDL_SRCCOLORKEY,SDL_MapRGB(IMGSkinTubes->format,255,0,255));


    sprintf(FileName,"./skins/%s/arrows.png",SkinName);
    if (FileExists(FileName))
        Tmp = IMG_Load(FileName);
    else
        Tmp  = IMG_Load("./graphics/arrows.png");
    if(SCREENPOSMULTIPLY > 1)
    {
        Tmp2 = SDL_DisplayFormat(Tmp);
        SDL_FreeSurface(Tmp);
        IMGSkinArrows = ScaleSurface(Tmp2,SCREENPOSMULTIPLY);
        SDL_FreeSurface(Tmp2);
    }
    else
    {
        IMGSkinArrows = SDL_DisplayFormat(Tmp);
        SDL_FreeSurface(Tmp);
    }
    SDL_SetColorKey(IMGSkinArrows,SDL_SRCCOLORKEY,SDL_MapRGB(IMGSkinArrows->format,255,0,255));


    sprintf(FileName,"./skins/%s/nolevelsfound.png",SkinName);
    if (FileExists(FileName))
        Tmp = IMG_Load(FileName);
    else
        Tmp = IMG_Load("./graphics/nolevelsfound.png");
    if(SCREENPOSMULTIPLY > 1)
    {
        Tmp2 = SDL_DisplayFormat(Tmp);
        SDL_FreeSurface(Tmp);
        IMGSkinNoLevels = ScaleSurface(Tmp2,SCREENPOSMULTIPLY);
        SDL_FreeSurface(Tmp2);
    }
    else
    {
        IMGSkinNoLevels = SDL_DisplayFormat(Tmp);
        SDL_FreeSurface(Tmp);
    }
    SDL_SetColorKey(IMGSkinNoLevels,SDL_SRCCOLORKEY,SDL_MapRGB(IMGSkinNoLevels->format,255,0,255));


    sprintf(FileName,"./skins/%s/back.png",SkinName);
    if (FileExists(FileName))
        Tmp = IMG_Load(FileName);
    else
        Tmp = IMG_Load("./graphics/back.png");
    if(SCREENPOSMULTIPLY > 1)
    {
        Tmp2 = SDL_DisplayFormat(Tmp);
        SDL_FreeSurface(Tmp);
        IMGSkinBackGround = ScaleSurface(Tmp2,SCREENPOSMULTIPLY);
        SDL_FreeSurface(Tmp2);
    }
    else
    {
        IMGSkinBackGround = SDL_DisplayFormat(Tmp);
        SDL_FreeSurface(Tmp);
    }


    sprintf(FileName,"./skins/%s/creditsinfo1.png",SkinName);
    if (FileExists(FileName))
        Tmp = IMG_Load(FileName);
    else
        Tmp = IMG_Load("./graphics/creditsinfo1.png");
    if(SCREENPOSMULTIPLY > 1)
    {
        Tmp2 = SDL_DisplayFormatAlpha(Tmp);
        SDL_FreeSurface(Tmp);
        IMGSkinCreditsInfo1 = ScaleSurface(Tmp2,SCREENPOSMULTIPLY);
        SDL_FreeSurface(Tmp2);
    }
    else
    {
        IMGSkinCreditsInfo1 = SDL_DisplayFormatAlpha(Tmp);
        SDL_FreeSurface(Tmp);
    }


    sprintf(FileName,"./skins/%s/creditsinfo2.png",SkinName);
    if (FileExists(FileName))
        Tmp = IMG_Load(FileName);
    else
        Tmp = IMG_Load("./graphics/creditsinfo2.png");
    if(SCREENPOSMULTIPLY > 1)
    {
        Tmp2 = SDL_DisplayFormatAlpha(Tmp);
        SDL_FreeSurface(Tmp);
        IMGSkinCreditsInfo2 = ScaleSurface(Tmp2,SCREENPOSMULTIPLY);
        SDL_FreeSurface(Tmp2);
    }
    else
    {
        IMGSkinCreditsInfo2 = SDL_DisplayFormatAlpha(Tmp);
        SDL_FreeSurface(Tmp);
    }


    sprintf(FileName,"./skins/%s/creditsinfo3.png",SkinName);
    if (FileExists(FileName))
        Tmp = IMG_Load(FileName);
    else
        Tmp = IMG_Load("./graphics/creditsinfo3.png");
    if(SCREENPOSMULTIPLY > 1)
    {
        Tmp2 = SDL_DisplayFormatAlpha(Tmp);
        SDL_FreeSurface(Tmp);
        IMGSkinCreditsInfo3 = ScaleSurface(Tmp2,SCREENPOSMULTIPLY);
        SDL_FreeSurface(Tmp2);
    }
    else
    {
        IMGSkinCreditsInfo3 = SDL_DisplayFormatAlpha(Tmp);
        SDL_FreeSurface(Tmp);
    }

    sprintf(FileName,"./skins/%s/intro1.png",SkinName);
    if (FileExists(FileName))
        Tmp = IMG_Load(FileName);
    else
        Tmp = IMG_Load("./graphics/intro1.png");
    if(SCREENPOSMULTIPLY > 1)
    {
        Tmp2 = SDL_DisplayFormatAlpha(Tmp);
        SDL_FreeSurface(Tmp);
        IMGSkinIntro1 = ScaleSurface(Tmp2,SCREENPOSMULTIPLY);
        SDL_FreeSurface(Tmp2);
    }
    else
    {
        IMGSkinIntro1 = SDL_DisplayFormatAlpha(Tmp);
        SDL_FreeSurface(Tmp);
    }


    sprintf(FileName,"./skins/%s/intro2.png",SkinName);
    if (FileExists(FileName))
        Tmp = IMG_Load(FileName);
    else
        Tmp = IMG_Load("./graphics/intro2.png");
    if(SCREENPOSMULTIPLY > 1)
    {
        Tmp2 = SDL_DisplayFormatAlpha(Tmp);
        SDL_FreeSurface(Tmp);
        IMGSkinIntro2 = ScaleSurface(Tmp2,SCREENPOSMULTIPLY);
        SDL_FreeSurface(Tmp2);
    }
    else
    {
        IMGSkinIntro2 = SDL_DisplayFormatAlpha(Tmp);
        SDL_FreeSurface(Tmp);
    }


    sprintf(FileName,"./skins/%s/intro3.png",SkinName);
    if (FileExists(FileName))
        Tmp = IMG_Load(FileName);
    else
        Tmp = IMG_Load("./graphics/intro3.png");
    if(SCREENPOSMULTIPLY > 1)
    {
        Tmp2 = SDL_DisplayFormatAlpha(Tmp);
        SDL_FreeSurface(Tmp);
        IMGSkinIntro3 = ScaleSurface(Tmp2,SCREENPOSMULTIPLY);
        SDL_FreeSurface(Tmp2);
    }
    else
    {
        IMGSkinIntro3 = SDL_DisplayFormatAlpha(Tmp);
        SDL_FreeSurface(Tmp);
    }


    sprintf(FileName,"./skins/%s/l1.png",SkinName);
    if (FileExists(FileName))
        Tmp = IMG_Load(FileName);
    else
        Tmp = IMG_Load("./graphics/l1.png");
    if(SCREENPOSMULTIPLY > 1)
    {
        Tmp2 = ScaleSurface(Tmp,SCREENPOSMULTIPLY);
        IMGSkinL1 = SDL_DisplayFormatAlpha(Tmp2);
        SDL_FreeSurface(Tmp);
        SDL_FreeSurface(Tmp2);
    }
    else
    {
        IMGSkinL1 = SDL_DisplayFormatAlpha(Tmp);
        SDL_FreeSurface(Tmp);
    }

    sprintf(FileName,"./skins/%s/l2.png",SkinName);
    if (FileExists(FileName))
        Tmp = IMG_Load(FileName);
    else
        Tmp = IMG_Load("./graphics/l2.png");
    if(SCREENPOSMULTIPLY > 1)
    {
        Tmp2 = ScaleSurface(Tmp,SCREENPOSMULTIPLY);
        IMGSkinL2 = SDL_DisplayFormatAlpha(Tmp2);
        SDL_FreeSurface(Tmp);
        SDL_FreeSurface(Tmp2);
    }
    else
    {
        IMGSkinL2 = SDL_DisplayFormatAlpha(Tmp);
        SDL_FreeSurface(Tmp);
    }

    sprintf(FileName,"./skins/%s/e.png",SkinName);
    if (FileExists(FileName))
        Tmp = IMG_Load(FileName);
    else
        Tmp = IMG_Load("./graphics/e.png");
    if(SCREENPOSMULTIPLY > 1)
    {
        Tmp2 = ScaleSurface(Tmp,SCREENPOSMULTIPLY);
        IMGSkinE = SDL_DisplayFormatAlpha(Tmp2);
        SDL_FreeSurface(Tmp);
        SDL_FreeSurface(Tmp2);
    }
    else
    {
        IMGSkinE = SDL_DisplayFormatAlpha(Tmp);
        SDL_FreeSurface(Tmp);
    }

    sprintf(FileName,"./skins/%s/v.png",SkinName);
    if (FileExists(FileName))
        Tmp = IMG_Load(FileName);
    else
        Tmp = IMG_Load("./graphics/v.png");
    if(SCREENPOSMULTIPLY > 1)
    {
        Tmp2 = ScaleSurface(Tmp,SCREENPOSMULTIPLY);
        IMGSkinV = SDL_DisplayFormatAlpha(Tmp2);
        SDL_FreeSurface(Tmp);
        SDL_FreeSurface(Tmp2);
    }
    else
    {
        IMGSkinV = SDL_DisplayFormatAlpha(Tmp);
        SDL_FreeSurface(Tmp);
    }

    sprintf(FileName,"./skins/%s/c.png",SkinName);
    if (FileExists(FileName))
        Tmp = IMG_Load(FileName);
    else
        Tmp = IMG_Load("./graphics/c.png");
    if(SCREENPOSMULTIPLY > 1)
    {
        Tmp2 = ScaleSurface(Tmp,SCREENPOSMULTIPLY);
        IMGSkinC = SDL_DisplayFormatAlpha(Tmp2);
        SDL_FreeSurface(Tmp);
        SDL_FreeSurface(Tmp2);
    }
    else
    {
        IMGSkinC = SDL_DisplayFormatAlpha(Tmp);
        SDL_FreeSurface(Tmp);
    }

    sprintf(FileName,"./skins/%s/o.png",SkinName);
    if (FileExists(FileName))
        Tmp = IMG_Load(FileName);
    else
        Tmp = IMG_Load("./graphics/o.png");
    if(SCREENPOSMULTIPLY > 1)
    {
        Tmp2 = ScaleSurface(Tmp,SCREENPOSMULTIPLY);
        IMGSkinO = SDL_DisplayFormatAlpha(Tmp2);
        SDL_FreeSurface(Tmp);
        SDL_FreeSurface(Tmp2);
    }
    else
    {
        IMGSkinO = SDL_DisplayFormatAlpha(Tmp);
        SDL_FreeSurface(Tmp);
    }


    sprintf(FileName,"./skins/%s/m.png",SkinName);
    if (FileExists(FileName))
        Tmp = IMG_Load(FileName);
    else
        Tmp = IMG_Load("./graphics/m.png");
    if(SCREENPOSMULTIPLY > 1)
    {
        Tmp2 = ScaleSurface(Tmp,SCREENPOSMULTIPLY);
        IMGSkinM = SDL_DisplayFormatAlpha(Tmp2);
        SDL_FreeSurface(Tmp);
        SDL_FreeSurface(Tmp2);
    }
    else
    {
        IMGSkinM = SDL_DisplayFormatAlpha(Tmp);
        SDL_FreeSurface(Tmp);
    }


    sprintf(FileName,"./skins/%s/p.png",SkinName);
    if (FileExists(FileName))
        Tmp = IMG_Load(FileName);
    else
        Tmp = IMG_Load("./graphics/p.png");
    if(SCREENPOSMULTIPLY > 1)
    {
        Tmp2 = ScaleSurface(Tmp,SCREENPOSMULTIPLY);
        IMGSkinP = SDL_DisplayFormatAlpha(Tmp2);
        SDL_FreeSurface(Tmp);
        SDL_FreeSurface(Tmp2);
    }
    else
    {
        IMGSkinP = SDL_DisplayFormatAlpha(Tmp);
        SDL_FreeSurface(Tmp);
    }


    sprintf(FileName,"./skins/%s/t.png",SkinName);
    if (FileExists(FileName))
        Tmp = IMG_Load(FileName);
    else
        Tmp = IMG_Load("./graphics/t.png");
    if(SCREENPOSMULTIPLY > 1)
    {
        Tmp2 = ScaleSurface(Tmp,SCREENPOSMULTIPLY);
        IMGSkinT = SDL_DisplayFormatAlpha(Tmp2);
        SDL_FreeSurface(Tmp);
        SDL_FreeSurface(Tmp2);
    }
    else
    {
        IMGSkinT = SDL_DisplayFormatAlpha(Tmp);
        SDL_FreeSurface(Tmp);
    }

    sprintf(FileName,"./skins/%s/d.png",SkinName);
    if (FileExists(FileName))
        Tmp = IMG_Load(FileName);
    else
        Tmp = IMG_Load("./graphics/d.png");
    if(SCREENPOSMULTIPLY > 1)
    {
        Tmp2 = ScaleSurface(Tmp,SCREENPOSMULTIPLY);
        IMGSkinD = SDL_DisplayFormatAlpha(Tmp2);
        SDL_FreeSurface(Tmp);
        SDL_FreeSurface(Tmp2);
    }
    else
    {
        IMGSkinD = SDL_DisplayFormatAlpha(Tmp);
        SDL_FreeSurface(Tmp);
    }

    sprintf(FileName,"./skins/%s/congratulations.png",SkinName);
    if (FileExists(FileName))
        Tmp = IMG_Load(FileName);
    else
        Tmp = IMG_Load("./graphics/congratulations.png");
    if(SCREENPOSMULTIPLY > 1)
    {
        Tmp2 = ScaleSurface(Tmp,SCREENPOSMULTIPLY);
        IMGSkinCongrats = SDL_DisplayFormatAlpha(Tmp2);
        SDL_FreeSurface(Tmp);
        SDL_FreeSurface(Tmp2);
    }
    else
    {
        IMGSkinCongrats = SDL_DisplayFormatAlpha(Tmp);
        SDL_FreeSurface(Tmp);
    }

    sprintf(FileName,"./skins/%s/youhavefinishedthe.png",SkinName);
    if (FileExists(FileName))
        Tmp = IMG_Load(FileName);
    else
        Tmp = IMG_Load("./graphics/youhavefinishedthe.png");
    if(SCREENPOSMULTIPLY > 1)
    {
        Tmp2 = ScaleSurface(Tmp,SCREENPOSMULTIPLY);
        IMGSkinYouHave = SDL_DisplayFormatAlpha(Tmp2);
        SDL_FreeSurface(Tmp);
        SDL_FreeSurface(Tmp2);
    }
    else
    {
        IMGSkinYouHave = SDL_DisplayFormatAlpha(Tmp);
        SDL_FreeSurface(Tmp);
    }

    sprintf(FileName,"./skins/%s/easylevels.png",SkinName);
    if (FileExists(FileName))
        Tmp = IMG_Load(FileName);
    else
        Tmp = IMG_Load("./graphics/easylevels.png");
    if(SCREENPOSMULTIPLY > 1)
    {
        Tmp2 = ScaleSurface(Tmp,SCREENPOSMULTIPLY);
        IMGSkinEasyLevels = SDL_DisplayFormatAlpha(Tmp2);
        SDL_FreeSurface(Tmp);
        SDL_FreeSurface(Tmp2);
    }
    else
    {
        IMGSkinEasyLevels = SDL_DisplayFormatAlpha(Tmp);
        SDL_FreeSurface(Tmp);
    }

    sprintf(FileName,"./skins/%s/mediumlevels.png",SkinName);
    if (FileExists(FileName))
        Tmp = IMG_Load(FileName);
    else
        Tmp = IMG_Load("./graphics/mediumlevels.png");
    if(SCREENPOSMULTIPLY > 1)
    {
        Tmp2 = ScaleSurface(Tmp,SCREENPOSMULTIPLY);
        IMGSkinMediumLevels = SDL_DisplayFormatAlpha(Tmp2);
        SDL_FreeSurface(Tmp);
        SDL_FreeSurface(Tmp2);
    }
    else
    {
        IMGSkinMediumLevels = SDL_DisplayFormatAlpha(Tmp);
        SDL_FreeSurface(Tmp);
    }


    sprintf(FileName,"./skins/%s/hardlevels.png",SkinName);
    if (FileExists(FileName))
        Tmp = IMG_Load(FileName);
    else
        Tmp = IMG_Load("./graphics/hardlevels.png");
    if(SCREENPOSMULTIPLY > 1)
    {
        Tmp2 = ScaleSurface(Tmp,SCREENPOSMULTIPLY);
        IMGSkinHardLevels = SDL_DisplayFormatAlpha(Tmp2);
        SDL_FreeSurface(Tmp);
        SDL_FreeSurface(Tmp2);
    }
    else
    {
        IMGSkinHardLevels = SDL_DisplayFormatAlpha(Tmp);
        SDL_FreeSurface(Tmp);
    }

    sprintf(FileName,"./skins/%s/tutoriallevels.png",SkinName);
    if (FileExists(FileName))
        Tmp = IMG_Load(FileName);
    else
        Tmp = IMG_Load("./graphics/tutoriallevels.png");
    if(SCREENPOSMULTIPLY > 1)
    {
        Tmp2 = ScaleSurface(Tmp,SCREENPOSMULTIPLY);
        IMGSkinTutorialLevels = SDL_DisplayFormatAlpha(Tmp2);
        SDL_FreeSurface(Tmp);
        SDL_FreeSurface(Tmp2);
    }
    else
    {
        IMGSkinTutorialLevels = SDL_DisplayFormatAlpha(Tmp);
        SDL_FreeSurface(Tmp);
    }

    sprintf(FileName,"./skins/%s/customlevels.png",SkinName);
    if (FileExists(FileName))
        Tmp = IMG_Load(FileName);
    else
        Tmp = IMG_Load("./graphics/customlevels.png");
    if(SCREENPOSMULTIPLY > 1)
    {
        Tmp2 = ScaleSurface(Tmp,SCREENPOSMULTIPLY);
        IMGSkinCustomLevels = SDL_DisplayFormatAlpha(Tmp2);
        SDL_FreeSurface(Tmp);
        SDL_FreeSurface(Tmp2);
    }
    else
    {
        IMGSkinCustomLevels = SDL_DisplayFormatAlpha(Tmp);
        SDL_FreeSurface(Tmp);
    }


}

void UnLoadGraphics()
{

    SDL_FreeSurface(IMGSkinCursor);

    SDL_FreeSurface(IMGSkinTubesConnected);

    SDL_FreeSurface(IMGSkinTitleScreen);
    SDL_FreeSurface(IMGSkinPlay1);
    SDL_FreeSurface(IMGSkinPlay2);
    SDL_FreeSurface(IMGSkinCredits1);
    SDL_FreeSurface(IMGSkinCredits2);
    SDL_FreeSurface(IMGSkinQuit1);
    SDL_FreeSurface(IMGSkinQuit2);

    SDL_FreeSurface(IMGSkinHard);
    SDL_FreeSurface(IMGSkinEasy);
    SDL_FreeSurface(IMGSkinMedium);
    SDL_FreeSurface(IMGSkinCustom);
    SDL_FreeSurface(IMGSkinTutorial);
    SDL_FreeSurface(IMGSkinSelect);
    SDL_FreeSurface(IMGSkinRandomHard);
    SDL_FreeSurface(IMGSkinRandomMedium);
    SDL_FreeSurface(IMGSkinRandomEasy);


    SDL_FreeSurface(IMGSkinSelect2);
    SDL_FreeSurface(IMGSkinRotating);
    SDL_FreeSurface(IMGSkinSliding);
    SDL_FreeSurface(IMGSkinSlidingRotating);

    SDL_FreeSurface(IMGSkinTubes);
    SDL_FreeSurface(IMGSkinArrows);

    SDL_FreeSurface(IMGSkinBackGround);

    SDL_FreeSurface(IMGSkinL1);
    SDL_FreeSurface(IMGSkinL2);
    SDL_FreeSurface(IMGSkinE);
    SDL_FreeSurface(IMGSkinV);
    SDL_FreeSurface(IMGSkinC);
    SDL_FreeSurface(IMGSkinO);
    SDL_FreeSurface(IMGSkinM);
    SDL_FreeSurface(IMGSkinP);
    SDL_FreeSurface(IMGSkinT);
    SDL_FreeSurface(IMGSkinD);

    SDL_FreeSurface(IMGSkinCongrats);
    SDL_FreeSurface(IMGSkinYouHave);
    SDL_FreeSurface(IMGSkinEasyLevels);
    SDL_FreeSurface(IMGSkinMediumLevels);
    SDL_FreeSurface(IMGSkinHardLevels);
    SDL_FreeSurface(IMGSkinTutorialLevels);
    SDL_FreeSurface(IMGSkinCustomLevels);

    SDL_FreeSurface(IMGSkinCreditsInfo1);
    SDL_FreeSurface(IMGSkinCreditsInfo2);
    SDL_FreeSurface(IMGSkinCreditsInfo3);


    SDL_FreeSurface(IMGSkinIntro1);
    SDL_FreeSurface(IMGSkinIntro2);
    SDL_FreeSurface(IMGSkinIntro3);


}


void LoadSettings()
{
 	FILE *SettingsFile;
 	int Value=1,Teller;
 	SettingsFile = fopen("./settings.dat","r");
 	if(SettingsFile)
 	{
		fscanf(SettingsFile,"Volume=%d\n",&Volume);
		SetVolume(Volume);
		if(TvOutMode)
		{
            fscanf(SettingsFile,"StartScreenX=%d\n",&StartScreenX);
            fscanf(SettingsFile,"StartScreenY=%d\n",&StartScreenY);
		}
		else
        {
            fscanf(SettingsFile,"StartScreenX=%d\n",&InitialStartScreenX);
            fscanf(SettingsFile,"StartScreenY=%d\n",&InitialStartScreenY);
            StartScreenX = 0;
            StartScreenY = 0;
        }
        fscanf(SettingsFile,"00=%d\n",&UnLockedLevels[Rotating][Easy]);
        fscanf(SettingsFile,"01=%d\n",&UnLockedLevels[Rotating][Medium]);
        fscanf(SettingsFile,"02=%d\n",&UnLockedLevels[Rotating][Hard]);
        fscanf(SettingsFile,"03=%d\n",&UnLockedLevels[Rotating][Tutorial]);

        fscanf(SettingsFile,"10=%d\n",&UnLockedLevels[Sliding][Easy]);
        fscanf(SettingsFile,"11=%d\n",&UnLockedLevels[Sliding][Medium]);
        fscanf(SettingsFile,"12=%d\n",&UnLockedLevels[Sliding][Hard]);
        fscanf(SettingsFile,"13=%d\n",&UnLockedLevels[Sliding][Tutorial]);

        fscanf(SettingsFile,"20=%d\n",&UnLockedLevels[RotatingSliding][Easy]);
        fscanf(SettingsFile,"21=%d\n",&UnLockedLevels[RotatingSliding][Medium]);
        fscanf(SettingsFile,"22=%d\n",&UnLockedLevels[RotatingSliding][Hard]);
        fscanf(SettingsFile,"23=%d\n",&UnLockedLevels[RotatingSliding][Tutorial]);
        fscanf(SettingsFile,"Skin=%s\n",SkinName);
        for(Teller=0;Teller<InstalledSkinsCount;Teller++)
        {
            if (strcmp(InstalledSkins[Teller],SkinName) == 0)
            {
                SelectedSkin = Teller;
                break;
            }
            else
                SelectedSkin = -1;
        }
        fclose(SettingsFile);
 	}
 	else
 	{
 	    UnLockedLevels[Rotating][Easy] = 1;
 	    UnLockedLevels[Rotating][Medium] = 1;
 	    UnLockedLevels[Rotating][Hard] = 1;
 	    UnLockedLevels[Rotating][Tutorial] = 1;
 	    //UnLockedLevels[Rotating][Custom] = 1;
 	    UnLockedLevels[Sliding][Easy] = 1;
 	    UnLockedLevels[Sliding][Medium] = 1;
 	    UnLockedLevels[Sliding][Hard] = 1;
 	    UnLockedLevels[Sliding][Tutorial] = 1;

 	    UnLockedLevels[RotatingSliding][Easy] = 1;
 	    UnLockedLevels[RotatingSliding][Medium] = 1;
 	    UnLockedLevels[RotatingSliding][Hard] = 1;
 	    UnLockedLevels[RotatingSliding][Tutorial] = 1;

 	    Volume = 128;
		SetVolume(Volume);
		StartScreenX = (Screen->w - Buffer->w) / 2;
        StartScreenY = (Screen->h - Buffer->h) / 2;
	}
}

void SaveSettings()
{
 	FILE *SettingsFile;
 	SettingsFile = fopen("./settings.dat","w");
 	if(SettingsFile)
 	{
		fprintf(SettingsFile,"Volume=%d\n",Volume);
        if(TvOutMode)
        {
            fprintf(SettingsFile,"StartScreenX=%d\n",StartScreenX);
            fprintf(SettingsFile,"StartScreenY=%d\n",StartScreenY);
        }
        else
        {
            fprintf(SettingsFile,"StartScreenX=%d\n",InitialStartScreenX);
            fprintf(SettingsFile,"StartScreenY=%d\n",InitialStartScreenY);
        }
        fprintf(SettingsFile,"00=%d\n",UnLockedLevels[Rotating][Easy]);
        fprintf(SettingsFile,"01=%d\n",UnLockedLevels[Rotating][Medium]);
        fprintf(SettingsFile,"02=%d\n",UnLockedLevels[Rotating][Hard]);
        fprintf(SettingsFile,"03=%d\n",UnLockedLevels[Rotating][Tutorial]);

        fprintf(SettingsFile,"10=%d\n",UnLockedLevels[Sliding][Easy]);
        fprintf(SettingsFile,"11=%d\n",UnLockedLevels[Sliding][Medium]);
        fprintf(SettingsFile,"12=%d\n",UnLockedLevels[Sliding][Hard]);
        fprintf(SettingsFile,"13=%d\n",UnLockedLevels[Sliding][Tutorial]);

        fprintf(SettingsFile,"20=%d\n",UnLockedLevels[RotatingSliding][Easy]);
        fprintf(SettingsFile,"21=%d\n",UnLockedLevels[RotatingSliding][Medium]);
        fprintf(SettingsFile,"22=%d\n",UnLockedLevels[RotatingSliding][Hard]);
        fprintf(SettingsFile,"23=%d\n",UnLockedLevels[RotatingSliding][Tutorial]);
        fprintf(SettingsFile,"Skin=%s\n",SkinName);
		fclose(SettingsFile);
		#ifdef GP2X
		sync();
		#endif
 	}
}




void WriteText(SDL_Surface* Surface,TTF_Font* FontIn,char* Tekst,int NrOfChars,int X,int Y,int YSpacing,SDL_Color ColorIn)
{
	char List[100][255];
	int Lines,Teller,Chars;
	SDL_Rect DstRect;
	SDL_Surface *TmpSurface1;
	Lines = 0;
	Chars = 0;
	for (Teller=0;Teller<NrOfChars;Teller++)
	{
		if(Lines > 100)
			break;
		if((Tekst[Teller] == '\n') || (Chars==255))
		{
			List[Lines][Chars]='\0';
			Lines++;
			Chars = 0;
		}
		else
		{
		 	List[Lines][Chars]=Tekst[Teller];
		 	Chars++;
		}
	}
	List[Lines][Chars] = '\0';
	for (Teller=0;Teller <= Lines;Teller++)
	{
		if(strlen(List[Teller]) > 0)
		{
			TmpSurface1 = TTF_RenderText_Blended(FontIn,List[Teller],ColorIn);
			if (TmpSurface1)
			{
				DstRect.x = X;
				DstRect.y = Y + (Teller) * TTF_FontLineSkip(FontIn) + (Teller*YSpacing);
				DstRect.w = TmpSurface1->w;
				DstRect.h = TmpSurface1->h;
				SDL_BlitSurface(TmpSurface1,NULL,Surface,&DstRect);
				SDL_FreeSurface(TmpSurface1);
			}
		}
	}
}

void NextSkin()
{
    char Text[500];
    SDL_Color Color = {255,255,255,255};
    SDL_Rect Rect;
    Rect.w = Buffer->w;
    Rect.h = Buffer->h;
    Rect.x = StartScreenX;
    Rect.y = StartScreenY;
    if(InstalledSkinsCount > 0)
    {
        SelectedSkin++;
        if (SelectedSkin >=InstalledSkinsCount)
            SelectedSkin = -1;
        if(SelectedSkin == -1)
        {
            sprintf(SkinName,"%s","");
            sprintf(Text,"Please Wait Loading Skin:\nDefault Skin");
        }
        else
        {
            sprintf(SkinName,"%s",InstalledSkins[SelectedSkin]);
            sprintf(Text,"Please Wait Loading Skin:\n%s",SkinName);
        }
        boxRGBA(Buffer,0,0,319*SCREENPOSMULTIPLY,48*SCREENPOSMULTIPLY,255,255,255,200);
        boxRGBA(Buffer,1*SCREENPOSMULTIPLY,1*SCREENPOSMULTIPLY,318*SCREENPOSMULTIPLY,47*SCREENPOSMULTIPLY,0,0,0,200);
        WriteText(Buffer,BigFont,Text,strlen(Text),5*SCREENPOSMULTIPLY,0,0,Color);
        SDL_FillRect(Buffer1,NULL,SDL_MapRGB(Buffer1->format,0,0,0));
        SDL_BlitSurface(Buffer,NULL,Buffer1,&Rect);
       /* #ifdef WIN32
            ScreenBufferZoom = zoomSurface(Buffer1,2,2,0);
            SDL_BlitSurface(ScreenBufferZoom,NULL,Screen,NULL);
            SDL_FreeSurface(ScreenBufferZoom);
        #else*/
        SDL_BlitSurface(Buffer1,NULL,Screen,NULL);
       // #endif
        SDL_Flip(Screen);
        LoadGraphics();
    }
}

void PreviousSkin()
{
    char Text[500];
    SDL_Color Color = {255,255,255,255};
    SDL_Rect Rect;
    Rect.w = Buffer->w;
    Rect.h = Buffer->h;
    Rect.x = StartScreenX;
    Rect.y = StartScreenY;
    if(InstalledSkinsCount > 0)
    {
        SelectedSkin--;
        if (SelectedSkin <=-2)
            SelectedSkin = InstalledSkinsCount-1;
        if(SelectedSkin == -1)
        {
            sprintf(SkinName,"%s","");
            sprintf(Text,"Please Wait Loading Skin:\nDefault Skin");
        }
        else
        {
            sprintf(SkinName,"%s",InstalledSkins[SelectedSkin]);
            sprintf(Text,"Please Wait Loading Skin:\n%s",SkinName);
        }
        boxRGBA(Buffer,0,0,319*SCREENPOSMULTIPLY,48*SCREENPOSMULTIPLY,255,255,255,200);
        boxRGBA(Buffer,1*SCREENPOSMULTIPLY,1*SCREENPOSMULTIPLY,318*SCREENPOSMULTIPLY,47*SCREENPOSMULTIPLY,0,0,0,200);
        WriteText(Buffer,BigFont,Text,strlen(Text),5*SCREENPOSMULTIPLY,0,0,Color);
        SDL_FillRect(Buffer1,NULL,SDL_MapRGB(Buffer1->format,0,0,0));
        SDL_BlitSurface(Buffer,NULL,Buffer1,&Rect);
       /* #ifdef WIN32
            ScreenBufferZoom = zoomSurface(Buffer1,2,2,0);
            SDL_BlitSurface(ScreenBufferZoom,NULL,Screen,NULL);
            SDL_FreeSurface(ScreenBufferZoom);
        #else*/
        SDL_BlitSurface(Buffer1,NULL,Screen,NULL);
       // #endif
        SDL_Flip(Screen);
        LoadGraphics();
    }
}


void TitleScreen()
{
    SDL_Rect Rect;
    SDL_Event Event;
    bool RPressed = false;
    SDL_Surface *Tmp;
    Tmp = SDL_DisplayFormat(Buffer);
    CMainMenu *Menu = new CMainMenu;
    CurrentMainMenu = Menu;
    Uint8 alpha = 0;
    while (GameState == GSTitleScreen)
    {
        if(GlobalSoundEnabled)
            if(!Mix_PlayingMusic())
                Mix_PlayMusic(Music[MUS_TITLE],0);
        while (SDL_PollEvent(&Event))
		{
		    if (Event.type == SDL_JOYBUTTONUP)
                switch (Event.jbutton.button)
                {
                    case GP2X_BUTTON_R:
                        RPressed = false;
                        break;
                    case GP2X_BUTTON_LEFT:
                        if (RPressed)
                            if(StartScreenX - 2 >= 0)
                                StartScreenX -=2;
                        break;
                    case GP2X_BUTTON_RIGHT:
                        if (RPressed)
                            if(StartScreenX + 2 + Buffer->w <= Screen->w)
                                StartScreenX +=2;
                        break;
                    case GP2X_BUTTON_UP:
                        if(RPressed)
                            if(StartScreenY - 2 >=0)
                                StartScreenY -=2;
                        break;
                    case GP2X_BUTTON_DOWN:
                        if(RPressed)
                            if(StartScreenY + 2 + Buffer->h <= Screen->h)
                                StartScreenY +=2;
                        break;
                }

            if (Event.type == SDL_QUIT)
                GameState=GSQuit;

            if (Event.type == SDL_KEYDOWN)
                switch (Event.key.keysym.sym)
                {
                    case DINGOO_BUTTON_SELECT:
                        GameState=GSQuit;
                        break;
                    case DINGOO_BUTTON_X:
                        IncVolume();
                        break;
                    case DINGOO_BUTTON_Y:
                        DecVolume();
                        break;
                    case DINGOO_BUTTON_R :
                        NextSkin();
                        break;
                    case DINGOO_BUTTON_L:
                        PreviousSkin();
                        break;
                    case DINGOO_BUTTON_UP:
                        Menu->PreviousItem();
                        break;
                    case DINGOO_BUTTON_DOWN:
                        Menu->NextItem();
                        break;
                    case DINGOO_BUTTON_A:
                        if (GlobalSoundEnabled)
                            Mix_PlayChannel(-1,Sounds[SND_SELECT],0);
                        switch(Menu->GetSelection())
                        {
                            case 1:
                                GameState = GSGameTypeMenu;
                                break;
                            case 2:
                                GameState = GSCredits;
                                break;
                            case 3:
                                GameState = GSQuit;
                                break;
                        }
                        break;
                    default:
                        printf("%d\n",Event.key.keysym.sym);
                        break;
                }

            if (Event.type == SDL_JOYBUTTONDOWN)
                switch (Event.jbutton.button)
                {
                    case GP2X_BUTTON_L:
                        PreviousSkin();
                        break;
                    case GP2X_BUTTON_R:
                        RPressed=true;
                        if(!TvOutMode)
                            NextSkin();
                        break;
                    case GP2X_BUTTON_VOLUP:
						IncVolume();
						break;
					case GP2X_BUTTON_VOLDOWN:
						DecVolume();
						break;
                    case GP2X_BUTTON_UP:
                        Menu->PreviousItem();
                        break;
                    case GP2X_BUTTON_DOWN:
                        Menu->NextItem();
                        break;
                    case GP2X_BUTTON_B:
                    case GP2X_BUTTON_X:
                    case GP2X_BUTTON_Y:
                    case GP2X_BUTTON_A:
                    case GP2X_BUTTON_START:
                        if (GlobalSoundEnabled)
                            Mix_PlayChannel(-1,Sounds[SND_SELECT],0);
                        switch(Menu->GetSelection())
                        {
                            case 1:
                                GameState = GSGameTypeMenu;
                                break;
                            case 2:
                                GameState = GSCredits;
                                break;
                            case 3:
                                GameState = GSQuit;
                                break;
                        }
                        break;
                    case GP2X_BUTTON_SELECT:
                        GameState = GSQuit;
                        break;
                }
        }
        Menu->Draw(Tmp);
        //tekenen naar buffer
        Rect.w = Buffer->w;
        Rect.h = Buffer->h;
        Rect.x = StartScreenX;
        Rect.y = StartScreenY;
        if (alpha < 255)
        {
            if(alpha+AlphaIncrease > MaxAlpha)
            {
                alpha = 255;
                SDL_SetAlpha(Tmp,SDL_SRCALPHA | SDL_RLEACCEL,alpha);
            }
            else
            {
                alpha+=10;
                SDL_SetAlpha(Tmp,SDL_SRCALPHA | SDL_RLEACCEL,alpha);
            }
        }
        SDL_BlitSurface(Tmp,NULL,Buffer,NULL);
        SDL_FillRect(Buffer1,NULL,SDL_MapRGB(Buffer1->format,0,0,0));

        SDL_BlitSurface(Buffer,NULL,Buffer1,&Rect);
      /*  #ifdef WIN32
            ScreenBufferZoom = zoomSurface(Buffer1,2,2,0);
            SDL_BlitSurface(ScreenBufferZoom,NULL,Screen,NULL);
            SDL_FreeSurface(ScreenBufferZoom);
        #else*/
        SDL_BlitSurface(Buffer1,NULL,Screen,NULL);
     //   #endif
        SDL_Flip(Screen);
        #ifdef WIN32
        SDL_Delay(10);
        #endif


    }
    CurrentMainMenu = NULL;
    delete Menu;
    SDL_FreeSurface(Tmp);
}

void NoLevelsFound()
{
    SDL_Surface *Tmp;
    SDL_Rect Rect,Rect1;
    SDL_Event Event;
    Uint8 alpha = 0;
    Tmp = SDL_DisplayFormat(Buffer);
    while (GameState == GSNoLevelsFound)
    {
        while (SDL_PollEvent(&Event))
		{
		   if (Event.type == SDL_JOYBUTTONDOWN)
                switch (Event.jbutton.button)
                {

                    case GP2X_BUTTON_L:
                        PreviousSkin();
                        break;
                    case GP2X_BUTTON_R:
                        NextSkin();
                        break;
                    case GP2X_BUTTON_VOLUP:
						IncVolume();
						break;
					case GP2X_BUTTON_VOLDOWN:
						DecVolume();
						break;
                    default :
                        GameState = GSLevelTypeMenu;
                        break;
                }

            if (Event.type == SDL_QUIT)
                GameState=GSQuit;

            if (Event.type == SDL_KEYDOWN)
                switch (Event.key.keysym.sym)
                {
                   case DINGOO_BUTTON_R :
                        NextSkin();
                        break;
                    case DINGOO_BUTTON_L:
                        PreviousSkin();
                        break;
                    case DINGOO_BUTTON_X:
                        IncVolume();
                        break;
                    case DINGOO_BUTTON_Y:
                        DecVolume();
                        break;
                    default :
                        GameState = GSLevelTypeMenu;
                        break;

                }
        }

        //tekenen naar buffer
        Rect.w = Buffer->w;
        Rect.h = Buffer->h;
        Rect.x = StartScreenX;
        Rect.y = StartScreenY;
        Rect1.y = 118*SCREENPOSMULTIPLY;
        Rect1.x = 38*SCREENPOSMULTIPLY;
        Rect1.w = IMGSkinNoLevels->w;
        Rect1.h = IMGSkinNoLevels->h;
        SDL_BlitSurface(IMGSkinTitleScreen,NULL,Tmp,NULL);
        SDL_BlitSurface(IMGSkinNoLevels,NULL,Tmp,&Rect1);
        if (alpha < 255)
        {
            if(alpha+AlphaIncrease > MaxAlpha)
            {
                alpha = 255;
                SDL_SetAlpha(Tmp,SDL_SRCALPHA | SDL_RLEACCEL,alpha);
            }
            else
            {
                alpha+=10;
                SDL_SetAlpha(Tmp,SDL_SRCALPHA | SDL_RLEACCEL,alpha);
            }
        }
        SDL_BlitSurface(Tmp,NULL,Buffer,NULL);
        SDL_FillRect(Buffer1,NULL,SDL_MapRGB(Buffer1->format,0,0,0));
        SDL_BlitSurface(Buffer,NULL,Buffer1,&Rect);
        /*#ifdef WIN32
            ScreenBufferZoom = zoomSurface(Buffer1,2,2,0);
            SDL_BlitSurface(ScreenBufferZoom,NULL,Screen,NULL);
            SDL_FreeSurface(ScreenBufferZoom);
        #else*/
        SDL_BlitSurface(Buffer1,NULL,Screen,NULL);
        //#endif
        SDL_Flip(Screen);
        #ifdef WIN32
        SDL_Delay(10);
        #endif
    }
    SDL_FreeSurface(Tmp);
}


void LevelTypeMenu()
{
    SDL_Rect Rect;
    SDL_Event Event;
    Uint8 alpha = 0;
    SDL_Surface *Tmp;
    Tmp = SDL_DisplayFormat(Buffer);
    CLevelTypeMenu *Menu = new CLevelTypeMenu(LevelType);
    while (GameState == GSLevelTypeMenu)
    {
         if(GlobalSoundEnabled)
            if(!Mix_PlayingMusic())
                Mix_PlayMusic(Music[MUS_TITLE],0);
        while (SDL_PollEvent(&Event))
		{

            if (Event.type == SDL_JOYBUTTONDOWN)
                switch (Event.jbutton.button)
                {
                    case GP2X_BUTTON_L:
                        PreviousSkin();
                        break;
                    case GP2X_BUTTON_R:
                        NextSkin();
                        break;

                   case GP2X_BUTTON_VOLUP:
						IncVolume();
						break;
					case GP2X_BUTTON_VOLDOWN:
						DecVolume();
						break;
                    case GP2X_BUTTON_LEFT:
                        Menu->PreviousItem();
                        break;
                    case GP2X_BUTTON_RIGHT:
                        Menu->NextItem();
                        break;
                    case GP2X_BUTTON_B:
                    case GP2X_BUTTON_X:
                    case GP2X_BUTTON_Y:
                    case GP2X_BUTTON_A:
                    case GP2X_BUTTON_START:
                        if (GlobalSoundEnabled)
                            Mix_PlayChannel(-1,Sounds[SND_SELECT],0);
                        switch(Menu->GetSelection())
                        {
                            case Easy:
                                LevelType = Easy;
                                break;
                            case Hard:
                                LevelType = Hard;
                                break;
                            case Medium:
                                LevelType = Medium;
                                break;
                            case Custom:
                                LevelType = Custom;
                                break;
                            case Tutorial:
                                LevelType = Tutorial;
                                break;
                            case RandomEasy:
                                LevelType = RandomEasy;
                                break;
                            case RandomMedium:
                                LevelType = RandomMedium;
                                break;
                            case RandomHard:
                                LevelType = RandomHard;
                                break;
                        }
                        Level = 1;
                        GameState = GSLevelSelector;
                        break;
                    case GP2X_BUTTON_SELECT:
                        GameState = GSGameTypeMenu;
                        break;
                }

            if (Event.type == SDL_KEYDOWN)
                switch (Event.key.keysym.sym)
                {
                    case DINGOO_BUTTON_X:
                        IncVolume();
                        break;
                    case DINGOO_BUTTON_Y:
                        DecVolume();
                        break;
                    case DINGOO_BUTTON_R :
                        NextSkin();
                        break;
                    case DINGOO_BUTTON_L:
                        PreviousSkin();
                        break;
                    case DINGOO_BUTTON_LEFT:
                        Menu->PreviousItem();
                        break;
                    case DINGOO_BUTTON_RIGHT:
                        Menu->NextItem();
                        break;
                    case DINGOO_BUTTON_SELECT:
                        GameState = GSGameTypeMenu;
                        break;
                    case DINGOO_BUTTON_A:
                        if (GlobalSoundEnabled)
                            Mix_PlayChannel(-1,Sounds[SND_SELECT],0);
                        switch(Menu->GetSelection())
                        {
                            case Easy:
                                LevelType = Easy;
                                break;
                            case Hard:
                                LevelType = Hard;
                                break;
                            case Medium:
                                LevelType = Medium;
                                break;
                            case Custom:
                                LevelType = Custom;
                                break;
                            case Tutorial:
                                LevelType = Tutorial;
                                break;
                            case RandomEasy:
                                LevelType = RandomEasy;
                                break;
                            case RandomMedium:
                                LevelType = RandomMedium;
                                break;
                            case RandomHard:
                                LevelType = RandomHard;
                                break;
                        }
                        Level = 1;
                        GameState = GSLevelSelector;
                        break;
                    default:
                        break;
                }

            if (Event.type == SDL_QUIT)
                GameState=GSQuit;

        }
        Menu->Draw(Tmp);
        //tekenen naar buffer
        Rect.w = Buffer->w;
        Rect.h = Buffer->h;
        Rect.x = StartScreenX;
        Rect.y = StartScreenY;
        if (alpha < 255)
        {
            if(alpha+AlphaIncrease > MaxAlpha)
            {
                alpha = 255;
                SDL_SetAlpha(Tmp,SDL_SRCALPHA | SDL_RLEACCEL,alpha);
            }
            else
            {
                alpha+=10;
                SDL_SetAlpha(Tmp,SDL_SRCALPHA | SDL_RLEACCEL,alpha);
            }
        }
        SDL_BlitSurface(Tmp,NULL,Buffer,NULL);
        SDL_FillRect(Buffer1,NULL,SDL_MapRGB(Buffer1->format,0,0,0));
        SDL_BlitSurface(Buffer,NULL,Buffer1,&Rect);
       /* #ifdef WIN32
            ScreenBufferZoom = zoomSurface(Buffer1,2,2,0);
            SDL_BlitSurface(ScreenBufferZoom,NULL,Screen,NULL);
            SDL_FreeSurface(ScreenBufferZoom);
        #else*/
        SDL_BlitSurface(Buffer1,NULL,Screen,NULL);
        //#endif
        SDL_Flip(Screen);
        #ifdef WIN32
        SDL_Delay(10);
        #endif

    }
    delete Menu;
    SDL_FreeSurface(Tmp);
}

void GameTypeMenu()
{
    SDL_Rect Rect;
    SDL_Event Event;
    Uint8 alpha = 0;
    SDL_Surface *Tmp;
    Tmp = SDL_DisplayFormat(Buffer);
    GameTypes PrevSelGameType = GameType;
    CGameTypeMenu *Menu = new CGameTypeMenu(GameType);
    while (GameState == GSGameTypeMenu)
    {
         if(GlobalSoundEnabled)
            if(!Mix_PlayingMusic())
                Mix_PlayMusic(Music[MUS_TITLE],0);
        while (SDL_PollEvent(&Event))
		{
            if (Event.type == SDL_QUIT)
                GameState=GSQuit;

            if (Event.type == SDL_JOYBUTTONDOWN)
                switch (Event.jbutton.button)
                {
                   case GP2X_BUTTON_L:
                        PreviousSkin();
                        break;
                    case GP2X_BUTTON_R:
                        NextSkin();
                        break;

                   case GP2X_BUTTON_VOLUP:
						IncVolume();
						break;
					case GP2X_BUTTON_VOLDOWN:
						DecVolume();
						break;
                    case GP2X_BUTTON_LEFT:
                        Menu->PreviousItem();
                        break;
                    case GP2X_BUTTON_RIGHT:
                        Menu->NextItem();
                        break;
                    case GP2X_BUTTON_B:
                    case GP2X_BUTTON_X:
                    case GP2X_BUTTON_Y:
                    case GP2X_BUTTON_A:
                    case GP2X_BUTTON_START:
                        if (GlobalSoundEnabled)
                            Mix_PlayChannel(-1,Sounds[SND_SELECT],0);
                        switch(Menu->GetSelection())
                        {
                            case Rotating:
                                GameType = Rotating;
                                break;
                            case Sliding:
                                GameType = Sliding;
                                break;
                            case RotatingSliding:
                                GameType = RotatingSliding;
                                break;
                        }
                        if (GameType != PrevSelGameType)
                            LevelType = Tutorial;
                        GameState = GSLevelTypeMenu;
                        break;
                    case GP2X_BUTTON_SELECT:
                        GameState = GSTitleScreen;
                        break;
                }

            if (Event.type == SDL_KEYDOWN)
                switch (Event.key.keysym.sym)
                {
                    case DINGOO_BUTTON_R :
                        NextSkin();
                        break;
                    case DINGOO_BUTTON_L:
                        PreviousSkin();
                        break;
                    case DINGOO_BUTTON_LEFT:
                        Menu->PreviousItem();
                        break;
                    case DINGOO_BUTTON_RIGHT:
                        Menu->NextItem();
                        break;
                    case DINGOO_BUTTON_X:
                        IncVolume();
                        break;
                    case DINGOO_BUTTON_Y:
                        DecVolume();
                        break;
                    case DINGOO_BUTTON_SELECT:
                        GameState = GSTitleScreen;
                        break;
                    case DINGOO_BUTTON_A:
                        if (GlobalSoundEnabled)
                            Mix_PlayChannel(-1,Sounds[SND_SELECT],0);
                        switch(Menu->GetSelection())
                        {
                            case Rotating:
                                GameType = Rotating;
                                break;
                            case Sliding:
                                GameType = Sliding;
                                break;
                            case RotatingSliding:
                                GameType = RotatingSliding;
                                break;
                        }
                        if (GameType != PrevSelGameType)
                            LevelType = Tutorial;
                        GameState = GSLevelTypeMenu;
                        break;
                    default:
                        break;
                }
        }
        Menu->Draw(Tmp);
        //tekenen naar buffer
        Rect.w = Buffer->w;
        Rect.h = Buffer->h;
        Rect.x = StartScreenX;
        Rect.y = StartScreenY;
        if (alpha < 255)
        {
            if(alpha+AlphaIncrease > MaxAlpha)
            {
                alpha = 255;
                SDL_SetAlpha(Tmp,SDL_SRCALPHA | SDL_RLEACCEL,alpha);
            }
            else
            {
                alpha+=10;
                SDL_SetAlpha(Tmp,SDL_SRCALPHA | SDL_RLEACCEL,alpha);
            }
        }
        SDL_BlitSurface(Tmp,NULL,Buffer,NULL);
        SDL_FillRect(Buffer1,NULL,SDL_MapRGB(Buffer1->format,0,0,0));
        SDL_BlitSurface(Buffer,NULL,Buffer1,&Rect);
      /*  #ifdef WIN32
            ScreenBufferZoom = zoomSurface(Buffer1,2,2,0);
            SDL_BlitSurface(ScreenBufferZoom,NULL,Screen,NULL);
            SDL_FreeSurface(ScreenBufferZoom);
        #else*/
        SDL_BlitSurface(Buffer1,NULL,Screen,NULL);
       // #endif
        SDL_Flip(Screen);
        #ifdef WIN32
        SDL_Delay(10);
        #endif

    }
    delete Menu;
    SDL_FreeSurface(Tmp);
}


void LevelCompleted()
{
    SDL_Event Event;
    SDL_Rect Rect,Rect1;
    char Text[500],Text1[50];
    bool ButtonPressed = false;
    int StartX = 4,StartY = 100,Teller=0,Teller1=-20,CompletedLevel = Level;
    //IMGLevelCompleted = IMG_Load("./graphics/test.png");
    //IMGLevelCompleted = SDL_DisplayFormat(Tmp);
    //SDL_FreeSurface(Tmp);
    Rect.w = Buffer->w;
    Rect.h = Buffer->h;
    Rect.x = StartScreenX;
    Rect.y = StartScreenY;
    if (GlobalSoundEnabled)
        Mix_PlayChannel(-1,Sounds[SND_WINNER],0);
    while (GameState == GSLevelCompleted)
    {
        while (SDL_PollEvent(&Event))
		{
		    if (Event.type == SDL_QUIT)
                GameState=GSQuit;

			if (Event.type == SDL_JOYBUTTONDOWN)
			{
				switch (Event.jbutton.button)
				{
				    case GP2X_BUTTON_L:
                        PreviousSkin();
                        break;
                    case GP2X_BUTTON_R:
                        NextSkin();
                        break;
				    case GP2X_BUTTON_VOLUP:
						IncVolume();
						break;
					case GP2X_BUTTON_VOLDOWN:
						DecVolume();
						break;
                    default:
                        if(!ButtonPressed)
                        {
                            if(LevelType < Custom)
                            {
                                if(Level == UnLockedLevels[GameType][LevelType])
                                {
                                    Level++;
                                    UnLockedLevels[GameType][LevelType]++;
                                }
                            }
                            else
                                if(LevelType == Custom)
                                    Level++;
                            ButtonPressed = true;
                        }
                        break;
				}
			}

			if (Event.type == SDL_KEYDOWN)
                switch (Event.key.keysym.sym)
                {
                    case DINGOO_BUTTON_X:
                        IncVolume();
                        break;
                    case DINGOO_BUTTON_Y:
                        DecVolume();
                        break;
                    case DINGOO_BUTTON_R :
                        NextSkin();
                        break;
                    case DINGOO_BUTTON_L:
                        PreviousSkin();
                        break;
                    default:
                        if(!ButtonPressed)
                        {
                            if(LevelType < Custom)
                            {
                                if(Level == UnLockedLevels[GameType][LevelType])
                                {
                                    Level++;
                                    UnLockedLevels[GameType][LevelType]++;
                                }
                            }
                            else
                                if(LevelType == Custom)
                                    Level++;
                            ButtonPressed = true;
                        }
                        break;
                }
		}
        SDL_BlitSurface(IMGSkinBackGround,NULL,Buffer,NULL);
        switch (GameType)
        {
            case Sliding :
                sprintf(Text1,"Sliding");
                break;
            case Rotating:
                sprintf(Text1,"Rotating");
                break;
            case RotatingSliding:
                sprintf(Text1,"Slide & Rotate");
                break;
        }
        switch(LevelType)
        {
            case Easy :
                sprintf(Text,"Game Type:\n%s\n\nLevel Type:\nEasy\n\nLevel Number:\n%d\n\nLevel Completed",Text1,CompletedLevel);
                WriteText(Buffer,font,Text,strlen(Text),243*SCREENPOSMULTIPLY,1*SCREENPOSMULTIPLY,0,TextColor);
                break;
            case Medium :
                sprintf(Text,"Game Type:\n%s\n\nLevel Type:\nMedium\n\nLevel Number:\n%d\n\nLevel Completed",Text1,CompletedLevel);
                WriteText(Buffer,font,Text,strlen(Text),243*SCREENPOSMULTIPLY,1*SCREENPOSMULTIPLY,0,TextColor);
                break;
            case Hard :
                sprintf(Text,"Game Type:\n%s\n\nLevel Type:\nHard\n\nLevel Number:\n%d\n\nLevel Completed",Text1,CompletedLevel);
                WriteText(Buffer,font,Text,strlen(Text),243*SCREENPOSMULTIPLY,1*SCREENPOSMULTIPLY,0,TextColor);
                break;
            case Tutorial :
                sprintf(Text,"Game Type:\n%s\n\nLevel Type:\nTutorial\n\nLevel Number:\n%d\n\nLevel Completed",Text1,CompletedLevel);
                WriteText(Buffer,font,Text,strlen(Text),243*SCREENPOSMULTIPLY,1*SCREENPOSMULTIPLY,0,TextColor);
                break;
            case Custom :
                sprintf(Text,"Game Type:\n%s\n\nLevel Type:\nCustom\n\nLevel Number:\n%d\n\nLevel Completed",Text1,CompletedLevel);
                WriteText(Buffer,font,Text,strlen(Text),243*SCREENPOSMULTIPLY,1*SCREENPOSMULTIPLY,0,TextColor);
                break;
            case RandomEasy :
                sprintf(Text,"Game Type:\n%s\n\nLevel Type:\nRandom Easy\n\nLevel Number:\n%d\n\nLevel Completed",Text1,CompletedLevel);
                WriteText(Buffer,font,Text,strlen(Text),243*SCREENPOSMULTIPLY,1*SCREENPOSMULTIPLY,0,TextColor);
                break;
            case RandomMedium :
                sprintf(Text,"Game Type:\n%s\n\nLevel Type:\nRandom Medium\n\nLevel Number:\n%d\n\nLevel Completed",Text1,CompletedLevel);
                WriteText(Buffer,font,Text,strlen(Text),243*SCREENPOSMULTIPLY,1*SCREENPOSMULTIPLY,0,TextColor);
                break;
            case RandomHard :
                sprintf(Text,"Game Type:\n%s\n\nLevel Type:\nRandom Hard\n\nLevel Number:\n%d\n\nLevel Completed",Text1,CompletedLevel);
                WriteText(Buffer,font,Text,strlen(Text),243*SCREENPOSMULTIPLY,1*SCREENPOSMULTIPLY,0,TextColor);
                break;
        }
        Teller++;


        if(Teller > 14*359)
            Teller = 1;
        if (Teller1 != 100 || ButtonPressed)
            StartY = Teller1;
        else
            StartY = Teller1 + (10)*Teller*(sin(Teller))/(2*Teller);
        World->Draw(Buffer);
        World->DrawConnected(Buffer);
        Rect1.w = IMGSkinL1->w;
        Rect1.h = IMGSkinL1->h;
        Rect1.x = StartX*SCREENPOSMULTIPLY;
        Rect1.y = StartY*SCREENPOSMULTIPLY;
        SDL_BlitSurface(IMGSkinL1,NULL,Buffer,&Rect1);
        Teller++;
        if(Teller > 7*359)
            Teller = 1;
        if (Teller1 != 100 || ButtonPressed)
            StartY = Teller1;
        else
            StartY = Teller1 + (10)*Teller*(sin(Teller))/(2*Teller);
        Rect1.w = IMGSkinE->w;
        Rect1.h = IMGSkinE->h;
        Rect1.x = (StartX+18)*SCREENPOSMULTIPLY;
        Rect1.y = (StartY+10)*SCREENPOSMULTIPLY;
        SDL_BlitSurface(IMGSkinE,NULL,Buffer,&Rect1);
        Teller++;
        if(Teller > 7*359)
            Teller = 1;
        if (Teller1 != 100 || ButtonPressed)
            StartY = Teller1;
        else
            StartY = Teller1 + (10)*Teller*(sin(Teller))/(2*Teller);
        Rect1.w = IMGSkinV->w;
        Rect1.h = IMGSkinV->h;
        Rect1.x = (StartX+34)*SCREENPOSMULTIPLY;
        Rect1.y = (StartY+10)*SCREENPOSMULTIPLY;
        SDL_BlitSurface(IMGSkinV,NULL,Buffer,&Rect1);
        Teller++;
        if(Teller > 7*359)
            Teller = 1;
        if (Teller1 != 100 || ButtonPressed)
            StartY = Teller1;
        else
           StartY = Teller1 + (10)*Teller*(sin(Teller))/(2*Teller);
        Rect1.w = IMGSkinE->w;
        Rect1.h = IMGSkinE->h;
        Rect1.x = (StartX+50)*SCREENPOSMULTIPLY;
        Rect1.y = (StartY+10)*SCREENPOSMULTIPLY;
        SDL_BlitSurface(IMGSkinE,NULL,Buffer,&Rect1);
        Teller++;
        if(Teller > 7*359)
            Teller = 1;
        if (Teller1 != 100 || ButtonPressed)
            StartY = Teller1;
        else
            StartY = Teller1 + (10)*Teller*(sin(Teller))/(2*Teller);
        Rect1.w = IMGSkinL2->w;
        Rect1.h = IMGSkinL2->h;
        Rect1.x = (StartX+66)*SCREENPOSMULTIPLY;
        Rect1.y = (StartY)*SCREENPOSMULTIPLY;
        SDL_BlitSurface(IMGSkinL2,NULL,Buffer,&Rect1);
        Teller++;
        if(Teller > 7*359)
            Teller = 1;
        if (Teller1 != 100 || ButtonPressed)
            StartY = Teller1;
        else
            StartY = Teller1 + (10)*Teller*(sin(Teller))/(2*Teller);
        Rect1.w = IMGSkinC->w;
        Rect1.h = IMGSkinC->h;
        Rect1.x = (StartX+84)*SCREENPOSMULTIPLY;
        Rect1.y = (StartY)*SCREENPOSMULTIPLY;
        SDL_BlitSurface(IMGSkinC,NULL,Buffer,&Rect1);
        Teller++;
        if(Teller > 7*359)
            Teller = 1;
        if (Teller1 != 100 || ButtonPressed)
            StartY = Teller1;
        else
            StartY = Teller1 + (10)*Teller*(sin(Teller))/(2*Teller);
        Rect1.w = IMGSkinO->w;
        Rect1.h = IMGSkinO->h;
        Rect1.x = (StartX+105)*SCREENPOSMULTIPLY;
        Rect1.y = (StartY+10)*SCREENPOSMULTIPLY;
        SDL_BlitSurface(IMGSkinO,NULL,Buffer,&Rect1);
        Teller++;
        if(Teller >7*359)
            Teller = 1;
        if (Teller1 != 100 || ButtonPressed)
            StartY = Teller1;
        else
            StartY = Teller1 + (10)*Teller*(sin(Teller))/(2*Teller);
        Rect1.w = IMGSkinM->w;
        Rect1.h = IMGSkinM->h;
        Rect1.x = (StartX+123)*SCREENPOSMULTIPLY;
        Rect1.y = (StartY+10)*SCREENPOSMULTIPLY;
        SDL_BlitSurface(IMGSkinM,NULL,Buffer,&Rect1);
        Teller++;
        if(Teller > 7*359)
            Teller = 1;
        if (Teller1 != 100 || ButtonPressed)
            StartY = Teller1;
        else
            StartY = Teller1 + (10)*Teller*(sin(Teller))/(2*Teller);
        Rect1.w = IMGSkinP->w;
        Rect1.h = IMGSkinP->h;
        Rect1.x = (StartX+147)*SCREENPOSMULTIPLY;
        Rect1.y = (StartY+10)*SCREENPOSMULTIPLY;
        SDL_BlitSurface(IMGSkinP,NULL,Buffer,&Rect1);
        Teller++;
        if(Teller > 7*359)
            Teller = 1;
        if (Teller1 != 100 || ButtonPressed)
            StartY = Teller1;
        else
            StartY = Teller1 + (10)*Teller*(sin(Teller))/(2*Teller);
        Rect1.w = IMGSkinL2->w;
        Rect1.h = IMGSkinL2->h;
        Rect1.x = (StartX+165)*SCREENPOSMULTIPLY;
        Rect1.y = (StartY)*SCREENPOSMULTIPLY;
        SDL_BlitSurface(IMGSkinL2,NULL,Buffer,&Rect1);
        Teller++;
        if(Teller > 7*359)
            Teller = 1;
        if (Teller1 != 100 || ButtonPressed)
            StartY = Teller1;
        else
            StartY = Teller1 + (10)*Teller*(sin(Teller))/(2*Teller);
        Rect1.w = IMGSkinE->w;
        Rect1.h = IMGSkinE->h;
        Rect1.x = (StartX+174)*SCREENPOSMULTIPLY;
        Rect1.y = (StartY+10)*SCREENPOSMULTIPLY;
        SDL_BlitSurface(IMGSkinE,NULL,Buffer,&Rect1);
        Teller++;
        if(Teller > 7*359)
            Teller = 1;
        if (Teller1 != 100 || ButtonPressed)
            StartY = Teller1;
        else
            StartY = Teller1 + (10)*Teller*(sin(Teller))/(2*Teller);
        Rect1.w = IMGSkinT->w;
        Rect1.h = IMGSkinT->h;
        Rect1.x = (StartX+190)*SCREENPOSMULTIPLY;
        Rect1.y = (StartY)*SCREENPOSMULTIPLY;
        SDL_BlitSurface(IMGSkinT,NULL,Buffer,&Rect1);
        Teller++;
        if(Teller > 7*359)
            Teller = 1;
        if (Teller1 != 100 || ButtonPressed)
            StartY = Teller1;
        else
            StartY = Teller1 + (10)*Teller*(sin(Teller))/(2*Teller);
        Rect1.w = IMGSkinE->w;
        Rect1.h = IMGSkinE->h;
        Rect1.x = (StartX+201)*SCREENPOSMULTIPLY;
        Rect1.y = (StartY+10)*SCREENPOSMULTIPLY;
        SDL_BlitSurface(IMGSkinE,NULL,Buffer,&Rect1);
        Teller++;
        if(Teller > 7*359)
            Teller = 1;
        if (Teller1 != 100 || ButtonPressed)
            StartY = Teller1;
        else
            StartY = Teller1 + (10)*Teller*(sin(Teller))/(2*Teller);
        Rect1.w = IMGSkinD ->w;
        Rect1.h = IMGSkinD ->h;
        Rect1.x = (StartX+217)*SCREENPOSMULTIPLY;
        Rect1.y = (StartY)*SCREENPOSMULTIPLY;
        SDL_BlitSurface(IMGSkinD ,NULL,Buffer,&Rect1);
        SDL_FillRect(Buffer1,NULL,SDL_MapRGB(Buffer1->format,0,0,0));
        SDL_BlitSurface(Buffer,NULL,Buffer1,&Rect);
       /* #ifdef WIN32
            ScreenBufferZoom = zoomSurface(Buffer1,2,2,0);
            SDL_BlitSurface(ScreenBufferZoom,NULL,Screen,NULL);
            SDL_FreeSurface(ScreenBufferZoom);
        #else*/
        SDL_BlitSurface(Buffer1,NULL,Screen,NULL);
     //   #endif
        SDL_Flip(Screen);
        #ifdef WIN32
        SDL_Delay(75);
        #else
        SDL_Delay(65);
        #endif
        if (Teller1 != 100 || ButtonPressed)
            Teller1+=5;
        if (Teller1 >= 260)
            GameState=GSLevelSelector;
    }

    //SDL_FreeSurface(IMGLevelCompleted);
}

void Game()
{
    SDL_Surface *Tmp;
    char Text [500],Text1[50],Text2[50],Text3[50];
    SDL_Rect Rect;
    int turns = 0,FrameCount=0,Fps=0;
    Uint32 Time;
    Uint8 alpha = 0;
    SDL_Event Event;
    CSelector *Selector;
    Selector = new CSelector(World->GetStartPointX(),World->GetStartPointY());
    CurrentSelector = Selector;
    //Tmp = SDL_CreateRGBSurface(SDL_HWSURFACE,WINDOW_WIDTH,WINDOW_HEIGHT,16,Screen->format->Rmask,Screen->format->Gmask,Screen->format->Bmask,Screen->format->Amask);
    Tmp = SDL_DisplayFormat(Buffer);
    Rect.w = Buffer->w;
    Rect.h = Buffer->h;
    Rect.x = StartScreenX;
    Rect.y = StartScreenY;
    //Time = SDL_GetTicks();
    while (GameState == GSGame)
    {
        if(GlobalSoundEnabled)
            if(!Mix_PlayingMusic())
                Mix_PlayMusic(Music[MUS_GAME],0);
        while (SDL_PollEvent(&Event))
		{
		    if (Event.type == SDL_QUIT)
                GameState=GSQuit;

			if (Event.type == SDL_JOYBUTTONDOWN)
			{
				switch (Event.jbutton.button)
				{
				    case GP2X_BUTTON_L:
                        PreviousSkin();
                        break;
                    case GP2X_BUTTON_R:
                        NextSkin();
                        break;
				    case GP2X_BUTTON_VOLUP:
						IncVolume();
						break;
					case GP2X_BUTTON_VOLDOWN:
						DecVolume();
						break;
					case GP2X_BUTTON_SELECT:
						GameState = GSLevelSelector;
						break;
                    case GP2X_BUTTON_LEFT:
                        Selector->SetPosition(Selector->GetPosition().X -1,Selector->GetPosition().Y);
                        break;
                    case GP2X_BUTTON_RIGHT:
                        Selector->SetPosition(Selector->GetPosition().X +1,Selector->GetPosition().Y);
                        break;
                    case GP2X_BUTTON_UP:
                        Selector->SetPosition(Selector->GetPosition().X,Selector->GetPosition().Y-1);
                        break;
                    case GP2X_BUTTON_DOWN:
                        Selector->SetPosition(Selector->GetPosition().X,Selector->GetPosition().Y+1);
                        break;
                    case GP2X_BUTTON_A:
                    case GP2X_BUTTON_X:
                        if((GameType == Rotating) || (GameType == RotatingSliding))
                        {
                            if (World->Rotate2(Selector->GetPosition().X,Selector->GetPosition().Y))
                            {
                                if (GlobalSoundEnabled)
                                Mix_PlayChannel(-1,Sounds[SND_ROTATE],0);
                                turns++;
                            }
                        }

                        if((GameType== Sliding) || (GameType == RotatingSliding))
                        {
                            if (World->Slide(Selector->GetPosition().X,Selector->GetPosition().Y))
                            {
                                if (GlobalSoundEnabled)
                                Mix_PlayChannel(-1,Sounds[SND_ROTATE],0);
                                turns++;
                            }
                        }
                        break;

                    case GP2X_BUTTON_Y:
                    case GP2X_BUTTON_B:
                        if((GameType == Rotating) || (GameType == RotatingSliding))
                        {
                            if (World->Rotate1(Selector->GetPosition().X,Selector->GetPosition().Y))
                            {
                                if (GlobalSoundEnabled)
                                Mix_PlayChannel(-1,Sounds[SND_ROTATE],0);
                                turns++;
                            }
                        }

                        if((GameType== Sliding) || (GameType == RotatingSliding))
                        {
                            if (World->Slide(Selector->GetPosition().X,Selector->GetPosition().Y))
                            {
                                if (GlobalSoundEnabled)
                                Mix_PlayChannel(-1,Sounds[SND_ROTATE],0);
                                turns++;
                            }
                        }

				}
			}

			if (Event.type == SDL_KEYDOWN)
                switch (Event.key.keysym.sym)
                {
                    case DINGOO_BUTTON_X:
                        IncVolume();
                        break;
                    case DINGOO_BUTTON_Y:
                        DecVolume();
                        break;
                    case DINGOO_BUTTON_SELECT:
                        GameState = GSLevelSelector;
                        break;
                    case DINGOO_BUTTON_R :
                        NextSkin();
                        break;
                    case DINGOO_BUTTON_L:
                        PreviousSkin();
                        break;
                     case DINGOO_BUTTON_LEFT:
                        Selector->SetPosition(Selector->GetPosition().X -1,Selector->GetPosition().Y);
                        break;
                    case DINGOO_BUTTON_RIGHT:
                        Selector->SetPosition(Selector->GetPosition().X +1,Selector->GetPosition().Y);
                        break;
                    case DINGOO_BUTTON_UP:
                        Selector->SetPosition(Selector->GetPosition().X,Selector->GetPosition().Y-1);
                        break;
                    case DINGOO_BUTTON_DOWN:
                        Selector->SetPosition(Selector->GetPosition().X,Selector->GetPosition().Y+1);
                        break;
                    case DINGOO_BUTTON_A:
                        if((GameType == Rotating) || (GameType == RotatingSliding))
                        {
                            if (World->Rotate1(Selector->GetPosition().X,Selector->GetPosition().Y))
                            {
                                if (GlobalSoundEnabled)
                                Mix_PlayChannel(-1,Sounds[SND_ROTATE],0);
                                turns++;
                            }
                        }

                        if((GameType== Sliding) || (GameType == RotatingSliding))
                        {
                            if (World->Slide(Selector->GetPosition().X,Selector->GetPosition().Y))
                            {
                                if (GlobalSoundEnabled)
                                Mix_PlayChannel(-1,Sounds[SND_ROTATE],0);
                                turns++;
                            }
                        }
                        break;
                    case DINGOO_BUTTON_B: // android port: rotate the other way (as on the GP2X)
                        if((GameType == Rotating) || (GameType == RotatingSliding))
                        {
                            if (World->Rotate2(Selector->GetPosition().X,Selector->GetPosition().Y))
                            {
                                if (GlobalSoundEnabled)
                                Mix_PlayChannel(-1,Sounds[SND_ROTATE],0);
                                turns++;
                            }
                        }

                        if((GameType== Sliding) || (GameType == RotatingSliding))
                        {
                            if (World->Slide(Selector->GetPosition().X,Selector->GetPosition().Y))
                            {
                                if (GlobalSoundEnabled)
                                Mix_PlayChannel(-1,Sounds[SND_ROTATE],0);
                                turns++;
                            }
                        }
                        break;
                    default:
                        break;
                }

		}
        SDL_BlitSurface(IMGSkinBackGround,NULL,Tmp,NULL);
        World->Draw(Tmp);
        World->DrawConnected(Tmp);
        switch (GameType)
        {
            case Sliding :
                sprintf(Text1,"Sliding");
                sprintf(Text2,"Slides:");
                sprintf(Text3,"To Slide:");
                break;
            case Rotating:
                sprintf(Text1,"Rotating");
                sprintf(Text2,"Turns:");
                sprintf(Text3,"To Rotate:");
                break;
            case RotatingSliding:
                sprintf(Text1,"Slide & Rotate");
                sprintf(Text2,"Slides + Turns:");
                sprintf(Text3,"To Slide/Rotate:");
                break;

        }
        switch(LevelType)
        {
            case Easy :
                sprintf(Text,"Game Type:\n%s\n\nLevel Type:\nEasy\n\nLevel Number:\n%d\n\n%s\n%d\n\n\n\n\n\nSelect with:\nJoystick\n\n%s\nPress Button",Text1,Level,Text2,turns,Text3);
                WriteText(Tmp,font,Text,strlen(Text),243*SCREENPOSMULTIPLY,1*SCREENPOSMULTIPLY,0,TextColor);
                break;
            case Medium :
                sprintf(Text,"Game Type:\n%s\n\nLevel Type:\nMedium\n\nLevel Number:\n%d\n\n%s\n%d\n\n\n\n\n\nSelect with:\nJoystick\n\n%s\nPress Button",Text1,Level,Text2,turns,Text3);
                WriteText(Tmp,font,Text,strlen(Text),243*SCREENPOSMULTIPLY,1*SCREENPOSMULTIPLY,0,TextColor);
                break;
            case Hard :
                sprintf(Text,"Game Type:\n%s\n\nLevel Type:\nHard\n\nLevel Number:\n%d\n\n%s\n%d\n\n\n\n\n\nSelect with:\nJoystick\n\n%s\nPress Button",Text1,Level,Text2,turns,Text3);
                WriteText(Tmp,font,Text,strlen(Text),243*SCREENPOSMULTIPLY,1*SCREENPOSMULTIPLY,0,TextColor);
                break;
            case Tutorial :
                sprintf(Text,"Game Type:\n%s\n\nLevel Type:\nTutorial\n\nLevel Number:\n%d\n\n%s\n%d\n\n\n\n\n\nSelect with:\nJoystick\n\n%s\nPress Button",Text1,Level,Text2,turns,Text3);
                WriteText(Tmp,font,Text,strlen(Text),243*SCREENPOSMULTIPLY,1*SCREENPOSMULTIPLY,0,TextColor);
                break;
            case Custom :
                sprintf(Text,"Game Type:\n%s\n\nLevel Type:\nCustom\n\nLevel Number:\n%d\n\n%s\n%d\n\n\n\n\n\nSelect with:\nJoystick\n\n%s\nPress Button",Text1,Level,Text2,turns,Text3);
                WriteText(Tmp,font,Text,strlen(Text),243*SCREENPOSMULTIPLY,1*SCREENPOSMULTIPLY,0,TextColor);
                break;
            case RandomEasy :
                sprintf(Text,"Game Type:\n%s\n\nLevel Type:\nRandom Easy\n\nLevel Number:\n%d\n\n%s\n%d\n\n\n\n\n\nSelect with:\nJoystick\n\n%s\nPress Button",Text1,Level,Text2,turns,Text3);
                WriteText(Tmp,font,Text,strlen(Text),243*SCREENPOSMULTIPLY,1*SCREENPOSMULTIPLY,0,TextColor);
                break;
            case RandomMedium :
                sprintf(Text,"Game Type:\n%s\n\nLevel Type:\nRandom Medium\n\nLevel Number:\n%d\n\n%s\n%d\n\n\n\n\n\nSelect with:\nJoystick\n\n%s\nPress Button",Text1,Level,Text2,turns,Text3);
                WriteText(Tmp,font,Text,strlen(Text),243*SCREENPOSMULTIPLY,1*SCREENPOSMULTIPLY,0,TextColor);
                break;
            case RandomHard :
                sprintf(Text,"Game Type:\n%s\n\nLevel Type:\nRandom Hard\n\nLevel Number:\n%d\n\n%s\n%d\n\n\n\n\n\nSelect with:\nJoystick\n\n%s\nPress Button",Text1,Level,Text2,turns,Text3);
                WriteText(Tmp,font,Text,strlen(Text),243*SCREENPOSMULTIPLY,1*SCREENPOSMULTIPLY,0,TextColor);
                break;
        }
       // sprintf(Text,"FPS : %d",Fps);
       //WriteText(Tmp,font,Text,strlen(Text),243*SCREENPOSMULTIPLY,140*SCREENPOSMULTIPLY,0,TextColor);
        //WriteText(Tmp,font,Text,strlen(Text),0,0,0,TextColor);
        if (World->LevelCompleted())
            GameState = GSLevelCompleted;
        if((GameType == Sliding) || (GameType == RotatingSliding))
            World->DrawArrows(Tmp,Selector->GetPosition().X,Selector->GetPosition().Y);
        Selector->Draw(Tmp);
        if (alpha < 255)
        {
            if(alpha+AlphaIncrease > MaxAlpha)
            {
                alpha = 255;
                SDL_SetAlpha(Tmp,SDL_SRCALPHA | SDL_RLEACCEL,alpha);
            }
            else
            {
                alpha+=10;
                SDL_SetAlpha(Tmp,SDL_SRCALPHA | SDL_RLEACCEL,alpha);
            }
        }
        SDL_BlitSurface(Tmp,NULL,Buffer,NULL);
       // SDL_FillRect(Buffer1,NULL,SDL_MapRGB(Buffer1->format,0,0,0));
        SDL_BlitSurface(Buffer,NULL,Buffer1,&Rect);
       /* #ifdef WIN32
            ScreenBufferZoom = ScaleSurface(Buffer1,SCREENPOSMULTIPLY);
            //ScreenBufferZoom = SDL_DisplayFormat(ScreenBufferZoomTmp);
            SDL_BlitSurface(ScreenBufferZoom,NULL,Screen,NULL);
            SDL_FreeSurface(ScreenBufferZoom);
           // SDL_FreeSurface(ScreenBufferZoomTmp);
        #else*/
            SDL_BlitSurface(Buffer1,NULL,Screen,NULL);
       // #endif
        SDL_Flip(Screen);
        #ifdef WIN32
        SDL_Delay(10);
        #endif
       // FrameCount++;
       /* if (Time +1000 < SDL_GetTicks())
        {
            Fps = FrameCount;
            FrameCount = 0;
            Time = SDL_GetTicks();
        }*/

    }
    CurrentSelector = NULL;
    delete Selector;
    SDL_FreeSurface(Tmp);
    if(GlobalSoundEnabled)
    {
        Mix_FadeOutMusic(500);
    }


}

void LevelsFinished()
{

    #ifdef WIN32
    int TimeWait = 30;
    #else
    int TimeWait = -1;
    #endif
    SDL_Event Event;
    SDL_Rect Rect,Rect1;
    SDL_Surface *Tmp,*ImgLevels;
    Uint32 Time = SDL_GetTicks();
    double angle=0,zoom1=0.75,zoomdx1=-0.03,zoom2= 0.80,zoomdx2=-0.03,zoom3=0.85, zoomdx3=-0.03;
    char Text [500];
    Rect.w = Buffer->w;
    Rect.h = Buffer->h;
    Rect.x = StartScreenX;
    Rect.y = StartScreenY;
    World->Clear();
    Time = SDL_GetTicks();
     if (GlobalSoundEnabled)
        Mix_PlayChannel(-1,Sounds[SND_LEVELSFINISHED],0);
    while (GameState == GSLevelsFinished)
    {

        if (Time + TimeWait < SDL_GetTicks())
        {
            zoom1 = zoom1 + zoomdx1;
            if (zoom1 <= 0.70)
                zoomdx1*=-1;
            if (zoom1 >= 0.85)
                zoomdx1*=-1;
            zoom2 = zoom2 + zoomdx2;
            if (zoom2 <= 0.70)
                zoomdx2*=-1;
            if (zoom2 >= 0.85)
                zoomdx2*=-1;
            zoom3 = zoom3 + zoomdx3 ;
            if (zoom3 <= 0.70)
                zoomdx3*=-1;
            if (zoom3 >= 0.85)
                zoomdx3*=-1;
            Time = SDL_GetTicks();
        }
        SDL_BlitSurface(IMGSkinBackGround,NULL,Buffer,NULL);
        Tmp = zoomSurface(IMGSkinCongrats,zoom1,zoom1,1);
		Rect1.x = ((120*SCREENPOSMULTIPLY)-(Tmp->w/2));
		Rect1.y = ((120*SCREENPOSMULTIPLY)-(34*SCREENPOSMULTIPLY)-(Tmp->h/2));
		Rect1.w = Tmp->w;
		Rect1.h = Tmp->h;
        SDL_BlitSurface(Tmp,NULL,Buffer,&Rect1);
        SDL_FreeSurface(Tmp);
        Tmp = zoomSurface(IMGSkinYouHave,zoom2,zoom2,1);
		Rect1.x = ((120*SCREENPOSMULTIPLY)-(Tmp->w/2));
		Rect1.y = ((120*SCREENPOSMULTIPLY)-(Tmp->h/2));
		Rect1.w = Tmp->w;
		Rect1.h = Tmp->h;
        SDL_BlitSurface(Tmp,NULL,Buffer,&Rect1);
        SDL_FreeSurface(Tmp);
        switch(LevelType)
        {
            case Easy :
                Tmp = zoomSurface(IMGSkinEasyLevels,zoom3,zoom3,1); ;
                break;
            case Medium :
                Tmp = zoomSurface(IMGSkinMediumLevels,zoom3,zoom3,1); ;
                break;
            case Hard :
                Tmp = zoomSurface(IMGSkinHardLevels,zoom3,zoom3,1); ;
                break;
            case Tutorial :
                Tmp = zoomSurface(IMGSkinTutorialLevels,zoom3,zoom3,1); ;
                break;
            case Custom :
                Tmp = zoomSurface(IMGSkinCustomLevels,zoom3,zoom3,1); ;
                break;
            default:
                break;
        }

		Rect1.x = ((120*SCREENPOSMULTIPLY)-(Tmp->w/2));
		Rect1.y = (((120+34)*SCREENPOSMULTIPLY)-(Tmp->h/2));
		Rect1.w = Tmp->w;
		Rect1.h = Tmp->h;
        SDL_BlitSurface(Tmp,NULL,Buffer,&Rect1);
        SDL_FreeSurface(Tmp);
        WriteText(Buffer,font,"Levels finished!\n\n\nCongratulations !",16,243*SCREENPOSMULTIPLY,1*SCREENPOSMULTIPLY,0,TextColor);
        SDL_FillRect(Buffer1,NULL,SDL_MapRGB(Buffer1->format,0,0,0));
        SDL_BlitSurface(Buffer,NULL,Buffer1,&Rect);
      /*  #ifdef WIN32
            ScreenBufferZoom = zoomSurface(Buffer1,2,2,0);
            SDL_BlitSurface(ScreenBufferZoom,NULL,Screen,NULL);
            SDL_FreeSurface(ScreenBufferZoom);
        #else*/
        SDL_BlitSurface(Buffer1,NULL,Screen,NULL);
       // #endif
        SDL_Flip(Screen);
        #ifdef WIN32
        SDL_Delay(10);
        #endif

        while (SDL_PollEvent(&Event))
		{
		    if (Event.type == SDL_QUIT)
                GameState=GSQuit;

			if (Event.type == SDL_JOYBUTTONDOWN)
			{
				switch (Event.jbutton.button)
				{
				    case GP2X_BUTTON_L:
                        PreviousSkin();
                        break;
                    case GP2X_BUTTON_R:
                        NextSkin();
                        break;
				    case GP2X_BUTTON_VOLUP:
						IncVolume();
						break;
					case GP2X_BUTTON_VOLDOWN:
						DecVolume();
						break;
					case GP2X_BUTTON_SELECT:
						GameState = GSTitleScreen;
						break;
                    default:
                        GameState = GSLevelSelector;
                        if (LevelType == Tutorial)
                            LevelType = Easy;
                        else
                            if(LevelType == Easy)
                                LevelType = Medium;
                            else
                                if(LevelType == Medium)
                                    LevelType = Hard;
                                else
                                    if((LevelType == Hard) || (LevelType == Custom))
                                    {
                                        LevelType = Easy;
                                        GameState = GSTitleScreen;
                                    }
                        Level = 1;
                        break;

				}
			}

			if (Event.type == SDL_KEYDOWN)
                switch (Event.key.keysym.sym)
                {
                    case DINGOO_BUTTON_X:
                        IncVolume();
                        break;
                    case DINGOO_BUTTON_Y:
                        DecVolume();
                        break;
                    case DINGOO_BUTTON_SELECT:
                        GameState = GSTitleScreen;
                        break;
                    case DINGOO_BUTTON_R :
                        NextSkin();
                        break;
                    case DINGOO_BUTTON_L:
                        PreviousSkin();
                        break;
                     default:
                        GameState = GSLevelSelector;
                        if (LevelType == Tutorial)
                            LevelType = Easy;
                        else
                            if(LevelType == Easy)
                                LevelType = Medium;
                            else
                                if(LevelType == Medium)
                                    LevelType = Hard;
                                else
                                    if((LevelType == Hard) || (LevelType == Custom))
                                    {
                                        LevelType = Easy;
                                        GameState = GSTitleScreen;
                                    }
                        Level = 1;
                        break;

                }
		}

    }
}



void LevelSelector()
{
    SDL_Surface *Tmp;
    SDL_Event Event;
    SDL_Rect Rect;
    int LevelSize =0;
    char Text [500];
    char Text1 [500];
    char Text2 [50];
    Uint8 alpha = 0;
    Tmp = SDL_DisplayFormat(Buffer);
    Rect.w = Buffer->w;
    Rect.h = Buffer->h;
    Rect.x = StartScreenX;
    Rect.y = StartScreenY;
    if(LevelType < RandomEasy)
    {
        if (!(World->LoadLevel(GameType,LevelType,Level)))
        {
            if((Level > 1) && (LevelType != Custom))
                UnLockedLevels[GameType][LevelType]--; //terug eentje minder zetten anders teveel unlocked van in levelcompleted() toe te voegen
            if(Level == 1)
            {
                GameState = GSNoLevelsFound;
            }
            else
                GameState = GSLevelsFinished;
        }
    }
    else
    {
        switch(LevelType)
        {
            case RandomEasy:
                if(GameType == Rotating)
                    LevelSize = 5+ rand()%2;
                else
                    LevelSize = 3+ rand()%2;
                break;
            case RandomMedium:
                if(GameType == Rotating)
                    LevelSize = 7+ rand()%2;
                else
                    LevelSize = 5+ rand()%2;
                break;
            case RandomHard:
                if(GameType == Rotating)
                    LevelSize = 9+ rand()%2;
                else
                    LevelSize = 7+ rand()%2;
                break;
            default:
                break;
        }
        World->GenerateLevel(LevelSize,LevelSize);
        World->CenterLevel();
        switch(GameType)
        {
            case Rotating:
                World->ShuffleGame1();
                break;
            case Sliding:
                World->ShuffleGame2();
                break;
            case RotatingSliding:
                World->ShuffleGame1();
                World->ShuffleGame2();
                break;
        }
    }
    while (GameState == GSLevelSelector)
    {
        if(GlobalSoundEnabled)
            if(!Mix_PlayingMusic())
                Mix_PlayMusic(Music[MUS_TITLE],0);
        while (SDL_PollEvent(&Event))
		{
		    if (Event.type == SDL_QUIT)
                GameState=GSQuit;

			if (Event.type == SDL_JOYBUTTONDOWN)
			{
				switch (Event.jbutton.button)
				{
				    case GP2X_BUTTON_L:
                        PreviousSkin();
                        break;
                    case GP2X_BUTTON_R:
                        NextSkin();
                        break;
					case GP2X_BUTTON_SELECT:
						GameState = GSLevelTypeMenu;
						break;
                    case GP2X_BUTTON_LEFT:
                        if(LevelType < 5)
                        {
                            if(Level > 1)
                            {
                                Level--;
                                World->LoadLevel(GameType,LevelType,Level);
                                if (GlobalSoundEnabled)
                                    Mix_PlayChannel(-1,Sounds[SND_MENU],0);
                            }
                        }
                        else
                        {
                            switch(LevelType)
                            {
                                case RandomEasy:
                                    if(GameType == Rotating)
                                        LevelSize = 5+ rand()%2;
                                    else
                                        LevelSize = 3+ rand()%2;
                                    break;
                                case RandomMedium:
                                    if(GameType == Rotating)
                                        LevelSize = 7+ rand()%2;
                                    else
                                        LevelSize = 5+ rand()%2;
                                    break;
                                case RandomHard:
                                    if(GameType == Rotating)
                                        LevelSize = 9+ rand()%2;
                                    else
                                        LevelSize = 7+ rand()%2;
                                    break;
                                default:
                                    break;
                            }
                            if(World->GenerateLevel(LevelSize,LevelSize))
                            {
                                World->CenterLevel();
                                switch(GameType)
                                {
                                    case Rotating:
                                        World->ShuffleGame1();
                                        break;
                                    case Sliding:
                                        World->ShuffleGame2();
                                        break;
                                    case RotatingSliding:
                                        World->ShuffleGame1();
                                        World->ShuffleGame2();
                                        break;
                                }
                                if (GlobalSoundEnabled)
                                    Mix_PlayChannel(-1,Sounds[SND_MENU],0);
                            }
                        }
                        break;
                    case GP2X_BUTTON_RIGHT:
                        if(LevelType < 5)
                        {
                            Level++;
                            if(! World->LoadLevel(GameType,LevelType,Level))
                                Level--;
                            else
                                if (GlobalSoundEnabled)
                                    Mix_PlayChannel(-1,Sounds[SND_MENU],0);
                        }
                        else
                        {
                            switch(LevelType)
                            {
                                case RandomEasy:
                                    if(GameType == Rotating)
                                        LevelSize = 5+ rand()%2;
                                    else
                                        LevelSize = 3+ rand()%2;
                                    break;
                                case RandomMedium:
                                    if(GameType == Rotating)
                                        LevelSize = 7+ rand()%2;
                                    else
                                        LevelSize = 5+ rand()%2;
                                    break;
                                case RandomHard:
                                    if(GameType == Rotating)
                                        LevelSize = 9+ rand()%2;
                                    else
                                        LevelSize = 7+ rand()%2;
                                    break;
                                default:
                                    break;
                            }
                            if(World->GenerateLevel(LevelSize,LevelSize))
                            {
                                World->CenterLevel();
                                switch(GameType)
                                {
                                    case Rotating:
                                        World->ShuffleGame1();
                                        break;
                                    case Sliding:
                                        World->ShuffleGame2();
                                        break;
                                    case RotatingSliding:
                                        World->ShuffleGame1();
                                        World->ShuffleGame2();
                                        break;
                                }
                                if (GlobalSoundEnabled)
                                    Mix_PlayChannel(-1,Sounds[SND_MENU],0);
                            }
                        }
                        break;
                    case GP2X_BUTTON_B:
                    case GP2X_BUTTON_X:
                    case GP2X_BUTTON_Y:
                    case GP2X_BUTTON_A:
                    case GP2X_BUTTON_START:
                        if (LevelType< Custom)
                        {
                            if ((Level <= UnLockedLevels[GameType][LevelType]) )
                            {
                                if (GlobalSoundEnabled)
                                {
                                    Mix_PlayChannel(-1,Sounds[SND_SELECT],0);
                                    Mix_FadeOutMusic(500);
                                }
                                    GameState = GSGame;
                            }
                            else
                                if (GlobalSoundEnabled)
                                {
                                    Mix_PlayChannel(-1,Sounds[SND_ERROR],0);
                                }
                        }
                        else
                        {
                            if (GlobalSoundEnabled)
                            {
                                Mix_PlayChannel(-1,Sounds[SND_SELECT],0);
                                Mix_FadeOutMusic(500);
                            }
                            GameState = GSGame;
                        }
                        break;
                    case GP2X_BUTTON_VOLUP:
						IncVolume();
						break;
					case GP2X_BUTTON_VOLDOWN:
						DecVolume();
						break;
				}
			}

			if (Event.type == SDL_KEYDOWN)
                switch (Event.key.keysym.sym)
                {
                    case DINGOO_BUTTON_X:
                        IncVolume();
                        break;
                    case DINGOO_BUTTON_Y:
                        DecVolume();
                        break;
                    case DINGOO_BUTTON_SELECT:
                        GameState = GSLevelTypeMenu;
                        break;
                    case DINGOO_BUTTON_R :
                        NextSkin();
                        break;
                    case DINGOO_BUTTON_L:
                        PreviousSkin();
                        break;
                    case DINGOO_BUTTON_LEFT:
                        if(LevelType < 5)
                        {
                            if(Level > 1)
                            {
                                Level--;
                                World->LoadLevel(GameType,LevelType,Level);
                                if (GlobalSoundEnabled)
                                    Mix_PlayChannel(-1,Sounds[SND_MENU],0);
                            }
                        }
                        else
                        {
                            switch(LevelType)
                            {
                                case RandomEasy:
                                    if(GameType == Rotating)
                                        LevelSize = 5+ rand()%2;
                                    else
                                        LevelSize = 3+ rand()%2;
                                    break;
                                case RandomMedium:
                                    if(GameType == Rotating)
                                        LevelSize = 7+ rand()%2;
                                    else
                                        LevelSize = 5+ rand()%2;
                                    break;
                                case RandomHard:
                                    if(GameType == Rotating)
                                        LevelSize = 9+ rand()%2;
                                    else
                                        LevelSize = 7+ rand()%2;
                                    break;
                                default:
                                    break;
                            }
                            if(World->GenerateLevel(LevelSize,LevelSize))
                            {
                                World->CenterLevel();
                                switch(GameType)
                                {
                                    case Rotating:
                                        World->ShuffleGame1();
                                        break;
                                    case Sliding:
                                        World->ShuffleGame2();
                                        break;
                                    case RotatingSliding:
                                        World->ShuffleGame1();
                                        World->ShuffleGame2();
                                        break;
                                }
                                if (GlobalSoundEnabled)
                                    Mix_PlayChannel(-1,Sounds[SND_MENU],0);
                            }
                        }
                        break;
                    case DINGOO_BUTTON_RIGHT:
                        if(LevelType < 5)
                        {
                            Level++;
                            if(! World->LoadLevel(GameType,LevelType,Level))
                                Level--;
                            else
                                if (GlobalSoundEnabled)
                                    Mix_PlayChannel(-1,Sounds[SND_MENU],0);
                        }
                        else
                        {
                            switch(LevelType)
                            {
                                case RandomEasy:
                                    if(GameType == Rotating)
                                        LevelSize = 5+ rand()%2;
                                    else
                                        LevelSize = 3+ rand()%2;
                                    break;
                                case RandomMedium:
                                    if(GameType == Rotating)
                                        LevelSize = 7+ rand()%2;
                                    else
                                        LevelSize = 5+ rand()%2;
                                    break;
                                case RandomHard:
                                    if(GameType == Rotating)
                                        LevelSize = 9+ rand()%2;
                                    else
                                        LevelSize = 7+ rand()%2;
                                    break;
                                default:
                                    break;
                            }
                            if(World->GenerateLevel(LevelSize,LevelSize))
                            {
                                World->CenterLevel();
                                switch(GameType)
                                {
                                    case Rotating:
                                        World->ShuffleGame1();
                                        break;
                                    case Sliding:
                                        World->ShuffleGame2();
                                        break;
                                    case RotatingSliding:
                                        World->ShuffleGame1();
                                        World->ShuffleGame2();
                                        break;
                                }
                                if (GlobalSoundEnabled)
                                    Mix_PlayChannel(-1,Sounds[SND_MENU],0);
                            }
                        }
                        break;
                   case DINGOO_BUTTON_A:
                        if (LevelType< Custom)
                        {
                            if ((Level <= UnLockedLevels[GameType][LevelType]) )
                            {
                                if (GlobalSoundEnabled)
                                {
                                    Mix_PlayChannel(-1,Sounds[SND_SELECT],0);
                                    Mix_FadeOutMusic(500);
                                }
                                    GameState = GSGame;
                            }
                            else
                                if (GlobalSoundEnabled)
                                {
                                    Mix_PlayChannel(-1,Sounds[SND_ERROR],0);
                                }
                        }
                        else
                        {
                            if (GlobalSoundEnabled)
                            {
                                Mix_PlayChannel(-1,Sounds[SND_SELECT],0);
                                Mix_FadeOutMusic(500);
                            }
                            GameState = GSGame;
                        }
                        break;
                    default:
                        break;
                }
		}
		SDL_BlitSurface(IMGSkinBackGround,NULL,Tmp,NULL);
        World->Draw(Tmp);
        World->DrawConnected(Tmp);
        if(LevelType < Custom)
        {
            if(Level<= UnLockedLevels[GameType][LevelType])
                sprintf(Text1,"Unocked");
            else
                sprintf(Text1,"Locked");
        }
        else
            sprintf(Text1,"");

        switch (GameType)
        {
            case Sliding :
                sprintf(Text2,"Sliding");
                break;
            case Rotating:
                sprintf(Text2,"Rotating");
                break;
            case RotatingSliding:
                sprintf(Text2,"Slide & Rotate");
                break;

        }
        switch(LevelType)
        {
            case Easy :
                sprintf(Text,"Level Selector\n\nGame Type:\n%s\n\nLevel Type:\nEasy\n\nLevel Number:\n%d\n\n%s\n\n\n\n\nSelect with:\nLeft or Right\n\nTo Play:\nPress Button",Text2,Level,Text1);
                WriteText(Tmp,font,Text,strlen(Text),243*SCREENPOSMULTIPLY,1*SCREENPOSMULTIPLY,0,TextColor);
                break;
            case Medium :
                sprintf(Text,"Level Selector\n\nGame Type:\n%s\n\nLevel Type:\nMedium\n\nLevel Number:\n%d\n\n%s\n\n\n\n\nSelect with:\nLeft or Right\n\nTo Play:\nPress Button",Text2,Level,Text1);
                WriteText(Tmp,font,Text,strlen(Text),243*SCREENPOSMULTIPLY,1*SCREENPOSMULTIPLY,0,TextColor);
                break;
            case Hard :
                sprintf(Text,"Level Selector\n\nGame Type:\n%s\n\nLevel Type:\nHard\n\nLevel Number:\n%d\n\n%s\n\n\n\n\nSelect with:\nLeft or Right\n\nTo Play:\nPress Button",Text2,Level,Text1);
                WriteText(Tmp,font,Text,strlen(Text),243*SCREENPOSMULTIPLY,1*SCREENPOSMULTIPLY,0,TextColor);
                break;
            case Tutorial :
                sprintf(Text,"Level Selector\n\nGame Type:\n%s\n\nLevel Type:\nTutorial\n\nLevel Number:\n%d\n\n%s\n\n\n\n\nSelect with:\nLeft or Right\n\nTo Play:\nPress Button",Text2,Level,Text1);
                WriteText(Tmp,font,Text,strlen(Text),243*SCREENPOSMULTIPLY,1*SCREENPOSMULTIPLY,0,TextColor);
                break;
            case Custom :
                sprintf(Text,"Level Selector\n\nGame Type:\n%s\n\nLevel Type:\nCustom\n\nLevel Number:\n%d\n\n%s\n\n\n\n\nSelect with:\nLeft or Right\n\nTo Play:\nPress Button",Text2,Level,Text1);
                WriteText(Tmp,font,Text,strlen(Text),243*SCREENPOSMULTIPLY,1*SCREENPOSMULTIPLY,0,TextColor);
                break;
            case RandomEasy :
                sprintf(Text,"Level Selector\n\nGame Type:\n%s\n\nLevel Type:\nRandom Easy\n\nLevel Number:\n%d\n\n%s\n\n\n\n\nSelect new with:\nLeft or Right\n\nTo Play:\nPress Button",Text2,Level,Text1);
                WriteText(Tmp,font,Text,strlen(Text),243*SCREENPOSMULTIPLY,1*SCREENPOSMULTIPLY,0,TextColor);
                break;
            case RandomMedium :
                sprintf(Text,"Level Selector\n\nGame Type:\n%s\n\nLevel Type:\nRandom Medium\n\nLevel Number:\n%d\n\n%s\n\n\n\n\nSelect new with:\nLeft or Right\n\nTo Play:\nPress Button",Text2,Level,Text1);
                WriteText(Tmp,font,Text,strlen(Text),243*SCREENPOSMULTIPLY,1*SCREENPOSMULTIPLY,0,TextColor);
                break;
            case RandomHard :
                sprintf(Text,"Level Selector\n\nGame Type:\n%s\n\nLevel Type:\nRandom Hard\n\nLevel Number:\n%d\n\n%s\n\n\n\n\nSelect new with:\nLeft or Right\n\nTo Play:\nPress Button",Text2,Level,Text1);
                WriteText(Tmp,font,Text,strlen(Text),243*SCREENPOSMULTIPLY,1*SCREENPOSMULTIPLY,0,TextColor);
                break;

        }
        if((GameType == Sliding) || (GameType == RotatingSliding))
            World->DrawArrows(Tmp,0,0);

        if (alpha < 255)
        {
            if(alpha+AlphaIncrease > MaxAlpha)
            {
                alpha = 255;
                SDL_SetAlpha(Tmp,SDL_SRCALPHA | SDL_RLEACCEL,alpha);
            }
            else
            {
                alpha+=10;
                SDL_SetAlpha(Tmp,SDL_SRCALPHA | SDL_RLEACCEL,alpha);
            }
        }
        SDL_BlitSurface(Tmp,NULL,Buffer,NULL);
        SDL_FillRect(Buffer1,NULL,SDL_MapRGB(Buffer1->format,0,0,0));
        SDL_BlitSurface(Buffer,NULL,Buffer1,&Rect);
       /* #ifdef WIN32
            ScreenBufferZoom = zoomSurface(Buffer1,2,2,0);
            SDL_BlitSurface(ScreenBufferZoom,NULL,Screen,NULL);
            SDL_FreeSurface(ScreenBufferZoom);
        #else*/
        SDL_BlitSurface(Buffer1,NULL,Screen,NULL);
       // #endif
        SDL_Flip(Screen);
        #ifdef WIN32
        SDL_Delay(10);
        #endif

    }
    SDL_FreeSurface(Tmp);
}

void Credits()
{
    SDL_Rect Rect,Rect1;
    SDL_Event Event;
    Uint8 alpha = 0;
    Uint32 Time = 0;
    int CreditsInfoNr = 1;
    SDL_Surface *Tmp;
    Tmp = SDL_DisplayFormat(Buffer);
    Time = SDL_GetTicks();
    while (GameState == GSCredits)
    {
        if(GlobalSoundEnabled)
            if(!Mix_PlayingMusic())
                Mix_PlayMusic(Music[MUS_TITLE],0);
        while (SDL_PollEvent(&Event))
		{

            if (Event.type == SDL_QUIT)
                GameState=GSQuit;

            if (Event.type == SDL_JOYBUTTONDOWN)
                switch (Event.jbutton.button)
                {
                    case GP2X_BUTTON_L:
                        PreviousSkin();
                        break;
                    case GP2X_BUTTON_R:
                        NextSkin();
                        break;
                    case GP2X_BUTTON_VOLUP:
						IncVolume();
						break;
					case GP2X_BUTTON_VOLDOWN:
						DecVolume();
						break;
                }

            if (Event.type == SDL_KEYDOWN)
                switch (Event.key.keysym.sym)
                {
                    case DINGOO_BUTTON_X:
                        IncVolume();
                        break;
                    case DINGOO_BUTTON_Y:
                        DecVolume();
                        break;
                    case DINGOO_BUTTON_R :
                        NextSkin();
                        break;
                    case DINGOO_BUTTON_L:
                        PreviousSkin();
                        break;
                    default :
                        break;
                }
        }

        //tekenen naar buffer

        Rect1.x = 17*SCREENPOSMULTIPLY;
        Rect1.y = 110*SCREENPOSMULTIPLY;
        SDL_BlitSurface(IMGSkinTitleScreen,NULL,Tmp,NULL);
        switch(CreditsInfoNr)
            {
                case 1 :
                    SDL_BlitSurface(IMGSkinCreditsInfo1,NULL,Tmp,&Rect1);
                    Rect1.w = IMGSkinCreditsInfo1->w;
                    Rect1.h = IMGSkinCreditsInfo1->h;
                    break;
                case 2 :
                    SDL_BlitSurface(IMGSkinCreditsInfo2,NULL,Tmp,&Rect1);
                    Rect1.w = IMGSkinCreditsInfo2->w;
                    Rect1.h = IMGSkinCreditsInfo2->h;
                    break;
                case 3 :
                    SDL_BlitSurface(IMGSkinCreditsInfo3,NULL,Tmp,&Rect1);
                    Rect1.w = IMGSkinCreditsInfo3->w;
                    Rect1.h = IMGSkinCreditsInfo3->h;
                    break;
            }

        if (alpha < 255)
        {
            if(alpha+AlphaIncrease > MaxAlpha)
            {
                alpha = 255;
                SDL_SetAlpha(Tmp,SDL_SRCALPHA | SDL_RLEACCEL,alpha);
            }
            else
            {
                alpha+=10;
                SDL_SetAlpha(Tmp,SDL_SRCALPHA | SDL_RLEACCEL,alpha);
            }
        }




        Rect.w = Buffer->w;
        Rect.h = Buffer->h;
        Rect.x = StartScreenX*SCREENPOSMULTIPLY;
        Rect.y = StartScreenY*SCREENPOSMULTIPLY;
        SDL_FillRect(Buffer1,NULL,SDL_MapRGB(Buffer1->format,0,0,0));
        SDL_BlitSurface(Tmp,NULL,Buffer,NULL);

        SDL_BlitSurface(Buffer,NULL,Buffer1,&Rect);
      /* #ifdef WIN32
            ScreenBufferZoom = zoomSurface(Buffer1,2,2,0);
            SDL_BlitSurface(ScreenBufferZoom,NULL,Screen,NULL);
            SDL_FreeSurface(ScreenBufferZoom);
        #else*/
        SDL_BlitSurface(Buffer1,NULL,Screen,NULL);
       // #endif
        SDL_Flip(Screen);
        #ifdef WIN32
        SDL_Delay(10);
        #endif
        if(Time + 3500 < SDL_GetTicks())
        {
            alpha = 0;
            CreditsInfoNr++;
            if(CreditsInfoNr > 3)
            {
                SDL_Delay(1500);
                GameState = GSTitleScreen;
            }
            Time = SDL_GetTicks();
        }


    }
    SDL_FreeSurface(Tmp);

}

void intro()
{
    SDL_Rect Rect;
    SDL_Event Event;
    Uint8 alpha = 0;
    Uint32 Time=0;
    bool RPressed=false;
    int IntroScreenNr = 1;
    SDL_Surface *Tmp;
    Tmp = SDL_DisplayFormat(Buffer);
    Time = SDL_GetTicks();
    while (GameState == GSIntro)
    {
        while (SDL_PollEvent(&Event))
		{
            if (Event.type == SDL_QUIT)
                GameState=GSQuit;

            if (Event.type == SDL_KEYDOWN)
                switch (Event.key.keysym.sym)
                {
                    case DINGOO_BUTTON_X:
                        IncVolume();
                        break;
                    case DINGOO_BUTTON_Y:
                        DecVolume();
                        break;
                    case DINGOO_BUTTON_R :
                        NextSkin();
                        break;
                    case DINGOO_BUTTON_L:
                        PreviousSkin();
                        break;
                    case DINGOO_BUTTON_SELECT:
                    case DINGOO_BUTTON_A:
                        GameState = GSTitleScreen;
                        break;
                    default:
                        break;
                }

            if (Event.type == SDL_JOYBUTTONUP)
                switch (Event.jbutton.button)
                {
                    case GP2X_BUTTON_R:
                        RPressed = false;
                        break;
                    case GP2X_BUTTON_LEFT:
                        if (RPressed)
                            if(StartScreenX - 2 >= 0)
                                StartScreenX -=2;
                        break;
                    case GP2X_BUTTON_RIGHT:
                        if (RPressed)
                            if(StartScreenX + 2 + Buffer->w <= Screen->w)
                                StartScreenX +=2;
                        break;
                    case GP2X_BUTTON_UP:
                        if(RPressed)
                            if(StartScreenY - 2 >=0)
                                StartScreenY -=2;
                        break;
                    case GP2X_BUTTON_DOWN:
                        if(RPressed)
                            if(StartScreenY + 2 + Buffer->h <= Screen->h)
                                StartScreenY +=2;
                        break;
                }

            if (Event.type == SDL_JOYBUTTONDOWN)
                switch (Event.jbutton.button)
                {
                    case GP2X_BUTTON_L:
                        PreviousSkin();
                        break;
                    case GP2X_BUTTON_R:
                        RPressed = true;
                        if(!TvOutMode)
                            NextSkin();
                        break;
                   case GP2X_BUTTON_VOLUP:
						IncVolume();
						break;
					case GP2X_BUTTON_VOLDOWN:
						DecVolume();
						break;
                    case GP2X_BUTTON_A:
                    case GP2X_BUTTON_B:
                    case GP2X_BUTTON_X:
                    case GP2X_BUTTON_Y:
                    case GP2X_BUTTON_SELECT:
                    case GP2X_BUTTON_START:
                        GameState = GSTitleScreen;
                        break;
                }
        }

        //tekenen naar buffer

        switch(IntroScreenNr)
        {
            case 1 :
                SDL_BlitSurface(IMGSkinIntro1,NULL,Tmp,NULL);
                break;
            case 2 :
                SDL_BlitSurface(IMGSkinIntro2,NULL,Tmp,NULL);
                break;
            case 3 :
                SDL_BlitSurface(IMGSkinIntro3,NULL,Tmp,NULL);
                break;
        }

        if (alpha < 255)
        {
            if(alpha+AlphaIncrease > MaxAlpha)
            {
                alpha = 255;
                SDL_SetAlpha(Tmp,SDL_SRCALPHA | SDL_RLEACCEL,alpha);
            }
            else
            {
                alpha+=10;
                SDL_SetAlpha(Tmp,SDL_SRCALPHA | SDL_RLEACCEL,alpha);
            }
        }




        Rect.w = Buffer->w;
        Rect.h = Buffer->h;
        Rect.x = StartScreenX;
        Rect.y = StartScreenY;


        SDL_FillRect(Buffer1,NULL,SDL_MapRGB(Buffer1->format,0,0,0));
        SDL_BlitSurface(Tmp,NULL,Buffer,NULL);
        SDL_BlitSurface(Buffer,NULL,Buffer1,&Rect);
      /*  #ifdef WIN32
            ScreenBufferZoom = zoomSurface(Buffer1,2,2,0);
            SDL_BlitSurface(ScreenBufferZoom,NULL,Screen,NULL);
            SDL_FreeSurface(ScreenBufferZoom);
        #else*/
        SDL_BlitSurface(Buffer1,NULL,Screen,NULL);
       // #endif

        SDL_Flip(Screen);
        #ifdef WIN32
        SDL_Delay(10);
        #endif
        if(Time + 3500 < SDL_GetTicks())
        {
            alpha = 0;
            IntroScreenNr++;
            if(IntroScreenNr > 3)
                GameState = GSTitleScreen;
            Time = SDL_GetTicks();
        }

    }
    SDL_FreeSurface(Tmp);

}

void LoadSounds()
{
    if(GlobalSoundEnabled)
    {
		Sounds[SND_SELECT] = Mix_LoadWAV("./sound/select.wav");
		Sounds[SND_WINNER] = Mix_LoadWAV("./sound/winner.wav");
		Sounds[SND_MENU] = Mix_LoadWAV("./sound/menu.wav");
		Sounds[SND_LEVELSFINISHED] = Mix_LoadWAV("./sound/congrats.wav");
		Sounds[SND_ROTATE] = Mix_LoadWAV("./sound/rotate.wav");
		Sounds[SND_ERROR] = Mix_LoadWAV("./sound/error.wav");
		Music[MUS_TITLE] = Mix_LoadMUS("./music/title.ogg");
		Music[MUS_GAME] = Mix_LoadMUS("./music/music1.ogg");
    }
}

void UnloadSounds()
{
    if (GlobalSoundEnabled)
    {
        if (!Mix_PlayingMusic())
            Mix_HaltMusic();

        int Teller;
        for (Teller=0;Teller<NrOfSounds;Teller++)
            if(Sounds[Teller])
                Mix_FreeChunk(Sounds[Teller]);
        for (Teller=0;Teller<NrOfMusic;Teller++)
            if(Music[Teller])
                Mix_FreeMusic(Music[Teller]);
    }

}


// ---------------------------------------------------------------------------
// android port: touch on the game picture (game coordinates 0..319 x 0..239)
// ---------------------------------------------------------------------------
bool Game_HandleTap(int x, int y)
{
    switch (GameState)
    {
        case GSGame:
            // tap a pipe (or a sliding arrow): move the cursor there and press A
            if (CurrentSelector && (x >= 0) && (y >= 0) && (x < NumCols * TileWidth) && (y < NumRows * TileHeight))
            {
                CurrentSelector->SetPosition(x / TileWidth, y / TileHeight);
                Port_PushKey(DINGOO_BUTTON_A);
            }
            return true; // taps on the info panel do nothing
        case GSTitleScreen:
            if (CurrentMainMenu && (x >= 79) && (x < 79 + 172))
            {
                int Item = 0;
                if ((y >= 73) && (y < 73 + 48)) Item = 1;
                if ((y >= 121) && (y < 121 + 46)) Item = 2;
                if ((y >= 167) && (y < 167 + 48)) Item = 3;
                if (Item)
                {
                    CurrentMainMenu->SetSelection(Item);
                    Port_PushKey(DINGOO_BUTTON_A);
                }
            }
            return true;
        case GSGameTypeMenu:
        case GSLevelTypeMenu:
            // the choice is shown in the middle: tap left/right of it to browse
            if (x < 100) Port_PushKey(DINGOO_BUTTON_LEFT);
            else if (x >= 230) Port_PushKey(DINGOO_BUTTON_RIGHT);
            else Port_PushKey(DINGOO_BUTTON_A);
            return true;
        case GSLevelSelector:
            // tap the edges of the board to browse levels, the middle to play
            if (x < 50) Port_PushKey(DINGOO_BUTTON_LEFT);
            else if ((x >= 190) && (x < 240)) Port_PushKey(DINGOO_BUTTON_RIGHT);
            else if (x < 240) Port_PushKey(DINGOO_BUTTON_A);
            return true;
        default:
            return false; // any other screen: a tap works like pressing A
    }
}

void Game_SaveState()
{
    SaveSettings();
}

int main(int argc, char **argv)
{
    SDL_Rect Rect;
	//mtrace();
    SDL_EnableUNICODE(1);
    #ifdef WIN32
        if (argc ==2)
        {
            SCREENPOSMULTIPLY = atoi(argv[1]);
            if((SCREENPOSMULTIPLY < 1) || (SCREENPOSMULTIPLY > 10))
                SCREENPOSMULTIPLY = 1;
            WINDOW_WIDTH = 320 *SCREENPOSMULTIPLY,
            WINDOW_HEIGHT = 240*SCREENPOSMULTIPLY,
            TileWidth = 24*SCREENPOSMULTIPLY,
            TileHeight = 24*SCREENPOSMULTIPLY;
        }
    #endif
    srand(time(NULL));
    if (!Port_PrepareData()) // android port: unpack data files and chdir to them
        printf("Could not prepare the game data\n");
	if (SDL_Init(SDL_INIT_GAMECONTROLLER | SDL_INIT_VIDEO | SDL_INIT_AUDIO ) == 0)
	{
		printf("SDL Succesfully initialized\n");
        #ifdef GP2X
       // if(SDL_GP2X_GetPhysicalScreenSize(&Rect) == 0)
        #endif
            Screen = SDL_SetVideoMode( WINDOW_WIDTH, WINDOW_HEIGHT,SCREEN_BPP, SDL_HWSURFACE );
     //  #ifdef GP2X
        /*else

        {
            TvOutMode = true;
            Screen = SDL_SetVideoMode( 360, 288,SCREEN_BPP, SDL_HWSURFACE );
        }
        #endif*/
		if(Screen)
		{
		    printf("Succesfully Set %dx%dx16\n",WINDOW_WIDTH,WINDOW_HEIGHT);
		    Buffer = SDL_CreateRGBSurface(SDL_HWSURFACE,WINDOW_WIDTH,WINDOW_HEIGHT,SCREEN_BPP,Screen->format->Rmask,Screen->format->Gmask,Screen->format->Bmask,Screen->format->Amask);
			if (TvOutMode)
                Buffer1 = SDL_CreateRGBSurface(SDL_HWSURFACE,360,288,SCREEN_BPP,Screen->format->Rmask,Screen->format->Gmask,Screen->format->Bmask,Screen->format->Amask);
            else
                Buffer1 = SDL_CreateRGBSurface(SDL_HWSURFACE,WINDOW_WIDTH,WINDOW_HEIGHT,SCREEN_BPP,Screen->format->Rmask,Screen->format->Gmask,Screen->format->Bmask,Screen->format->Amask);
			if (Buffer && Buffer1)
			{
			    SDL_WM_SetCaption("Waternet GP2X",0);
                printf("Succesfully Created Buffer\n");
                SDL_FillRect(Buffer,NULL,SDL_MapRGB(Buffer->format,0,0,0));
                SDL_ShowCursor(SDL_DISABLE);
                // android port: no GP2X joystick; touch/keys/gamepads come in through port.cpp
                {
                    if (Mix_OpenAudio(22050,AUDIO_S16,MIX_DEFAULT_CHANNELS,1024) < 0)
                    {
                        GlobalSoundEnabled = false;
                        printf("Failed to initialise sound!\n");
                        printf("%s\n",Mix_GetError());
                    }
                    else
                    {
                        printf("Audio Succesfully initialised!\n");
                    }
                    if (TTF_Init() == 0)
                    {
                        printf("Succesfully initialized TTF\n");
                        font = TTF_OpenFont("./font.ttf",9*SCREENPOSMULTIPLY);
                        BigFont = TTF_OpenFont("./font.ttf",16*SCREENPOSMULTIPLY);
                        if (font && BigFont)
                        {
                            printf("Succesfully Loaded fonts\n");
                            TTF_SetFontStyle(font,TTF_STYLE_NORMAL);
                            // Main game loop that loops untile the gamestate = GSQuit
                            // and calls the procedure according to the gamestate.
                            SearchForSkins();
                            LoadSettings();
                            LoadSounds();
                            LoadGraphics();
                            World = new CWorld();
                            while (GameState != GSQuit)
                            {
                                switch(GameState)
                                {
                                    case GSGame :
                                        Game();
                                        break;
                                    case GSTitleScreen:
                                        TitleScreen();
                                        break;
                                    case GSIntro :
                                        intro();
                                        break;
                                    case GSLevelCompleted :
                                        LevelCompleted();
                                        break;
                                    case GSLevelSelector :
                                        LevelSelector();
                                        break;
                                    case GSLevelsFinished:
                                        LevelsFinished();
                                        break;
                                    case GSCredits :
                                        Credits();
                                        break;
                                    case GSLevelTypeMenu:
                                        LevelTypeMenu();
                                        break;
                                    case GSGameTypeMenu:
                                        GameTypeMenu();
                                        break;
                                    case GSNoLevelsFound:
                                        NoLevelsFound();
                                        break;
                                    default:
                                        break;
                                }
                            }
                            delete World;
                            UnLoadGraphics();
                            UnloadSounds();
                            SaveSettings();
                            Mix_CloseAudio();
                            TTF_CloseFont(font);
                            TTF_CloseFont(BigFont);
                            font=NULL;
                            Port_Shutdown();
                        }
                        else
                        {
                            printf("Failed to Load fonts\n");
                        }
                        TTF_Quit();
                    }
                    else
                    {
                        printf("Failed to initialize TTF\n");
                    }
                }
                SDL_FreeSurface(Buffer);
                Buffer=NULL;
			}
			else
                printf("Failed To Create Buffer\n");
            SDL_FreeSurface(Buffer1);
            Buffer1=NULL;
		}
		else
		{
			printf("Failed to Set Videomode %dx%dx16\n",WINDOW_WIDTH, WINDOW_HEIGHT);
		}
		SDL_Quit();
	}
	else
	{
		printf("Couldn't initialise SDL!\n");
	}
	// Shouldn't be needed but i just placed it again to be sure all the boardparts are free
	// Call the menu
	//chdir("/usr/gp2x");
	//execl("/usr/gp2x/gp2xmenu", "/usr/gp2x/gp2xmenu", NULL);

	return EXIT_SUCCESS;

}
