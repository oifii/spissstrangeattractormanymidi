/*
 * Copyright (c) 2010-2016 Stephane Poirier
 *
 * stephane.poirier@oifii.org
 *
 * Stephane Poirier
 * 3532 rue Ste-Famille, #3
 * Montreal, QC, H2X 2L1
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

// spissstrangeattractormanymidi.cpp : Defines the entry point for the console application.
// nakedsoftware.org, spi@oifii.org, stephane.poirier@oifii.org

#include "stdafx.h"

/*
int _tmain(int argc, _TCHAR* argv[])
{
	return 0;
}
*/



#include <windows.h>
#include  <scrnsave.h>
#include  <GL/gl.h>
#include <GL/glu.h>
//#include "glut.h" // include GLUT library header
#include <math.h>
#include <ctime>
//#include <iostream>
//#include <fstream>
#include <stdio.h>
#include <vector>
#include "resource.h"

#include "portmidi.h"
#include <map>
using namespace std; //for glfont.h
#include "glfont.h"
#include "spiwavsetlib.h"
//#include "spiws_midiutility.h"


//std::ofstream myofstream;
FILE* pFILE = NULL;

BYTE global_alpha=200;

map<string,int> global_midioutputdevicemap;
string global_midioutputdevicename="Out To MIDI Yoke:  1";
//string global_midioutputdevicename="E-DSP MIDI Port [FFC0]";
//string global_midioutputdevicename="E-DSP MIDI Port 2 [FFC0]";
int global_outputmidichannel=0; //channelid 0 is midi channelname 1
int global_midicontrolnumber=9; //9 is undefined, so it is available for us to use

bool global_bdrawlabel=true;
bool global_bsendmidi=true;
bool global_bsendmidi_usingremap=true;
PmStream* global_pPmStream = NULL; // midi output
int global_prevnote=-1;

//get rid of these warnings:
//truncation from const double to float
//conversion from double to float
#pragma warning(disable: 4305 4244) 
                        

//Define a Windows timer

/*
#define TIMER 1
*/
UINT global_miditimer=1;
UINT global_miditimer_programchange=2;



//These forward declarations are just for readability,
//so the big three functions can come first 

void InitGL(HWND hWnd, HDC & hDC, HGLRC & hRC);
void CloseGL(HWND hWnd, HDC hDC, HGLRC hRC);
void SetupAnimation(HDC hDC, int Width, int Height);
void CleanupAnimation();
void GetConfig();               
void WriteConfig(HWND hDlg);
void OnTimer(HDC hDC);
void OnTimerMidiProgramChange();


int Width, Height; //globals for size of screen

GLFont global_GLFont;

/*
float	x = 0.1, y = 0.1,		// starting point
	a = -0.966918,			// coefficients for "The King's Dream"
	b = 2.879879,
	c = 0.765145,
	d = 0.744728;
*/
float x = 0.1, y = 0.1;		// starting point
float a = -0.966918;		// coefficients for "The King's Dream"
float b = 2.879879;			// coefficients a and b will be modified at random between a range of -3.0 to + 3.0 
float c = 0.765145;
float d = 0.744728;


int	initialIterations = 100,	// initial number of iterations
					// to allow the attractor to settle
	iterations = 10000;		// number of times to iterate through
	//iterations = 100000;		// number of times to iterate through
	//iterations = 1000000;		// number of times to iterate through
					// the functions and draw a point

class glfloatpair
{
public:
	GLfloat x;
	GLfloat y;

	glfloatpair(GLfloat xx, GLfloat yy)
	{
		x = xx;
		y = yy;
	}
	glfloatpair(double xx, double yy)
	{
		x = xx;
		y = yy;
	}
};

std::vector<glfloatpair*> global_pairvector;
std::vector<glfloatpair*>::iterator it;


int RandomInteger(int lowest, int highest) 
{
	int random_integer;
	int range=(highest-lowest)+1; 
	//random_integer = lowest+int(range*rand()/(RAND_MAX + 1.0));
	random_integer = lowest+rand()%range;
	return random_integer;
}

float RandomFloat(float a, float b) 
{
    float random = ((float) rand()) / (float) RAND_MAX;
    float diff = b - a;
    float r = random * diff;
    return a + r;
}

