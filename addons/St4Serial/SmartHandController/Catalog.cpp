#include <Arduino.h>
#include <math.h>
#include "Config.h"
#include "Catalog.h"
#include "CatalogTypes.h"

// --------------------------------------------------------------------------------
// Data for different catalogs, each is a collection of certain celestial objects
// ** In this section is everything you need touch to add a new "catalog.h"      **
// ** file and use it within the SHC.                                            **

// Simply include the catalog you want here:
// Each catalog type has the possiblity of several different combinations depending on the _select, _c, and _vc suffixes available.
// For example:
// ngc.h          // for the full ngc catalog at the highest accuracy.
// ngc_vc.h       // for a nearly complete ngc catalog at somewhat reduced accuracy, but much smaller size (50% of above.)
// ngc_select_c.h // for a selection of the brighter objects from the ngc catalog at somewhat reduced accuracy.

// Note: You can navigate to and open the SmartHandController's catalogs directory in the Arduino IDE to see the available catalogs.
  #include "catalogs/stars_vc.h"        // Catalog of 408 bright stars
  #include "catalogs/messier_c.h"       // Charles Messier's famous catalog of 109 DSO's
  #include "catalogs/caldwell_c.h"      // The Caldwell (supplement) catalog of 109 DSO's
  #include "catalogs/herschel_c.h"      // Herschel's "400 best of the NGC" catalog
  #include "catalogs/collinder_vc.h"    // The Collinder catalog of 471 open clusters
  #include "catalogs/stf_select_c.h"    // Struve STF catalog of of 931 double stars brighter than Magnitude 9.0
  #include "catalogs/stt_select_c.h"    // Struve STT catalog of of 766 double stars brighter than Magnitude 9.0
  #include "catalogs/ngc_select_c.h"    // The New General Catalog of 3438 DSO's
  #include "catalogs/ic_select_c.h"     // The Index Catalog (supplement) of 1024 DSO's

// And uncomment the matching line below:
catalog_t catalog[] = {
// Note: Alignment always uses the first catalog!
// Title         Prefix   Num records   Catalog data  Catalog name string  Catalog subId string  Type                Epoch
  {"Stars",       "Star ", NUM_STARS,    Cat_Stars,    Cat_Stars_Names,     Cat_Stars_SubId,      Cat_Stars_Type,     2000, 0},
  {"Messier",     "M",     NUM_MESSIER,  Cat_Messier,  Cat_Messier_Names,   Cat_Messier_SubId,    Cat_Messier_Type,   2000, 0},
  {"Caldwell",    "C",     NUM_CALDWELL, Cat_Caldwell, Cat_Caldwell_Names,  Cat_Caldwell_SubId,   Cat_Caldwell_Type,  2000, 0},
  {"Herschel400", "N",     NUM_HERSCHEL, Cat_Herschel, Cat_Herschel_Names,  Cat_Herschel_SubId,   Cat_Herschel_Type,  2000, 0},
  {"Collinder",   "Cr",    NUM_COLLINDER,Cat_Collinder,Cat_Collinder_Names, Cat_Collinder_SubId,  Cat_Collinder_Type, 2000, 0},
  {"Slct STF**",  "STF",   NUM_STF,      Cat_STF,      Cat_STF_Names,       Cat_STF_SubId,        Cat_STF_Type,       2000, 0},
  {"Slct STT**",  "STT",   NUM_STT,      Cat_STT,      Cat_STT_Names,       Cat_STT_SubId,        Cat_STT_Type,       2000, 0},
  {"Select NGC",  "N",     NUM_NGC,      Cat_NGC,      Cat_NGC_Names,       Cat_NGC_SubId,        Cat_NGC_Type,       2000, 0},
  {"Select IC",   "I",     NUM_IC,       Cat_IC,       Cat_IC_Names,        Cat_IC_SubId,         Cat_IC_Type,        2000, 0},
  {"",            "",      0,            NULL,         NULL,                NULL,                 CAT_NONE,           0,    0}
};

// --------------------------------------------------------------------------------
// Catalog Manager

// initialization
void CatMgr::setLat(double lat) {
  _lat=lat;
  if (lat<9999) {
    _cosLat=cos(lat/Rad);
    _sinLat=sin(lat/Rad);
  }
}

// Set Local Sidereal Time, and number of milliseconds
void CatMgr::setLstT0(double lstT0) {
  _lstT0=lstT0;
  _lstMillisT0=millis();
}

// Set last Tele RA/Dec
void CatMgr::setLastTeleEqu(double RA, double Dec) {
  _lastTeleRA=RA;
  _lastTeleDec=Dec;
}

bool CatMgr::isInitialized() {
  return ((_lat<9999) && (_lstT0!=0));
}

// Get Local Sidereal Time, converted from hours to degrees
double CatMgr::lstDegs() {
  return (lstHours()*15.0);
}

