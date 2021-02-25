/// @file       LTAPI.cpp
/// @brief      LiveTraffic API - Example Plugin
/// @details    This plugin demonstrates a simple and a more complex
///             way of using LTAPI in a fully functional plugin,
///             which opens two windows with aircraft information
///             read from LiveTraffic.
///
///             This example plugin is based on the Hello World SDK 3 plugin
///             downloaded from https://developer.x-plane.com/code-sample/hello-world-sdk-3/
///             and comes with no copyright notice.
///             It is changed, however, to also work with SDK 2.10, i.e. XP10
///             But you can also define all up to XPLM301, if you want.
/// @see        https://twinfan.github.io/LTAPI/
/// @author     Birger Hoppe
/// @copyright  (c) 2019-2020 Birger Hoppe
/// @copyright  Permission is hereby granted, free of charge, to any person obtaining a
///             copy of this software and associated documentation files (the "Software"),
///             to deal in the Software without restriction, including without limitation
///             the rights to use, copy, modify, merge, publish, distribute, sublicense,
///             and/or sell copies of the Software, and to permit persons to whom the
///             Software is furnished to do so, subject to the following conditions:\n
///             The above copyright notice and this permission notice shall be included in
///             all copies or substantial portions of the Software.\n
///             THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
///             IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
///             FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
///             AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
///             LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
///             OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
///             THE SOFTWARE.

#ifndef XPLM210
#error This is made to be compiled at least against the XPLM210 SDK
#endif

// include LTAPI header
#include "LTAPI.h"

// include other headers
#include "XPLMDisplay.h"
#include "XPLMGraphics.h"
#include "XPLMProcessing.h"
#include "XPLMUtilities.h"
#include <string.h>
#include <cmath>

//
// MARK: Globals
//

// An opaque handle to the window we will create
static XPLMWindowID	g_winSimple = NULL;
static XPLMWindowID g_winEnhanced = NULL;

float COL_WHITE[3]    = {1.00f, 1.00f, 1.00f};
float COL_YELLOW[3]   = {1.00f, 1.00f, 0.00f};
float COL_GREY[3]     = {0.75f, 0.75f, 0.75f};
float COL_CYAN[3]     = {0.50f, 1.00f, 1.00f};

// Callbacks we will register when we create our window
void				draw_list_simple(XPLMWindowID in_window_id, void * in_refcon);
void                draw_list_enhanced(XPLMWindowID in_window_id, void * in_refcon);
int					dummy_mouse_handler(XPLMWindowID /*in_window_id*/, int /*x*/, int /*y*/, int /*is_down*/, void * /*in_refcon*/) { return 0; }
XPLMCursorStatus	dummy_cursor_status_handler(XPLMWindowID /*in_window_id*/, int /*x*/, int /*y*/, void * /*in_refcon*/) { return xplm_CursorDefault; }
int					dummy_wheel_handler(XPLMWindowID /*in_window_id*/, int /*x*/, int /*y*/, int /*wheel*/, int /*clicks*/, void * /*in_refcon*/) { return 0; }
void				dummy_key_handler(XPLMWindowID /*in_window_id*/, char /*key*/, XPLMKeyFlags /*flags*/, char /*virtual_key*/, void * /*in_refcon*/, int /*losing_focus*/) { }

// flight loop callback to update list of LiveTraffic aircrafts
constexpr float UPDATE_INTVL    = 1.0;      // [s] how often to call the callback
float LoopCBOneTimeInit (float, float, int, void*);
float LoopCBUpdateAcListSimple (float, float, int, void*);
float LoopCBUpdateAcListEnhanced (float, float, int, void*);

class EnhAircraft;
void SetEnhWndTitle(EnhAircraft* pAcOnCam);

//
// MARK: Plugin Main Functions
//