//////////////////////////////////////////////////
////   INFRASTRUCTURE -- THE THREE FUNCTIONS   ///
//////////////////////////////////////////////////


// Screen Saver Procedure
LRESULT WINAPI ScreenSaverProc(HWND hWnd, UINT message, 
                               WPARAM wParam, LPARAM lParam)
{

	static HDC hDC;
	static HGLRC hRC;
	static RECT rect;

	

  switch ( message ) 
  {

  case WM_CREATE: 
		//spi, avril 2015, begin
		SetWindowLong(hWnd, GWL_EXSTYLE, GetWindowLong(hWnd, GWL_EXSTYLE) | WS_EX_LAYERED);
		SetLayeredWindowAttributes(hWnd, 0, global_alpha, LWA_ALPHA);
		//SetLayeredWindowAttributes(h, 0, 200, LWA_ALPHA);
		//spi, avril 2015, end

		//int nShowCmd = false;
		ShellExecuteA(NULL, "open", "begin.bat", "", NULL, false);
		//debug
		//myofstream.open("debug.txt", std::ios::out);
		//myofstream << "test line\n";
		pFILE = fopen("debug.txt", "w");
		//fprintf(pFILE, "test1\n");

	
		//////////////////////////
		//initialize random number
		//////////////////////////
		srand((unsigned)time(0));

		// get window dimensions
		GetClientRect( hWnd, &rect );
		Width = rect.right;         
		Height = rect.bottom;
    
        //get configuration from registry
        GetConfig();

        // setup OpenGL, then animation
		InitGL( hWnd, hDC, hRC );
		SetupAnimation(hDC, Width, Height);

		if(global_bsendmidi)
		{
			/////////////////////////
			//portmidi initialization
			/////////////////////////
			PmError err;
			Pm_Initialize();
			// list device information 
			if(pFILE) fprintf(pFILE, "MIDI output devices:\n");
			for (int i = 0; i < Pm_CountDevices(); i++) 
			{
				const PmDeviceInfo *info = Pm_GetDeviceInfo(i);
				if (info->output) 
				{
					if(pFILE) fprintf(pFILE, "%d: %s, %s\n", i, info->interf, info->name);
					string devicename = info->name;
					global_midioutputdevicemap.insert(pair<string,int>(devicename,i));
				}
			}
			int midioutputdeviceid = 13;
			map<string,int>::iterator it;
			it = global_midioutputdevicemap.find(global_midioutputdevicename);
			if(it!=global_midioutputdevicemap.end())
			{
				midioutputdeviceid = (*it).second;
				if(pFILE) fprintf(pFILE, "%s maps to %d\n", global_midioutputdevicename.c_str(), midioutputdeviceid);
			}
			if(pFILE) fprintf(pFILE, "device %d selected\n", midioutputdeviceid);
			//err = Pm_OpenInput(&midi_in, inp, NULL, 512, NULL, NULL);
			err = Pm_OpenOutput(&global_pPmStream, midioutputdeviceid, NULL, 512, NULL, NULL, 0); //0 latency
			if (err) 
			{
				if(pFILE) fprintf(pFILE, Pm_GetErrorText(err));
				//Pt_Stop();
				//Terminate();
				//mmexit(1);
				global_bsendmidi = false;
			}
		}

		//set timer to tick every 10 ms
		//SetTimer( hWnd, TIMER, 10, NULL );
		//SetTimer( hWnd, TIMER, 10000, NULL );
		//SetTimer( hWnd, TIMER, 5000, NULL );
		//SetTimer( hWnd, TIMER, 2500, NULL );
		//SetTimer( hWnd, TIMER, 1000, NULL );
		SetTimer( hWnd, global_miditimer, 400, NULL );

		//SetTimer( hWnd, global_miditimer_programchange,180000, NULL );
		SetTimer( hWnd, global_miditimer_programchange, 5000, NULL );

    return 0;
 
  case WM_DESTROY:
	{
		KillTimer( hWnd, global_miditimer );
		KillTimer( hWnd, global_miditimer_programchange );
        CleanupAnimation();
		CloseGL( hWnd, hDC, hRC );
		if(pFILE) fclose(pFILE);
		//erase vector
		for(it=global_pairvector.begin(); it!=global_pairvector.end(); it++)
		{
			if(*it) delete *it;
		}
		global_pairvector.clear();
		////////////////////
		//terminate portmidi
		////////////////////
		if(global_pPmStream) 
		{
			if(global_prevnote>-1 && global_prevnote<=127)
			{
				PmEvent myPmEvent;
				myPmEvent.timestamp = 0;
				myPmEvent.message = Pm_Message(0x90+global_outputmidichannel, global_prevnote, 0);
				//send midi event
				Pm_Write(global_pPmStream, &myPmEvent, 1);
			}

			Pm_Close(global_pPmStream);
		}
		//Pt_Stop();
		Pm_Terminate();

		//nShowCmd = false;
		ShellExecuteA(NULL, "open", "end.bat", "", NULL, false);

	}
    return 0;

  case WM_TIMER:
	  if(wParam==global_miditimer)
	  {
		  OnTimer(hDC); //animate!
	  }  
	  else if(wParam==global_miditimer_programchange)
	  {
		  OnTimerMidiProgramChange();
	  }
    return 0;                           

  }

  return DefScreenSaverProc(hWnd, message, wParam, lParam );

}

