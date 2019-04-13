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

#include <stdio.h>
#include <cstring>
#include "LTAPI.h"

#include "XPLMPlugin.h"

//
// MARK: Globals
//

#define LT_PLUGIN_SIGNATURE     "TwinFan.plugin.LiveTraffic"

// The following macros define dataRef access statically,
// then assign its current value to the passed-in variable:
#define ASSIGN_DR(var,dataRef,type)                             \
static LTDataRef DR##var(dataRef);                              \
var = DR##var.get##type();

#define ASSIGN_DR_NAME(var,drName,dataRef,type)                 \
static LTDataRef DR##drName(dataRef);                           \
var = DR##drName.get##type();

#define RETURN_DR(dataRef,type)                                 \
static LTDataRef DR(dataRef);                                   \
return DR.get##type();

namespace LTAPI {
    // kinda inverse for gmtime, i.e. convert struct tm to time_t in ZULU timezone
    time_t timegm(struct tm* _Tm)
    {
        time_t t = mktime(_Tm);
        return t + (mktime(localtime(&t)) - mktime(gmtime(&t)));
    }
    
    // read the current a/c key from LiveTraffic and return it
    // both as hex string and as digital number
    std::string readKey (unsigned& keyNum)
    {
        ASSIGN_DR(keyNum, "livetraffic/ac/key", Int);
        char buf[20];                   // convert to upper case hex string
        snprintf(buf, sizeof(buf), "%06X", keyNum);
        return buf;
    }
}

//
// MARK: LTAPIAircraft
//
// Represents one aircraft as controlled by LiveTraffic.
//

LTAPIAircraft::LTAPIAircraft()
{}

LTAPIAircraft::~LTAPIAircraft()
{}

// Main function: Updates an aircraft from LiveTraffic's dataRefs

bool LTAPIAircraft::updateAircraft()
{
    int i = 0;
    
    // key is special: Let's first verify we want to overwrite our current values
    unsigned currKey = 0;
    std::string currKeyS = LTAPI::readKey(currKey);
    // first time init of this LTAPIAircraft object?
    if (key.empty()) {
        // yes, so we accept the offered aircraft as ours now:
        keyNum = currKey;
        key = currKeyS;
    } else {
        // our key isn't empty, so we continue only if the aircraft offered
        // is the same!
        if (currKey != keyNum)
            return false;
    }
    
    // aircraft model/operator
    // flight data
    // position, attitude
    ASSIGN_DR(lat,              "livetraffic/ac/lat",           Float);
    ASSIGN_DR(lon,              "livetraffic/ac/lon",           Float);
    ASSIGN_DR(alt_ft,           "livetraffic/ac/alt",           Float);
    ASSIGN_DR(heading,          "livetraffic/ac/heading",       Float);
    track = heading;
    ASSIGN_DR(roll,             "livetraffic/ac/roll",          Float);
    ASSIGN_DR(pitch,            "livetraffic/ac/pitch",         Float);
    ASSIGN_DR(speed_kn,         "livetraffic/ac/speed",         Float);
    ASSIGN_DR(vsi_ft,           "livetraffic/ac/vsi",           Float);
    ASSIGN_DR(terrainAlt_ft,    "livetraffic/ac/terrain_alt",   Float);
    ASSIGN_DR(height_ft,        "livetraffic/ac/height",        Float);
    ASSIGN_DR(onGnd,            "livetraffic/ac/on_gnd",        Bool);
    ASSIGN_DR(i,                "livetraffic/ac/phase",         Int);
    phase = (LTFlightPhase)i;
    // configuration
    ASSIGN_DR(flaps,            "livetraffic/ac/flaps",         Float);
    ASSIGN_DR(gear,             "livetraffic/ac/gear",          Float);
    ASSIGN_DR_NAME(lights.beacon,   LBeacon,    "livetraffic/ac/lights/beacon", Bool);
    ASSIGN_DR_NAME(lights.strobe,   LStrobe,    "livetraffic/ac/lights/strobe", Bool);
    ASSIGN_DR_NAME(lights.nav,      LNav,       "livetraffic/ac/lights/nav",    Bool);
    ASSIGN_DR_NAME(lights.landing,  LLanding,   "livetraffic/ac/lights/landing",Bool);
    lights.taxi = lights.landing;
    // simulation
    ASSIGN_DR(bearing,          "livetraffic/ac/bearing",       Float);
    ASSIGN_DR(dist_nm,          "livetraffic/ac/dist",          Float);
    
    // has been updated
    bUpdated = true;
    return true;
}