// Get Local Sidereal Time, in hours, and adjust for time inside the menus
double CatMgr::lstHours() {
  double msSinceT0=(unsigned long)(millis()-_lstMillisT0);
  // Convert from Solar to Sidereal
  double siderealSecondsSinceT0=(msSinceT0/1000.0)*1.00277778;
  return _lstT0+siderealSecondsSinceT0/3600.0;
}

// number of catalogs available
int CatMgr::numCatalogs() {
  for (int i=0; i<MaxCatalogs; i++) {
    if (catalog[i].NumObjects==0) return i;
  }
  return 32;
}

gen_star_t*       _genStarCatalog      = NULL;
gen_star_vcomp_t* _genStarVCompCatalog = NULL;
dbl_star_t*       _dblStarCatalog      = NULL;
dbl_star_comp_t*  _dblStarCompCatalog  = NULL;
var_star_t*       _varStarCatalog      = NULL;
dso_t*            _dsoCatalog          = NULL;
dso_comp_t*       _dsoCompCatalog      = NULL;
dso_vcomp_t*      _dsoVCompCatalog     = NULL;

// handle catalog selection (0..n)
void CatMgr::select(int number) {
  _genStarCatalog      =NULL;
  _genStarVCompCatalog =NULL;
  _dblStarCatalog      =NULL;
  _dsoCatalog          =NULL;
  _dsoCompCatalog      =NULL;
  _dsoVCompCatalog     =NULL;
  if ((number<0) || (number>=numCatalogs())) number=-1; // invalid catalog?
  _selected=number;
  if (_selected>=0) {
    if (catalog[_selected].CatalogType==CAT_GEN_STAR)       _genStarCatalog     =(gen_star_t*)catalog[_selected].Objects; else
    if (catalog[_selected].CatalogType==CAT_GEN_STAR_VCOMP) _genStarVCompCatalog=(gen_star_vcomp_t*)catalog[_selected].Objects; else
    if (catalog[_selected].CatalogType==CAT_DBL_STAR)       _dblStarCatalog     =(dbl_star_t*)catalog[_selected].Objects; else
    if (catalog[_selected].CatalogType==CAT_DBL_STAR_COMP)  _dblStarCompCatalog =(dbl_star_comp_t*)catalog[_selected].Objects; else
    if (catalog[_selected].CatalogType==CAT_VAR_STAR)       _varStarCatalog     =(var_star_t*)catalog[_selected].Objects; else
    if (catalog[_selected].CatalogType==CAT_DSO)            _dsoCatalog         =(dso_t*)catalog[_selected].Objects; else
    if (catalog[_selected].CatalogType==CAT_DSO_COMP)       _dsoCompCatalog     =(dso_comp_t*)catalog[_selected].Objects; else
    if (catalog[_selected].CatalogType==CAT_DSO_VCOMP)      _dsoVCompCatalog    =(dso_vcomp_t*)catalog[_selected].Objects; else _selected=-1;
  }
}

//  Get active catalog type
CAT_TYPES CatMgr::catalogType()  {
  return catalog[_selected].CatalogType;
}

// check so see if there's a double star catalog present
bool CatMgr::hasDblStarCatalog() {
  for (int i=0; i<MaxCatalogs; i++) {
    if (catalog[i].CatalogType==CAT_DBL_STAR) return true;
    if (catalog[i].CatalogType==CAT_DBL_STAR_COMP) return true;
    if (catalog[i].CatalogType==CAT_NONE) break;
  }
  return false;
}

// check so see if theres a variable star catalog present
bool CatMgr::hasVarStarCatalog() {
  for (int i=0; i<MaxCatalogs; i++) {
    if (catalog[i].CatalogType==CAT_VAR_STAR) return true;
    if (catalog[i].CatalogType==CAT_NONE) break;
  }
  return false;
}

bool CatMgr::isStarCatalog() {
  return (catalogType()==CAT_GEN_STAR) || (catalogType()==CAT_GEN_STAR_VCOMP) || 
         (catalogType()==CAT_DBL_STAR) || (catalogType()==CAT_DBL_STAR_COMP) ||
         (catalogType()==CAT_VAR_STAR);
}

bool CatMgr::isDblStarCatalog() {
  if (catalogType()==CAT_DBL_STAR) return true;
  if (catalogType()==CAT_DBL_STAR_COMP) return true;
  return false;
}

bool CatMgr::isVarStarCatalog() {
  return catalogType()==CAT_VAR_STAR;
}

bool CatMgr::isDsoCatalog()  {
  return (catalogType()==CAT_DSO) || (catalogType()==CAT_DSO_COMP) || (catalogType()==CAT_DSO_VCOMP);
}

// Get active catalog title
const char* CatMgr::catalogTitle() {
  if (_selected<0) return "";
  return catalog[_selected].Title;
}

// Get active catalog prefix
const char* CatMgr::catalogPrefix() {
  if (_selected<0) return "";
  return catalog[_selected].Prefix;
}

// catalog filtering
void CatMgr::filtersClear() {
  _fm=FM_NONE;
}