PLUGIN_API int XPluginStart(
							char *		outName,
							char *		outSig,
							char *		outDesc)
{
	strcpy(outName, "LT API Example");
	strcpy(outSig, "TwinFan.plugin.LTAPIExample");
	strcpy(outDesc, "Example plugin using LT API, also requires LiveTraffic to provide data");
    
	XPLMCreateWindow_t params;
	params.structSize = sizeof(params);
	params.visible = 1;
	params.drawWindowFunc = draw_list_simple;
	// Note on "dummy" handlers:
	// Even if we don't want to handle these events, we have to register a "do-nothing" callback for them
	params.handleMouseClickFunc = dummy_mouse_handler;
#ifdef XPLM301
	params.handleRightClickFunc = dummy_mouse_handler;
#endif
	params.handleMouseWheelFunc = dummy_wheel_handler;
	params.handleKeyFunc = dummy_key_handler;
	params.handleCursorFunc = dummy_cursor_status_handler;
	params.refcon = NULL;
#ifdef XPLM301
	params.layer = xplm_WindowLayerFloatingWindows;
    // Opt-in to styling our window like an X-Plane 11 native window
	// If you're on XPLM300, not XPLM301, swap this enum for the literal value 1.
	params.decorateAsFloatingWindow = xplm_WindowDecorationRoundRectangle;
#endif

	// Set the window's initial bounds
	// Note that we're not guaranteed that the main monitor's lower left is at (0, 0)...
	// We'll need to query for the global desktop bounds!
	int left=0, right=0, top=0;
#ifdef XPLM301
    int bottom=0;
	XPLMGetScreenBoundsGlobal(&left, &top, &right, &bottom);
#else
    XPLMGetScreenSize(&right,&top);
#endif
	params.left = left + 50;
    params.right = params.left + 400;           // width: 400
    params.top  = top - 200;
    params.bottom = params.top - 200;           // height: 200
	
	g_winSimple = XPLMCreateWindowEx(&params);
	
#ifdef XPLM301
	// Position the window as a "free" floating window, which the user can drag around
	XPLMSetWindowPositioningMode(g_winSimple, xplm_WindowPositionFree, -1);
	// Limit resizing our window: maintain a minimum width/height of 100 boxels and a max width/height of 300 boxels
	// XPLMSetWindowResizingLimits(g_window, 200, 200, 300, 300);
    XPLMSetWindowTitle(g_winSimple, "LTAPI Example: Simple List");
#endif
    
    // *** Create a second window for the enhanced list ***
    // put it below the simple window:
    params.top  = params.bottom - 20;
    params.bottom = params.top - 200;           // height: 200
    params.right = params.left + 920;           // width: 670

    // use the drawing function for the enhanced example
    params.drawWindowFunc = draw_list_enhanced;

    g_winEnhanced = XPLMCreateWindowEx(&params);
    
#ifdef XPLM301
    // Position the window as a "free" floating window, which the user can drag around
    XPLMSetWindowPositioningMode(g_winEnhanced, xplm_WindowPositionFree, -1);
    // Limit resizing our window: maintain a minimum width/height of 100 boxels and a max width/height of 300 boxels
    // XPLMSetWindowResizingLimits(g_window, 200, 200, 300, 300);
    XPLMSetWindowTitle(g_winEnhanced, "LTAPI Example: Enhanced List");
#endif

	return g_winSimple != NULL && g_winEnhanced != NULL;
}

PLUGIN_API void	XPluginStop(void)
{
	// Since we created the window, we'll be good citizens and clean it up
	XPLMDestroyWindow(g_winSimple);
	g_winSimple = NULL;
}


// Enable/Dsiable register/unregister a flight loop callback to be called
// every second. This callback then updates the list of aircrafts.
// (We don't want to update the list with every drawing cycle,
//  just to safe on performance. That's why we don't call UpdateAcList
//  from the window drawing callback.)
// Also, all plugins should have finished initialization when the flight loop
// starts, no matter which startup order. So it should be safe querying
// dataRefs then.
PLUGIN_API int  XPluginEnable(void)
{
    XPLMRegisterFlightLoopCallback(LoopCBOneTimeInit, -1, NULL);
    XPLMRegisterFlightLoopCallback(LoopCBUpdateAcListSimple, UPDATE_INTVL, NULL);
    XPLMRegisterFlightLoopCallback(LoopCBUpdateAcListEnhanced, UPDATE_INTVL, NULL);
    return 1;
}