// return a human readable string for current flight phase
std::string LTAPIAircraft::getPhaseStr () const
{
    switch (phase) {
        case FPH_UNKNOWN:           return "Unknown";
        case FPH_TAXI:              return "Taxi";
        case FPH_TAKE_OFF:          return "Take Off";
        case FPH_TO_ROLL:           return "Take Off Roll";
        case FPH_ROTATE:            return "Rotate";
        case FPH_LIFT_OFF:          return "Lift Off";
        case FPH_INITIAL_CLIMB:     return "Initial Climb";
        case FPH_CLIMB:             return "Climb";
        case FPH_CRUISE:            return "Cruise";
        case FPH_DESCEND:           return "Descend";
        case FPH_APPROACH:          return "Approach";
        case FPH_FINAL:             return "Final";
        case FPH_LANDING:           return "Landing";
        case FPH_FLARE:             return "Flare";
        case FPH_TOUCH_DOWN:        return "Touch Down";
        case FPH_ROLL_OUT:          return "Roll Out";
        case FPH_STOPPED_ON_RWY:    return "Stopped";
    }
    // must not get here...then we missed a value in the above switch
    return "?";
}

//
// MARK: LTAPIConnect
//

LTAPIConnect::LTAPIConnect(fCreateAcObject* _pfCreateAcObject) :
pfCreateAcObject(_pfCreateAcObject)
{}

LTAPIConnect::~LTAPIConnect()
{}

// LiveTraffic available? (checks via XPLMFindPluginBySignature)
bool LTAPIConnect::isLTAvail ()
{
    return XPLMFindPluginBySignature(LT_PLUGIN_SIGNATURE) != XPLM_NO_PLUGIN_ID;
}

// Does LiveTraffic display aircrafts? (Is it activated?)
bool LTAPIConnect::doesLTDisplayAc ()
{
    static LTDataRef DRAcDisplayed("livetraffic/cfg/aircrafts_displayed");
    // this is the only function which tries to find the dataRef over and over again
    if (!DRAcDisplayed.isValid())
        DRAcDisplayed.FindDataRef();
    return DRAcDisplayed.getBool();
}

// How many of them right now?
int LTAPIConnect::getLTNumAc ()
{
    RETURN_DR("livetraffic/ac/num",Int);
}

// Does it (also) control AI planes?
bool LTAPIConnect::doesLTControlAI ()
{
    RETURN_DR("livetraffic/cfg/ai_controlled",Bool);
}

// What's current simulated time in LiveTraffic (usually 'now' minus buffering period)?
time_t LTAPIConnect::getLTSimTime ()
{
    struct tm t;
    memset(&t, 0, sizeof(t));
    
    int i = 0;
    ASSIGN_DR_NAME(i, Date, "livetraffic/sim/date", Int);
    t.tm_year = i / 10000;
    i -= t.tm_year * 10000;
    t.tm_mon = i / 100 - 1;
    t.tm_mday = i % 100;

    ASSIGN_DR_NAME(i, Time, "livetraffic/sim/time", Int);
    t.tm_hour   = i / 10000;
    t.tm_min    = (i % 10000) / 100;
    t.tm_sec    = i % 100;
    
    return LTAPI::timegm(&t);
}

std::chrono::system_clock::time_point LTAPIConnect::getLTSimTimePoint ()
{
    return std::chrono::system_clock::from_time_t(getLTSimTime());
}