bool bTumble = true;


BOOL WINAPI
ScreenSaverConfigureDialog(HWND hDlg, UINT message, 
                           WPARAM wParam, LPARAM lParam)
{

  //InitCommonControls();  
  //would need this for slider bars or other common controls

  HWND aCheck;

  switch ( message ) 
  {

        case WM_INITDIALOG:
                LoadString(hMainInstance, IDS_DESCRIPTION, szAppName, 40);

                GetConfig();
				/*
                aCheck = GetDlgItem( hDlg, IDC_TUMBLE );
                SendMessage( aCheck, BM_SETCHECK, 
                bTumble ? BST_CHECKED : BST_UNCHECKED, 0 );
				*/

    return TRUE;

  case WM_COMMAND:
    switch( LOWORD( wParam ) ) 
                { 
			/*
            case IDC_TUMBLE:
                        bTumble = (IsDlgButtonChecked( hDlg, IDC_TUMBLE ) == BST_CHECKED);
                        return TRUE;
			*/
                //cases for other controls would go here

            case IDOK:
                        WriteConfig(hDlg);      //get info from controls
                        EndDialog( hDlg, LOWORD( wParam ) == IDOK ); 
                        return TRUE; 

                case IDCANCEL: 
                        EndDialog( hDlg, LOWORD( wParam ) == IDOK ); 
                        return TRUE;   
                }

  }     //end command switch

  return FALSE; 
}



// needed for SCRNSAVE.LIB
BOOL WINAPI RegisterDialogClasses(HANDLE hInst)
{
  return TRUE;
}


/////////////////////////////////////////////////
////   INFRASTRUCTURE ENDS, SPECIFICS BEGIN   ///
////                                          ///
////    In a more complex scr, I'd put all    ///
////     the following into other files.      ///
/////////////////////////////////////////////////


// Initialize OpenGL
static void InitGL(HWND hWnd, HDC & hDC, HGLRC & hRC)
{
  
  PIXELFORMATDESCRIPTOR pfd;
  ZeroMemory( &pfd, sizeof pfd );
  pfd.nSize = sizeof pfd;
  pfd.nVersion = 1;
  //pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL; //blaine's
  pfd.dwFlags = PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
  pfd.iPixelType = PFD_TYPE_RGBA;
  pfd.cColorBits = 24;
  
  hDC = GetDC( hWnd );
  
  int i = ChoosePixelFormat( hDC, &pfd );  
  SetPixelFormat( hDC, i, &pfd );

  hRC = wglCreateContext( hDC );
  wglMakeCurrent( hDC, hRC );

}

// Shut down OpenGL
static void CloseGL(HWND hWnd, HDC hDC, HGLRC hRC)
{
  wglMakeCurrent( NULL, NULL );
  wglDeleteContext( hRC );

  ReleaseDC( hWnd, hDC );
}