PLUGIN_API void XPluginDisable(void)
{
    XPLMUnregisterFlightLoopCallback(LoopCBUpdateAcListSimple, NULL);
    XPLMUnregisterFlightLoopCallback(LoopCBUpdateAcListEnhanced, NULL);
}

PLUGIN_API void XPluginReceiveMessage(XPLMPluginID /*inFrom*/, int /*inMsg*/, void * /*inParam*/) { }

//
// MARK: Late init callback
//
// How to properly read one-time information.
// Avoid trying in XPluhinStart or XPluginEnable, as
// - depending on Startup order - LT might not be available by that time.
// During a first flight loop callback all plugins have been initialized and should be available.
float LoopCBOneTimeInit(float, float, int, void*)
{
    // Make a fancy window title displaying LiveTraffic's version information
    SetEnhWndTitle(nullptr);

    // don't call me again
    return 0.0f;
}



//
// MARK: LTAPI Simple Example
//
// Most simplistic usage of LTAPI:
// 1. Have one (probably even static) object of LTAPIConnect
// 2. Call LTAPIConnect::UpdateAcList regularly (but probably not with every drawing cycle!)
// 3. Access the map of aircrafts in a loop and use its data
//

// 1. Have one (probably even static) object of LTAPIConnect
LTAPIConnect ltSimple;

float LoopCBUpdateAcListSimple (float, float, int, void*)
{
    // 2. Call LTAPIConnect::UpdateAcList regularly (but probably not with every drawing cycle!)
    ltSimple.UpdateAcList();
    return UPDATE_INTVL;
}

void	draw_list_simple(XPLMWindowID in_window_id, void * /*in_refcon*/)
{
	// Mandatory: We *must* set the OpenGL state before drawing
	// (we can't make any assumptions about it)
	XPLMSetGraphicsState(
						 0 /* no fog */,
						 0 /* 0 texture units */,
						 0 /* no lighting */,
						 0 /* no alpha testing */,
						 1 /* do alpha blend */,
						 1 /* do depth testing */,
						 0 /* no depth writing */
						 );
	
    // our window coordinates
	int l, t, r, b;
	XPLMGetWindowGeometry(in_window_id, &l, &t, &r, &b);
    
    // translucent background
    XPLMDrawTranslucentDarkBox(l, t, r, b);
    
    // first writing pos
    l += 10;
    t -= 20;
    
    // 3. Access the map of aircrafts in a loop and use its data
    // cycle list of aircrafts and output some info, line by line
    char buf[500];
    for (const MapLTAPIAircraft::value_type &pair: ltSimple.getAcMap())
    {
        // put together some information about the a/c
        const LTAPIAircraft& ac = *pair.second;
        snprintf(buf, sizeof(buf),
                 "%s (%s) %6.3f%c %6.3f%c %5.0fft %03.0f° %3.0fkn - %s",
                 ac.getKey().c_str(),
                 ac.getModelIcao().c_str(),
                 std::fabs(ac.getLat()),
                 ac.getLat() >= 0.0f ? 'N' : 'S',
                 std::fabs(ac.getLon()),
                 ac.getLon() >= 0.0f ? 'E' : 'W',
                 ac.getAltFt(),
                 ac.getHeading(),
                 ac.getSpeedKn(),
                 ac.getPhaseStr().c_str());
        // output a line (for simplicity we don't care about the window's width...)
        XPLMDrawString(COL_WHITE, l, t, buf, NULL, xplmFont_Proportional);

        // next line...past bottom of window?
        if ((t -= 15) <= b)
            break;
    }
    
	
}

//
// MARK: LTAPI Enhanced Example
//
// Demonstrates how to use LTAPIAircraft as a base class for some
// own class, which stores additional information
//

//
// Example subclass manages the line number in the output display,
// i.e. once it found a line it stays there.
// Also allows to show text "---removed---" for some time when a/c was removed
//
constexpr int MAX_LN = 100;