void CatMgr::filterAdd(int fm) {
  _fm|=fm;
}

void CatMgr::filterAdd(int fm, int param) {
  _fm|=fm;
  if (fm&FM_CONSTELLATION) _fm_con=param;
  if (fm&FM_BY_MAG) {
    if (param==0) _fm_mag_limit=10.0; else
    if (param==1) _fm_mag_limit=12.0; else
    if (param==2) _fm_mag_limit=13.0; else
    if (param==3) _fm_mag_limit=14.0; else
    if (param==4) _fm_mag_limit=15.0; else
    if (param==5) _fm_mag_limit=16.0; else
    if (param==6) _fm_mag_limit=17.0; else _fm_mag_limit=100.0;
  }
  if (fm&FM_NEARBY) {
    if (param==0) _fm_nearby_dist=1.0; else
    if (param==1) _fm_nearby_dist=5.0; else
    if (param==2) _fm_nearby_dist=10.0; else
    if (param==3) _fm_nearby_dist=15.0; else _fm_nearby_dist=9999.0;
  }
  if (fm&FM_OBJ_TYPE) _fm_obj_type=param;
  if (fm&FM_DBL_MIN_SEP) {
    if (param==0) _fm_dbl_min=0.2; else
    if (param==1) _fm_dbl_min=0.5; else
    if (param==2) _fm_dbl_min=1.0; else
    if (param==3) _fm_dbl_min=1.5; else
    if (param==4) _fm_dbl_min=2.0; else
    if (param==5) _fm_dbl_min=3.0; else
    if (param==6) _fm_dbl_min=5.0; else
    if (param==7) _fm_dbl_min=10.0; else
    if (param==8) _fm_dbl_min=20.0; else
    if (param==9) _fm_dbl_min=50.0; else _fm_dbl_min=0.0;
  }
  if (fm&FM_DBL_MAX_SEP) {
    if (param==0) _fm_dbl_max=0.5; else
    if (param==1) _fm_dbl_max=1.0; else
    if (param==2) _fm_dbl_max=1.5; else
    if (param==3) _fm_dbl_max=2.0; else
    if (param==4) _fm_dbl_max=3.0; else
    if (param==5) _fm_dbl_max=5.0; else
    if (param==6) _fm_dbl_max=10.0; else
    if (param==7) _fm_dbl_max=20.0; else
    if (param==8) _fm_dbl_max=50.0; else
    if (param==9) _fm_dbl_max=100.0; else _fm_dbl_max=0.0;
  }
  if (fm&FM_VAR_MAX_PER) {
    if (param==0) _fm_var_max=0.5; else
    if (param==1) _fm_var_max=1.0; else
    if (param==2) _fm_var_max=2.0; else
    if (param==3) _fm_var_max=5.0; else
    if (param==4) _fm_var_max=10.0; else
    if (param==5) _fm_var_max=20.0; else
    if (param==6) _fm_var_max=50.0; else
    if (param==7) _fm_var_max=100.0; else _fm_var_max=0.0;  }
}

// select catalog record
bool CatMgr::setIndex(long index) {
  if (_selected<0) return false;
  catalog[_selected].Index=index;
  decIndex();
  return incIndex();
}

long CatMgr::getIndex() {
  return catalog[_selected].Index;
}

long CatMgr::getMaxIndex() {
  return catalog[_selected].NumObjects-1;
}

bool CatMgr::incIndex() {
  long i=getMaxIndex()+1;
  do {
    i--;
    catalog[_selected].Index++;
    if (catalog[_selected].Index>getMaxIndex()) catalog[_selected].Index=0;
  } while (isFiltered() && (i>0));
  if (isFiltered()) return false; else return true;
}

bool CatMgr::decIndex() {
  long i=getMaxIndex()+1;
  do {
    i--;
    catalog[_selected].Index--;
    if (catalog[_selected].Index<0) catalog[_selected].Index=getMaxIndex();
  } while (isFiltered() && (i>0));
  if (isFiltered()) return false; else return true;
}

// get catalog contents

// RA, converted from hours to degrees
double CatMgr::ra() {
  return rah()*15.0;
}

// RA in hours
double CatMgr::rah() {
  if (_selected<0) return 0;
  if (catalogType()==CAT_GEN_STAR)       return _genStarCatalog[catalog[_selected].Index].RA; else
  if (catalogType()==CAT_GEN_STAR_VCOMP) return _genStarVCompCatalog[catalog[_selected].Index].RA/2730.6666666666666; else
  if (catalogType()==CAT_DBL_STAR)       return _dblStarCatalog[catalog[_selected].Index].RA; else
  if (catalogType()==CAT_DBL_STAR_COMP)  return _dblStarCompCatalog[catalog[_selected].Index].RA/2730.6666666666666; else
  if (catalogType()==CAT_VAR_STAR)       return _varStarCatalog[catalog[_selected].Index].RA; else
  if (catalogType()==CAT_DSO)            return _dsoCatalog[catalog[_selected].Index].RA; else
  if (catalogType()==CAT_DSO_COMP)       return _dsoCompCatalog[catalog[_selected].Index].RA/2730.6666666666666; else
  if (catalogType()==CAT_DSO_VCOMP)      return _dsoVCompCatalog[catalog[_selected].Index].RA/2730.6666666666666; else return 0;
}

