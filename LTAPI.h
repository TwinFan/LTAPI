//
//  LiveTraffic API
//

/*
 * Copyright (c) 2019, Birger Hoppe
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef LTAPI_h
#define LTAPI_h

#include <memory>
#include <string>
#include <list>
#include <map>
#include <chrono>

#include "XPLMDataAccess.h"

//
// LTAPIAircraft
//
// Represents one aircraft as controlled by LiveTraffic.
//
// You can derive subclasses from this class if you want to
// add information specific to you app. Then you would need to
// provide a callback function "fCreateAcObject" so that you
// create new aircraft objects when required by LTAPIConnect.
//

// NOTE: Some fields aren't yet populated and are included
//       as future extension only.
//       They are marked with {{{ }}}
//       If temporarily another value/constact is returned
//       it is added with =value/"constant"

class LTAPIAircraft
{
private:
    // usually ICAO transponder hex code, but could also be any other
    // truly unique id per aircraft (FLARM ID, tail number...)
    unsigned        keyNum = 0;
    std::string     key;
protected:
    // identification
    std::string     transpHexIcao;
    std::string     registration;       // {{{aka tail number}}}

    // aircraft model/operator
    std::string     modelIcao = "A320"; // {{{ICAO aircraft type}}}="A320"
    std::string     acClass = "L2J";    // {{{a/c class like "L2J"}}}="L2J"
    std::string     opIcao;             // {{{ICAO-code of operator}}}
    
    // flight data
    std::string     callSign;           // {{{call sig}}}
    std::string     squawk;             // {{{squawk code}}}
    std::string     flightNumber;       // {{{flight number}}}
    std::string     origin;             // {{{origin airport (IATA or ICAO)}}}
    std::string     destination;        // {{{destination airport (IATA or ICAO)}}}
    
    // position, attitude
    float           lat     = 0.0f;         // [°] latitude
    float           lon     = 0.0f;         // [°] longitude
    float           alt_ft  = 0.0f;         // [ft] altitude
    float           heading = 0.0f;         // [°] heading
    float           track   = 0.0f;         // [°] {{{track}}}=heading
    float           roll    = 0.0f;         // [°] positive right
    float           pitch   = 0.0f;         // [°] positive up
    float           speed_kn= 0.0f;         // [kn] ground speed
    float           vsi_ft  = 0.0f;         // [ft/minute] vertical speed, positive up
    float           terrainAlt_ft=0.0f;     // [ft] terrain altitude beneath plane
    float           height_ft   = 0.0f;     // [ft] height AGL
    bool            onGnd       = false;    // Is plane on ground?
    enum LTFlightPhase {
        FPH_UNKNOWN     = 0,
        FPH_TAXI        = 10,
        FPH_TAKE_OFF    = 20,
        FPH_TO_ROLL,
        FPH_ROTATE,
        FPH_LIFT_OFF,
        FPH_INITIAL_CLIMB,
        FPH_CLIMB       = 30,
        FPH_CRUISE      = 40,
        FPH_DESCEND     = 50,
        FPH_APPROACH    = 60,
        FPH_FINAL,
        FPH_LANDING     = 70,
        FPH_FLARE,
        FPH_TOUCH_DOWN,                 // this is a one-frame-only 'phase'!
        FPH_ROLL_OUT,
        FPH_STOPPED_ON_RWY              // ...after artifically roll-out with no more live positions remaining
    }               phase = FPH_UNKNOWN;
    
    // configuration
    float           flaps = 0.0f;       // flap position: 0.0 retracted, 1.0 fully extended
    float           gear  = 0.0f;       // gear position: 0.0 retracted, 1.0 fully extended
    struct LTLights {
        bool beacon     : 1;
        bool strobe     : 1;
        bool nav        : 1;
        bool landing    : 1;
        bool taxi       : 1;            // {{{taxi light}}} = landing
    } lights = {false,false,false,false,false};
    
    // simulation
    float           bearing = 0.0f;     // [°] to current camera position
    float           dist_nm = 0.0f;     // [nm] distance to current camera
    
    // update helpers
    bool            bUpdated = false;
    
public:
    LTAPIAircraft();
    virtual ~LTAPIAircraft();
    
    // Updates an aircraft. If our key is defined it first verifies that the
    // key matches with the one currently available in the dataRefs.
    // Returns false if not.
    // If our key is not defined it just accepts anything available.
    // Updates all fields, set bUpdated and returns true.
    virtual bool updateAircraft();
    // helpers in update loop to detected removed aircrafts
    bool isUpdated () const { return bUpdated; }
    void resetUpdated ()    { bUpdated = false; }
    
    // data access
public:
    std::string     getKey()            const { return key; }
    // identification
    std::string     getTranspHexIcao()  const { return transpHexIcao; }
    std::string     getRegistration()   const { return registration; }          // {{{aka tail number}}}
    // aircraft model/operator
    std::string     getModelIcao()      const { return modelIcao; }             // {{{ICAO aircraft type}}}="A320"
    std::string     getAcClass()        const { return acClass; }               // {{{a/c class like "L2J"}}}="L2J"
    std::string     getOpIcao()         const { return opIcao; }                // {{{ICAO-code of operator}}}
    // flight data
    std::string     getCallSign()       const { return callSign; }              // {{{call sig}}}
    std::string     getSquawk()         const { return squawk; }                // {{{squawk code}}}
    std::string     getFlightNumber()   const { return flightNumber; }          // {{{flight number}}}
    std::string     getOrigin()         const { return origin; }                // {{{origin airport (IATA or ICAO)}}}
    std::string     getDestination()    const { return destination; }           // {{{destination airport (IATA or ICAO)}}}
    // position, attitude
    float           getLat()            const { return lat; }                   // [°] latitude
    float           getLon()            const { return lon; }                   // [°] longitude
    float           getAltFt()          const { return alt_ft; }                // [ft] altitude
    float           getHeading()        const { return heading; }               // [°] heading
    float           getTrack()          const { return track; }                 // [°] {{{track}}}=heading
    float           getRoll()           const { return roll; }                  // [°] positive right
    float           getPitch()          const { return pitch; }                 // [°] positive up
    float           getSpeedKn()        const { return speed_kn; }              // [kn] ground speed
    float           getVSIft()          const { return vsi_ft; }                // [ft/minute] vertical speed, positive up
    float           getTerrainFt()      const { return terrainAlt_ft; }         // [ft] terrain altitude beneath plane
    float           getHeightFt()       const { return height_ft; }             // [ft] height AGL
    bool            isOnGnd()           const { return onGnd; }                 // Is plane on ground?
    LTFlightPhase   getPhase()          const { return phase; }                 // flight phase
    std::string     getPhaseStr()       const;                                  //    "     "   as string
    // configuration
    float           getFlaps()          const { return flaps; }                 // flap position: 0.0 retracted, 1.0 fully extended
    float           getGear()           const { return gear; }                  // gear position: 0.0 retracted, 1.0 fully extended
    LTLights        getLights()         const { return lights; }                // all lights
    // simulation
    float           getBearing()        const { return bearing; }               // [°] to current camera position
    float           getDistNm()         const { return dist_nm; }               // [nm] distance to current camera

public:
    // This is the standard object creation callback:
    // It just returns an empty LTAPIAircraft object.
    static LTAPIAircraft* CreateNewObject() { return new LTAPIAircraft(); }
};

//
// MapLTAPIAircraft
//
// This is what LTAPIConnect returns: a map of all aircrafts.
// They key into the map is the aircraft's key (most often the
// ICAO transponder hex code).
// The value is a smart pointer to an LTAPIAircraft object.
// As we use smart pointers, object storage is deallocated as soon
// as objects are removed from the map. Effectively, the map manages
// storage.
//

typedef std::unique_ptr<LTAPIAircraft> UPtrLTAPIAircraft;
typedef std::map<std::string,UPtrLTAPIAircraft> MapLTAPIAircraft;
typedef std::list<UPtrLTAPIAircraft> ListLTAPIAircraft;

//
// LTAPIConnect
//
// Connects to LiveTraffic's dataRefs and returns aircraft information.
// Typically, one object of this class is used.
//

class LTAPIConnect
{
public:
    // callback function type: returns new LTAPIAircraft object (or derived class)
    typedef LTAPIAircraft* fCreateAcObject();
protected:
    fCreateAcObject* pfCreateAcObject = nullptr;
    
    // THE map of aircrafts
    MapLTAPIAircraft mapAc;
    
public:
    LTAPIConnect(fCreateAcObject* _pfCreateAcObject = LTAPIAircraft::CreateNewObject);
    virtual ~LTAPIConnect();
    
    // LiveTraffic available? (checks via XPLMFindPluginBySignature)
    static bool isLTAvail ();
    // Does LiveTraffic display aircrafts? (Is it activated?)
    // This is the only function which checks again and again if LiveTraffic's
    // dataRefs are available. Use this to verify if LiveTraffic is (now)
    // available before calling any other function on LiveTraffic's dataRefs.
    static bool doesLTDisplayAc ();
    // How many of them right now?
    static int getLTNumAc ();
    // Does it (also) control AI planes?
    // NOTE: If your plugin usually deals with AI/multiplayer planes,
    //       then you don't need to check for AI/multiplayer planes if
    //       doesLTControlAI is true: In this case the planes returned in the
    //       AI/multiplayer dataRefs are just a subset selected by LiveTraffic
    //       of what you get via LTAPI anyway. Avoid duplicates, just use LTAPI
    //       if doesLTControlAI.
    static bool doesLTControlAI ();
    // What's current simulated time in LiveTraffic (usually 'now' minus buffering period)?
    static time_t getLTSimTime ();
    static std::chrono::system_clock::time_point getLTSimTimePoint ();
    
    // Main function: updates map of aircrafts and returns reference to it.
    // If you want to know which a/c are removed during this call then pass
    // a ListLTAPIAircraft object; LTAPI will transfer otherwise removed
    // objects there and management of them is then up to you.
    // (LTAPI will only _emplace_back_ to the list, not remove anything.)
    const MapLTAPIAircraft& UpdateAcList (ListLTAPIAircraft* plistRemovedAc = nullptr);
    const MapLTAPIAircraft& getAcMap () const { return mapAc; }
};

//
// LTDataRef
//
// Represents a dataRef and covers late binding. Actually a helper only.
// Late binding is important: We read another plugins dataRefs. The other
// plugin (here: LiveTraffic) needs to register the dataRefs first before
// we can find them. So we would potentially fail if we search for them
// during startup (like when declaring statically).
// With this wrapper we still can do static declaration because the actual
// call to XPLMFindDataRef happens only the first time we actually access it.
//
// {{{Later, it will also encapsulate retrieving strings via data refs}}}
//

class LTDataRef {
protected:
    std::string     sDataRef;
    XPLMDataRef     dataRef = NULL;
    XPLMDataTypeID  dataTypes = xplmType_Unknown;
    bool            bValid = true;
public:
    LTDataRef (std::string _sDataRef);
    inline bool needsInit () const { return bValid && !dataRef; }
    bool    isValid ();         // dataRef found? (would FindDataRef if needed)
    bool    FindDataRef ();     // finds data ref (and would try again and again, no matter what bValid says)

    // types
    XPLMDataTypeID getDataRefTypes() const { return dataTypes; }
    bool    hasInt ()   const { return dataTypes & xplmType_Int; }
    bool    hasFloat () const { return dataTypes & xplmType_Float; }
    static constexpr XPLMDataTypeID usefulTypes = xplmType_Int | xplmType_Float;

    // retrieve values
    // (silently return 0 / 0.0 if dataRef doesn't exist)
    int     getInt();
    float   getFloat();
    inline bool getBool() { return getInt() != 0; }
    
    // write values
    void    set(int i);
    void    set(float f);

protected:
};



#endif /* LTAPI_h */