class EnhAircraft : public LTAPIAircraft
{
public:
    int ln = -1;                // line number of display output
    enum EnhDispTy {
        ED_NONE = 0,            // not yet displayed
        ED_SHOWN,               // has a line to display
        ED_SHOW_REMOVED,        // a/c gone, displaying "--- removed ---"
        ED_SHOW_REMOVED_2,      // we display that line for 3s
        ED_SHOW_REMOVED_3,
        ED_OUTDATED             // remove me!
    } dispStatus = ED_NONE;
    bool bAcDeleted = false;    // has this a/c been removed from LT?
public:
    EnhAircraft();
    ~EnhAircraft() override;
    // we add some simplistic logic to derive a line number for output
    bool updateAircraft(const LTAPIBulkData& __bulk, size_t __inSize) override;
    // test out notifications of camera toggle
    void toggleCamera (bool bCameraActive, SPtrLTAPIAircraft spPrevAc) override;
    
    static EnhAircraft* lnTaken[MAX_LN];
    // we move the ability to output a line into this class
    void DrawOutput(int x, int y, int r, int b);
    // this creates a new EnhAircraft object
    static LTAPIAircraft* CreateNewObject() { return new EnhAircraft(); }
};

// keeps track of taken output lines
EnhAircraft* EnhAircraft::lnTaken[MAX_LN] =
{
    nullptr,nullptr,nullptr,nullptr,nullptr,
    nullptr,nullptr,nullptr,nullptr,nullptr,
    nullptr,nullptr,nullptr,nullptr,nullptr,
};

EnhAircraft::EnhAircraft() : LTAPIAircraft()
{ /* we could do additional init work here*/ }

EnhAircraft::~EnhAircraft()
{
    // if we occupied a line then we free it
    if (0 <= ln && ln < MAX_LN)
        lnTaken[ln] = nullptr;
}

bool EnhAircraft::updateAircraft(const LTAPIBulkData& __bulk, size_t __inSize)
{
    // first we call the LTAPI do fetch (updated) data for the a/c
    if (!LTAPIAircraft::updateAircraft(__bulk,__inSize))
        return false;
    
    // then we do our own logic
    // here we just quickly find an empty display line
    if (ln < 0) {
        for (int i = 0; i < MAX_LN; i++) {
            if (!lnTaken[i]) {
                // we take this line
                ln = i;
                lnTaken[i] = this;
                break;
            }
        }
    } else {
        // move on the status (so that color changes from yellow to white)
        if (dispStatus == ED_NONE)
            dispStatus = ED_SHOWN;
    }
    
    return true;
}

// test out notifications of camera toggle
void EnhAircraft::toggleCamera (bool bCameraActive, SPtrLTAPIAircraft spPrevAc)
{
    // format a nice log message about camera toggling events
    char buf[200];
    if (bCameraActive && spPrevAc)
        snprintf (buf, sizeof(buf),
                  "LTAPIExample: Camera moved from '%s' to '%s'\n",
                  spPrevAc->getDescription().c_str(),
                  getDescription().c_str());
    else if (bCameraActive && !spPrevAc)
        snprintf (buf, sizeof(buf),
                  "LTAPIExample: Camera now on '%s'\n",
                  getDescription().c_str());
    else
        snprintf (buf, sizeof(buf),
                  "LTAPIExample: Camera now off, was previously on '%s'\n",
                  getDescription().c_str());
    XPLMDebugString(buf);
    
    // Update Window Title with this info
    SetEnhWndTitle(bCameraActive ? this : nullptr);
}

// we move the ability to output a line into this class

// Draw a C string with a given pixel length (simplified)
#define DRAW_C(w,s,fnt)                                                             \
if (x+w > r) { strcpy(buf,">>"); XPLMDrawString(col,r-20,y,buf,NULL,fnt); return; } \
XPLMDrawString(col,x,y,s,NULL,fnt);                                                 \
x += w

// Draw a constant text with a given pixel length (simplified)
#define DRAW_T(w,s,fnt)                                         \
strcpy(buf,s);              \
DRAW_C(w,buf,fnt)

// Draw a std::string with a given pixel length (simplified)
#define DRAW_S(w,s)                                         \
strcpy(buf,s.substr(0,sizeof(buf)-1).c_str());              \
DRAW_C(w,buf,xplmFont_Proportional)

// Draw a double with a given pixel length (simplified)
#define DRAW_N(w,n,dig,dec)                                 \
snprintf(buf,sizeof(buf),"%*.*f",dig,dec,n);                \
DRAW_C(w,buf,xplmFont_Basic)