void SetupAnimation(HDC hDC, int Width, int Height)
{
		//glFont font
		GLuint textureName;
		glGenTextures(1, &textureName);
		global_GLFont.Create("arial-10.glf", textureName);
		//global_GLFont.Create("arial-20.glf", textureName);
		//global_GLFont.Create("arial-bold-10.glf", textureName);
		//global_GLFont.Create("segeo-script-10.glf", textureName);

		/*
		//spi, begin
		createPalette();
		stepX = (maxX-minX)/(GLfloat)Width; // calculate new value of step along X axis
		stepY = (maxY-minY)/(GLfloat)Height; // calculate new value of step along Y axis
		glShadeModel(GL_SMOOTH);
		glEnable(GL_DEPTH_TEST);
		//spi, end
		*/
        //window resizing stuff
        glViewport(0, 0, (GLsizei) Width, (GLsizei) Height);

        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        
		//spi, begin
        //glOrtho(-300, 300, -240, 240, 25, 75); //original
        //glOrtho(-300, 300, -8, 8, 25, 75); //spi, last
		glOrtho(-2.0f, 2.0f, -2.0f, 2.0f, ((GLfloat)-1), (GLfloat)1); //spi
        //glOrtho(-150, 150, -120, 120, 25, 75); //spi
		//spi, end
        
		
		glMatrixMode(GL_MODELVIEW);

        glLoadIdentity();
		/*
        gluLookAt(0.0, 0.0, 50.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0); //original
                //camera xyz, the xyz to look at, and the up vector (+y is up)
        */

        //background
        glClearColor(0.0, 0.0, 0.0, 0.0); //0.0s is black

		// set the foreground (pen) color
		//glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
		glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

		// enable blending
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		// enable point smoothing
		glEnable(GL_POINT_SMOOTH);
		glPointSize(1.0f);		
		
		/*
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL); 
        glShadeModel(GL_SMOOTH); 
		*/

        //no need to initialize any objects
        //but this is where I'd do it

		/*
        glColor3f(0.1, 1.0, 0.3); //green
		*/
		// compute some initial iterations to settle into the orbit of the attractor
		//for (int i = 0; i &lt; initialIterations; i++) 
		for (int i = 0; i<initialIterations; i++) 
		{
 
			// compute a new point using the strange attractor equations
			float xnew = sin(y*b) + c*sin(x*b);
			float ynew = sin(x*a) + d*sin(y*a);
 
			// save the new point
			x = xnew;
			y = ynew;
		}
		//spi, begin
		//draw strange attractor once
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glPushMatrix();
		glBegin(GL_POINTS);
			// go through the equations many times, drawing a point for each iteration
			for (int i = 0; i<iterations; i++) 
			//for (int i = 0; i<10; i++) 
			{
				// compute a new point using the strange attractor equations
				float xnew = sin(y*b) + c*sin(x*b);
				float ynew = sin(x*a) + d*sin(y*a);
 
				// save the new point
				x = xnew;
				y = ynew;
 
				// draw the new point
				glVertex3f(x, y, 0.0f);
			}
		glEnd();
		glFlush();
        SwapBuffers(hDC);
        glPopMatrix();

		glColor4f(1.0f, 0.0f, 0.0f, 1.0f); //red
		//spi, end
}

static GLfloat spin=0;   //a global to keep track of the square's spinning