// HA in degrees
double CatMgr::ha() {
  if (!isInitialized()) return 0;
  double h=(lstDegs()-ra());
  while (h>180.0) h-=360.0;
  while (h<-180.0) h+=360.0;
  return h;
}

// Get RA of an object, in hours, minutes, seconds
void CatMgr::raHMS(uint8_t &h, uint8_t &m, uint8_t &s) {
  double f=rah();
  double h1,m1,s1;

  h1=floor(f);
  m1=(f-h1)*60;
  s1=(m1-floor(m1))*60.0;

  h = (int)h1;
  m = (int)m1;
  s = (int)s1;
}

// Dec in degrees
double CatMgr::dec() {
  if (catalogType()==CAT_GEN_STAR)       return _genStarCatalog[catalog[_selected].Index].DE; else
  if (catalogType()==CAT_GEN_STAR_VCOMP) return _genStarVCompCatalog[catalog[_selected].Index].DE/364.07777777777777; else
  if (catalogType()==CAT_DBL_STAR)       return _dblStarCatalog[catalog[_selected].Index].DE; else
  if (catalogType()==CAT_DBL_STAR_COMP)  return _dblStarCompCatalog[catalog[_selected].Index].DE/364.07777777777777; else
  if (catalogType()==CAT_VAR_STAR)       return _varStarCatalog[catalog[_selected].Index].DE; else
  if (catalogType()==CAT_DSO)            return _dsoCatalog[catalog[_selected].Index].DE; else
  if (catalogType()==CAT_DSO_COMP)       return _dsoCompCatalog[catalog[_selected].Index].DE/364.07777777777777; else
  if (catalogType()==CAT_DSO_VCOMP)      return _dsoVCompCatalog[catalog[_selected].Index].DE/364.07777777777777; else return 0;
}

// Declination as degrees, minutes, seconds
void CatMgr::decDMS(short& d, uint8_t& m, uint8_t& s) {
  double f=dec();
  double d1, m1, s1;
  int sign=1; if (f<0) sign=-1;

  f=f*sign;
  d1=floor(f);
  m1=(f-d1)*60;
  s1=(m1-floor(m1))*60.0;

  d = (int)d1*sign;
  m = (int)m1;
  s = (int)s1;
}

// Epoch for catalog
int CatMgr::epoch() {
  if (_selected<0) return -1;
  return catalog[_selected].Epoch;
}

// Alt in degrees
double CatMgr::alt() {
  double a,z;
  EquToHor(ra(),dec(),&a,&z);
  return a;
}

// Declination as degrees, minutes, seconds
void CatMgr::altDMS(short &d, uint8_t &m, uint8_t &s) {
  double f=alt();
  double d1, m1, s1;
  int sign=1; if (f<0) sign=-1;

  f=f*sign;
  d1=floor(f);
  m1=(f-d1)*60;
  s1=(m1-floor(m1))*60.0;

  d = (int)d1*sign;
  m = (int)m1;
  s = (int)s1;
}

// Azm in degrees
double CatMgr::azm() {
  double a,z;
  EquToHor(ra(),dec(),&a,&z);
  return z;
}

// Get Azm of an object, in degrees, minutes, seconds
void CatMgr::azmDMS(short &d, uint8_t &m, uint8_t &s) {
  double f=azm();
  double d1,m1,s1;

  d1=floor(f);
  m1=(f-d1)*60;
  s1=(m1-floor(m1))*60.0;

  d = (int)d1;
  m = (int)m1;
  s = (int)s1;
}

// apply refraction, this converts from the "Topocentric" to "Observed" place for higher accuracy, RA in hours Dec in degrees
void CatMgr::topocentricToObservedPlace(float *RA, float *Dec) {
  if (isInitialized()) {
    double Alt,Azm;
    double r=*RA*15.0;
    double d=*Dec;
    EquToHor(r,d,&Alt,&Azm);
    Alt = Alt+TrueRefrac(Alt) / 60.0;
    HorToEqu(Alt,Azm,&r,&d);
    *RA=r/15.0; *Dec=d;
  }
}

// Period of variable star, in days
float CatMgr::period() {
  if (_selected<0) return -1;
  float p=-1;
  if (catalogType()==CAT_VAR_STAR) p=_varStarCatalog[catalog[_selected].Index].Period; else return -1;
  
  // Period 0.00 to 9.99 days (0 to 999) period 10.0 to 3186.6 days (1000 to 32766), 32767 = Unknown
  if ((p>0)   && (p<=999)) return p/100.0; else
  if ((p>999) && (p<=32766)) return (p-900)/10.0; else return -1;
}