void EnhAircraft::DrawOutput(int x, int y, int r, int)
{
    char buf[500];
    if (dispStatus == ED_SHOWN || dispStatus == ED_NONE)
    {
        float* const col = dispStatus == ED_NONE ? COL_YELLOW : COL_WHITE;

        // write output in more or less well aligned colums
        DRAW_S(55, getRegistration());
        DRAW_S(60, getCallSign());
        DRAW_S(60, getFlightNumber());
        DRAW_S(40, getOrigin());
        DRAW_S(50, getDestination());
        DRAW_S(40, getModelIcao());
        DRAW_S(30, getAcClass());
        DRAW_S(30, getWtc());
        snprintf(buf, sizeof(buf),              // nice location format
                 "%6.3f%c %6.3f%c",
                 std::fabs(getLat()),
                 getLat() >= 0.0f ? 'N' : 'S',
                 std::fabs(getLon()),
                 getLon() >= 0.0f ? 'E' : 'W');
        DRAW_C(110, buf, xplmFont_Basic);
        DRAW_N(35, getAltFt(), 5, 0);
        DRAW_T(15, getVSIft() < -100 ? "v" : getVSIft() > 100 ? "^" : "", xplmFont_Proportional);
        DRAW_N(30, getHeading(), 3, 0);
        DRAW_N(30, getSpeedKn(), 3, 0);
        DRAW_N(30, getBearing(), 4, 0);
        DRAW_N(35, getDistNm(), 4, 1);
        DRAW_S(80, getPhaseStr());
        DRAW_S(60, getKey());
        if (getMultiIdx() > 0) {
            DRAW_N(20, double(getMultiIdx()), 2, 0);
        } else {
            DRAW_T(20, "", xplmFont_Proportional);
        }
        DRAW_T(20, isOnCamera() ? "X" : "", xplmFont_Proportional);
        DRAW_S(180, getCslModel());
        DRAW_S(150, getTrackedBy());
        DRAW_S(200, getCatDescr());
    }
    else if (dispStatus >= ED_SHOW_REMOVED)
    {
        float* col = COL_WHITE;
        DRAW_S(55, getRegistration());
        DRAW_S(60, getCallSign());
        DRAW_S(60, getFlightNumber());
        col = COL_GREY;
        DRAW_T(40, "--- removed ---", xplmFont_Proportional);
    }
}



// 1. Have one (probably even static) object of LTAPIConnect
//    This time we pass in our own object creation callback,
//    so that objects are of type EnhAircraft
LTAPIConnect ltEnhanced(EnhAircraft::CreateNewObject, 10);
// And we manage removed aircrafts ourself!
ListLTAPIAircraft listRemovedAc;

float LoopCBUpdateAcListEnhanced (float, float, int, void*)
{
    // 2. Call LTAPIConnect::UpdateAcList regularly (but probably not with every drawing cycle!)
    ltEnhanced.UpdateAcList(&listRemovedAc);
    
    // Maintenance of removed aircraft entries
    for (ListLTAPIAircraft::iterator rmIter = listRemovedAc.begin();
         rmIter != listRemovedAc.end();
         /* no loop increment, is done in switch below */)
    {
        EnhAircraft* pEnh = dynamic_cast<EnhAircraft*>(rmIter->get());
        if (pEnh) {         // As we created all objects they should all be of _our_ type!
            switch (pEnh->dispStatus) {
                case EnhAircraft::ED_OUTDATED:
                    // finally remove object also from out list
                    // this removes the object and calls its destructor
                    // (which in turn frees up the line)
                    rmIter = listRemovedAc.erase(rmIter);
                    break;
                case EnhAircraft::ED_NONE:
                case EnhAircraft::ED_SHOWN:
                    // tell the object it is now removed and shall say so:
                    pEnh->dispStatus = EnhAircraft::ED_SHOW_REMOVED;
                    rmIter++;       // next element in loop
                    break;
                default:
                    // (includes EnhAircraft::ED_SHOW_REMOVED*)
                    // Move on to the next REMOVED-status,
                    // so that after 3 increments we reach ED_OUTDATED:
                    pEnh->dispStatus = (EnhAircraft::EnhDispTy)(pEnh->dispStatus + 1);
                    rmIter++;
                    break;
            }
        } else {
            // should not get here!
            rmIter++;
        }
    }
    
    // call me again in a second
    return UPDATE_INTVL;
}