int	global_count=0;
int global_axiscoef=1;
void OnTimer(HDC hDC) //increment and display
{
		char buffer[100];
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		/* //no rotation
        spin = spin + 1; 
		*/
        glPushMatrix();
        /*
		glRotatef(spin, 0.0, 0.0, 1.0);
		*/
        glPushMatrix();
		

		global_pairvector.clear();

		// draw some points
		glBegin(GL_POINTS);
 
			// go through the equations many times, drawing a point for each iteration
			//for (int i = 0; i &lt; iterations; i++) 
			for (int i = 0; i<iterations; i++) 
			{
 
				// compute a new point using the strange attractor equations
				float xnew = sin(y*b) + c*sin(x*b);
				float ynew = sin(x*a) + d*sin(y*a);
 
				// save the new point
				x = xnew;
				y = ynew;
 
				// draw the new point
				glVertex3f(x, y, 0.0f);
			}
 
		glEnd();

		//add last pair to vector
		global_pairvector.push_back(new glfloatpair(x,y));

		if(global_bdrawlabel)
		{
			glEnable(GL_TEXTURE_2D);
			//glColor4f(1.0f, 0.0f, 0.0f, 1.0f); //red
			//glColor4f(0.0f, 0.0f, 1.0f, 1.0f); //blue
			glScalef(0.02, 0.02, 0.02);
			for (int ii = 0; ii<1; ii++) 
			//for (int ii = 0; ii<iterations; ii++) 
			//for (int ii = 0; ii<10; ii++) 
			{
 
				// compute a new point using the strange attractor equations
				//float xnew = sin(y*b) + c*sin(x*b);
				//float ynew = sin(x*a) + d*sin(y*a);
 
				// save the new point
				//x = xnew;
				//y = ynew;
 				//spi, begin
				GLfloat xlabel = (global_pairvector[ii])->x;
				GLfloat ylabel = (global_pairvector[ii])->y; 

				//sprintf_s(buffer, 100-1, "%d", ii); 
				sprintf_s(buffer, 100-1, "+", ii); 
				global_GLFont.TextOut(buffer, xlabel/0.02,ylabel/0.02,0);
				//glColor4f(1.0f, 1.0f, 1.0f, 1.0f); //white
				//glScalef(1.0, 1.0, 1.0);
				//spi, end
				//glFlush();
			}
			glDisable(GL_TEXTURE_2D);
			//glColor4f(1.0f, 1.0f, 1.0f, 1.0f); //white
			glScalef(1.0, 1.0, 1.0);
		}

        glPopMatrix();

        glFlush();
        SwapBuffers(hDC);
        glPopMatrix();


		if(global_bsendmidi)
		{
			PmEvent myPmEvent;

			if(global_prevnote>-1 && global_prevnote<=127)
			{
				//PmEvent myPmEvent;
				myPmEvent.timestamp = 0;
				myPmEvent.message = Pm_Message(0x90+global_outputmidichannel, global_prevnote, 0);
				//send midi event
				Pm_Write(global_pPmStream, &myPmEvent, 1);
			}
			myPmEvent.timestamp = 0;
			int midinote = (x/2.0f)*64+64;
			if(midinote>=128) midinote=127;
			if(midinote<=0) midinote=0;
			if(global_bsendmidi_usingremap)
			{
				midinote = GetNoteRemap_C_MinorPentatonic(midinote);
			}
			if(midinote>=128) midinote=127;
			if(midinote<=0) midinote=0;
			//myPmEvent.message = Pm_Message(0x90+global_outputmidichannel, midinote, 100);
			int midinotevelocity = (y/2.0f)*64+64;
			if(midinotevelocity>=128) midinotevelocity=127;
			if(midinotevelocity<=0) midinotevelocity=0;
			myPmEvent.message = Pm_Message(0x90+global_outputmidichannel, midinote, midinotevelocity);
			//send midi event
			Pm_Write(global_pPmStream, &myPmEvent, 1);
			global_prevnote=midinote;

			//PmEvent myPmEvent;
			myPmEvent.timestamp = 0;
			int midicontrolvalue = (y/2.0f)*64+64;
			if(midicontrolvalue>=128) midicontrolvalue=127;
			if(midicontrolvalue<=0) midicontrolvalue=0;
			myPmEvent.message = Pm_Message(0xB0+global_outputmidichannel, global_midicontrolnumber, midicontrolvalue);
			//send midi event
			Pm_Write(global_pPmStream, &myPmEvent, 1);
		}

		/*
		//change coefficients a and b for next blit
		a = RandomFloat(-3.0, 3.0);
		b = RandomFloat(-3.0, 3.0);
		//change coefficients c and d for next blit
		c = RandomFloat(-0.5, 1.0);
		d = RandomFloat(-0.5, 1.0);
		*/
		/*
		//change only one coefficient gradually along one axis
		a = a+0.1;
		if(a>3.0) a=-3.0;
		b = b+0.1;
		if(b>3.0) b=-3.0;
		c = c+0.1;
		if(c>1.0) c=-0.5;
		d = d+0.1;
		if(d>1.0) d=-0.5;
		*/
		/*
		a = a+0.1;
		if(a>3.0) a=-3.0;
		c = c+0.1;
		if(c>1.0) c=-0.5;
		*/
		switch(global_axiscoef)
		{
		case 1:
			a = a+0.1;
			if(a>3.0) 
			{
				a=-3.0;
				global_count++;
				if(global_count==2) 
				{
					global_count=0;
					a = -0.966918;		// coefficients for "The King's Dream"
					b = 2.879879;		
					c = 0.765145;
					d = 0.744728;
					global_axiscoef=RandomInteger(1,4);
				}
			}
			break;
		case 2:
			b = b+0.1;
			if(b>3.0) 
			{
				b=-3.0;
				global_count++;
				if(global_count==2) 
				{
					global_count=0;
					a = -0.966918;		// coefficients for "The King's Dream"
					b = 2.879879;		
					c = 0.765145;
					d = 0.744728;
					global_axiscoef=RandomInteger(1,4);
				}
			}
			break;
		case 3:
			c = c+0.1;
			if(c>1.0) 
			{
				c=-0.5;
				global_count++;
				if(global_count==2) 
				{
					global_count=0;
					a = -0.966918;		// coefficients for "The King's Dream"
					b = 2.879879;		
					c = 0.765145;
					d = 0.744728;
					global_axiscoef=RandomInteger(1,4);
				}
			}
			break;
		case 4:
			d = d+0.1;
			if(d>1.0) 
			{
				d=-0.5;
				global_count++;
				if(global_count==2) 
				{
					global_count=0;
					a = -0.966918;		// coefficients for "The King's Dream"
					b = 2.879879;		
					c = 0.765145;
					d = 0.744728;
					global_axiscoef=RandomInteger(1,4);
				}
			}
			break;
		}
		//reset position for net blit
		x = 0.1;
		y = 0.1;
}