// Main function: updates map of aircrafts and returns reference to it.
// If you want to know which a/c are removed during this call then pass
// a ListLTAPIAircraft object; LTAPI will transfer otherwise removed
// objects there and management of them is then up to you.
// (LTAPI will only _emplace_back_ to the list, not remove anything.)
const MapLTAPIAircraft& LTAPIConnect::UpdateAcList (ListLTAPIAircraft* plistRemovedAc)
{
    // this is an input/output dataRef in LiveTraffic,
    // with which we control which aircraft we want to read.
    static LTDataRef DRAcKey("livetraffic/ac/key");

    // a few sanity checks...without LT displaying aircrafts
    // and access to ac/key there is nothing to do.
    // (Calling doesLTDisplayAc before calling any other dataRef
    //  makes sure we only try accessing dataRefs when they are available.)
    int numAc = isLTAvail() && doesLTDisplayAc() && DRAcKey.isValid() ? getLTNumAc() : 0;
    if (numAc <= 0) {
        // does caller want to know about removed aircrafts?
        if (plistRemovedAc)
            for (MapLTAPIAircraft::value_type& p: mapAc)
                // move all objects over to the caller's list's end
                plistRemovedAc->emplace_back(std::move(p.second));
        // clear our map
        mapAc.clear();
        return mapAc;
    }
    
    // *** There are numAc aircrafts to be reported ***
    
    // To figure out which aircraft has gone we keep an update flag
    // with the aircraft. Let's reset that flag first.
    for (MapLTAPIAircraft::value_type& p: mapAc)
        p.second->resetUpdated();
    
    // Now we just loop over all aircrafts one by one
    for (int n = 1; n <= numAc; n++) {
        // Tell LiveTraffic which aircraft we want to read next
        DRAcKey.set(n);
        // read back the key, which now is a unique identifier
        unsigned keyNum = 0;
        std::string keyS = LTAPI::readKey(keyNum);
        // Is there an aircraft in the map already for this key?
        MapLTAPIAircraft::iterator iter = mapAc.find(keyS);
        if (iter == mapAc.end())
            // creates a new object calling the provided function to create new objects
            iter = mapAc.emplace(keyS, pfCreateAcObject()).first;
        // update the aircraft object with values from LiveTraffic
        iter->second->updateAircraft();
    }

    // Now handle aircrafts in our map, which did _not_ get updated
    for (MapLTAPIAircraft::iterator iter = mapAc.begin();
         iter != mapAc.end();
         /* no loop increment*/)
    {
        // not updated?
        if (!iter->second->isUpdated()) {
            // Does caller want to take over them?
            if (plistRemovedAc)
                // here you go...your object now
                plistRemovedAc->emplace_back(std::move(iter->second));
            // in any case: remove from our map and increment to next element
            iter = mapAc.erase(iter);
        }
        else
            // go to next element (without removing this one)
            iter++;
    }
    
    // We're done, return the result
    return mapAc;
}


//
// MARK: LTDataRef
//

LTDataRef::LTDataRef (std::string _sDataRef) :
sDataRef(_sDataRef)
{}

// Found the dataRef and it contains formats we can work with?
bool LTDataRef::isValid ()
{
    if (needsInit()) FindDataRef();
    return bValid;
}

// binds to the dataRef and sets bValid
bool LTDataRef::FindDataRef ()
{
    dataRef = XPLMFindDataRef(sDataRef.c_str());
    // check available data types; we only work with a subset
    dataTypes = dataRef ? (XPLMGetDataRefTypes(dataRef) & usefulTypes) : xplmType_Unknown;
    return bValid = dataTypes != xplmType_Unknown;
}

int LTDataRef::getInt()
{
    if (needsInit()) FindDataRef();
    return XPLMGetDatai(dataRef);
}

float LTDataRef::getFloat()
{
    if (needsInit()) FindDataRef();
    return XPLMGetDataf(dataRef);
}

void LTDataRef::set(int i)
{
    if (needsInit()) FindDataRef();
    XPLMSetDatai(dataRef, i);
}

void LTDataRef::set(float f)
{
    if (needsInit()) FindDataRef();
    XPLMSetDataf(dataRef, f);
}