// Position angle of double star, in degrees
int CatMgr::positionAngle() {
  if (_selected<0) return -1;
  if (catalogType()==CAT_DBL_STAR) return _dblStarCatalog[catalog[_selected].Index].PA; else
  if (catalogType()==CAT_DBL_STAR_COMP) return _dblStarCompCatalog[catalog[_selected].Index].PA; else return -1;
}

// Separation of double star, in arc-seconds
float CatMgr::separation() {
  if (_selected<0) return 999.9;
  if (catalogType()==CAT_DBL_STAR) return _dblStarCatalog[catalog[_selected].Index].Sep/10.0; else
  if (catalogType()==CAT_DBL_STAR_COMP) return _dblStarCompCatalog[catalog[_selected].Index].Sep/10.0; else return 999.9;
}

// Magnitude of an object
float CatMgr::magnitude() {
  if (_selected<0) return 99.9;
  if (catalogType()==CAT_GEN_STAR)       return _genStarCatalog[catalog[_selected].Index].Mag/100.0; else
  if (catalogType()==CAT_GEN_STAR_VCOMP) { int m=_genStarVCompCatalog[catalog[_selected].Index].Mag; if (m==255) return 99.9; else return (m/10.0)-2.5; } else
  if (catalogType()==CAT_DBL_STAR)       return _dblStarCatalog[catalog[_selected].Index].Mag/100.0; else
  if (catalogType()==CAT_DBL_STAR_COMP)  { int m=_dblStarCompCatalog[catalog[_selected].Index].Mag; if (m==255) return 99.9; else return (m/10.0)-2.5; } else
  if (catalogType()==CAT_VAR_STAR)       return _varStarCatalog[catalog[_selected].Index].Mag/100.0; else
  if (catalogType()==CAT_DSO)            return _dsoCatalog[catalog[_selected].Index].Mag/100.0; else
  if (catalogType()==CAT_DSO_COMP)       { int m=_dsoCompCatalog[catalog[_selected].Index].Mag; if (m==255) return 99.9; else return (m/10.0)-2.5; } else
  if (catalogType()==CAT_DSO_VCOMP)      { int m=_dsoVCompCatalog[catalog[_selected].Index].Mag; if (m==255) return 99.9; else return (m/10.0)-2.5; } else return 99.9;
}

// Secondary magnitude of an star.  For double stars this is the magnitude of the secondary.  For variables this is the minimum brightness.
float CatMgr::magnitude2() {
  if (_selected<0) return 99.9;
  if (catalogType()==CAT_DBL_STAR)       return _dblStarCatalog[catalog[_selected].Index].Mag2/100.0; else
  if (catalogType()==CAT_DBL_STAR_COMP)  { int m=_dblStarCompCatalog[catalog[_selected].Index].Mag2; if (m==255) return 99.9; else return (m/10.0)-2.5; } else
  if (catalogType()==CAT_VAR_STAR)       return _varStarCatalog[catalog[_selected].Index].Mag2/100.0; else return 99.9;
}

// Constellation number
byte CatMgr::constellation() {
  if (_selected<0) return 89;
  if (catalogType()==CAT_GEN_STAR)       return _genStarCatalog[catalog[_selected].Index].Cons; else
  if (catalogType()==CAT_GEN_STAR_VCOMP) return _genStarVCompCatalog[catalog[_selected].Index].Cons; else
  if (catalogType()==CAT_DBL_STAR)       return _dblStarCatalog[catalog[_selected].Index].Cons; else
  if (catalogType()==CAT_DBL_STAR_COMP)  return _dblStarCompCatalog[catalog[_selected].Index].Cons; else
  if (catalogType()==CAT_VAR_STAR)       return _varStarCatalog[catalog[_selected].Index].Cons; else
  if (catalogType()==CAT_DSO)            return _dsoCatalog[catalog[_selected].Index].Cons; else
  if (catalogType()==CAT_DSO_COMP)       return _dsoCompCatalog[catalog[_selected].Index].Cons; else
  if (catalogType()==CAT_DSO_VCOMP)      return _dsoVCompCatalog[catalog[_selected].Index].Cons; else return 0;
}

// Constellation string
const char* CatMgr::constellationStr() {
  return Txt_Constellations[constellation()];
}

// Constellation string, from constellation number
const char* CatMgr::constellationCodeToStr(int code) {
  if ((code>=0) && (code<=87)) return Txt_Constellations[code]; else return "";
}

// Object type code
byte CatMgr::objectType() {
  if (_selected<0) return -1;
  if (catalogType()==CAT_GEN_STAR)       return 2; else
  if (catalogType()==CAT_GEN_STAR_VCOMP) return 2; else
  if (catalogType()==CAT_DBL_STAR)       return 2; else
  if (catalogType()==CAT_DBL_STAR_COMP)  return 2; else
  if (catalogType()==CAT_DSO)            return _dsoCatalog[catalog[_selected].Index].Obj_type; else
  if (catalogType()==CAT_DSO_COMP)       return _dsoCompCatalog[catalog[_selected].Index].Obj_type; else
  if (catalogType()==CAT_DSO_VCOMP)      return _dsoVCompCatalog[catalog[_selected].Index].Obj_type; else return -1;
}