void OnTimerMidiProgramChange()
{
	if(global_bsendmidi)
	{
		PmEvent myPmEvent;

		int random_integer;
		int lowest=0;
		int highest=128-1;
		//int highest=7-1;
		int range=(highest-lowest)+1;
		//random_integer = lowest+int(range*rand()/(RAND_MAX + 1.0));
		random_integer = lowest+rand()%range;
		//fprintf(pFILE, "random_integer=%d\n", random_integer);	
		
		//PmEvent myPmEvent;
		myPmEvent.timestamp = 0;
		int midiprogramnumber = random_integer;
		if(midiprogramnumber>=128) midiprogramnumber=127;
		if(midiprogramnumber<=0) midiprogramnumber=0;
		int notused = 0;
		myPmEvent.message = Pm_Message(0xC0+global_outputmidichannel, midiprogramnumber, 0x00);
		//myPmEvent.message = Pm_Message(192+global_outputmidichannel, midiprogramnumber, 0);
		//send midi event
		Pm_Write(global_pPmStream, &myPmEvent, 1);
		
	}
}


void CleanupAnimation()
{
        //didn't create any objects, so no need to clean them up
}



/////////   REGISTRY ACCESS FUNCTIONS     ///////////

void GetConfig()
{

        HKEY key;
        //DWORD lpdw;

        if (RegOpenKeyEx( HKEY_CURRENT_USER,
                //"Software\\GreenSquare", //lpctstr
                L"Software\\Oifii\\spissstrangeattractormanymidi", //lpctstr
                0,                      //reserved
                KEY_QUERY_VALUE,
                &key) == ERROR_SUCCESS) 
        {
                DWORD dsize = sizeof(bTumble);
                DWORD dwtype =  0;

                //RegQueryValueEx(key,"Tumble", NULL, &dwtype, (BYTE*)&bTumble, &dsize);
                RegQueryValueEx(key,L"Tumble", NULL, &dwtype, (BYTE*)&bTumble, &dsize);


                //Finished with key
                RegCloseKey(key);
        }
        else //key isn't there yet--set defaults
        {
                bTumble = true;
        }
        
}

void WriteConfig(HWND hDlg)
{

        HKEY key;
        DWORD lpdw;

        if (RegCreateKeyEx( HKEY_CURRENT_USER,
                //"Software\\GreenSquare", //lpctstr
                L"Software\\Oifii\spissstrangeattractormanymidi", //lpctstr
                0,                      //reserved
                L"",                     //ptr to null-term string specifying the object type of this key
                REG_OPTION_NON_VOLATILE,
                KEY_WRITE,
                NULL,
                &key,
                &lpdw) == ERROR_SUCCESS)
                
        {
                //RegSetValueEx(key,"Tumble", 0, REG_DWORD, 
                RegSetValueEx(key,L"Tumble", 0, REG_DWORD, 
                        (BYTE*)&bTumble, sizeof(bTumble));

                //Finished with keys
                RegCloseKey(key);
        }

}