void    draw_header (int x, int y, int r)
{
    float* const col = COL_CYAN;
    char buf[50];
    DRAW_T(55,  "Reg",      xplmFont_Proportional);
    DRAW_T(60,  "Call",     xplmFont_Proportional);
    DRAW_T(60,  "Flight",   xplmFont_Proportional);
    DRAW_T(40,  "from",     xplmFont_Proportional);
    DRAW_T(50,  "to",       xplmFont_Proportional);
    DRAW_T(40,  "Mdl",      xplmFont_Proportional);
    DRAW_T(30,  "Cls",      xplmFont_Proportional);
    DRAW_T(30,  "WTC",      xplmFont_Proportional);
    DRAW_T(110, "Position", xplmFont_Proportional);
    DRAW_T(35,  "   ft",    xplmFont_Basic);
    DRAW_T(15,  "",         xplmFont_Basic);
    DRAW_T(30,  "  °",      xplmFont_Basic);
    DRAW_T(30,  " kn",      xplmFont_Basic);
    DRAW_T(30,  "Brng",     xplmFont_Basic);
    DRAW_T(35,  "Dist",     xplmFont_Basic);
    DRAW_T(80,  "Phase",    xplmFont_Proportional);
    DRAW_T(60,  "key",      xplmFont_Proportional);
    DRAW_T(20,  "#",        xplmFont_Proportional);
    DRAW_T(20,  "cam",      xplmFont_Proportional);
    DRAW_T(180, "CSL Model", xplmFont_Proportional);
    DRAW_T(150, "tracked by", xplmFont_Proportional);
    DRAW_T(200, "Category", xplmFont_Proportional);
}

void    draw_list_enhanced(XPLMWindowID in_window_id, void * /*in_refcon*/)
{
    // Mandatory: We *must* set the OpenGL state before drawing
    // (we can't make any assumptions about it)
    XPLMSetGraphicsState(
                         0 /* no fog */,
                         0 /* 0 texture units */,
                         0 /* no lighting */,
                         0 /* no alpha testing */,
                         1 /* do alpha blend */,
                         1 /* do depth testing */,
                         0 /* no depth writing */
                         );
    
    // our window coordinates
    int l, t, r, b;
    XPLMGetWindowGeometry(in_window_id, &l, &t, &r, &b);
    
    // translucent background
    XPLMDrawTranslucentDarkBox(l, t, r, b);
    
    // first writing pos
    l += 10;
    t -= 20;
    
    // Header
    draw_header(l, t, r);
    
    // next line...past bottom of window?
    if ((t -= 15) <= b)
        return;

    // We now cycle the 20 line items that our enhanced object keeps track of
    for (EnhAircraft* pEnh: EnhAircraft::lnTaken)
    {
        // output that aircraft's info
        if (pEnh)
            pEnh->DrawOutput(l,t,r,b);
        
        // next line...past bottom of window?
        if ((t -= 15) <= b)
            break;
    }
    
    
}

// Makes a nice title to the enhanced window
void SetEnhWndTitle(EnhAircraft* pAcOnCam)
{
    char szTitle[150];
    if (!pAcOnCam) {
        snprintf(szTitle, sizeof(szTitle), "LTAPI Example: Enhanced List - LiveTraffic v%.2f %d",
                 float(LTAPIConnect::getLTVerNr()) / 100.0f, LTAPIConnect::getLTVerDate());
    } else {
        snprintf(szTitle, sizeof(szTitle), "LTAPI Example: Enhanced List - LiveTraffic v%.2f %d viewing %s",
                 float(LTAPIConnect::getLTVerNr()) / 100.0f, LTAPIConnect::getLTVerDate(),
                 pAcOnCam->getDescription().c_str());
    }
    XPLMSetWindowTitle(g_winEnhanced, szTitle);
}