// Object type string
const char* CatMgr::objectTypeStr() {
  int t=objectType();
  if ((t>=0) && (t<=20)) return Txt_Object_Type[t]; else return "";
}

// Object Type string, from code number
const char* CatMgr::objectTypeCodeToStr(int code) {
  if ((code>=0) && (code<=20)) return Txt_Object_Type[code]; else return "";
}

// Object name code (encoded by Has_name.)  Returns -1 if the object doesn't have a name code.
long CatMgr::objectName() {
  if (_selected<0) return -1;

  // does it have a name? if not just return
  if (catalogType()==CAT_GEN_STAR)       { if (!_genStarCatalog[catalog[_selected].Index].Has_name) return -1; } else
  if (catalogType()==CAT_GEN_STAR_VCOMP) { if (!_genStarVCompCatalog[catalog[_selected].Index].Has_name) return -1; } else
  if (catalogType()==CAT_DBL_STAR)       { if (!_dblStarCatalog[catalog[_selected].Index].Has_name) return -1; } else
  if (catalogType()==CAT_DBL_STAR_COMP)  { if (!_dblStarCompCatalog[catalog[_selected].Index].Has_name) return -1; } else
  if (catalogType()==CAT_VAR_STAR)       { if (!_varStarCatalog[catalog[_selected].Index].Has_name) return -1; } else
  if (catalogType()==CAT_DSO)            { if (!_dsoCatalog[catalog[_selected].Index].Has_name) return -1; } else
  if (catalogType()==CAT_DSO_COMP)       { if (!_dsoCompCatalog[catalog[_selected].Index].Has_name) return -1; } else
  if (catalogType()==CAT_DSO_VCOMP)      { if (!_dsoVCompCatalog[catalog[_selected].Index].Has_name) return -1; } else return -1;

  // find the code
  long result=-1;
  long j=catalog[_selected].Index;
  if (j>getMaxIndex()) j=-1;
  if (j<0) return -1;
  if (catalogType()==CAT_GEN_STAR)       { for (long i=0; i<=j; i++) { if (_genStarCatalog[i].Has_name) result++; } } else
  if (catalogType()==CAT_GEN_STAR_VCOMP) { for (long i=0; i<=j; i++) { if (_genStarVCompCatalog[i].Has_name) result++; } } else
  if (catalogType()==CAT_DBL_STAR)       { for (long i=0; i<=j; i++) { if (_dblStarCatalog[i].Has_name) result++; } } else
  if (catalogType()==CAT_DBL_STAR_COMP)  { for (long i=0; i<=j; i++) { if (_dblStarCompCatalog[i].Has_name) result++; } } else
  if (catalogType()==CAT_VAR_STAR)       { for (long i=0; i<=j; i++) { if (_varStarCatalog[i].Has_name) result++; } } else
  if (catalogType()==CAT_DSO)            { for (long i=0; i<=j; i++) { if (_dsoCatalog[i].Has_name) result++; } } else
  if (catalogType()==CAT_DSO_COMP)       { for (long i=0; i<=j; i++) { if (_dsoCompCatalog[i].Has_name) result++; } } else
  if (catalogType()==CAT_DSO_VCOMP)      { for (long i=0; i<=j; i++) { if (_dsoVCompCatalog[i].Has_name) result++; } } else return -1;

  return result;
}

// Object name type string
const char* CatMgr::objectNameStr() {
  if (_selected<0) return "";
  long elementNum=objectName();
  if (elementNum>=0) return getElementFromString(catalog[_selected].ObjectNames,elementNum); else return "";
}

// Object Id
long CatMgr::primaryId() {
  if (_selected<0) return -1;
  if (catalogType()==CAT_GEN_STAR)       return _genStarCatalog[catalog[_selected].Index].Obj_id; else
  if (catalogType()==CAT_GEN_STAR_VCOMP) return catalog[_selected].Index+1; else
  if (catalogType()==CAT_DBL_STAR)       return _dblStarCatalog[catalog[_selected].Index].Obj_id; else
  if (catalogType()==CAT_DBL_STAR_COMP)  return _dblStarCompCatalog[catalog[_selected].Index].Obj_id; else
  if (catalogType()==CAT_VAR_STAR)       return _varStarCatalog[catalog[_selected].Index].Obj_id; else
  if (catalogType()==CAT_DSO)            return _dsoCatalog[catalog[_selected].Index].Obj_id; else
  if (catalogType()==CAT_DSO_COMP)       return _dsoCompCatalog[catalog[_selected].Index].Obj_id; else
  if (catalogType()==CAT_DSO_VCOMP)      return catalog[_selected].Index+1; else return -1;
}

// Object note code (encoded by Has_note.)  Returns -1 if the object doesn't have a note code.
long CatMgr::subId() {
  if (_selected<0) return -1;

  // does it have a note? if not just return
  if (catalogType()==CAT_GEN_STAR)       { if (!_genStarCatalog[catalog[_selected].Index].Has_subId) return -1; } else
  if (catalogType()==CAT_GEN_STAR_VCOMP) { if (!_genStarVCompCatalog[catalog[_selected].Index].Has_subId) return -1; } else
  if (catalogType()==CAT_DBL_STAR)       { if (!_dblStarCatalog[catalog[_selected].Index].Has_subId) return -1; } else
  if (catalogType()==CAT_DBL_STAR_COMP)  { if (!_dblStarCompCatalog[catalog[_selected].Index].Has_subId) return -1; } else
  if (catalogType()==CAT_VAR_STAR)       { if (!_varStarCatalog[catalog[_selected].Index].Has_subId) return -1; } else
  if (catalogType()==CAT_DSO)            { if (!_dsoCatalog[catalog[_selected].Index].Has_subId) return -1; } else
  if (catalogType()==CAT_DSO_COMP)       { if (!_dsoCompCatalog[catalog[_selected].Index].Has_subId) return -1; } else
  if (catalogType()==CAT_DSO_VCOMP)      { if (!_dsoVCompCatalog[catalog[_selected].Index].Has_subId) return -1; } else return -1;

  // find the code
  long result=-1;
  long j=catalog[_selected].Index;
  if (j>getMaxIndex()) j=-1;
  if (j<0) return -1;
  if (catalogType()==CAT_GEN_STAR)       { for (long i=0; i<=j; i++) { if (_genStarCatalog[i].Has_subId) result++; } } else
  if (catalogType()==CAT_GEN_STAR_VCOMP) { for (long i=0; i<=j; i++) { if (_genStarVCompCatalog[i].Has_subId) result++; } } else
  if (catalogType()==CAT_DBL_STAR)       { for (long i=0; i<=j; i++) { if (_dblStarCatalog[i].Has_subId) result++; } } else
  if (catalogType()==CAT_DBL_STAR_COMP)  { for (long i=0; i<=j; i++) { if (_dblStarCompCatalog[i].Has_subId) result++; } } else
  if (catalogType()==CAT_VAR_STAR)       { for (long i=0; i<=j; i++) { if (_varStarCatalog[i].Has_subId) result++; } } else
  if (catalogType()==CAT_DSO)            { for (long i=0; i<=j; i++) { if (_dsoCatalog[i].Has_subId) result++; } } else
  if (catalogType()==CAT_DSO_COMP)       { for (long i=0; i<=j; i++) { if (_dsoCompCatalog[i].Has_subId) result++; } } else
  if (catalogType()==CAT_DSO_VCOMP)      { for (long i=0; i<=j; i++) { if (_dsoVCompCatalog[i].Has_subId) result++; } } else return -1;

  return result;
}

// Object note string
const char* CatMgr::subIdStr() {
  if (_selected<0) return "";
  long elementNum=subId();
  if (elementNum>=0) return getElementFromString(catalog[_selected].ObjectSubIds,elementNum); else return "";
}

// For Bayer designated Stars 0 = Alp, etc. to 23. For Fleemstead designated Stars 25 = '1', etc.
int CatMgr::bayerFlam() {
  if (_selected<0) return -1;
  int bf=24;
  if (catalogType()==CAT_GEN_STAR)       bf=_genStarCatalog[catalog[_selected].Index].BayerFlam; else
  if (catalogType()==CAT_GEN_STAR_VCOMP) bf=_genStarVCompCatalog[catalog[_selected].Index].BayerFlam; else
  if (catalogType()==CAT_DBL_STAR)       bf=_dblStarCatalog[catalog[_selected].Index].BayerFlam; else
  if (catalogType()==CAT_DBL_STAR_COMP)  bf=_dblStarCompCatalog[catalog[_selected].Index].BayerFlam; else
  if (catalogType()==CAT_VAR_STAR)       bf=_varStarCatalog[catalog[_selected].Index].BayerFlam; else return -1;
  if (bf==24) return -1; else return bf;
}

// For Bayer designated Stars return greek letter or Flamsteed designated stars return number
const char* CatMgr::bayerFlamStr() {
  if (_selected<0) return "";
  static char bfStr[3]="";
  int bfNum=bayerFlam();
  if ((bfNum>=0) && (bfNum<24)) {
    sprintf(bfStr,"%d",bfNum);
    return bfStr;
  } else
  if (bfNum>24) {
    sprintf(bfStr,"%d",bfNum-24);
    return bfStr;
  } else
  return "";
}

// support functions

// checks to see if the currently selected object is filtered (returns true if filtered out)
bool CatMgr::isFiltered() {
  if (!isInitialized()) return false;
  if (_fm==FM_NONE) return false;
  if (_fm & FM_ABOVE_HORIZON) { if (alt()<0.0) return true; }
  if (_fm & FM_ALIGN_ALL_SKY) {
    if (alt()<10.0) return true;      // minimum 10 degrees altitude
    if (abs(dec())>85.0) return true; // minimum 5 degrees from the pole (for accuracy)
  }
  if (_fm & FM_CONSTELLATION) {
    if (constellation()!=_fm_con) return true;
  }
  if (isDsoCatalog() && (_fm & FM_OBJ_TYPE)) {
    if (objectType()!=_fm_obj_type) return true;
  }
  if (_fm & FM_BY_MAG) {
    if (magnitude()>=_fm_mag_limit) return true;
  }
  if (_fm & FM_NEARBY) {
    double d=DistFromEqu(_lastTeleRA,_lastTeleDec);
    if (d>=_fm_nearby_dist) return true;
  }
  if (_fm & FM_DBL_MAX_SEP) {
    double s=separation();
    if (isDblStarCatalog()) {
      if (s>_fm_dbl_max) return true;
    }
  }
  if (_fm & FM_DBL_MIN_SEP) {
    double s=separation();
    if (isDblStarCatalog()) {
      if (s<_fm_dbl_min) return true;
    }
  }
  if (_fm & FM_VAR_MAX_PER) {
    double p=period();
    if (isVarStarCatalog()) {
      if (p>_fm_var_max) return true;
    }
  }

  return false;
}  

// returns elementNum 'th element from the comma delimited string where the 0th element is the first etc.
const char* CatMgr::getElementFromString(const char *data, long elementNum) {
  static char result[40] = "";

  // find the string start index
  bool found=false;
  long j=0;
  long n=elementNum;
  long len=strlen(data);
  for (long i=0; i<len; i++) {
    if (n==0) { j=i; found=true; break; }
    if (data[i]==';') { n--; }
  }

  // return the string
  if (found) {
    long k=0;
    for (long i=j; i<len; i++) {
      result[k++]=data[i];
      if (result[k-1]==';') { result[k-1]=0; break; }
      if (i==len-1) { result[k]=0; break; }
    }
    return result;
  } else return "";
}

// angular distance from current Equ coords, in degrees
double CatMgr::DistFromEqu(double RA, double Dec) {
  RA=RA/Rad; Dec=Dec/Rad;
  return acos( sin(dec()/Rad)*sin(Dec) + cos(dec()/Rad)*cos(Dec)*cos(ra()/Rad - RA))*Rad;
}

// convert an HA to RA, in degrees
double CatMgr::HAToRA(double HA) {
  return (lstDegs()-HA);
}

// convert equatorial coordinates to horizon, in degrees
void CatMgr::EquToHor(double RA, double Dec, double *Alt, double *Azm) {
  double HA=lstDegs()-RA;
  while (HA<0.0)    HA=HA+360.0;
  while (HA>=360.0) HA=HA-360.0;
  HA =HA/Rad;
  Dec=Dec/Rad;
  double SinAlt = (sin(Dec) * _sinLat) + (cos(Dec) * _cosLat * cos(HA));  
  *Alt   = asin(SinAlt);
  double t1=sin(HA);
  double t2=cos(HA)*_sinLat-tan(Dec)*_cosLat;
  *Azm=atan2(t1,t2)*Rad;
  *Azm=*Azm+180.0;
  *Alt = *Alt*Rad;
}

// convert horizon coordinates to equatorial, in degrees
void CatMgr::HorToEqu(double Alt, double Azm, double *RA, double *Dec) { 
  while (Azm<0)      Azm=Azm+360.0;
  while (Azm>=360.0) Azm=Azm-360.0;
  Alt  = Alt/Rad;
  Azm  = Azm/Rad;
  double SinDec = (sin(Alt) * _sinLat) + (cos(Alt) * _cosLat * cos(Azm));  
  *Dec = asin(SinDec); 
  double t1=sin(Azm);
  double t2=cos(Azm)*_sinLat-tan(Alt)*_cosLat;
  double HA=atan2(t1,t2)*Rad;
  HA=HA+180.0;
  *Dec = *Dec*Rad;

  while (HA<0.0)    HA=HA+360.0;
  while (HA>=360.0) HA=HA-360.0;
  *RA=(lstDegs()-HA);
}

// returns the amount of refraction (in arcminutes) at the given true altitude (degrees), pressure (millibars), and temperature (celsius)
double CatMgr::TrueRefrac(double Alt, double Pressure, double Temperature) {
  double TPC=(Pressure/1010.0) * (283.0/(273.0+Temperature));
  double r=( ( 1.02*cot( (Alt+(10.3/(Alt+5.11)))/Rad ) ) ) * TPC;  if (r<0.0) r=0.0;
  return r;
}

double CatMgr::cot(double n) {
  return 1.0/tan(n);
}

CatMgr cat_mgr;
