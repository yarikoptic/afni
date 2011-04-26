#include "mrilib.h"
#include "afni.h"
#include "thd_atlas.h"


#ifdef KILLTHIS /* Remove all old sections framed by #ifdef KILLTHIS
                  in the near future.  ZSS May 2011   */ 
/* THESE NEED TO BE WIPED OUT                VVVVVVVVVVVVVVVVVVV*/
static int           have_dseTT_old = -1   ;
static THD_3dim_dataset * dseTT_old = NULL ;
static THD_3dim_dataset * dseTT_big_old = NULL ; /* 01 Aug 2001 */
static int           have_dseCA_EZ_MPM_old = -1   ;
static THD_3dim_dataset * dseCA_EZ_MPM_old = NULL ;
static int           have_dseCA_EZ_PMaps_old = -1   ;
static THD_3dim_dataset * dseCA_EZ_PMaps_old = NULL ;
static int           have_dseCA_EZ_ML_old = -1   ;
static THD_3dim_dataset * dseCA_EZ_ML_old = NULL ;
static int           have_dseCA_EZ_LR_old = -1   ;
static THD_3dim_dataset * dseCA_EZ_LR_old = NULL ;
/* THESE NEED TO BE WIPED OUT                ^^^^^^^^^^^^^^^^^^^*/
#endif

/* These global arrays should be accessed directly in just a few places:
      Some of the init_* functions and the get_G_* accessor functions 
      below */
static ATLAS_SPACE_LIST *global_atlas_spaces=NULL;
static ATLAS_XFORM_LIST *global_atlas_xfl=NULL;
static ATLAS_LIST *global_atlas_alist=NULL;
static ATLAS_TEMPLATE_LIST *global_atlas_templates=NULL;

ATLAS_SPACE_LIST *get_G_space_list(void) { 
   static int icall = 0;
   if (!icall && !global_atlas_spaces) {
      ++icall;
      init_global_atlas_list();
   }
   return(global_atlas_spaces); 
}

ATLAS_XFORM_LIST *get_G_xform_list(void) { 
   static int icall = 0;
   if (!icall && !global_atlas_xfl) {
      ++icall;
      init_global_atlas_list();
   }
   return(global_atlas_xfl); 
}

ATLAS_LIST* get_G_atlas_list(void) {
   static int icall = 0;
   if (!icall && !global_atlas_alist) {
      ++icall;
      init_global_atlas_list();
   }
   return(global_atlas_alist); 

}

ATLAS_TEMPLATE_LIST *get_G_templates_list(void) { 
   static int icall = 0;
   if (!icall && !global_atlas_templates) {
      ++icall;
      init_global_atlas_list();
   }
   return(global_atlas_templates); 
}


#define MAX_FIND_DEFAULT 9            /* max number to find within WAMIRAD  */
#define WAMIRAD_DEFAULT  7.5           /* search radius: must not exceed 9.5 */

#if 0
# define POPUP_MESSAGE(sss)  /*nuthin fer now -- RWC quick fix*/
#else
  static void PMESS(char *sss){ return; }
  static void (*POPUP_MESSAGE)(char *) = PMESS ;
  void TT_setup_popup_func( void (*pf)(char *) )
   { POPUP_MESSAGE = (pf==NULL) ? PMESS : pf; return; }
#endif

static int wami_verb_val = -100; /* Don't access directly */

void set_wami_verb(int lev) {
   wami_verb_val = lev;
}

int wami_verb(void) { 
   if (wami_verb_val < -1) {
      char * ept = NULL;
      if( (ept= my_getenv("AFNI_WAMI_DEBUG")) ) {
         set_wami_verb(atoi(ept));       /* adjust if set */
      } else if( (ept= my_getenv("AFNI_NIML_DEBUG")) ) {
         WARNING_message("Daniel, I think we should stop using "
                         "AFNI_NIML_DEBUG for wami purpose. "
                         "Kill this section if you agree.");
         set_wami_verb(atoi(ept)+1);       /* adjust if set */
      } else {
         set_wami_verb(0);
      }
   }
   return(wami_verb_val); 
}

int wami_lh(void) { return(wami_verb() > 1 ? wami_verb():0); }

int Init_Whereami_Max_Find(void) {

   char *eee = getenv("AFNI_WHEREAMI_MAX_FIND");
   if (eee) {
      return(atoi(eee));
   }
   return(MAX_FIND_DEFAULT);
}

float Init_Whereami_Max_Rad(void) {

   char *eee = getenv("AFNI_WHEREAMI_MAX_SEARCH_RAD");
   if (eee) {
      if (atof(eee) > 9.5) {
         WARNING_message(  
            "Maximum search radius cannot exceed 9.5. \n"
            "Complain to authors if you really need this changed.\n"
            "Clipping search radius to 9.5\n");
         return(9.5);
      }
      return(atof(eee));
   }
   return(WAMIRAD_DEFAULT);
}

static int MAX_FIND = -1;
static float WAMIRAD = -1.0;

void Set_Whereami_Max_Find(int n) {
   if (n > 0) {
      MAX_FIND = n;
   } else {
      MAX_FIND = Init_Whereami_Max_Find();
   }
   return;
}
void Set_Whereami_Max_Rad(float n) {
   if (n > 9.5) {
      WARNING_message("Maximum search radius cannot exceed 9.5");
      n = 9.5;
   }
   if (n > 0.0) {
      WAMIRAD = n;
   } else {
      WAMIRAD = Init_Whereami_Max_Rad();
   }
   return;
}

static MCW_cluster * wamiclust=NULL ;
static MCW_cluster * wamiclust_CA_EZ=NULL ;

static int TT_whereami_mode = 1;
static char lsep = '\n';

/*! These atlas lists, used to be in afni.h and only included if
Main is defined. That was nice and dandy when only afni and whereami
used them. Now, every main has a use for them since input datasets
can be a string referring to a particular atlas zone. So they were
added to libmri.a and accessible without restrictions. libmri.a and other
binaries are fatter as a result. But you dont' get nothin for nothin.
ZSS Feb. 2006 with a nod of approval by RickR */
/* TTatlas by Jack Lancaster and Peter Fox */
#define TTO_COUNT_HARD 241
ATLAS_POINT TTO_list_HARD[TTO_COUNT_HARD] = {
      {  0,"Anterior Commissure.....................",  0, -1, -1,0, -999, "" } ,
      {  0,"Posterior Commissure....................",  0, 23,  0,0, -999, "" } ,
      {  0,"Corpus Callosum.........................",  0,  7, 21,0, -999, "" } ,
      { 68,"Left  Hippocampus.......................", 30, 24, -9,4, -999, "" } ,
      { 68,"Right Hippocampus.......................",-30, 24, -9,4, -999, "" } ,
      { 71,"Left  Amygdala..........................", 23,  5,-15,4, -999, "" } ,
      { 71,"Right Amygdala..........................",-23,  5,-15,4, -999, "" } ,
      { 20,"Left  Posterior Cingulate...............", 10, 54, 14,2, -999, "" } ,
      { 20,"Right Posterior Cingulate...............",-10, 54, 14,2, -999, "" } ,
      { 21,"Left  Anterior Cingulate................",  8,-32,  7,2, -999, "" } ,
      { 21,"Right Anterior Cingulate................", -8,-32,  7,2, -999, "" } ,
      { 22,"Left  Subcallosal Gyrus.................", 11,-11,-12,2, -999, "" } ,
      { 22,"Right Subcallosal Gyrus.................",-11,-11,-12,2, -999, "" } ,
      { 24,"Left  Transverse Temporal Gyrus.........", 50, 22, 12,2, -999, "" } ,
      { 24,"Right Transverse Temporal Gyrus.........",-50, 22, 12,2, -999, "" } ,
      { 25,"Left  Uncus.............................", 25,  2,-28,2, -999, "" } ,
      { 25,"Right Uncus.............................",-25,  2,-28,2, -999, "" } ,
      { 26,"Left  Rectal Gyrus......................",  7,-30,-23,2, -999, "" } ,
      { 26,"Right Rectal Gyrus......................", -7,-30,-23,2, -999, "" } ,
      { 27,"Left  Fusiform Gyrus....................", 40, 48,-16,2, -999, "" } ,
      { 27,"Right Fusiform Gyrus....................",-40, 48,-16,2, -999, "" } ,
      { 28,"Left  Inferior Occipital Gyrus..........", 35, 86, -7,2, -999, "" } ,
      { 28,"Right Inferior Occipital Gyrus..........",-35, 86, -7,2, -999, "" } ,
      { 29,"Left  Inferior Temporal Gyrus...........", 56, 39,-13,2, -999, "" } ,
      { 29,"Right Inferior Temporal Gyrus...........",-56, 39,-13,2, -999, "" } ,
      { 30,"Left  Insula............................", 39,  7,  9,2, -999, "" } ,
      { 30,"Right Insula............................",-39,  7,  9,2, -999, "" } ,
      { 31,"Left  Parahippocampal Gyrus.............", 25, 25,-12,2, -999, "" } ,
      { 31,"Right Parahippocampal Gyrus.............",-25, 25,-12,2, -999, "" } ,
      { 32,"Left  Lingual Gyrus.....................", 14, 78, -3,2, -999, "" } ,
      { 32,"Right Lingual Gyrus.....................",-14, 78, -3,2, -999, "" } ,
      { 33,"Left  Middle Occipital Gyrus............", 35, 83,  9,2, -999, "" } ,
      { 33,"Right Middle Occipital Gyrus............",-35, 83,  9,2, -999, "" } ,
      { 34,"Left  Orbital Gyrus.....................", 11,-39,-25,2, -999, "" } ,
      { 34,"Right Orbital Gyrus.....................",-11,-39,-25,2, -999, "" } ,
      { 35,"Left  Middle Temporal Gyrus.............", 52, 39,  0,2, -999, "" } ,
      { 35,"Right Middle Temporal Gyrus.............",-52, 39,  0,2, -999, "" } ,
      { 36,"Left  Superior Temporal Gyrus...........", 51, 17,  0,2, -999, "" } ,
      { 36,"Right Superior Temporal Gyrus...........",-51, 17,  0,2, -999, "" } ,
      { 37,"Left  Superior Occipital Gyrus..........", 37, 82, 27,2, -999, "" } ,
      { 37,"Right Superior Occipital Gyrus..........",-37, 82, 27,2, -999, "" } ,
      { 39,"Left  Inferior Frontal Gyrus............", 44,-24,  2,2, -999, "" } ,
      { 39,"Right Inferior Frontal Gyrus............",-44,-24,  2,2, -999, "" } ,
      { 40,"Left  Cuneus............................", 13, 83, 18,2, -999, "" } ,
      { 40,"Right Cuneus............................",-13, 83, 18,2, -999, "" } ,
      { 41,"Left  Angular Gyrus.....................", 45, 64, 33,2, -999, "" } ,
      { 41,"Right Angular Gyrus.....................",-45, 64, 33,2, -999, "" } ,
      { 42,"Left  Supramarginal Gyrus...............", 51, 48, 31,2, -999, "" } ,
      { 42,"Right Supramarginal Gyrus...............",-51, 48, 31,2, -999, "" } ,
      { 43,"Left  Cingulate Gyrus...................", 10, 11, 34,2, -999, "" } ,
      { 43,"Right Cingulate Gyrus...................",-10, 11, 34,2, -999, "" } ,
      { 44,"Left  Inferior Parietal Lobule..........", 48, 41, 39,2, -999, "" } ,
      { 44,"Right Inferior Parietal Lobule..........",-48, 41, 39,2, -999, "" } ,
      { 45,"Left  Precuneus.........................", 14, 61, 41,2, -999, "" } ,
      { 45,"Right Precuneus.........................",-14, 61, 41,2, -999, "" } ,
      { 46,"Left  Superior Parietal Lobule..........", 27, 59, 53,2, -999, "" } ,
      { 46,"Right Superior Parietal Lobule..........",-27, 59, 53,2, -999, "" } ,
      { 47,"Left  Middle Frontal Gyrus..............", 37,-29, 26,2, -999, "" } ,
      { 47,"Right Middle Frontal Gyrus..............",-37,-29, 26,2, -999, "" } ,
      { 48,"Left  Paracentral Lobule................",  7, 32, 53,2, -999, "" } ,
      { 48,"Right Paracentral Lobule................", -7, 32, 53,2, -999, "" } ,
      { 49,"Left  Postcentral Gyrus.................", 43, 25, 43,2, -999, "" } ,
      { 49,"Right Postcentral Gyrus.................",-43, 25, 43,2, -999, "" } ,
      { 50,"Left  Precentral Gyrus..................", 44,  8, 38,2, -999, "" } ,
      { 50,"Right Precentral Gyrus..................",-44,  8, 38,2, -999, "" } ,
      { 51,"Left  Superior Frontal Gyrus............", 19,-40, 27,2, -999, "" } ,
      { 51,"Right Superior Frontal Gyrus............",-19,-40, 27,2, -999, "" } ,
      { 52,"Left  Medial Frontal Gyrus..............",  9,-24, 35,2, -999, "" } ,
      { 52,"Right Medial Frontal Gyrus..............", -9,-24, 35,2, -999, "" } ,
      { 70,"Left  Lentiform Nucleus.................", 22,  1,  2,2, -999, "" } ,
      { 70,"Right Lentiform Nucleus.................",-22,  1,  2,2, -999, "" } ,
      { 72,"Left  Hypothalamus......................",  4,  3, -9,4, -999, "" } ,
      { 72,"Right Hypothalamus......................", -4,  3, -9,4, -999, "" } ,
      { 73,"Left  Red Nucleus.......................",  5, 19, -4,4, -999, "" } ,
      { 73,"Right Red Nucleus.......................", -5, 19, -4,4, -999, "" } ,
      { 74,"Left  Substantia Nigra..................", 11, 18, -7,4, -999, "" } ,
      { 74,"Right Substantia Nigra..................",-11, 18, -7,4, -999, "" } ,
      { 75,"Left  Claustrum.........................", 32,  1,  5,2, -999, "" } ,
      { 75,"Right Claustrum.........................",-32,  1,  5,2, -999, "" } ,
      { 76,"Left  Thalamus..........................", 12, 19,  8,2, -999, "" } ,
      { 76,"Right Thalamus..........................",-12, 19,  8,2, -999, "" } ,
      { 77,"Left  Caudate...........................", 11, -7,  9,2, -999, "" } ,
      { 77,"Right Caudate...........................",-11, -7,  9,2, -999, "" } ,
      {124,"Left  Caudate Tail......................", 27, 35,  9,4, -999, "" } ,
      {124,"Right Caudate Tail......................",-27, 35,  9,4, -999, "" } ,
      {125,"Left  Caudate Body......................", 12, -6, 14,4, -999, "" } ,
      {125,"Right Caudate Body......................",-12, -6, 14,4, -999, "" } ,
      {126,"Left  Caudate Head......................",  9,-13,  0,4, -999, "" } ,
      {126,"Right Caudate Head......................", -9,-13,  0,4, -999, "" } ,
      {128,"Left  Ventral Anterior Nucleus..........", 11,  6,  9,4, -999, "" } ,
      {128,"Right Ventral Anterior Nucleus..........",-11,  6,  9,4, -999, "" } ,
      {129,"Left  Ventral Posterior Medial Nucleus..", 15, 20,  4,4, -999, "" } ,
      {129,"Right Ventral Posterior Medial Nucleus..",-15, 20,  4,4, -999, "" } ,
      {130,"Left  Ventral Posterior Lateral Nucleus.", 18, 19,  5,4, -999, "" } ,
      {130,"Right Ventral Posterior Lateral Nucleus.",-18, 19,  5,4, -999, "" } ,
      {131,"Left  Medial Dorsal Nucleus.............",  6, 16,  8,4, -999, "" } ,
      {131,"Right Medial Dorsal Nucleus.............", -6, 16,  8,4, -999, "" } ,
      {132,"Left  Lateral Dorsal Nucleus............", 12, 20, 16,4, -999, "" } ,
      {132,"Right Lateral Dorsal Nucleus............",-12, 20, 16,4, -999, "" } ,
      {133,"Left  Pulvinar..........................", 16, 27,  8,4, -999, "" } ,
      {133,"Right Pulvinar..........................",-16, 27,  8,4, -999, "" } ,
      {134,"Left  Lateral Posterior Nucleus.........", 17, 20, 14,4, -999, "" } ,
      {134,"Right Lateral Posterior Nucleus.........",-17, 20, 14,4, -999, "" } ,
      {135,"Left  Ventral Lateral Nucleus...........", 14, 12,  9,4, -999, "" } ,
      {135,"Right Ventral Lateral Nucleus...........",-14, 12,  9,4, -999, "" } ,
      {136,"Left  Midline Nucleus...................",  7, 18, 16,4, -999, "" } ,
      {136,"Right Midline Nucleus...................", -7, 18, 16,4, -999, "" } ,
      {137,"Left  Anterior Nucleus..................",  8,  8, 12,4, -999, "" } ,   /* 04 Mar 2002 */
      {137,"Right Anterior Nucleus..................", -8,  8, 12,4, -999, "" } ,
      {138,"Left  Mammillary Body...................", 11, 20,  2,4, -999, "" } ,
      {138,"Right Mammillary Body...................",-11, 20,  2,4, -999, "" } ,
      {144,"Left  Medial Globus Pallidus............", 15,  4, -2,4, -999, "" } ,
      {144,"Right Medial Globus Pallidus............",-15,  4, -2,4, -999, "" } ,
      {145,"Left  Lateral Globus Pallidus...........", 20,  5,  0,4, -999, "" } ,
      {145,"Right Lateral Globus Pallidus...........",-20,  5,  0,4, -999, "" } ,
      {151,"Left  Putamen...........................", 24,  0,  3,4, -999, "" } ,
      {151,"Right Putamen...........................",-24,  0,  3,4, -999, "" } ,
      {146,"Left  Nucleus Accumbens.................", 12, -8, -8,4, -999, "" } , /* 20 Aug */
      {146,"Right Nucleus Accumbens.................",-12, -8, -8,4, -999, "" } , /* 2001 */
      {147,"Left  Medial Geniculum Body.............", 17, 24, -2,4, -999, "" } ,
      {147,"Right Medial Geniculum Body.............",-17, 24, -2,4, -999, "" } ,
      {148,"Left  Lateral Geniculum Body............", 22, 24, -1,4, -999, "" } ,
      {148,"Right Lateral Geniculum Body............",-22, 24, -1,4, -999, "" } ,
      {149,"Left  Subthalamic Nucleus...............", 10, 13, -3,4, -999, "" } ,
      {149,"Right Subthalamic Nucleus...............",-10, 13, -3,4, -999, "" } ,
      { 81,"Left  Brodmann area 1...................", 53, 19, 50,4, -999, "" } ,
      { 81,"Right Brodmann area 1...................",-53, 19, 50,4, -999, "" } ,
      { 82,"Left  Brodmann area 2...................", 49, 26, 43,4, -999, "" } ,
      { 82,"Right Brodmann area 2...................",-49, 26, 43,4, -999, "" } ,
      { 83,"Left  Brodmann area 3...................", 39, 23, 50,4, -999, "" } ,
      { 83,"Right Brodmann area 3...................",-39, 23, 50,4, -999, "" } ,
      { 84,"Left  Brodmann area 4...................", 39, 18, 49,4, -999, "" } ,
      { 84,"Right Brodmann area 4...................",-39, 18, 49,4, -999, "" } ,
      { 85,"Left  Brodmann area 5...................", 16, 40, 57,4, -999, "" } ,
      { 85,"Right Brodmann area 5...................",-16, 40, 57,4, -999, "" } ,
      { 86,"Left  Brodmann area 6...................", 29,  0, 50,4, -999, "" } ,
      { 86,"Right Brodmann area 6...................",-29,  0, 50,4, -999, "" } ,
      { 87,"Left  Brodmann area 7...................", 16, 60, 48,4, -999, "" } ,
      { 87,"Right Brodmann area 7...................",-16, 60, 48,4, -999, "" } ,
      { 88,"Left  Brodmann area 8...................", 24,-30, 44,4, -999, "" } ,
      { 88,"Right Brodmann area 8...................",-24,-30, 44,4, -999, "" } ,
      { 89,"Left  Brodmann area 9...................", 32,-33, 30,4, -999, "" } ,
      { 89,"Right Brodmann area 9...................",-32,-33, 30,4, -999, "" } ,
      { 90,"Left  Brodmann area 10..................", 24,-56,  6,4, -999, "" } ,
      { 90,"Right Brodmann area 10..................",-24,-56,  6,4, -999, "" } ,
      { 91,"Left  Brodmann area 11..................", 17,-43,-18,4, -999, "" } ,
      { 91,"Right Brodmann area 11..................",-17,-43,-18,4, -999, "" } ,
      { 93,"Left  Brodmann area 13..................", 39,  4,  8,4, -999, "" } ,
      { 93,"Right Brodmann area 13..................",-39,  4,  8,4, -999, "" } ,
      { 94,"Left  Brodmann area 17..................", 10, 88,  5,4, -999, "" } ,
      { 94,"Right Brodmann area 17..................",-10, 88,  5,4, -999, "" } ,
      { 95,"Left  Brodmann area 18..................", 19, 85,  4,4, -999, "" } ,
      { 95,"Right Brodmann area 18..................",-19, 85,  4,4, -999, "" } ,
      { 96,"Left  Brodmann area 19..................", 34, 80, 18,4, -999, "" } ,
      { 96,"Right Brodmann area 19..................",-34, 80, 18,4, -999, "" } ,
      { 97,"Left  Brodmann area 20..................", 47, 21,-23,4, -999, "" } ,
      { 97,"Right Brodmann area 20..................",-47, 21,-23,4, -999, "" } ,
      { 98,"Left  Brodmann area 21..................", 58, 18,-10,4, -999, "" } ,
      { 98,"Right Brodmann area 21..................",-58, 18,-10,4, -999, "" } ,
      { 99,"Left  Brodmann area 22..................", 57, 23,  5,4, -999, "" } ,
      { 99,"Right Brodmann area 22..................",-57, 23,  5,4, -999, "" } ,
      {100,"Left  Brodmann area 23..................",  4, 37, 24,4, -999, "" } ,
      {100,"Right Brodmann area 23..................", -4, 37, 24,4, -999, "" } ,
      {101,"Left  Brodmann area 24..................",  6, -6, 30,4, -999, "" } ,
      {101,"Right Brodmann area 24..................", -6, -6, 30,4, -999, "" } ,
      {102,"Left  Brodmann area 25..................",  6,-15,-13,4, -999, "" } ,
      {102,"Right Brodmann area 25..................", -6,-15,-13,4, -999, "" } ,
      {103,"Left  Brodmann area 27..................", 15, 35,  0,4, -999, "" } ,
      {103,"Right Brodmann area 27..................",-15, 35,  0,4, -999, "" } ,
      {104,"Left  Brodmann area 28..................", 22, -2,-24,4, -999, "" } ,
      {104,"Right Brodmann area 28..................",-22, -2,-24,4, -999, "" } ,
      {105,"Left  Brodmann area 29..................",  6, 48, 11,4, -999, "" } ,
      {105,"Right Brodmann area 29..................", -6, 48, 11,4, -999, "" } ,
      {106,"Left  Brodmann area 30..................", 13, 62, 10,4, -999, "" } ,
      {106,"Right Brodmann area 30..................",-13, 62, 10,4, -999, "" } ,
      {107,"Left  Brodmann area 31..................",  9, 47, 32,4, -999, "" } ,
      {107,"Right Brodmann area 31..................", -9, 47, 32,4, -999, "" } ,
      {108,"Left  Brodmann area 32..................",  8,-24, 30,4, -999, "" } ,
      {108,"Right Brodmann area 32..................", -8,-24, 30,4, -999, "" } ,
      {109,"Left  Brodmann area 33..................",  5,-12, 24,4, -999, "" } ,
      {109,"Right Brodmann area 33..................", -5,-12, 24,4, -999, "" } ,
      {110,"Left  Brodmann area 34..................", 18,  0,-16,4, -999, "" } ,
      {110,"Right Brodmann area 34..................",-18,  0,-16,4, -999, "" } ,
      {111,"Left  Brodmann area 35..................", 23, 25,-15,4, -999, "" } ,
      {111,"Right Brodmann area 35..................",-23, 25,-15,4, -999, "" } ,
      {112,"Left  Brodmann area 36..................", 33, 33,-15,4, -999, "" } ,
      {112,"Right Brodmann area 36..................",-33, 33,-15,4, -999, "" } ,
      {113,"Left  Brodmann area 37..................", 48, 55, -7,4, -999, "" } ,
      {113,"Right Brodmann area 37..................",-48, 55, -7,4, -999, "" } ,
      {114,"Left  Brodmann area 38..................", 41,-12,-23,4, -999, "" } ,
      {114,"Right Brodmann area 38..................",-41,-12,-23,4, -999, "" } ,
      {115,"Left  Brodmann area 39..................", 48, 64, 28,4, -999, "" } ,
      {115,"Right Brodmann area 39..................",-48, 64, 28,4, -999, "" } ,
      {116,"Left  Brodmann area 40..................", 51, 40, 38,4, -999, "" } ,
      {116,"Right Brodmann area 40..................",-51, 40, 38,4, -999, "" } ,
      {117,"Left  Brodmann area 41..................", 47, 26, 11,4, -999, "" } ,
      {117,"Right Brodmann area 41..................",-47, 26, 11,4, -999, "" } ,
      {118,"Left  Brodmann area 42..................", 63, 22, 12,4, -999, "" } ,
      {118,"Right Brodmann area 42..................",-63, 22, 12,4, -999, "" } ,
      {119,"Left  Brodmann area 43..................", 58, 10, 16,4, -999, "" } ,
      {119,"Right Brodmann area 43..................",-58, 10, 16,4, -999, "" } ,
      {120,"Left  Brodmann area 44..................", 53,-11, 12,4, -999, "" } ,
      {120,"Right Brodmann area 44..................",-53,-11, 12,4, -999, "" } ,
      {121,"Left  Brodmann area 45..................", 54,-23, 10,4, -999, "" } ,
      {121,"Right Brodmann area 45..................",-54,-23, 10,4, -999, "" } ,
      {122,"Left  Brodmann area 46..................", 50,-38, 16,4, -999, "" } ,
      {122,"Right Brodmann area 46..................",-50,-38, 16,4, -999, "" } ,
      {123,"Left  Brodmann area 47..................", 38,-24,-11,4, -999, "" } ,
      {123,"Right Brodmann area 47..................",-38,-24,-11,4, -999, "" } ,
      { 53,"Left  Uvula of Vermis...................",  2, 65,-32,2, -999, "" } ,
      { 53,"Right Uvula of Vermis...................", -2, 65,-32,2, -999, "" } ,
      { 54,"Left  Pyramis of Vermis.................",  2, 73,-28,2, -999, "" } ,
      { 54,"Right Pyramis of Vermis.................", -2, 73,-28,2, -999, "" } ,
      { 55,"Left  Tuber of Vermis...................",  2, 71,-24,2, -999, "" } ,
      { 55,"Right Tuber of Vermis...................", -2, 71,-24,2, -999, "" } ,
      { 56,"Left  Declive of Vermis.................",  2, 72,-17,2, -999, "" } ,
      { 56,"Right Declive of Vermis.................", -2, 72,-17,2, -999, "" } ,
      { 57,"Left  Culmen of Vermis..................",  3, 63, -3,2, -999, "" } ,
      { 57,"Right Culmen of Vermis..................", -3, 63, -3,2, -999, "" } ,
      { 58,"Left  Cerebellar Tonsil.................", 28, 51,-36,2, -999, "" } ,
      { 58,"Right Cerebellar Tonsil.................",-28, 51,-36,2, -999, "" } ,
      { 59,"Left  Inferior Semi-Lunar Lobule........", 29, 71,-38,2, -999, "" } ,
      { 59,"Right Inferior Semi-Lunar Lobule........",-29, 71,-38,2, -999, "" } ,
      { 60,"Left  Fastigium.........................",  7, 54,-20,2, -999, "" } ,
      { 60,"Right Fastigium.........................", -7, 54,-20,2, -999, "" } ,
      { 61,"Left  Nodule............................",  7, 55,-27,2, -999, "" } ,
      { 61,"Right Nodule............................", -7, 55,-27,2, -999, "" } ,
      { 62,"Left  Uvula.............................", 21, 76,-26,2, -999, "" } ,
      { 62,"Right Uvula.............................",-21, 76,-26,2, -999, "" } ,
      { 63,"Left  Pyramis...........................", 27, 74,-30,2, -999, "" } ,
      { 63,"Right Pyramis...........................",-27, 74,-30,2, -999, "" } ,
      { 66,"Left  Culmen............................", 20, 46,-16,2, -999, "" } ,
      { 66,"Right Culmen............................",-20, 46,-16,2, -999, "" } ,
      { 65,"Left  Declive...........................", 26, 69,-17,2, -999, "" } ,
      { 65,"Right Declive...........................",-26, 69,-17,2, -999, "" } ,
      {127,"Left  Dentate...........................", 14, 54,-23,4, -999, "" } ,
      {127,"Right Dentate...........................",-14, 54,-23,4, -999, "" } ,
      { 64,"Left  Tuber.............................", 44, 71,-27,2, -999, "" } ,
      { 64,"Right Tuber.............................",-44, 71,-27,2, -999, "" } ,
      { 67,"Left  Cerebellar Lingual................",  4, 45,-13,2, -999, "" } ,
      { 67,"Right Cerebellar Lingual................", -4, 45,-13,2, -999, "" }
} ;

int TTO_current = 0 ;  /* last chosen TTO */

/*! CA_EZ atlas material is now automatically prepared
from a downloaded SPM toolbox. See the matlab function
CA_EZ_Prep.m */

#include "thd_ttatlas_CA_EZ.c"


/*-----------------------------------------------------------------------*/
/* 
   szflag controls the number of slices in the atlas for TT_Daemon only:
       1 --> return Big TT atlas
      -1 --> return Small TT atlas
       0 --> Do nothing
*/
THD_3dim_dataset * TT_retrieve_atlas_dset(char *aname, int szflag) {
   ATLAS *atlas=NULL;
   char sbuf[256];
   THD_3dim_dataset *dset=NULL;
   
   if (!(atlas = Atlas_With_Trimming (aname, 1, NULL)) || !ATL_DSET(atlas)) {
      if (wami_verb()) 
         ERROR_message("Failed getting atlas for retrieval");
      return(NULL);
   }
   if (szflag) {
      /* The big or small option to TT_Daemon. Should not let this be done to
      other dsets. This is only legit for TT_Daemon and AFNI's interactive
         usage. A better approach is to use the resampler to match any grid... */
      if (strcmp(Atlas_Name(atlas),"TT_Daemon")) {
         if (wami_verb()) {
            INFO_message(
         "Nothing to do with szflag for atlases other than TT_Daemon\n"
         "Returning atlas %s's dset unchanged", Atlas_Name(atlas));
         }
         return(ATL_DSET(atlas));
      }
      if (szflag == 1 && is_small_TT(atlas)) { 
         /* want big, have small */
         sprintf(sbuf,"%s_big", DSET_PREFIX(ATL_DSET(atlas)) );
         if (!(dset = THD_zeropad(ATL_DSET(atlas), 10,0,0,0,0,0 , sbuf , 0 ))) {
            ERROR_message("Failed to fatten atlas\n");
            return(NULL);
         }
         DSET_delete(ATL_DSET(atlas)); 
         atlas->adh->adset = dset; dset = NULL;
      } else if (szflag == -1 && is_big_TT(atlas)) { 
         char *scut, *spref;
         /* want small, have big */
         spref = DSET_PREFIX(ATL_DSET(atlas));
         scut = strstr(spref, "_big");
         if (scut) {
            snprintf(sbuf,strlen(spref)-4,"%s", spref );
         } else {
            snprintf(sbuf,255,"%s", spref );
         }
         if (!(dset = THD_zeropad(ATL_DSET(atlas), 10,0,0,0,0,0 , sbuf , 0 ))) {
            ERROR_message("Failed to thin atlas\n");
            return(NULL);
         }
         DSET_delete(ATL_DSET(atlas)); 
         atlas->adh->adset = dset; dset = NULL;
      }
   }
   return(ATL_DSET(atlas));
}

int is_big_TT(ATLAS *atlas) {
   if (ATL_DSET(atlas) &&
       DSET_NZ(ATL_DSET(atlas)) == TT_ATLAS_NZ_BIG &&
       !strcmp(Atlas_Name(atlas),"TT_Daemon")) {
         return(1);
   }
   return(0);
}

int is_small_TT(ATLAS *atlas) {
   if (ATL_DSET(atlas) &&
       DSET_NZ(ATL_DSET(atlas)) == TT_ATLAS_NZ_SMALL &&
       !strcmp(Atlas_Name(atlas),"TT_Daemon")) {
         return(1);
   }
   return(0);
}



#ifdef KILLTHIS /* Remove all old sections framed by #ifdef KILLTHIS
                  in the near future.  ZSS May 2011   */ 
THD_3dim_dataset * TT_retrieve_atlas_old(void)
{
   if (wami_verb()) {
      WARNING_message("Obsolete, use Atlas_With_Trimming instead");
   }
   if( have_dseTT_old < 0 ) TT_load_atlas_old() ;
   return dseTT_old ;                         /* might be NULL */
}

/*-----------------------------------------------------------------------*/

THD_3dim_dataset * TT_retrieve_atlas_big_old(void) /* 01 Aug 2001 */
{
   char sbuf[256];

   if (wami_verb()) {
      WARNING_message("Obsolete, use Atlas_With_Trimming instead");
   }
   if( dseTT_big_old != NULL ) return dseTT_big_old ;
   if( have_dseTT_old < 0    ) TT_load_atlas_old() ;
   if( dseTT_old == NULL     ) return NULL ;
   sprintf(sbuf,"%s_big", TT_DAEMON_TT_PREFIX);
   dseTT_big_old = THD_zeropad( dseTT_old , 10,0,0,0,0,0 , sbuf , 0 ) ;
   DSET_unload( dseTT_old ) ; /* probably won't need again */
   return dseTT_big_old ;
}

/*-----------------------------------------------------------------------*/

THD_3dim_dataset * TT_retrieve_atlas_either_old(void) /* 22 Aug 2001 */
{
   if (wami_verb()) {
      WARNING_message("Obsolete, use Atlas_With_Trimming instead");
   }
   if( dseTT_big_old != NULL ) return dseTT_big_old ;
   if( dseTT_old     != NULL ) return dseTT_old     ;
   if( have_dseTT_old < 0    ) TT_load_atlas_old()  ;
   return dseTT_old ;
}

#endif

/*-----------------------------------------------------------------------*/
/*! Get name of directory contained TTatlas -- if it exists.  Name
    returned will be NULL (that's bad) or will end in a '/' character.
*//*---------------------------------------------------------------------*/

char * get_atlas_dirname(void)  /* 31 Jan 2008 -- RWCox */
{
   static char *adnam=NULL ; static int first=1 ;
   char *epath , *elocal , ename[THD_MAX_NAME] , dname[THD_MAX_NAME] ;
   int ll , ii , id , epos ;

   if( !first ) return adnam ;
   first = 0 ;
                       epath = getenv("AFNI_PLUGINPATH") ;
   if( epath == NULL ) epath = getenv("AFNI_PLUGIN_PATH") ;
   if( epath == NULL ) epath = getenv("PATH") ;
   if( epath == NULL ) return NULL ;  /* this is bad */

   ll = strlen(epath) ;
   elocal = AFMALL(char, sizeof(char) * (ll+2) ) ;
   strcpy(elocal,epath); elocal[ll] = ' '; elocal[ll+1] = '\0';
   for( ii=0 ; ii < ll ; ii++ ) if( elocal[ii] == ':' ) elocal[ii] = ' ';

   epos = 0 ;
   do{
     ii = sscanf( elocal+epos, "%s%n", ename, &id ); if( ii < 1 ) break;
     epos += id ;
     ii = strlen(ename) ;
     if( ename[ii-1] != '/' ){ ename[ii] = '/'; ename[ii+1] = '\0'; }
/* may want to not rely on this specific atlas for locations of all atlases */
     strcpy(dname,ename); strcat(dname,"TTatlas+tlrc.HEAD");
     if( THD_is_file(dname) ){
       free((void *)elocal); adnam = strdup(ename); return adnam;
     }
     strcpy(dname,ename); strcat(dname,"TTatlas.nii.gz");
     if( THD_is_file(dname) ){
       free((void *)elocal); adnam = strdup(ename); return adnam;
     }
   } while( epos < ll ) ;

   return NULL ;
}

/*-----------------------------------------------------------------------*/

THD_3dim_dataset * get_atlas(char *epath, char *aname)
{
   char dname[THD_MAX_NAME], ename[THD_MAX_NAME], *elocal=NULL;
   THD_3dim_dataset *dset = NULL;
   int epos=0, ll=0, ii=0, id = 0;
   int LocalHead = wami_lh();

   ENTRY("get_atlas");
/* change to allow full path in dataset name, or current directory,
   and not necessarily separate path*/
   if( epath != NULL ){ /* A path or dset was specified */
      if (aname == NULL) { /* all in epath */
         dset = THD_open_one_dataset( epath ) ;  /* try to open it */
         if(dset!=NULL) {
            DSET_mallocize (dset);
            DSET_load (dset);	                /* load dataset */
         }
      } else  {
         strncpy(dname,epath, (THD_MAX_NAME-2)*sizeof(char)) ;
         ii = strlen(dname);
         if (dname[ii-1] != '/') {
            dname[ii] = '/'; dname[ii+1] = '\0';
         }
         strncat(dname,aname, THD_MAX_NAME - strlen(dname)) ;
                                                /* add dataset name */
         dset = THD_open_one_dataset( dname ) ;
         if(dset!=NULL) {
            DSET_mallocize (dset);
            DSET_load (dset);	                /* load dataset */
         }
      }
      if( !dset ){                     /* got it!!! */
         ERROR_message("Failed to read dset %s\n", epath);
      }
      RETURN(dset);   /* return NULL or dset for specified path input */
   } else { /* no path given */
      if (aname == NULL) { /* nothing to work with here !*/
         ERROR_message("No path, no name, no soup for you.\n");
         RETURN(dset);
      }
      /* a name was given, search for it */
      /*----- get path to search -----*/

                          epath = getenv("AFNI_PLUGINPATH") ;
      if( epath == NULL ) epath = getenv("AFNI_PLUGIN_PATH") ;
      if( epath == NULL ) epath = getenv("PATH") ;
      if( epath == NULL ) RETURN(dset) ;

      /*----- copy path list into local memory -----*/

      ll = strlen(epath) ;
      elocal = AFMALL(char, sizeof(char) * (ll+2) ) ;

      /*----- put a blank at the end -----*/

      strcpy( elocal , epath ) ; elocal[ll] = ' ' ; elocal[ll+1] = '\0' ;

      /*----- replace colons with blanks -----*/

      for( ii=0 ; ii < ll ; ii++ )
        if( elocal[ii] == ':' ) elocal[ii] = ' ' ;

      /*----- extract blank delimited strings;
              use as directory names to look for atlas -----*/

      epos = 0 ;

      do{
         ii = sscanf( elocal+epos , "%s%n" , ename , &id ); /* next substring */
         if( ii < 1 ) break ;                               /* none -> done   */

         epos += id ;                                 /* char after last scanned */

         ii = strlen(ename) ;                         /* make sure name has   */
         if( ename[ii-1] != '/' ){                    /* a trailing '/' on it */
             ename[ii]  = '/' ; ename[ii+1] = '\0' ;
         }
         strcpy(dname,ename) ;
         strcat(dname,aname) ;               /* add dataset name */
         if(LocalHead)
             INFO_message("trying to open dataset:%s", dname);
         dset = THD_open_one_dataset( dname ) ;      /* try to open it */

         if( dset != NULL ){                         /* got it!!! */
            DSET_mallocize (dset);
            DSET_load (dset);	                /* load dataset */
            free(elocal); RETURN(dset);
         }

      } while(( epos < ll ) || (dset!=NULL)) ;  /* scan until 'epos' is after end of epath */


   } /* No path given */

   /* should only get here if no dataset found */
   RETURN(dset);
}

#ifdef KILLTHIS /* Remove all old sections framed by #ifdef KILLTHIS
                  in the near future.  ZSS May 2011   */ 

/*-----------------------------------------------------------------------*/
int TT_load_atlas_old(void)
{
   char *epath, sbuf[256] ;

ENTRY("TT_load_atlas_old") ;

   WARNING_message(
      "Obsolete, use Atlas_With_Trimming(\"TT_Daemon\", .) instead");

   if( have_dseTT_old >= 0 ) RETURN(have_dseTT_old) ;  /* for later calls */

   have_dseTT_old = 0 ;  /* don't have it yet */

   /*----- 20 Aug 2001: see if user specified alternate database -----*/

   epath = getenv("AFNI_TTATLAS_DATASET") ;   /* suggested path, if any */
   sprintf(sbuf,"%s+tlrc", TT_DAEMON_TT_PREFIX);
   dseTT_old = get_atlas( epath, sbuf ) ;  /* try to open it */
   if (!dseTT_old) { /* try for NIFTI */
      sprintf(sbuf,"%s.nii.gz", TT_DAEMON_TT_PREFIX);
      dseTT_old = get_atlas( epath, sbuf) ;
   }
   if( dseTT_old != NULL ){                     /* got it!!! */
      have_dseTT_old = 1; RETURN(1);
   }


   RETURN(0) ; /* got here -> didn't find it */
}

/*----------------------------------------------------------------------
  Allows the program to purge the memory used by the TT atlas dataset
------------------------------------------------------------------------*/

void TT_purge_atlas_old(void)
{
  PURGE_DSET(dseTT_old) ; return ;
}

void TT_purge_atlas_big_old(void)
{
   if( dseTT_big_old != NULL ){ 
      DSET_delete(dseTT_big_old) ; dseTT_big_old = NULL ; 
   }
   return ;
}

#endif


/*----------------------------------------------------------------------
   Begin coordinate transformation functions
------------------------------------------------------------------------*/

/*------------------------------------------------------------------------
   Forward transform a vector following a warp
   Don't mess with this function, many programs use it. ZSS Feb 06
--------------------------------------------------------------------------*/

THD_fvec3 AFNI_forward_warp_vector( THD_warp * warp , THD_fvec3 old_fv )
{
   THD_fvec3 new_fv ;

   if( warp == NULL ) return old_fv ;

   switch( warp->type ){

      default: new_fv = old_fv ; break ;

      case WARP_TALAIRACH_12_TYPE:{
         THD_linear_mapping map ;
         int iw ;

         /* forward transform each possible case,
            and test if result is in bot..top of defined map */

         for( iw=0 ; iw < 12 ; iw++ ){
            map    = warp->tal_12.warp[iw] ;
            new_fv = MATVEC_SUB(map.mfor,old_fv,map.bvec) ;

            if( new_fv.xyz[0] >= map.bot.xyz[0] &&
                new_fv.xyz[1] >= map.bot.xyz[1] &&
                new_fv.xyz[2] >= map.bot.xyz[2] &&
                new_fv.xyz[0] <= map.top.xyz[0] &&
                new_fv.xyz[1] <= map.top.xyz[1] &&
                new_fv.xyz[2] <= map.top.xyz[2]   ) break ;  /* leave loop */
         }
      }
      break ;

      case WARP_AFFINE_TYPE:{
         THD_linear_mapping map = warp->rig_bod.warp ;
         new_fv = MATVEC_SUB(map.mfor,old_fv,map.bvec) ;
      }
      break ;

   }
   return new_fv ;
}

/*------------------------------------------------------------------------
   Backward transform a vector following a warp
   Don't mess with this function, many programs use it. ZSS Feb 06
--------------------------------------------------------------------------*/
THD_fvec3 AFNI_backward_warp_vector( THD_warp * warp , THD_fvec3 old_fv )
{
   THD_fvec3 new_fv ;

   if( warp == NULL ) return old_fv ;

   switch( warp->type ){

      default: new_fv = old_fv ; break ;

      case WARP_TALAIRACH_12_TYPE:{
         THD_linear_mapping map ;
         int iw ;

         /* test if input is in bot..top of each defined map */

         for( iw=0 ; iw < 12 ; iw++ ){
            map = warp->tal_12.warp[iw] ;

            if( old_fv.xyz[0] >= map.bot.xyz[0] &&
                old_fv.xyz[1] >= map.bot.xyz[1] &&
                old_fv.xyz[2] >= map.bot.xyz[2] &&
                old_fv.xyz[0] <= map.top.xyz[0] &&
                old_fv.xyz[1] <= map.top.xyz[1] &&
                old_fv.xyz[2] <= map.top.xyz[2]   ) break ;  /* leave loop */
         }
         new_fv = MATVEC_SUB(map.mbac,old_fv,map.svec) ;
      }
      break ;

      case WARP_AFFINE_TYPE:{
         THD_linear_mapping map = warp->rig_bod.warp ;
         new_fv = MATVEC_SUB(map.mbac,old_fv,map.svec) ;
      }
      break ;

   }
   return new_fv ;
}

/*!
   Coordinate transformation between N27 MNI
   and AFNI's TLRC. Transform was obtained by manually AFNI TLRCing
   the N27 dset supplied in Eickhoff & Zilles' v12 dbase before
   the volume itself was placed in MNI Anatomical space as was done in v13.
   Input and output are in RAI
*/
THD_fvec3 THD_mni__tta_N27( THD_fvec3 mv, int dir )
{
   static THD_talairach_12_warp *ww=NULL;
   float tx,ty,tz ;
   int iw, ioff;
   THD_fvec3 tv, tv2;

   tx = ty = tz = -9000.0;
   LOAD_FVEC3( tv , tx,ty,tz ) ;
   LOAD_FVEC3( tv2 , tx,ty,tz ) ;

#if 0
   if (0) { /* Meth. 1, Left here should we allow user 
               someday to specify a .HEAD with their own transform... */
      INFO_message("What about the path?\nNeed something fool proof\n");
      if (!MNI_N27_to_TLRC_DSET) {
        MNI_N27_to_TLRC_DSET = 
               THD_open_one_dataset( MNI_N27_to_AFNI_TLRC_HEAD ) ;
        if (!MNI_N27_to_TLRC_DSET) {
         ERROR_message( "Failed to open transform dset %s\n"
                        "No transformation done.", MNI_N27_to_AFNI_TLRC_HEAD ) ;
         return tv ;
        }
      }
      /* get the warp */
      if (!MNI_N27_to_TLRC_DSET->warp) {
         ERROR_message("No Warp Found in %s\nNo transformation done.", 
                        MNI_N27_to_AFNI_TLRC_HEAD ) ;
         return tv ;
      }
      if (MNI_N27_to_TLRC_DSET->warp->type != WARP_TALAIRACH_12_TYPE) {
         ERROR_message( "Warp of unexpected type in %s.\n"
                        "No transformation done.", MNI_N27_to_AFNI_TLRC_HEAD ) ;
         return tv ;
      }

      if (dir > 0) tv = AFNI_forward_warp_vector(MNI_N27_to_TLRC_DSET->warp, mv);
      else tv = AFNI_backward_warp_vector(MNI_N27_to_TLRC_DSET->warp, mv);
      if (1) {
         INFO_message("tv(Meth 1): %f %f %f\n", tv.xyz[0], tv.xyz[1], tv.xyz[2]);
      }
   }
#endif

   /* Meth 2, xform in code, more fool proof*/
   if (!ww) {
      /* load the transform */
      ww = myXtNew( THD_talairach_12_warp ) ;
      ww->type = WARP_TALAIRACH_12_TYPE;
      ww->resam_type = 0;
      for (iw=0; iw < 12; ++iw) {
         ww->warp[iw].type = MAPPING_LINEAR_TYPE ;

         ioff = iw * MAPPING_LINEAR_FSIZE ;
         COPY_INTO_STRUCT( ww->warp[iw] ,
                           MAPPING_LINEAR_FSTART ,
                           float ,
                           &(MNI_N27_to_AFNI_TLRC_WRP_VEC[ioff]) ,
                           MAPPING_LINEAR_FSIZE ) ;

      }
   }

   if (!ww) {
      ERROR_message("Failed to form built-in warp.");
      return tv2;
   } else {
      if (dir > 0) tv2 = AFNI_forward_warp_vector((THD_warp *)ww, mv);
      else tv2 = AFNI_backward_warp_vector((THD_warp *)ww, mv);
   }

   if (0) {
      INFO_message("tv2(Meth. 2): %f %f %f\n", 
                   tv2.xyz[0], tv2.xyz[1], tv2.xyz[2]);
   }

   return tv2 ;
}

THD_fvec3 THD_mni_to_tta_N27( THD_fvec3 mv )
{
   return (THD_mni__tta_N27( mv , 1));
}

THD_fvec3 THD_tta_to_mni_N27( THD_fvec3 mv )
{
   return (THD_mni__tta_N27( mv , -1));
}

THD_fvec3 THD_mnia_to_tta_N27( THD_fvec3 mv )
{
   THD_fvec3 mva;
   /*go from MNI Anat to MNI (remember, shift is in RAI space, not LPI. See also script @Shift_volume)*/
   mva.xyz[0] = mv.xyz[0] + 0.0 ;
   mva.xyz[1] = mv.xyz[1] - 4.0 ;
   mva.xyz[2] = mv.xyz[2] - 5.0 ;

   return (THD_mni__tta_N27( mva , 1));
}

THD_fvec3 THD_tta_to_mnia_N27( THD_fvec3 mv )
{
   THD_fvec3 mva;

   mva = THD_mni__tta_N27( mv , -1);

   /*go from MNI to MNI Anat (remember, shift is in RAI space, not LPI. See also script @Shift_volume)*/
   mva.xyz[0] = mva.xyz[0] + 0.0 ;
   mva.xyz[1] = mva.xyz[1] + 4.0 ;
   mva.xyz[2] = mva.xyz[2] + 5.0 ;

   return (mva);
}

/*! are we on the left or right of Colin? */
char MNI_Anatomical_Side(ATLAS_COORD ac, ATLAS_LIST *atlas_list)
{
   THD_ivec3 ijk ;
   int  ix,jy,kz , nx,ny,nz,nxy, ii=0, kk=0;
   byte *ba=NULL;
   static int n_warn = 0;
   ATLAS *atlas=NULL;
   int LocalHead = wami_lh();

   ENTRY("MNI_Anatomical_Side");

   /* ONLY TLRC for now, must allow for others in future */
   if (!is_Coord_Space_Named(ac, "TLRC")) {
      ERROR_message("Coordinates must be in 'TLRC' space");
      RETURN('u');
   }

   if (!(atlas = Atlas_With_Trimming("CA_N27_LR", 1, atlas_list))) {
      if (ii == 0 && !n_warn) {
         WARNING_message("Could not read LR atlas named %s", "CA_N27_LR");
         ++n_warn;
      }
   }

   if (!atlas) {
      if (n_warn < 2) {
         WARNING_message("Relying on x coordinate to guess side");
         ++n_warn;
      }
      if (ac.x<0.0) {
         RETURN('r');
      } else {
         RETURN('l');
      }
   } else {
      /* where are we in the ijk grid ? */
      ijk = THD_3dmm_to_3dind( ATL_DSET(atlas) , TEMP_FVEC3(ac.x,ac.y,ac.z) ) ;
      UNLOAD_IVEC3(ijk,ix,jy,kz) ;                               

      nx = DSET_NX(ATL_DSET(atlas)) ;  /* size of atlas dataset axes */
      ny = DSET_NY(ATL_DSET(atlas)) ;
      nz = DSET_NZ(ATL_DSET(atlas)) ; nxy = nx*ny ;

      /*-- check the exact input location --*/
      ba = DSET_BRICK_ARRAY(ATL_DSET(atlas),0);
      kk = ix + jy*nx + kz*nxy ;        /* index into brick arrays */
      if( ba[kk] == 2 ){
         RETURN('l');
      }else if( ba[kk] == 1 ){
         RETURN('r');
      }else {
         /* not in mask, use coord */
         if (ac.x<0.0) {
            RETURN('r');
         } else {
            RETURN('l');
         }
      }
   }

   /* should not get here */
   RETURN('u');
}
/*! What side are we on ?*/
char Atlas_Voxel_Side( THD_3dim_dataset *dset, int k1d, byte *lrmask)
{
   THD_ivec3 ijk ;
   THD_fvec3 xyz ;
   int  ix,jy,kz , nx,ny,nz,nxy;

   ENTRY("Atlas_Voxel_Side");


   if ( lrmask ) { /* easy */
      if (lrmask[k1d] == 2 ){
         RETURN('l');
      }else if( lrmask[k1d] == 1 ){
         RETURN('r');
      } else {
         RETURN('u');
      }
   }

   if (!dset) {
      ERROR_message("Need an atlas dset");
      RETURN('u');
   }

   /* based on coords */
   nx = DSET_NX(dset) ;               /* size of atlas dataset axes */
   ny = DSET_NY(dset) ;
   nz = DSET_NZ(dset) ; nxy = nx*ny ;

   kz = (k1d / nxy);
   jy = (k1d % nxy);
   ix = (jy  % nx);
   jy = (jy  / nx);

   LOAD_IVEC3(ijk, ix, jy, kz);
   xyz = THD_3dind_to_3dmm(dset, ijk);

   if (xyz.xyz[0]<0.0) {
      RETURN('r');
   } else {
      RETURN('l');
   }


   /* should not get here */
   RETURN('u');
}
/*----------------------------------------------------------------------
   Return a multi-line string of TT atlas labels near the given point
   (xx,yy,zz are in Dicom order coordinates).
   If NULL is returned, an error happened.  If no labels are near the
   given point, then a single line saying "you're lost" is returned.
   The string returned is malloc()-ed and should be free()-ed later.
   The string will end with a newline '\n' character.
------------------------------------------------------------------------*/
/* ***tbd need to update this for new atlas structures -
    will need the space, xform and atlas list */

#if 0
#undef  SS
#define SS(c) fprintf(stderr,"sp %c\n",(c))
#else
#define SS(c) /*nada*/
#endif

char * genx_Atlas_Query_to_String (ATLAS_QUERY *wami,
                              ATLAS_COORD ac, WAMI_SORT_MODES mode,
                              ATLAS_LIST *atlas_list)
{
   char *rbuf = NULL;
   int max_spaces = 50;
   char  xlab[max_spaces][32], ylab[max_spaces][32] , zlab[max_spaces][32],
         clab[max_spaces][32], lbuf[1024]  , tmps[1024] ;
   THD_string_array *sar =NULL;
   ATLAS_COORD *acl=NULL;
   int iatlas = -1, N_out_spaces=0, it=0;
   int i, ii, nfind=0, nfind_one = 0, iq=0, il=0, newzone = 0;
   ATLAS *atlas=NULL;
   char **out_spaces=NULL; 
   int LocalHead = wami_lh();

   ENTRY("genx_Atlas_Query_to_String") ;

   if (!wami) {
      ERROR_message("NULL wami");
      RETURN(rbuf);
   }
   
   /* the classic three. */
   out_spaces =  add_to_names_list(out_spaces, &N_out_spaces, "TLRC");
   out_spaces =  add_to_names_list(out_spaces, &N_out_spaces, "MNI");
   out_spaces =  add_to_names_list(out_spaces, &N_out_spaces, "MNI_ANAT");
   
   if (N_out_spaces > max_spaces) {
      ERROR_message("Too many spaces for fixed allocation variables");
      RETURN(rbuf);
   }
   acl = (ATLAS_COORD *)calloc(N_out_spaces, sizeof(ATLAS_COORD));
   if (!transform_atlas_coords(ac, out_spaces, N_out_spaces, acl, "RAI")) {
      ERROR_message("Failed to transform coords");
      RETURN(rbuf);
   }
   
   if (LocalHead) {/* Show me all the coordinates */
      INFO_message("Original Coordinates \n");
      print_atlas_coord(ac);
      for (i=0; i<N_out_spaces; ++i) {
         INFO_message("Coordinate xformed to %s\n", out_spaces[i]);
         print_atlas_coord(acl[i]);
      }
   }

   if ((it = find_coords_in_space(acl, N_out_spaces, "TLRC"))<0) {
      ERROR_message("Need to have TLRC coords for chunk below");
      RETURN(rbuf); 
   }

   /* Prep the string toys */
   INIT_SARR(sar) ; ADDTO_SARR(sar,WAMI_HEAD) ;
   sprintf(lbuf, "Original input data coordinates in %s space\n", 
            ac.space_name);
   ADDTO_SARR(sar, lbuf);

   /* form the string */
   for (i=0; i<N_out_spaces; ++i) {
      if(strcmp(acl[i].space_name,"MNI_ANAT")) {
         sprintf(xlab[i],"%4.0f mm [%c]",-acl[i].x,(acl[i].x<0.0)?'R':'L') ;
      } else { 
         sprintf(xlab[i], "%4.0f mm [%c]", 
                 -acl[i].x, TO_UPPER(MNI_Anatomical_Side(acl[it], atlas_list))) ;
      }
      sprintf(ylab[i],"%4.0f mm [%c]",-acl[i].y,(acl[i].y<0.0)?'A':'P') ;
      sprintf(zlab[i],"%4.0f mm [%c]", acl[i].z,(acl[i].z<0.0)?'I':'S') ;
      sprintf(clab[i],"{%s}", acl[i].space_name);
   }
   free(acl); acl = NULL;
   
   /* form the Focus point part */
   switch (mode) {
      case CLASSIC_WAMI_ATLAS_SORT:
      case CLASSIC_WAMI_ZONE_SORT:
            SS('m');
            sprintf(lbuf,"Focus point (LPI)=%c", lsep);
            for (i=0; i<N_out_spaces; ++i) {
               sprintf(tmps, "   %s,%s,%s %s%c",
                        xlab[i], ylab[i], zlab[i], clab[i], lsep);
               strncat(lbuf, tmps, 1023*sizeof(char));
            }
            ADDTO_SARR(sar,lbuf);
         break;
      case TAB1_WAMI_ATLAS_SORT:
      case TAB1_WAMI_ZONE_SORT:
      case TAB2_WAMI_ATLAS_SORT:
      case TAB2_WAMI_ZONE_SORT:
            SS('o');
            sprintf(lbuf,"%-36s\t%-12s", "Focus point (LPI)", "Coord.Space");
            ADDTO_SARR(sar,lbuf);
            for (ii=0; ii<3; ++ii) {
               SS('p');sprintf(tmps,"%s,%s,%s", xlab[ii], ylab[ii], zlab[ii]);
               SS('q');sprintf(lbuf,"%-36s\t%-12s", tmps, clab[ii]);
               ADDTO_SARR(sar,lbuf);
            }
         break;
      default:
         ERROR_message("Bad mode %d", mode);
         RETURN(rbuf);
   }


   switch (mode) {
      case CLASSIC_WAMI_ATLAS_SORT: /* the olde ways */
            /*-- assemble output string(s) for each atlas --*/
            nfind = 0;
            /* for each atlas atcode */
            for (iatlas=0; iatlas < atlas_list->natlases; ++iatlas) { 
               atlas = &(atlas_list->atlas[iatlas]);
               nfind_one = 0;
               /* for each zone iq */
               for (iq=0; iq<wami->N_zone; ++iq) { 
                  newzone = 1;
                  /* for each label in a zone il */
                  for (il=0; il<wami->zone[iq]->N_label; ++il) { 
                     if (is_Atlas_Named(atlas, wami->zone[iq]->atname[il]))  {
                        if (!nfind_one) {
                           SS('r');
                           sprintf(lbuf, "Atlas %s: %s", ATL_NAME_S(atlas), 
                             ATL_DESCRIPTION_S(atlas));
                           ADDTO_SARR(sar,lbuf);
                        }
                        if (newzone) {
                           if (wami->zone[iq]->radius[il] == 0.0) {
                              SS('s');
                              sprintf(lbuf, "   Focus point: %s",
                                 Clean_Atlas_Label(wami->zone[iq]->label[il]));
                           } else {
                              SS('t');
                              sprintf(lbuf, "   Within %1d mm: %s",
                                  (int)wami->zone[iq]->radius[il],
                                  Clean_Atlas_Label(wami->zone[iq]->label[il]));
                           }
                           newzone = 0;
                        } else {
                           SS('u');
                           sprintf(lbuf, "          -AND- %s",  
                                 Clean_Atlas_Label(wami->zone[iq]->label[il]));
                        }
                        if (wami->zone[iq]->prob[il] > 0.0) {
                           SS('v');
                           sprintf(lbuf+strlen(lbuf), "   (p = %s)", 
                                 Atlas_Prob_String(wami->zone[iq]->prob[il]));
                        }
                        ADDTO_SARR(sar,lbuf);
                        ++nfind; ++nfind_one;
                     }
                  } /* il */
               } /* iq */
               if (nfind_one) {
                  ADDTO_SARR(sar,"");
               }
            } /* iatlas */

         break;
      case CLASSIC_WAMI_ZONE_SORT:
            /*-- assemble output string(s) for each atlas --*/
            nfind = 0;
            for (iq=0; iq<wami->N_zone; ++iq) { /* iq */
               if (wami->zone[iq]->level == 0) {
                  SS('w');sprintf(lbuf, "Focus point:");
               } else {
                  SS('x');sprintf(lbuf, "Within %1d mm:",
                                 (int)wami->zone[iq]->level);
               }
               ADDTO_SARR(sar,lbuf);
               for (il=0; il<wami->zone[iq]->N_label; ++il) { /* il */
                  SS('y');sprintf(lbuf, "   %-32s, Atlas %-15s",
                     Clean_Atlas_Label(wami->zone[iq]->label[il]),
                                       wami->zone[iq]->atname[il]);

                  if (wami->zone[iq]->prob[il] > 0.0) {
                     SS('z');
                     sprintf(lbuf+strlen(lbuf), 
                              ", prob. = %-3s", 
                              Atlas_Prob_String(wami->zone[iq]->prob[il]));
                  }
                  ADDTO_SARR(sar,lbuf);
                  ++nfind;
               } /* il */
            } /* iq */

         break;
      case TAB1_WAMI_ZONE_SORT:
      case TAB2_WAMI_ZONE_SORT:
         /*-- assemble output string(s) for each atlas --*/
            SS('E');
            sprintf(lbuf, "%-3s\t%-15s\t%-32s\t%-3s\t%-3s", 
                     "Within", "Atlas", "Label", "Prob.", "Code");
            ADDTO_SARR(sar,lbuf);
            nfind = 0;
               for (iq=0; iq<wami->N_zone; ++iq) { /* iq */
                  for (il=0; il<wami->zone[iq]->N_label; ++il) { /* il */
                     if (1) {
                        SS('F');
                        sprintf(tmps, "%.1f", wami->zone[iq]->radius[il]);
                        SS('G');
                        sprintf(lbuf, "%-3s\t%-15s\t%-32s\t%-3s\t%-3d",
                          tmps, wami->zone[iq]->atname[il],
                          Clean_Atlas_Label(wami->zone[iq]->label[il]),
                          Atlas_Prob_String(wami->zone[iq]->prob[il]),
                          wami->zone[iq]->code[il]);
                        ADDTO_SARR(sar,lbuf);
                        ++nfind; ++nfind_one;
                     }
                  } /* il */
               } /* iq */

         break;
      case TAB1_WAMI_ATLAS_SORT:
      case TAB2_WAMI_ATLAS_SORT: /* like TAB1_WAMI_ATLAS_SORT but more to my 
                                    liking for easy spreadsheet use */
            /*-- assemble output string(s) for each atlas --*/
            SS('H');
            sprintf( lbuf, "%-15s\t%-3s\t%-32s\t%-3s\t%-3s", 
                     "Atlas", "Within", "Label", "Prob.", "Code");
            ADDTO_SARR(sar,lbuf);
            nfind = 0;
            for (iatlas=0; iatlas < atlas_list->natlases; ++iatlas){ /* iatlas */
               atlas = &(atlas_list->atlas[iatlas]);
               nfind_one = 0;
               for (iq=0; iq<wami->N_zone; ++iq) { /* iq */
                  for (il=0; il<wami->zone[iq]->N_label; ++il) { /* il */
                     if (is_Atlas_Named(atlas, wami->zone[iq]->atname[il])) {
                        SS('I');
                        sprintf(tmps, "%.1f", wami->zone[iq]->radius[il]);
                        SS('J');sprintf(lbuf, "%-15s\t%-3s\t%-32s\t%-3s\t%-3d",
                                    wami->zone[iq]->atname[il],
                                    tmps,
                                    Clean_Atlas_Label(wami->zone[iq]->label[il]),
                                    Atlas_Prob_String(wami->zone[iq]->prob[il]),
                                    wami->zone[iq]->code[il]);
                        ADDTO_SARR(sar,lbuf);
                        ++nfind; ++nfind_one;
                     }
                  } /* il */
               } /* iq */

            } /* iatlas */

         break;
      default:
         ERROR_message("Bad mode (%d).", mode);
         RETURN(rbuf);
   }

   /* anything ? */
   if (!nfind) {
      ADDTO_SARR(sar,"***** Not near any region stored in databases *****\n");
   }
   /*- if didn't make any label, must produce something -*/

   if( sar->num == 1 ){    /* shouldn't ever happen */
      SS('K');sprintf(lbuf,"Found %d marked but unlabeled regions???\n",nfind) ;
      ADDTO_SARR(sar,lbuf) ;
   } else if( !AFNI_noenv("AFNI_TTATLAS_CAUTION") ){
      ADDTO_SARR(sar,WAMI_TAIL) ;  /* cautionary tail */
   }

   /*- convert list of labels into one big multi-line string -*/

   for( nfind=ii=0 ; ii < sar->num ; ii++ ) nfind += strlen(sar->ar[ii]) ;
   rbuf = AFMALL(char, nfind + 2*sar->num + 32 ) ; rbuf[0] = '\0' ;
   for( ii=0 ; ii < sar->num ; ii++ ){
      strcat(rbuf,sar->ar[ii]) ; strcat(rbuf,"\n") ;
   }

   DESTROY_SARR(sar) ;

   if (LocalHead) {
      INFO_message("Have:\n%s\n", rbuf);
   }

   RETURN(rbuf);
   
}

int transform_atlas_coords(ATLAS_COORD ac, char **out_spaces, 
                           int N_out_spaces, ATLAS_COORD *acl, char *orcodeout) 
{
   ATLAS_XFORM_LIST *xfl=NULL;
   int i;
   float xout=0.0, yout=0.0, zout=0.0;
   
   ENTRY("transform_atlas_coords");
   
   if (!out_spaces || !acl) RETURN(0);
   
   if (strncmp(ac.orcode, "RAI", 3)) {
      ERROR_message(
         "AC orientation (%s) not RAI\n"
         "Need a function to turn ac to RAI ",
                     ac.orcode);
      RETURN(0);
   }
   if (strncmp(orcodeout, "RAI", 3)) {
      ERROR_message(
         "Output orientation (%s) not RAI\n"
         "Need a function to go from RAI to desrired output orientation ",
                     ac.orcode);
      RETURN(0);
   }
   
   for (i=0; i<N_out_spaces; ++i) {
      if ((xfl = report_xform_chain(ac.space_name, out_spaces[i], 0))) {
         apply_xform_chain(xfl, ac.x, ac.y, ac.z, &xout, &yout, &zout);
         XYZ_to_AtlasCoord(xout, yout, zout, "RAI", 
                           out_spaces[i], &(acl[i]));
      } else {
         if (wami_verb()) {
            INFO_message("no route from %s to %s",
                        ac.space_name, out_spaces[i]);
         }
         XYZ_to_AtlasCoord(0.0, 0.0, 0.0, "RAI", 
                           "Unkown", &(acl[i]));
      }
   }   
   
   RETURN(1);
}

int find_coords_in_space(ATLAS_COORD *acl, int N_acl, char *space_name) 
{
   int i;
   
   if (!space_name || !acl) return(-1);
   for (i=0; i<N_acl; ++i) {
      if (!strcmp(acl[i].space_name,space_name)) return(i);
   }
   return(-1);
}

char * Atlas_Query_to_String (ATLAS_QUERY *wami,
                              ATLAS_COORD ac, WAMI_SORT_MODES mode,
                              ATLAS_LIST *atlas_list)
{
   char *rbuf = NULL;
   char  xlab[5][32], ylab[5][32] , zlab[5][32],
         clab[5][32], lbuf[1024]  , tmps[1024] ;
   THD_fvec3 t, m;
   THD_string_array *sar =NULL;
   ATLAS_COORD acv[NUMBER_OF_SPC];
   int iatlas = -1;
   int i, ii, nfind=0, nfind_one = 0, iq=0, il=0, newzone = 0;
   AFNI_STD_SPACES start_space;
   ATLAS *atlas=NULL;
   int LocalHead = wami_lh();

   ENTRY("Atlas_Query_to_String") ;

   if (wami_verb()) INFO_message("Obsolete, use genx_Atlas_Query_to_String") ;
   
   if (!wami) {
      ERROR_message("NULL wami");
      RETURN(rbuf);
   }
      
   /* get the coordinates into as many spaces as possible */
   /* first put ac in AFNI_TLRC */
   LOAD_FVEC3( m , ac.x, ac.y, ac.z ) ;
   /* the original starting space has the coordinates for that space */
   start_space = Space_Name_to_Space_Code(ac.space_name);  
                        /* save the index of the original space */
   acv[Space_Name_to_Space_Code(ac.space_name)] = ac;
/* use general transformation among spaces from NIML database here
    if possible. Need which output spaces or show all available output spaces */  
   switch (Space_Name_to_Space_Code(ac.space_name)) {
      default: ERROR_message("bad ac.space") ; RETURN(rbuf);
      case AFNI_TLRC_SPC:
         acv[AFNI_TLRC_SPC].x = ac.x;
         acv[AFNI_TLRC_SPC].y = ac.y;
         acv[AFNI_TLRC_SPC].z = ac.z;
         set_Coord_Space_Name(&(acv[AFNI_TLRC_SPC]), "TLRC");
         break;
      case MNI_ANAT_SPC:
         t = THD_mnia_to_tta_N27(m);
         if (t.xyz[0] < -500) { ERROR_message("Failed in xforming the data"); }
         acv[AFNI_TLRC_SPC].x = t.xyz[0];
         acv[AFNI_TLRC_SPC].y = t.xyz[1];
         acv[AFNI_TLRC_SPC].z = t.xyz[2];
         set_Coord_Space_Name(&(acv[AFNI_TLRC_SPC]), "TLRC");
         break;
      case MNI_SPC:
         /* function below expects coords in LPI !!!*/
         m.xyz[0] = -m.xyz[0]; m.xyz[1] = -m.xyz[1];
         t = THD_mni_to_tta(m);  /* Used to be: THD_mni_to_tta_N27 but that
                                    is inconsistent with inverse transformation
                                    below which used  THD_tta_to_mni to go back
                                    to mni space ZSS: Jul. 08*/
         if (t.xyz[0] < -500) { ERROR_message("Failed in xforming the data"); }
         acv[AFNI_TLRC_SPC].x = t.xyz[0];
         acv[AFNI_TLRC_SPC].y = t.xyz[1];
         acv[AFNI_TLRC_SPC].z = t.xyz[2];
         set_Coord_Space_Name(&(acv[AFNI_TLRC_SPC]), "TLRC");
         break;
   }


   if (LocalHead) {/* Show me all the coordinates */
      INFO_message("Original Coordinates in %s: %f %f %f\n",
                     ac.space_name, ac.x, ac.y, ac.z);
      /* got turned to "TLRC" */
      INFO_message("Coordinate in %s: %f %f %f\n",
                     acv[AFNI_TLRC_SPC].space_name,
            acv[AFNI_TLRC_SPC].x, acv[AFNI_TLRC_SPC].y, acv[AFNI_TLRC_SPC].z);
   }

   /* Prep the string toys */
   INIT_SARR(sar) ; ADDTO_SARR(sar,WAMI_HEAD) ;
   sprintf(lbuf, "Original input data coordinates in %s space\n", 
            ac.space_name);
   ADDTO_SARR(sar, lbuf);
   ac = acv[AFNI_TLRC_SPC]; /* TLRC from now on */
   for (i=UNKNOWN_SPC+1; i<NUMBER_OF_SPC; ++i) {
      if(i!=start_space) {  /* if not already set from starting space */
         LOAD_FVEC3(m,ac.x, ac.y, ac.z);
         t = m;      /* default no transformation of coordinates */
         if(i==MNI_SPC) {
            t = THD_tta_to_mni(m);   /* returns LPI */
            t.xyz[0] = -t.xyz[0]; t.xyz[1] = -t.xyz[1];
         }       
         else 
            if(i==MNI_ANAT_SPC)
               t = THD_tta_to_mnia_N27(m);
         acv[i].x = t.xyz[0]; acv[i].y = t.xyz[1]; acv[i].z = t.xyz[2];
      }
      /* for MNI anatomical output, check TLRC coordinates against L/R volume that is
      already in TLRC space. This LR volume had been converted from MNI_Anat space.
      The L/R volume is from a particular subject, and it is not completely aligned along
      any zero line separating left from right */
      if(i!=MNI_ANAT_SPC)
         sprintf(xlab[i-1],"%4.0f mm [%c]",-acv[i].x,(acv[i].x<0.0)?'R':'L') ;
      else  
         sprintf(xlab[i-1], "%4.0f mm [%c]", 
                   -acv[i].x, TO_UPPER(MNI_Anatomical_Side(acv[AFNI_TLRC_SPC],
                               atlas_list))) ;
      sprintf(ylab[i-1],"%4.0f mm [%c]",-acv[i].y,(acv[i].y<0.0)?'A':'P') ;
      sprintf(zlab[i-1],"%4.0f mm [%c]", acv[i].z,(acv[i].z<0.0)?'I':'S') ;
      sprintf(clab[i-1],"{%s}", Space_Code_to_Space_Name(i));
   }

   /* form the Focus point part */
   switch (mode) {
      case CLASSIC_WAMI_ATLAS_SORT:
      case CLASSIC_WAMI_ZONE_SORT:
            SS('m');sprintf(lbuf,"Focus point (LPI)=%c"
                         "   %s,%s,%s %s%c"
                         "   %s,%s,%s %s%c"
                         "   %s,%s,%s %s%c",
                         lsep,
                         xlab[0], ylab[0], zlab[0], clab[0], lsep,
                         xlab[1], ylab[1], zlab[1], clab[1], lsep,
                         xlab[2], ylab[2], zlab[2], clab[2], lsep);
            ADDTO_SARR(sar,lbuf);
         break;
      case TAB1_WAMI_ATLAS_SORT:
      case TAB1_WAMI_ZONE_SORT:
      case TAB2_WAMI_ATLAS_SORT:
      case TAB2_WAMI_ZONE_SORT:
            SS('o');
            sprintf(lbuf,"%-36s\t%-12s", "Focus point (LPI)", "Coord.Space");
            ADDTO_SARR(sar,lbuf);
            for (ii=0; ii<3; ++ii) {
               SS('p');sprintf(tmps,"%s,%s,%s", xlab[ii], ylab[ii], zlab[ii]);
               SS('q');sprintf(lbuf,"%-36s\t%-12s", tmps, clab[ii]);
               ADDTO_SARR(sar,lbuf);
            }
         break;
      default:
         ERROR_message("Bad mode %d", mode);
         RETURN(rbuf);
   }


   switch (mode) {
      case CLASSIC_WAMI_ATLAS_SORT: /* the olde ways */
            /*-- assemble output string(s) for each atlas --*/
            nfind = 0;
            /* for each atlas atcode */
            for (iatlas=0; iatlas < atlas_list->natlases; ++iatlas) { 
               atlas = &(atlas_list->atlas[iatlas]);
               nfind_one = 0;
               /* for each zone iq */
               for (iq=0; iq<wami->N_zone; ++iq) { 
                  newzone = 1;
                  /* for each label in a zone il */
                  for (il=0; il<wami->zone[iq]->N_label; ++il) { 
                     if (is_Atlas_Named(atlas, wami->zone[iq]->atname[il]))  {
                        if (!nfind_one) {
                           SS('r');
                           sprintf(lbuf, "Atlas %s: %s", ATL_NAME_S(atlas), 
                             ATL_DESCRIPTION_S(atlas));
                           ADDTO_SARR(sar,lbuf);
                        }
                        if (newzone) {
                           if (wami->zone[iq]->radius[il] == 0.0) {
                              SS('s');
                              sprintf(lbuf, "   Focus point: %s",
                                 Clean_Atlas_Label(wami->zone[iq]->label[il]));
                           } else {
                              SS('t');
                              sprintf(lbuf, "   Within %1d mm: %s",
                                  (int)wami->zone[iq]->radius[il],
                                  Clean_Atlas_Label(wami->zone[iq]->label[il]));
                           }
                           newzone = 0;
                        } else {
                           SS('u');
                           sprintf(lbuf, "          -AND- %s",  
                                 Clean_Atlas_Label(wami->zone[iq]->label[il]));
                        }
                        if (wami->zone[iq]->prob[il] > 0.0) {
                           SS('v');
                           sprintf(lbuf+strlen(lbuf), "   (p = %s)", 
                                 Atlas_Prob_String(wami->zone[iq]->prob[il]));
                        }
                        ADDTO_SARR(sar,lbuf);
                        ++nfind; ++nfind_one;
                     }
                  } /* il */
               } /* iq */
               if (nfind_one) {
                  ADDTO_SARR(sar,"");
               }
            } /* iatlas */

         break;
      case CLASSIC_WAMI_ZONE_SORT:
            /*-- assemble output string(s) for each atlas --*/
            nfind = 0;
            for (iq=0; iq<wami->N_zone; ++iq) { /* iq */
               if (wami->zone[iq]->level == 0) {
                  SS('w');sprintf(lbuf, "Focus point:");
               } else {
                  SS('x');sprintf(lbuf, "Within %1d mm:",
                                 (int)wami->zone[iq]->level);
               }
               ADDTO_SARR(sar,lbuf);
               for (il=0; il<wami->zone[iq]->N_label; ++il) { /* il */
                  SS('y');sprintf(lbuf, "   %-32s, Atlas %-15s",
                     Clean_Atlas_Label(wami->zone[iq]->label[il]),
                                       wami->zone[iq]->atname[il]);

                  if (wami->zone[iq]->prob[il] > 0.0) {
                     SS('z');
                     sprintf(lbuf+strlen(lbuf), 
                              ", prob. = %-3s", 
                              Atlas_Prob_String(wami->zone[iq]->prob[il]));
                  }
                  ADDTO_SARR(sar,lbuf);
                  ++nfind;
               } /* il */
            } /* iq */

         break;
      case TAB1_WAMI_ZONE_SORT:
      case TAB2_WAMI_ZONE_SORT:
         /*-- assemble output string(s) for each atlas --*/
            SS('E');
            sprintf(lbuf, "%-3s\t%-15s\t%-32s\t%-3s\t%-3s", 
                     "Within", "Atlas", "Label", "Prob.", "Code");
            ADDTO_SARR(sar,lbuf);
            nfind = 0;
               for (iq=0; iq<wami->N_zone; ++iq) { /* iq */
                  for (il=0; il<wami->zone[iq]->N_label; ++il) { /* il */
                     if (1) {
                        SS('F');
                        sprintf(tmps, "%.1f", wami->zone[iq]->radius[il]);
                        SS('G');
                        sprintf(lbuf, "%-3s\t%-15s\t%-32s\t%-3s\t%-3d",
                          tmps, wami->zone[iq]->atname[il],
                          Clean_Atlas_Label(wami->zone[iq]->label[il]),
                          Atlas_Prob_String(wami->zone[iq]->prob[il]),
                          wami->zone[iq]->code[il]);
                        ADDTO_SARR(sar,lbuf);
                        ++nfind; ++nfind_one;
                     }
                  } /* il */
               } /* iq */

         break;
      case TAB1_WAMI_ATLAS_SORT:
      case TAB2_WAMI_ATLAS_SORT: /* like TAB1_WAMI_ATLAS_SORT but more to my 
                                    liking for easy spreadsheet use */
            /*-- assemble output string(s) for each atlas --*/
            SS('H');
            sprintf( lbuf, "%-15s\t%-3s\t%-32s\t%-3s\t%-3s", 
                     "Atlas", "Within", "Label", "Prob.", "Code");
            ADDTO_SARR(sar,lbuf);
            nfind = 0;
            for (iatlas=0; iatlas < atlas_list->natlases; ++iatlas){ /* iatlas */
               atlas = &(atlas_list->atlas[iatlas]);
               nfind_one = 0;
               for (iq=0; iq<wami->N_zone; ++iq) { /* iq */
                  for (il=0; il<wami->zone[iq]->N_label; ++il) { /* il */
                     if (is_Atlas_Named(atlas, wami->zone[iq]->atname[il])) {
                        SS('I');
                        sprintf(tmps, "%.1f", wami->zone[iq]->radius[il]);
                        SS('J');sprintf(lbuf, "%-15s\t%-3s\t%-32s\t%-3s\t%-3d",
                                    wami->zone[iq]->atname[il],
                                    tmps,
                                    Clean_Atlas_Label(wami->zone[iq]->label[il]),
                                    Atlas_Prob_String(wami->zone[iq]->prob[il]),
                                    wami->zone[iq]->code[il]);
                        ADDTO_SARR(sar,lbuf);
                        ++nfind; ++nfind_one;
                     }
                  } /* il */
               } /* iq */

            } /* iatlas */

         break;
      default:
         ERROR_message("Bad mode (%d).", mode);
         RETURN(rbuf);
   }

   /* anything ? */
   if (!nfind) {
      ADDTO_SARR(sar,"***** Not near any region stored in databases *****\n");
   }
   /*- if didn't make any label, must produce something -*/

   if( sar->num == 1 ){    /* shouldn't ever happen */
      SS('K');sprintf(lbuf,"Found %d marked but unlabeled regions???\n",nfind) ;
      ADDTO_SARR(sar,lbuf) ;
   } else if( !AFNI_noenv("AFNI_TTATLAS_CAUTION") ){
      ADDTO_SARR(sar,WAMI_TAIL) ;  /* cautionary tail */
   }

   /*- convert list of labels into one big multi-line string -*/

   for( nfind=ii=0 ; ii < sar->num ; ii++ ) nfind += strlen(sar->ar[ii]) ;
   rbuf = AFMALL(char, nfind + 2*sar->num + 32 ) ; rbuf[0] = '\0' ;
   for( ii=0 ; ii < sar->num ; ii++ ){
      strcat(rbuf,sar->ar[ii]) ; strcat(rbuf,"\n") ;
   }

   DESTROY_SARR(sar) ;

   if (LocalHead) {
      INFO_message("Have:\n%s\n", rbuf);
   }

   RETURN(rbuf);
}
#undef SS

void TT_whereami_set_outmode(WAMI_SORT_MODES md)
{

   TT_whereami_mode = md;
   switch (md) {
      case TAB2_WAMI_ATLAS_SORT:
      case TAB2_WAMI_ZONE_SORT:
         lsep =  '\t';
         break;
      case TAB1_WAMI_ATLAS_SORT:
      case TAB1_WAMI_ZONE_SORT:
         lsep =  '\t';
         break;
      case CLASSIC_WAMI_ATLAS_SORT:
      case CLASSIC_WAMI_ZONE_SORT:
         lsep =  '\n';
         break;
      default:
         WARNING_message("Mode not supported.Using Default.");
         TT_whereami_mode = CLASSIC_WAMI_ATLAS_SORT;
         lsep =  '\n';
         break;
   }

   return;
}


char * TT_whereami_default_spc_name (void)
{
   char *eee = getenv("AFNI_DEFAULT_STD_SPACE");
   if (eee) {
      if (strncasecmp(eee, "TLRC", 4) == 0) {
         return (eee);
      } else if (strncasecmp(eee, "MNI_ANAT", 8) == 0) {
         return (eee);
      } else if (strncasecmp(eee, "MNI", 3) == 0) {
         return (eee);
      } else {
         WARNING_message(  "Bad value for AFNI_DEFAULT_STD_SPACE\n"
                           "%s is unrecognized. Assuming TLRC\n", eee);
         return ("TLRC");
      }
   } else {
      /* no env, return default */
      return("TLRC");
   }
}

int XYZ_to_AtlasCoord(float x, float y, float z, char *orcode, 
                              char *spacename, ATLAS_COORD *ac) 
{
   if (!ac) return(0);
   
   ac->x = x;
   ac->y = y;
   ac->z = z;
   if (orcode) { 
      ac->orcode[0] = orcode[0]; 
      ac->orcode[1] = orcode[1];
      ac->orcode[2] = orcode[2];
      ac->orcode[3] = '\0';
   } else {
      strncpy(ac->orcode,"RAI",3);
   }

   if (spacename && spacename[0] != '\0') {
      set_Coord_Space_Name(ac, spacename);
   } else {
      set_Coord_Space_Name(ac, "TT_Daemon");
   }

   return(1);
}
 
static int atlas_list_version = 1;  /* 1 --> Old style, hard coded atlases and
                                             transforms.
                                       2 --> Use .niml files defining atlases
                                             and transforms */
static int whereami_version = 1;    /* 1 --> Uses mid vintage whereami_9yards 
                                             function.
                                       2 --> Uses whereami_3rdbase
                                    */

char *find_atlas_niml_file() {
   
}

void set_TT_whereami_version(int atlas_version, int wami_version) {
   if (atlas_version > 0 && wami_version > 0) {
      atlas_list_version = atlas_version;
      whereami_version = wami_version;
   } else {
      char *wamifile = find_atlas_niml_file();
      if (wamifile) {
         atlas_list_version = 2;
         whereami_version = 2;
         free(wamifile);
      }
   }
}

/*! Load the atlas information from existing NIML files. 
    Revert to old approach if none were found */
int init_global_atlas_list () {
   ATLAS_LIST *atlas_alist = NULL;
   int  iatlas;
   AFNI_ATLAS_CODES TT_whereami_atlas_list[NUMBER_OF_ATLASES] = 
      {  AFNI_TLRC_ATLAS, CA_EZ_N27_MPM_ATLAS, CA_EZ_N27_ML_ATLAS, 
         CA_EZ_N27_PMAPS_ATLAS, CA_EZ_N27_LR_ATLAS, CUSTOM_ATLAS };
   int i;
   int LocalHead = wami_lh();
   
   ENTRY("init_global_atlas_list");
   
   
   if (global_atlas_alist) {
      if (wami_verb()) INFO_message("global_atlas_list initialized already");
      RETURN(1);
   }
   if (atlas_list_version > 1) {
      if (wami_verb()) INFO_message("Forming list from niml files");
      if (!init_global_atlas_from_niml_files()) {
         if (wami_verb())  INFO_message("No niml files");
      }
   } else {
      if (wami_verb()) {
         INFO_message("Forming list the old route");
      }
   }
   
   if (!global_atlas_alist) { /* try again */
      if (LocalHead) INFO_message("Going old route");
      atlas_alist = (ATLAS_LIST *) calloc(1,sizeof(ATLAS_LIST));
      if(!atlas_alist){
         ERROR_message("Could not initialize atlas list");
         RETURN(0);
      }
      if (LocalHead) INFO_message("Allocating atlas table");

      atlas_alist->natlases = NUMBER_OF_ATLASES;
      atlas_alist->atlas = (ATLAS *) calloc(
                              atlas_alist->natlases, sizeof(ATLAS));
      for(i=0;i<atlas_alist->natlases;i++) {
         atlas_alist->atlas[i].atlas_dset_name = NULL;
         atlas_alist->atlas[i].atlas_space = NULL;
         atlas_alist->atlas[i].atlas_name = NULL;
         atlas_alist->atlas[i].atlas_comment = NULL;
         atlas_alist->atlas[i].atlas_description = NULL;
         atlas_alist->atlas[i].adh = NULL;
      }

      if (LocalHead) INFO_message("Using old way to fill atlas table");
      for (iatlas=0; iatlas<atlas_alist->natlases; ++iatlas) {
         atlas_alist->atlas[iatlas].atlas_dset_name = nifti_strdup(
            Atlas_Code_to_Atlas_Dset_Name(TT_whereami_atlas_list[iatlas]));
         atlas_alist->atlas[iatlas].atlas_name = nifti_strdup(
            Atlas_Code_to_Atlas_Name(TT_whereami_atlas_list[iatlas]));
         atlas_alist->atlas[iatlas].atlas_description = nifti_strdup(
            Atlas_Code_to_Atlas_Description(TT_whereami_atlas_list[iatlas]));
         atlas_alist->atlas[iatlas].atlas_space = nifti_strdup("TLRC");
         if (LocalHead) INFO_message("adding atlas %s",
              atlas_alist->atlas[iatlas].atlas_dset_name);
      }
      global_atlas_alist = atlas_alist; 
   }
   if (!global_atlas_spaces) {
      ATLAS_SPACE_LIST *space_list=NULL;
      space_list = (ATLAS_SPACE_LIST *)calloc(1,sizeof(ATLAS_SPACE_LIST));
      if(!space_list){
         ERROR_message("Could not initialize space list");
         RETURN(0);
      }
      if (LocalHead) INFO_message("Using old way to fill space table");
      space_list->nspaces = 3;
      space_list->space = 
             (ATLAS_SPACE *) calloc(space_list->nspaces, sizeof(ATLAS_SPACE));
      for(i=0; i<space_list->nspaces; ++i) {
         space_list->space[i].atlas_space = "TLRC";
         space_list->space[i].generic_space = "MNI";
         space_list->space[i].generic_space = "MNI_ANAT";
      }
      global_atlas_spaces = space_list; 
   }
   
   if (wami_verb()) {
      INFO_message(
         "Daniel: Do we also need to initialize-the old way-"
         "global_atlas_templates, and more importantly, global_atlas_xfl ?");
   }
   
   if (global_atlas_alist && global_atlas_spaces) RETURN(1);
   else RETURN(0);
}

/* read various NIML files for atlas information*/
int init_global_atlas_from_niml_files()
{
   NI_stream space_niml;
   int valid_space_niml;
   char *ept = NULL;
   char suppfilestr[261];
   
   if(wami_verb() > 1) 
      INFO_message("opening AFNI_atlas_spaces.niml");   

   space_niml = NI_stream_open("file:AFNI_atlas_spaces.niml","r");

   if(space_niml==NULL){
      if (wami_verb()) 
         WARNING_message("Could not open global AFNI_atlas_spaces.niml\n");
      return(0);
   }

   if(wami_verb() > 1) 
      INFO_message("\nInitializing structures\n"); 
   if(!init_space_structs(&global_atlas_xfl, &global_atlas_alist,
                          &global_atlas_spaces, &global_atlas_templates)) {
      ERROR_message("Could not initialize structures for template spaces");
      return(0);
   }
   
   /* read atlas info from global atlas file */
   valid_space_niml = read_space_niml(space_niml, global_atlas_xfl,
          global_atlas_alist, global_atlas_spaces, global_atlas_templates);
   ept = my_getenv("AFNI_SUPP_ATLAS");
   if( ept ) {
      sprintf(suppfilestr, "file:%s", ept);
      if(wami_verb() > 1) 
         INFO_message("opening AFNI_supp_atlas_space.niml");   
      space_niml = NI_stream_open(suppfilestr,"r");
      if(space_niml==NULL){
            fprintf(stderr, "\nCould not open supplemental atlas niml file\n");
            return(0);
      }
      /* read atlas info from supplemental atlas file */
      /*  adding to existing structures */
      valid_space_niml = read_space_niml(space_niml, global_atlas_xfl,
             global_atlas_alist, global_atlas_spaces, global_atlas_templates);

   }


   /* read atlas info from local atlas file */
   ept = my_getenv("AFNI_LOCAL_ATLAS");
   if( ept ) {
      sprintf(suppfilestr, "file:%s", ept);
      if(wami_verb() > 1) 
         INFO_message("opening AFNI_local_atlas_space.niml");   
      space_niml = NI_stream_open(suppfilestr,"r");
      if(space_niml==NULL){
            fprintf(stderr, "\nCould not open supplemental atlas niml file\n");
            return(0);
      }
      /* read atlas info from local atlas file */
      /*  adding to existing structures */
      valid_space_niml = read_space_niml(space_niml, global_atlas_xfl,
             global_atlas_alist, global_atlas_spaces, global_atlas_templates);
   }

  
   /* set up the neighborhood for spaces */
   /*  how are the spaces related to each other */ 
   if(make_space_neighborhood(global_atlas_spaces, global_atlas_xfl)!=0) {
     return(0);
   }
   
   /* all ok */
   return(1);
}

/* free all the static lists */
void free_global_atlas_structs()
{
   free_xform_list(global_atlas_xfl);
   free_atlas_list(global_atlas_alist);
   free_space_list(global_atlas_spaces);
   free_template_list(global_atlas_templates);
}


char * TT_whereami(  float xx , float yy , float zz, 
                     char *space_name, void *alist)
{
   ATLAS_COORD ac;
   ATLAS_QUERY *wami = NULL;
   char *rbuf = NULL, *strg = NULL ;
   int k = 1, i;
   static int first_time = 1;
   ATLAS_LIST *atlas_alist=(ATLAS_LIST *)alist;
   ATLAS_SPACE_LIST *asl=get_G_space_list();
   int LocalHead = wami_lh();

   ENTRY("TT_whereami") ;

   /* initialize the single custom atlas entry */
   init_custom_atlas();

   if (!atlas_alist) { /* get global */
      atlas_alist = get_G_atlas_list();
   }
   if (!atlas_alist) {
      ERROR_message("No list to work with");
      RETURN(rbuf) ;
   }

   if (!asl){
      ERROR_message("No spaces defined");
      RETURN(rbuf) ;
   }
   
   if (!is_known_coord_space(space_name)) {
      ERROR_message("Unknown space_name %s", space_name);
      RETURN(rbuf) ;
   }
   
   /* build coord structure */
   if (!XYZ_to_AtlasCoord( xx, yy, zz, "RAI", 
                           space_name, &ac)) {
      ERROR_message("Failed to get AtlasCoord");
      RETURN(rbuf);
   }

   if(LocalHead) {
      INFO_message("current list of atlases:");
      print_atlas_list(atlas_alist);
      print_atlas_coord(ac);
   }
   
/* ***tbd whereami_9yards converts coordinates among spaces and
      looks up coordinates through radius among atlases in atlas list */
/* will need to change this to use new atlas_list from NIML database */ 
/* modify whereami_9yards to use xform list */
   if (whereami_version == 1) {
      if (wami_verb() > 1) INFO_message("whereami_9yards");
      if (!whereami_9yards( ac, &wami, atlas_alist)) {
         INFO_message("whereami_9yards returned error");
      }
   } else {
      if (wami_verb() > 1) INFO_message("whereami_3rdBase");
      if (!whereami_3rdBase( ac, &wami, NULL, atlas_alist)) {
         INFO_message("whereami_3rdBase returned error");
      }
   }

   if (!wami) {
      ERROR_message("No atlas regions found.");
      RETURN(rbuf) ;
   }

   /* Now form the string */
   if (atlas_list_version == 1) {
      if (wami_verb() > 1) INFO_message("Atlas_Query_to_String"); 
      rbuf =  Atlas_Query_to_String (wami, ac, TT_whereami_mode, 
                                          atlas_alist);
   } else {
      if (wami_verb() > 1) INFO_message("genx_Atlas_Query_to_String"); 
      rbuf =  genx_Atlas_Query_to_String (wami, ac, TT_whereami_mode, 
                                          atlas_alist);
   }
   
   /*cleanup*/
   if (wami)  wami = Free_Atlas_Query(wami);

   RETURN(rbuf) ;
}

#ifdef KILLTHIS /* Remove all old sections framed by #ifdef KILLTHIS
                  in the near future.  ZSS May 2011   */ 

char * TT_whereami_old( float xx , float yy , float zz ) /* ZSS */
{
   int ii,kk , ix,jy,kz , nx,ny,nz,nxy , aa,bb,cc , ff,b2f,b4f,rff ;
   THD_ivec3 ijk ;
   byte *b2 , *b4 ;
   THD_string_array *sar ;
   char *b2lab , *b4lab ;
   char lbuf[256] , *rbuf ;
   int nfind, *b2_find=NULL, *b4_find=NULL, *rr_find=NULL ;
   THD_3dim_dataset * dset ; /* 01 Aug 2001 */
   ATLAS_POINT_LIST *apl=NULL;
   
ENTRY("TT_whereami_old") ;

   apl = atlas_point_list("TT_Daemon");

   if (MAX_FIND < 0) {
      Set_Whereami_Max_Find(MAX_FIND);
   }
   b2_find = (int*)calloc(MAX_FIND, sizeof(int));
   b4_find = (int*)calloc(MAX_FIND, sizeof(int));
   rr_find = (int*)calloc(MAX_FIND, sizeof(int));
   if (!b2_find || !b4_find || !rr_find) {
      ERROR_message( "Jiminy Crickets!\n"
                     "Failed to allocate for finds!\nMAX_FIND = %d\n", MAX_FIND);
      RETURN(NULL);
   }


   /*-- setup stuff: load atlas dataset, prepare search mask --*/

   if( dseTT_old == NULL ){
      ii = TT_load_atlas_old() ; if( ii == 0 ) RETURN(NULL) ;
   }

   /* 01 Aug 2001: maybe use big dataset (so don't need both in memory) */

   dset = (dseTT_big_old != NULL) ? dseTT_big_old : dseTT_old ;

#if 0
if( dset == dseTT_big_old ) fprintf(stderr,"TT_whereami using dseTT_big\n") ;
else                    fprintf(stderr,"TT_whereami using dseTT\n") ;
#endif

   DSET_load(dset) ;
   b2 = DSET_BRICK_ARRAY(dset,0) ; if( b2 == NULL ) RETURN(NULL) ;
   b4 = DSET_BRICK_ARRAY(dset,1) ; if( b4 == NULL ) RETURN(NULL) ;

   if (WAMIRAD < 0.0) {
      WAMIRAD = Init_Whereami_Max_Rad();
   }
   if( wamiclust == NULL ){
      wamiclust = MCW_build_mask( 1.0,1.0,1.0 , WAMIRAD ) ;
      if( wamiclust == NULL ) RETURN(NULL) ;  /* should not happen! */

      for( ii=0 ; ii < wamiclust->num_pt ; ii++ )       /* load radius */
         wamiclust->mag[ii] = (int)rint(sqrt((double)(
                                         wamiclust->i[ii]*wamiclust->i[ii]
                                        +wamiclust->j[ii]*wamiclust->j[ii]
                                        +wamiclust->k[ii]*wamiclust->k[ii]))) ;

      MCW_sort_cluster( wamiclust ) ;  /* sort by radius */
   }

   /*-- find locations near the given one that are in the Atlas --*/

   ijk = THD_3dmm_to_3dind( dset , TEMP_FVEC3(xx,yy,zz) ) ;  /* get indexes */
   UNLOAD_IVEC3(ijk,ix,jy,kz) ;                               /* from coords */

   nx = DSET_NX(dset) ;               /* size of TT atlas dataset axes */
   ny = DSET_NY(dset) ;
   nz = DSET_NZ(dset) ; nxy = nx*ny ;

   nfind = 0 ;

   /*-- check the exact input location --*/

   kk = ix + jy*nx + kz*nxy ;        /* index into brick arrays */
   if( b2[kk] != 0 || b4[kk] != 0 ){
      b2_find[0] = b2[kk] ;
      b4_find[0] = b4[kk] ;
      rr_find[0] = 0      ; nfind++ ;
   }

   /*-- check locations near it --*/

   for( ii=0 ; ii < wamiclust->num_pt ; ii++ ){

      /* compute index of nearby location, skipping if outside atlas */

      aa = ix + wamiclust->i[ii] ; if( aa < 0 || aa >= nx ) continue ;
      bb = jy + wamiclust->j[ii] ; if( bb < 0 || bb >= ny ) continue ;
      cc = kz + wamiclust->k[ii] ; if( cc < 0 || cc >= nz ) continue ;

      kk  = aa + bb*nx + cc*nxy ;   /* index into bricks */
      b2f = b2[kk] ; b4f = b4[kk] ; /* TT structures markers there */

      if( b2f == 0 && b4f == 0 )                            continue ;

      for( ff=0 ; ff < nfind ; ff++ ){       /* cast out         */
         if( b2f == b2_find[ff] ) b2f = 0 ;  /* duplicate labels */
         if( b4f == b4_find[ff] ) b4f = 0 ;  /* we already found */
      }
      if( b2f == 0 && b4f == 0 )                            continue ;

      b2_find[nfind] = b2f ;  /* save what we found */
      b4_find[nfind] = b4f ;
      rr_find[nfind] = (int) wamiclust->mag[ii] ;
      nfind++ ;

      if( nfind == MAX_FIND ) break ;  /* don't find TOO much */
   }

   /*-- assemble output string(s) --*/

   if( nfind == 0 ){
      char xlab[24], ylab[24] , zlab[24] ;
      THD_fvec3 tv , mv ;
      float mx,my,mz ;
      char mxlab[24], mylab[24] , mzlab[24] ;

      sprintf(xlab,"%4.0f mm [%c]",-xx,(xx<0.0)?'R':'L') ;
      sprintf(ylab,"%4.0f mm [%c]",-yy,(yy<0.0)?'A':'P') ;
      sprintf(zlab,"%4.0f mm [%c]", zz,(zz<0.0)?'I':'S') ;

      LOAD_FVEC3(tv,xx,yy,zz);
      mv = THD_tta_to_mni(tv); UNLOAD_FVEC3(mv,mx,my,mz);
      sprintf(mxlab,"%4.0f mm [%c]",mx,(mx>=0.0)?'R':'L') ;
      sprintf(mylab,"%4.0f mm [%c]",my,(my>=0.0)?'A':'P') ;
      sprintf(mzlab,"%4.0f mm [%c]",mz,(mz< 0.0)?'I':'S') ;

      rbuf = AFMALL(char, 500) ;
      sprintf(rbuf,"%s\n"
                   "Focus point=%s,%s,%s {T-T Atlas}\n"
                   "           =%s,%s,%s {MNI Brain}\n"
                   "\n"
                   "***** Not near any region stored in database *****\n" ,
              WAMI_HEAD , xlab,ylab,zlab , mxlab,mylab,mzlab ) ;
      RETURN(rbuf) ;
   }

   /*-- bubble-sort what we found, by radius --*/

   if( nfind > 1 ){  /* don't have to sort only 1 result */
     int swap, tmp ;
     do{
        swap=0 ;
        for( ii=1 ; ii < nfind ; ii++ ){
           if( rr_find[ii-1] > rr_find[ii] ){
             tmp = rr_find[ii-1]; rr_find[ii-1] = rr_find[ii]; rr_find[ii] = tmp;
             tmp = b2_find[ii-1]; b2_find[ii-1] = b2_find[ii]; b2_find[ii] = tmp;
             tmp = b4_find[ii-1]; b4_find[ii-1] = b4_find[ii]; b4_find[ii] = tmp;
             swap++ ;
           }
        }
     } while(swap) ;
   }

   /*-- find anatomical label for each found marker, make result string --*/

   INIT_SARR(sar) ; ADDTO_SARR(sar,WAMI_HEAD) ;

   /* 04 Apr 2002: print coordinates (LPI) as well (the HH-PB addition) */

   { char lbuf[128], xlab[24], ylab[24] , zlab[24] ;
     sprintf(xlab,"%4.0f mm [%c]",-xx,(xx<0.0)?'R':'L') ;
     sprintf(ylab,"%4.0f mm [%c]",-yy,(yy<0.0)?'A':'P') ;
     sprintf(zlab,"%4.0f mm [%c]", zz,(zz<0.0)?'I':'S') ;
     sprintf(lbuf,"Focus point=%s,%s,%s {T-T Atlas}",xlab,ylab,zlab) ;
     ADDTO_SARR(sar,lbuf) ;
   }

   /* 29 Apr 2002: print MNI coords as well */

   { THD_fvec3 tv , mv ;
     float mx,my,mz ;
     char mxlab[24], mylab[24] , mzlab[24] , lbuf[128] ;
     LOAD_FVEC3(tv,xx,yy,zz);
     mv = THD_tta_to_mni(tv); UNLOAD_FVEC3(mv,mx,my,mz);
     sprintf(mxlab,"%4.0f mm [%c]",mx,(mx>=0.0)?'R':'L') ;
     sprintf(mylab,"%4.0f mm [%c]",my,(my>=0.0)?'A':'P') ;
     sprintf(mzlab,"%4.0f mm [%c]",mz,(mz< 0.0)?'I':'S') ;
     sprintf(lbuf,"Focus point=%s,%s,%s {MNI Brain}\n",mxlab,mylab,mzlab) ;
     ADDTO_SARR(sar,lbuf) ;
   }

   rff = -1 ;  /* rff = radius of last found label */

   for( ff=0 ; ff < nfind ; ff++ ){
      b2f = b2_find[ff] ; b4f = b4_find[ff] ; b2lab = NULL ; b4lab = NULL ;

      if( b2f != 0 ){                               /* find label     */
         for( ii=0 ; ii < apl->n_points ; ii++ )        /* in AFNI's list */
            if( b2f == apl->at_point[ii].tdval ) break ;
         if( ii < apl->n_points )                       /* always true? */
            b2lab = apl->at_point[ii].name ;

         if( b2lab != NULL && xx < 0 && strstr(b2lab,"Left") != NULL ) /* maybe is Right */
            b2lab = apl->at_point[ii+1].name ;
      }

      if( b4f != 0 ){
         for( ii=0 ; ii < apl->n_points ; ii++ )
            if( b4f == apl->at_point[ii].tdval ) break ;
         if( ii < apl->n_points )
            b4lab = apl->at_point[ii].name ;
         if( b4lab != NULL && xx < 0 && strstr(b4lab,"Left") != NULL )
            b4lab = apl->at_point[ii+1].name ;
      }

      if( b2lab == NULL && b4lab == NULL ) continue ;  /* no labels? */

      /* make output label into lbuf */

      lbuf[0] = '\0' ;
      if( b2lab != NULL ){
         if( rr_find[ff] != rff ){
            if( rr_find[ff] > 0 )
              sprintf( lbuf , "Within %d mm: %s" , rr_find[ff] , b2lab ) ;
            else
              sprintf( lbuf , "Focus point: %s" , b2lab ) ;
         } else {
            sprintf( lbuf , "             %s" , b2lab ) ;
         }

         for( kk=strlen(lbuf)-1 ; kk > 0 && lbuf[kk] == '.' ; kk-- )
            lbuf[kk] = '\0' ;                  /* trim trailing .'s */
      }

      if( b4lab != NULL ){
         kk = strlen(lbuf) ;
         if( kk > 0 ){
            sprintf( lbuf+kk , " -AND- %s" , b4lab ) ;
         } else if( rr_find[ff] != rff ){
            if( rr_find[ff] > 0 )
              sprintf( lbuf , "Within %d mm: %s" , rr_find[ff] , b4lab ) ;
            else
              sprintf( lbuf , "Focus point: %s" , b4lab ) ;
         } else {
            sprintf( lbuf , "             %s" , b4lab ) ;
         }

         for( kk=strlen(lbuf)-1 ; kk > 0 && lbuf[kk] == '.' ; kk-- )
            lbuf[kk] = '\0' ;
      }

      ADDTO_SARR(sar,lbuf) ;  /* make a list of labels */

      rff = rr_find[ff] ;  /* save for next time around */
   }

   /*- if didn't make any label, must produce something -*/

   if( sar->num == 1 ){    /* shouldn't ever happen */
      sprintf(lbuf,"Found %d marked but unlabeled regions???\n",nfind) ;
      ADDTO_SARR(sar,lbuf) ;
   } else if( !AFNI_noenv("AFNI_TTATLAS_CAUTION") ){
      ADDTO_SARR(sar,WAMI_TAIL) ;  /* cautionary tail */
   }

   /*- convert list of labels into one big multi-line string -*/

   for( nfind=ii=0 ; ii < sar->num ; ii++ ) nfind += strlen(sar->ar[ii]) ;
   rbuf = AFMALL(char, nfind + 2*sar->num + 32 ) ; rbuf[0] = '\0' ;
   for( ii=0 ; ii < sar->num ; ii++ ){
      strcat(rbuf,sar->ar[ii]) ; strcat(rbuf,"\n") ;
   }

   free(b2_find); b2_find = NULL; free(b4_find); b4_find = NULL; free(rr_find); rr_find = NULL;

   DESTROY_SARR(sar) ; RETURN(rbuf) ;
}

#endif

/* Begin ZSS: Additions for Eickhoff and Zilles Cytoarchitectonic maps */



/*!
    l left
    u unknown
    r right
*/
char Is_Side_Label(char *str, char *opt)
{
   int k, nc;
   char *strd=NULL;
   ENTRY("atlas_label_side");

   if (!str) RETURN('u');

   strd = strdup(str);
   nc = strlen(strd);
   for (k=0; k<nc; ++k) strd[k] = TO_LOWER(strd[k]);

   if (strncmp(strd,"left", 4) == 0) RETURN('l');
   else if (strncmp(strd,"right", 5) == 0) RETURN('r');

   free(strd); strd = NULL;
   RETURN('u');
}

/* inverse sort ints */
int *z_idqsort (int *x , int nx )
{/*z_idqsort*/
/*   static char FuncName[]={"z_idqsort"};*/
   int *I, k;
   Z_QSORT_INT *Z_Q_fStrct;

   ENTRY("z_idqsort");

   /* allocate for the structure */
   Z_Q_fStrct = (Z_QSORT_INT *) calloc(nx, sizeof (Z_QSORT_INT));
   I = (int *) calloc (nx, sizeof(int));

   if (!Z_Q_fStrct || !I)
      {
         ERROR_message("Allocation problem");
         RETURN (NULL);
      }

   for (k=0; k < nx; ++k) /* copy the data into a structure */
      {
         Z_Q_fStrct[k].x = x[k];
         Z_Q_fStrct[k].Index = k;
      }

   /* sort the structure by it's field value */
   qsort(Z_Q_fStrct, nx, sizeof(Z_QSORT_INT), (int(*) (const void *, const void *)) compare_Z_IQSORT_INT);

   /* recover the index table */
   for (k=0; k < nx; ++k) /* copy the data into a structure */
      {
         x[k] = Z_Q_fStrct[k].x;
         I[k] = Z_Q_fStrct[k].Index;
      }

   /* free the structure */
   free(Z_Q_fStrct);

   /* return */
   RETURN (I);


}/*z_idqsort*/

/*
  give a shuffled series between bot and top inclusive

*/
int *z_rand_order(int bot, int top, long int seed) {
   int i, *s=NULL, n;
   float *num=NULL;

   ENTRY("z_rand_order");
   if (!seed) seed = (long)time(NULL)+(long)getpid();
   srand48(seed);

   if (bot > top) { i = bot; bot = top; top = i; }
   n = top-bot+1;

   if (!(num = (float*)calloc(n , sizeof(float)))) {
      fprintf(stderr,"Failed to allocate for %d floats.\n", n);
      RETURN(s);
   }
   for (i=0;i<n;++i) num[i] = (float)drand48();

   if (!(s = z_iqsort(num, n))) {
      fprintf(stderr,"Failed to sort %d floats.\n", n);
      RETURN(s);
   }
   free(num); num = NULL;

   /* offset numbers to get to bot */
   for (i=0;i<n;++i) {
      /* fprintf(stderr,"s[%d]=%d (bot=%d)\n", i, s[i], bot); */
      s[i] += bot;
   }
   RETURN(s);
}

/* inverse sort floats */
int *z_iqsort (float *x , int nx )
{/*z_iqsort*/
/*   static char FuncName[]={"z_iqsort"};*/
   int *I, k;
   Z_QSORT_FLOAT *Z_Q_fStrct;

   ENTRY("z_iqsort");

   /* allocate for the structure */
   Z_Q_fStrct = (Z_QSORT_FLOAT *) calloc(nx, sizeof (Z_QSORT_FLOAT));
   I = (int *) calloc (nx, sizeof(int));

   if (!Z_Q_fStrct || !I)
      {
         ERROR_message("Allocation problem");
         RETURN (NULL);
      }

   for (k=0; k < nx; ++k) /* copy the data into a structure */
      {
         Z_Q_fStrct[k].x = x[k];
         Z_Q_fStrct[k].Index = k;
      }

   /* sort the structure by it's field value */
   qsort(Z_Q_fStrct, nx, sizeof(Z_QSORT_FLOAT), 
         (int(*) (const void *, const void *)) compare_Z_IQSORT_FLOAT);

   /* recover the index table */
   for (k=0; k < nx; ++k) /* copy the data into a structure */
      {
         x[k] = Z_Q_fStrct[k].x;
         I[k] = Z_Q_fStrct[k].Index;
      }

   /* free the structure */
   free(Z_Q_fStrct);

   /* return */
   RETURN (I);


}/*z_iqsort*/

/* inverse sort doubles */
int *z_idoubleqsort (double *x , int nx )
{/*z_idoubleqsort*/
   static char FuncName[]={"z_idoubleqsort"};
   int *I, k;
   Z_QSORT_DOUBLE *Z_Q_doubleStrct;

   ENTRY("z_idoubleqsort");

   /* allocate for the structure */
   Z_Q_doubleStrct = (Z_QSORT_DOUBLE *) calloc(nx, sizeof (Z_QSORT_DOUBLE));
   I = (int *) calloc (nx, sizeof(int));

   if (!Z_Q_doubleStrct || !I)
      {
         ERROR_message("Allocation problem");
         RETURN (NULL);
      }

   for (k=0; k < nx; ++k) /* copy the data into a structure */
      {
         Z_Q_doubleStrct[k].x = x[k];
         Z_Q_doubleStrct[k].Index = k;
      }

   /* sort the structure by it's field value */
   qsort(Z_Q_doubleStrct, nx, sizeof(Z_QSORT_DOUBLE), 
         (int(*) (const void *, const void *)) compare_Z_IQSORT_DOUBLE);

   /* recover the index table */
   for (k=0; k < nx; ++k) /* copy the data into a structure */
      {
         x[k] = Z_Q_doubleStrct[k].x;
         I[k] = Z_Q_doubleStrct[k].Index;
      }

   /* free the structure */
   free(Z_Q_doubleStrct);

   /* return */
   RETURN (I);


}/*z_idoubleqsort*/

/* return the unique values in y
   y : Vector of values
   ysz: Number of elements in y
   kunq: (int *) pointer to number of unique values
   Sorted: (1 means values in y are sorted)
*/
int * UniqueInt (int *y, int ysz, int *kunq, int Sorted )
{/*UniqueInt*/
   int  *xunq, *x;
   int k;
   static char FuncName[]={"UniqueInt"};

   ENTRY("UniqueInt");
   *kunq = 0;

   if (!ysz) {
      RETURN(NULL);
   }
   if (!Sorted)
    {/* must sort y , put in a new location so that y is not disturbed*/
      x = (int *)calloc(ysz, sizeof(int));
      if (!x)
         {
            fprintf (stderr,"Error %s: Failed to allocate for x.", FuncName);
            RETURN (NULL);
         }
      for (k=0; k < ysz; ++k)
         x[k] = y[k];
      qsort(x,ysz,sizeof(int), 
            (int(*) (const void *, const void *)) compare_int);
   }
   else
      x = y;

   xunq = (int *) calloc(ysz,sizeof(int));
   if (xunq == NULL)
    {
      fprintf (stderr,"Error %s: Could not allocate memory", FuncName);
      RETURN (NULL);
   }

   *kunq = 0;
   xunq[0] = x[0];
   for (k=1;k<ysz;++k)
    {
      if ((x[k] != x[k - 1]))
         {
            ++*kunq;
            xunq[*kunq] = x[k];
         }
   }
   ++*kunq;


   /* get rid of extra space allocated */
   xunq = (int *) realloc(xunq, *kunq*sizeof(int));

   if (!Sorted)
      free (x);

   RETURN (xunq);
}/*UniqueInt*/

/* return the unique values in y
   y : Vector of values
   ysz: Number of elements in y
   kunq: (int *) pointer to number of unique values
   Sorted: (1 means values in y are sorted)
*/
short * UniqueShort (short *y, int ysz, int *kunq, int Sorted )
{/*UniqueShort*/
   short  *xunq, *x;
   int k;
   static char FuncName[]={"UniqueShort"};

   ENTRY("UniqueShort");
   *kunq = 0;

   if (!ysz) {
      RETURN(NULL);
   }
   if (!Sorted)
    {/* must sort y , put in a new location so that y is not disturbed*/
      x = (short *)calloc(ysz, sizeof(short));
      if (!x)
         {
            fprintf (stderr,"Error %s: Failed to allocate for x.", FuncName);
            RETURN (NULL);
         }
      for (k=0; k < ysz; ++k)
         x[k] = y[k];
      qsort(x,ysz,sizeof(short), (int(*) (const void *, const void *)) compare_short);
   }
   else
      x = y;

   xunq = (short *) calloc(ysz,sizeof(short));
   if (xunq == NULL)
    {
      fprintf (stderr,"Error %s: Could not allocate memory", FuncName);
      RETURN (NULL);
   }

   *kunq = 0;
   xunq[0] = x[0];
   for (k=1;k<ysz;++k)
    {
      if ((x[k] != x[k - 1]))
         {
            ++*kunq;
            xunq[*kunq] = x[k];
         }
   }
   ++*kunq;


   /* get rid of extra space allocated */
   xunq = (short *) realloc(xunq, *kunq*sizeof(short));

   if (!Sorted)
      free (x);

   RETURN (xunq);
}/*UniqueShort*/

/* return the unique values in y
   y : Vector of values
   ysz: Number of elements in y
   kunq: (int *) pointer to number of unique values
   Sorted: (1 means values in y are sorted)
*/
byte * UniqueByte (byte *y, int ysz, int *kunq, int Sorted )
{/*UniqueByte*/
   byte  *xunq, *x;
   int k;
   static char FuncName[]={"UniqueByte"};

   ENTRY("UniqueByte");
   *kunq = 0;

   if (!ysz) {
      RETURN(NULL);
   }

   if (!Sorted)
    {/* must sort y , put in a new location so that y is not disturbed*/
      x = (byte *)calloc(ysz, sizeof(byte));
      if (!x)
         {
            fprintf (stderr,"Error %s: Failed to allocate for x.", FuncName);
            RETURN (NULL);
         }
      for (k=0; k < ysz; ++k)
         x[k] = y[k];
      qsort(x,ysz,sizeof(byte), (int(*) (const void *, const void *)) compare_char);
   }
   else
      x = y;

   xunq = (byte *) calloc(ysz,sizeof(byte));
   if (xunq == NULL)
    {
      fprintf (stderr,"Error %s: Could not allocate memory", FuncName);
      RETURN (NULL);
   }

   *kunq = 0;
   xunq[0] = x[0];
   for (k=1;k<ysz;++k)
    {
      if ((x[k] != x[k - 1]))
         {
            ++*kunq;
            xunq[*kunq] = x[k];
         }
   }
   ++*kunq;


   /* get rid of extra space allocated */
   xunq = (byte *) realloc(xunq, *kunq*sizeof(byte));

   if (!Sorted)
      free (x);

   RETURN (xunq);
}/*UniqueByte*/

static int AtlasShowMode = 0; /* 0 = nice mode, 1 = debug mode  */
void Set_Show_Atlas_Mode(int md)
{
   AtlasShowMode = md;
   return;
}
void Show_Atlas_Region (AFNI_ATLAS_REGION *aar)
{
   int k = 0;

   ENTRY("Show_Atlas_Region") ;

   if (!aar) {
      WARNING_message("NULL atlas region structure");
      EXRETURN;
   }

   if (AtlasShowMode) {
      fprintf(stdout,""
                     "Atlas_name: %s\n"
                     "Side      : %c\n"
                     "orig_label: %s\n"
                     "id        : %d\n"
                     "N_chnks     : %d\n",
                     STR_PRINT(aar->atlas_name), aar->side, 
                     STR_PRINT(aar->orig_label), aar->id, aar->N_chnks);
      for (k=0; k<aar->N_chnks; ++k) {
         fprintf(stdout,"aar->chnks[%d] = %s\n", k, STR_PRINT(aar->chnks[k]));
      }
      fprintf(stdout,"\n");
   } else {
      fprintf(stdout,"%c:%s:%-3d\n",
                     aar->side, STR_PRINT(aar->orig_label), aar->id);
   }

   EXRETURN;
}


AFNI_ATLAS_REGION * Free_Atlas_Region (AFNI_ATLAS_REGION *aar)
{
   int k = 0;

   ENTRY("Free_Atlas_Region");

   if (!aar) {
      WARNING_message("NULL aar");
      RETURN(NULL);
   }

   if (aar->chnks) {
      for (k=0; k<aar->N_chnks; ++k) {
         if (aar->chnks[k]) free(aar->chnks[k]);
      }
      free(aar->chnks);
   }

   if (aar->orig_label) free(aar->orig_label);
   if (aar->atlas_name) free(aar->atlas_name);
   free(aar);

   RETURN(NULL);
}

byte Same_Chunks(AFNI_ATLAS_REGION *aar1, AFNI_ATLAS_REGION *aar2)
{
   int i;

   ENTRY("Same_Chunks");
   if (!aar1 || !aar2) RETURN(0);
   if (aar1->N_chnks != aar2->N_chnks) RETURN(0);
   for (i=0; i<aar1->N_chnks; ++i) {
      if (strcmp(aar1->chnks[i], aar2->chnks[i])) RETURN(0);
   }
   RETURN(1);
}

/*!
   given a string, chunk it
*/
AFNI_ATLAS_REGION * Atlas_Chunk_Label(char *lbli, int id, char *atlas_name)
{
   AFNI_ATLAS_REGION *aar = NULL;
   char lachunk[500], sd, *lbl = NULL;
   int ic = 0, nc = 0, k = 0, block = 0, isnum=0;
   int LocalHead = wami_lh();

   ENTRY("Atlas_Chunk_Label") ;
   if (!lbli) {
      ERROR_message("NULL label");
      RETURN(aar) ;
   }

   nc = strlen(lbli);

   if (lbli[0] == '\0') {
      ERROR_message("Empty label");
      RETURN(aar) ;
   }

   lbl = strdup(lbli);

   aar = (AFNI_ATLAS_REGION*) calloc(1,sizeof(AFNI_ATLAS_REGION));
   aar->side = 'u';
   aar->orig_label = strdup(lbl);
   aar->atlas_name = NULL;
   if (atlas_name) aar->atlas_name = nifti_strdup(atlas_name); /* for clarity*/
   aar->id = id;
   aar->N_chnks = 0;
   aar->chnks = NULL;

   /* is this all numbers ? */
   isnum = 1;
   for (k=0; k<nc; ++k) {
      if (!IS_NUMBER(lbl[k])) isnum = 0;
   }
   /* it is an integer, stop the machines */
   if (isnum) {
      isnum = atoi(lbl);
      free(lbl); lbl = NULL;  /* no need no more */
      if (aar->id && isnum != aar->id) {
         ERROR_message("Information conflict!");
         RETURN(Free_Atlas_Region(aar)) ;
      }
      aar->id = isnum;
      if (LocalHead) fprintf(stderr,"Have number (%d), will travel\n", aar->id);
      RETURN(aar);
   }

   /* change any '.' surrounded by digits to a 0 */
   for (k=1; k<nc-1; ++k) {
      if (  lbl[k] == '.' && 
            IS_NUMBER(lbl[k+1]) && 
            IS_NUMBER(lbl[k-1]) ) lbl[k] = '0';
   }

   ic = 0;
   k = 0;
   block = 0;
   while (!IS_LETTER(lbl[k]) && !IS_NUMBER(lbl[k]) && k < nc) ++k;
   if (IS_LETTER(lbl[k])) block = 1;
   else if (IS_NUMBER(lbl[k])) block = 2;
   else block = 0;

   while (k < nc) {
      if (IS_LETTER(lbl[k]) && block == 1) {
         lachunk[ic] = TO_LOWER(lbl[k]); ++ic;
         ++k;
      } else if (IS_NUMBER(lbl[k]) && block == 2) {
         lachunk[ic] = TO_LOWER(lbl[k]); ++ic;
         ++k;
      } else {
         lachunk[ic] = '\0'; /* seal */
         if (LocalHead) fprintf(stderr,"Have chunk %s, will eat...\n", lachunk);
         sd = '\0';
         if (aar->N_chnks == 0) { /* check on side */
            sd = Is_Side_Label(lachunk, NULL);
            if (LocalHead) 
               fprintf(stderr,"Side check on %s returned %c\n", lachunk, sd);
            if (sd == 'l' || sd == 'r' || sd == 'b') {
               aar->side = sd;
            } else {
               /* unknown */
               sd = '\0';
            }
         }
         if (sd == '\0') { /* new, non left/right chunk */
            /* store lachunk, skip to next char or number */
            aar->chnks = (char **)realloc(aar->chnks, 
                                          sizeof(char*)*(aar->N_chnks+1));
            aar->chnks[aar->N_chnks] = strdup(lachunk);
            ++ aar->N_chnks;
         }
         ic = 0; lachunk[ic] = '\0'; /* seal */
         while (!IS_LETTER(lbl[k]) && !IS_NUMBER(lbl[k]) && k < nc) ++k;
         if (IS_LETTER(lbl[k])) block = 1;
         else if (IS_NUMBER(lbl[k])) block = 2;
         else block = 0;
      }
   }

   /* add last chunk */
   if (lachunk[0] != '\0') {
      lachunk[ic] = '\0';
      /* first check on side */
      sd = '\0';
      if (aar->N_chnks == 0) { /* check on side */
         sd = Is_Side_Label(lachunk, NULL);
         if (LocalHead) 
            fprintf(stderr,"Side check on %s returned %c\n", lachunk, sd);
         if (sd == 'l' || sd == 'r' || sd == 'b') {
            aar->side = sd;
         } else {
            /* unknown */
            sd = '\0';
         }
      }
      if (sd == '\0') { /* new, non left/right chunk */
         aar->chnks = (char **)realloc(aar->chnks, 
                                       sizeof(char*)*(aar->N_chnks+1));
         aar->chnks[aar->N_chnks] = strdup(lachunk);
         ++ aar->N_chnks;
      }
      ic = 0; lachunk[ic] = '\0'; /* seal */
   }

   if (LocalHead) {
      fprintf(stderr,"Atlas_Chunk_Label:\n" );
      Show_Atlas_Region (aar);
   }
   free(lbl); lbl = NULL;
   RETURN(aar) ;
}

AFNI_ATLAS *Build_Atlas (char *aname, ATLAS_LIST *atlas_list)
{
   AFNI_ATLAS *aa=NULL;
   int k = 0;
   int LocalHead = wami_lh();
   ATLAS *atlas=NULL;
   
   ENTRY("Build_Atlas") ;

   /* Load the dataset */
   if (LocalHead) fprintf(stderr,"Building AFNI ATLAS %s\n", aname);
   if (!(atlas = Atlas_With_Trimming(aname, 1, atlas_list))) {
      ERROR_message("Failed to get %s", aname);
      RETURN(NULL);
   }
   /* Call this function just to force TT_Daemon to end up in BIG format*/
   TT_retrieve_atlas_dset(aname, 1);
   
   if (LocalHead) fprintf(stderr,"%s loaded\n", aname);

   aa = (AFNI_ATLAS *)calloc(1,sizeof(AFNI_ATLAS));
   aa->atlas_name = strdup(atlas->atlas_name);
   aa->N_regions = MAX_ELM(atlas->adh->apl2);
   aa->reg = (AFNI_ATLAS_REGION **)
                  calloc(aa->N_regions, sizeof(AFNI_ATLAS_REGION *));
   for (k=0; k<aa->N_regions; ++k) {
      aa->reg[k] = Atlas_Chunk_Label(atlas->adh->apl2->at_point[k].name, 
                                     atlas->adh->apl2->at_point[k].tdval,
                                     Atlas_Name(atlas));
      /* Show_Atlas_Region (aa->reg[k]); */
   }

   RETURN(aa);
}

void Show_Atlas (AFNI_ATLAS *aa)
{
   int k = 0;

   ENTRY("Show_Atlas");

   if (!aa) {
      WARNING_message("NULL atlas");
      EXRETURN;
   }

   if (AtlasShowMode) {
      fprintf(stdout,"\n"
                     "Atlas     :%s\n"
                     "N_regions :%d\n"
                     "----------- Begin regions for %s atlas-----------\n"
                     , STR_PRINT(aa->atlas_name), aa->N_regions, STR_PRINT(aa->atlas_name));
      for (k=0; k<aa->N_regions; ++k) {
         fprintf(stdout,"%d%s region:\n", k, COUNTER_SUFFIX(k));
         Show_Atlas_Region(aa->reg[k]);
      }
      fprintf(stdout,"----------- End regions for %s atlas --------------\n\n", STR_PRINT(aa->atlas_name));
   } else {
      fprintf(stdout,"\n"
                     "Atlas %s,      %d regions\n"
                     "----------- Begin regions for %s atlas-----------\n"
                     , STR_PRINT(aa->atlas_name), aa->N_regions, STR_PRINT(aa->atlas_name));
      for (k=0; k<aa->N_regions; ++k) {
         Show_Atlas_Region(aa->reg[k]);
      }
      fprintf(stdout,"----------- End regions for %s atlas --------------\n\n", STR_PRINT(aa->atlas_name));
   }
   EXRETURN;
}

AFNI_ATLAS *Free_Atlas(AFNI_ATLAS *aa)
{
   int k = 0;

   ENTRY("Free_Atlas");

   if (!aa) {
      ERROR_message("NULL atlas");
      RETURN(aa);
   }

   if (aa->atlas_name) free(aa->atlas_name);
   for (k=0; k<aa->N_regions; ++k) {
      if (aa->reg[k]) Free_Atlas_Region(aa->reg[k]);
   }
   free(aa->reg);
   free(aa);

   RETURN(NULL);
}
static int SpeakEasy = 1;
void Set_ROI_String_Decode_Verbosity(byte lvl)
{
   SpeakEasy = lvl;
   return;
}
/*!
   Decode a given ROI specifying string

*/
AFNI_ATLAS_REGION *ROI_String_Decode(char *str, ATLAS_LIST *atlas_list)
{
   AFNI_ATLAS_REGION *aar = NULL;
   int nc=0, k, icol[10], shft=0, ncol = 0;
   char *lbl = NULL;
   char atlas_name[64]={""};
   int LocalHead = wami_lh();
   
   ENTRY("ROI_String_Decode");

   if (!str) {
      if (LocalHead || SpeakEasy) ERROR_message("NULL input");
      RETURN(aar) ;
   }
   atlas_name[0] = '\0';

   nc = strlen(str);
   if (nc < 3) {
      if (LocalHead || SpeakEasy) ERROR_message("Get Shorty");
      RETURN(aar) ;
   }

   /* find my ':' */
   icol[0] = 0;
   ncol = 0;
   k = 0;
   while (k<nc) {
      if (str[k] == ':') {
         if (ncol < 2) {
            ++ncol;
            icol[ncol] = k;
         } else {
            if (LocalHead || SpeakEasy) 
               ERROR_message("Too many ':' in ROI string");
            RETURN(aar) ;
         }
      }
      ++k;
   }


   if (!ncol){
      if (LocalHead || SpeakEasy) 
         ERROR_message("Failed to find ':'\nin '%s'\n", str);
      RETURN(aar) ;
   }
   
   if (icol[1]-icol[0] > 62) {
      ERROR_message("Atlas name in %s more than 62 characters. Not good.\n", 
                     str);
      RETURN(aar) ;
   }
   /* by now, we have at least one colon */
   /* get the atlas name, 1st item*/
   for (k=icol[0]; k<icol[1]; ++k) atlas_name[k] = str[k];
   atlas_name[icol[1]] = '\0';
   if (wami_verb() > 2) 
      fprintf(stderr,"atlas_name from %s is: %s\n", str, atlas_name); 
   /* is this an OK atlas ?*/
   if (!get_Atlas_Named(atlas_name, atlas_list)){  
      if (LocalHead || SpeakEasy) {
         ERROR_message( "Atlas %s not recognized in specified atlas_list\n"
                        "Available atlas names are:\n", atlas_name);
         print_atlas_list(atlas_list);
      }
      WARNING_message("Proceeding with hope for an impossible miracle...\n");
   }

   /* get the label, last item */
   lbl = (char*)calloc((nc+1), sizeof(char));
   shft = icol[ncol]+1;
   for (k=shft; k<nc; ++k) lbl[k-shft] = str[k];
   lbl[nc-shft] = '\0';
   if (wami_verb() > 2) 
      fprintf(stderr,"lbl from %s(%d to %d) is : '%s'\n", str, shft, nc, lbl); 

   /* Now get aar */
   if (!(aar = Atlas_Chunk_Label(lbl, 0, atlas_name))) {
      if (LocalHead || SpeakEasy) ERROR_message("Failed in processing label");
      RETURN(aar) ;
   }

   free(lbl); lbl = NULL;

   /* set the side if possible */
   if (ncol == 2 && (icol[2] - icol[1] > 1)) {
      aar->side = TO_LOWER(str[icol[1]+1]);
      if (  aar->side != 'l' && aar->side != 'r' 
            &&  aar->side != 'u'  &&  aar->side != 'b') {
         if (LocalHead || SpeakEasy) ERROR_message("Bad side specifier");
         aar = Free_Atlas_Region(aar);
         RETURN(aar) ;
      }
   }

   RETURN(aar) ;
}

char *Report_Found_Regions(AFNI_ATLAS *aa, AFNI_ATLAS_REGION *ur , 
                           ATLAS_SEARCH *as, int *nexact)
{
   char *rbuf = NULL, lbuf[500];
   THD_string_array *sar = NULL;
   int nfind = 0, ii = 0, k= 0;

   ENTRY("Find_Atlas_Regions");

   if (!as || !ur || !aa) {
      ERROR_message("NULL input");
      RETURN(rbuf);
   }
   /* Prep the string toys */
   INIT_SARR(sar) ;

   *nexact = 0;
   /* do we have a search by number ? */
   if (ur->id > 0 && ur->N_chnks == 0) {
      if (as->nmatch) {
         snprintf (lbuf, 480*sizeof(char), 
                     "Best match for %s (code %-3d):", ur->orig_label, ur->id);
         for (ii=0; ii<as->nmatch; ++ii) {
            snprintf (lbuf, 480*sizeof(char), "%s\n   %s", lbuf, 
                     aa->reg[as->iloc[ii]]->orig_label);
         }
         *nexact = as->nmatch;
      }else {
         snprintf (lbuf, 480*sizeof(char),  
                  "No match for integer code %-3d", ur->id);
      }
      ADDTO_SARR(sar,lbuf) ;
      goto PACK_AND_GO;
   }

   /* the whole deal */
   if (!as->nmatch) {
      snprintf (lbuf, 480*sizeof(char),  
                  "No exact match for %s", ur->orig_label);
      ADDTO_SARR(sar,lbuf) ;
      if (as->score[0] > 0 && as->score[0] > as->score[5]) { 
                                 /* maybe some useful suggestions */
         snprintf (lbuf, 480*sizeof(char),  "Closest few guesses:");
         ADDTO_SARR(sar,lbuf) ;
         k = 0;
         while (as->score[k] == as->score[0] && k < 5) {
            snprintf (lbuf, 480*sizeof(char),  "   %s, as->score %.3f", 
                     aa->reg[as->iloc[0]]->orig_label, as->score[k]);
            ADDTO_SARR(sar,lbuf) ;
            ++k;
         }
      } else {
         snprintf (lbuf, 480*sizeof(char),  
                           "   I don't even have good suggestions.\n"
                           "   Try to be more explicit.");
         ADDTO_SARR(sar,lbuf) ;
      }
   } else {
      if (as->score[0] > as->score[1]) { /* unique best fit */
         snprintf (lbuf, 480*sizeof(char),
                     "Best match for %s:\n   %s (code %-3d)", 
                     ur->orig_label, aa->reg[as->iloc[0]]->orig_label, 
                     aa->reg[as->iloc[0]]->id);
         ADDTO_SARR(sar,lbuf) ;
         *nexact = 1;
      } else if ( as->score[0] == as->score[1] &&
                  (as->N > 2 && as->score[1] > as->score[2]) &&
                  Same_Chunks(aa->reg[as->iloc[0]], aa->reg[as->iloc[1]])) { 
                                                            /* LR unspecified  */
         snprintf (lbuf, 480*sizeof(char),
            "Best match for %s:\n   %s (code %-3d)\n   %s (code %-3d)",
            ur->orig_label,
            aa->reg[as->iloc[0]]->orig_label, aa->reg[as->iloc[0]]->id,
            aa->reg[as->iloc[1]]->orig_label, aa->reg[as->iloc[1]]->id);
         ADDTO_SARR(sar,lbuf) ;
         *nexact = 2;
      } else {
         k=0;
         snprintf (lbuf, 480*sizeof(char),
                   "%d potential matches for %s:", as->nmatch, ur->orig_label);
         ADDTO_SARR(sar,lbuf) ;
         while (as->score[k] == as->score[0] && k<aa->N_regions) {
            snprintf (lbuf, 480*sizeof(char),
                  "         %s (code %-3d):", 
                  aa->reg[as->iloc[k]]->orig_label, aa->reg[as->iloc[k]]->id);
            ADDTO_SARR(sar,lbuf) ;
            ++k;
         }
      }
   }


   PACK_AND_GO:
   /*- convert list of labels into one big multi-line string -*/
   for( nfind=ii=0 ; ii < sar->num ; ii++ ) nfind += strlen(sar->ar[ii]) ;
   rbuf = AFMALL(char, nfind + 2*sar->num + 32 ) ; rbuf[0] = '\0' ;
   for( ii=0 ; ii < sar->num ; ii++ ){
      strcat(rbuf,sar->ar[ii]) ; strcat(rbuf,"\n") ;
   }

   DESTROY_SARR(sar) ;  sar = NULL;

   if (0) {
      INFO_message("Have:\n%s\n", rbuf);
   }

   RETURN(rbuf);
}

ATLAS_SEARCH *Free_Atlas_Search(ATLAS_SEARCH *as)
{
   ENTRY("Free_Atlas_Search");
   if (!as) RETURN(NULL);

   if (as->iloc) free(as->iloc);
   if (as->score) free(as->score);
   free(as);
   RETURN(NULL);
}

/*!
   Return a byte mask for a particular atlas region
*/
THD_3dim_dataset *Atlas_Region_Mask(AFNI_ATLAS_REGION *aar, 
                                    int *codes, int n_codes, 
                                    ATLAS_LIST *atlas_list)
{
   short *ba=NULL;
   int LocalHead = wami_lh();
   byte *bba = NULL;
   float *fba = NULL;
   byte *bmask = NULL;
   int ii=0, sb, nxyz, kk, ll = 0, fnd = -1, fnd2=-1;
   int ba_val, dset_kind, have_brik;
   THD_3dim_dataset * maskset = NULL;
   char madeupname[500], madeuplabel[40];
   ATLAS *atlas=NULL;

   ENTRY("Atlas_Region_Mask");

   if (!codes || n_codes == 0 || !aar || !aar->atlas_name) {
      ERROR_message("Nothing to do");
      RETURN(NULL);
   }

   if (LocalHead) {
      fprintf(stderr,"Looking for %d codes: \n   ", n_codes);
      for (kk=0; kk<n_codes; ++kk) {
         fprintf(stderr,"%d   ", codes[kk]);
      }
      fprintf(stderr,"\n");
   }

   if (!(atlas = Atlas_With_Trimming (aar->atlas_name, 1, atlas_list))) {
      ERROR_message("Failed getting atlas for mask");
      RETURN(NULL);
   }

   if (LocalHead)
      fprintf(stderr,"Found atlas for mask\n   ");

   nxyz = DSET_NX(ATL_DSET(atlas)) * 
          DSET_NY(ATL_DSET(atlas)) * DSET_NZ(ATL_DSET(atlas));
   if (!(bmask = (byte *)calloc(nxyz, sizeof(byte)))) {
      ERROR_message("Failed to allocate for mask");
      RETURN(maskset);
   }

   if (!is_Atlas_Named(atlas, "CA_N27_PM")) {
      for (sb=0; sb < DSET_NVALS(ATL_DSET(atlas)); ++sb) {
         dset_kind = DSET_BRICK_TYPE(ATL_DSET(atlas),sb);
         if(dset_kind == MRI_short) {
            ba = DSET_BRICK_ARRAY(ATL_DSET(atlas),sb); /* short type */
            if (!ba) { 
               ERROR_message("Unexpected NULL array");
               free(bmask); bmask = NULL;
               RETURN(maskset);
            }
         }
         else {
            bba = DSET_BRICK_ARRAY(ATL_DSET(atlas),sb); /* byte array */
            if (!bba) { 
               ERROR_message("Unexpected NULL array");
               free(bmask); bmask = NULL;
               RETURN(maskset);
            }
         }

         for (kk=0; kk < n_codes; ++kk) {
            for (ii=0; ii< nxyz; ++ii) {
               if (dset_kind == MRI_short)
                  ba_val = ba[ii];
               else
                  ba_val = (int) bba[ii];
               /* if voxel value matches code value
                  make byte mask 1 at that voxel */
               if (ba_val == codes[kk]) bmask[ii] = 1; 
                /* used to assign bmask[ii] = codes[kk]; */
            }
         }

      }
   } else { /* got to dump a particular sub-brick for probability maps*/
      if (LocalHead) INFO_message("Speciality");

      /* assume all sub-bricks are the same type */
      dset_kind = DSET_BRICK_TYPE(ATL_DSET(atlas),0);

      for (kk=0; kk < n_codes; ++kk) {
         /* find label to go with code */
         ll = 0;
         fnd = -1;
         while (ll<DSET_NVALS(ATL_DSET(atlas)) && fnd < 0) {
            if (atlas->adh->apl2->at_point[ll].tdval == codes[kk]) fnd = ll;
            else ++ll;
         }
         if (fnd < 0) {
            ERROR_message("Unexpected negative find"); 
            free(bmask); bmask = NULL; RETURN(maskset); }

         if (LocalHead)
            INFO_message("Looking for sub-brick labeled %s\n",
                Clean_Atlas_Label(atlas->adh->apl2->at_point[fnd].dsetpref));
         fnd2 = -1;
         sb = 0;
         while (sb < DSET_NVALS(ATL_DSET(atlas)) && fnd2 < 0) { 
            /* sb in question should be nothing but fnd. 
               But be careful nonetheless */
            if (DSET_BRICK_LAB(ATL_DSET(atlas),sb) && 
             !strcmp(DSET_BRICK_LAB(ATL_DSET(atlas),sb), 
               Clean_Atlas_Label(atlas->adh->apl2->at_point[fnd].dsetpref)))
                 fnd2 = sb;
            else ++sb;
         }
         if (fnd2 < 0) {
             ERROR_message("Unexpected negative find");
             free(bmask); bmask = NULL; RETURN(maskset);
         }

         /* fill byte mask with values wherever probability is non-zero */
         have_brik = 0;
         switch(dset_kind) {
           case MRI_short :
              ba = DSET_BRICK_ARRAY(ATL_DSET(atlas),fnd2); /* short type */
              if (ba) {
                 have_brik = 1;
                 for (ii=0; ii< nxyz; ++ii) {
                    if (ba[ii]) bmask[ii] = ba[ii];
                 }
              }
              break;
           case MRI_byte :
              bba = DSET_BRICK_ARRAY(ATL_DSET(atlas),fnd2); /* byte array */
              if (bba) {
                 have_brik = 1;
                 for (ii=0; ii< nxyz; ++ii) {
                    if (bba[ii]) bmask[ii] = bba[ii];
                 }
              }
              break;
           case MRI_float : /* floating point probability */
              fba = DSET_BRICK_ARRAY(ATL_DSET(atlas),fnd2);/* float array */
              if (fba) {
                 have_brik = 1;
                 for (ii=0; ii< nxyz; ++ii) {
                    if (fba[ii]>0.0) bmask[ii] = 250.0*fba[ii];
                                                   /* save as 0-250 */
                 }
              }
              break;
           default :
              ERROR_message("Unexpected data type for probability maps");
         }

         if(have_brik==0){
             ERROR_message("Could not get sub-brick from probability atlas");
             free(bmask); bmask = NULL; RETURN(maskset);
         }
      }
   }

   /* Now trim the LR business, if required. */
   if (aar->side == 'l' || aar->side == 'r') {
      for (ii=0; ii<nxyz; ++ii) {
         if ( bmask[ii] && 
        Atlas_Voxel_Side(ATL_DSET(atlas), ii, 
                         atlas->adh->lrmask) != aar->side ) 
            bmask[ii] = 0;
      }
      snprintf(madeupname, 400*sizeof(char), "%s.%s.%c",
               atlas->atlas_name, 
               Clean_Atlas_Label_to_Prefix(
                        Clean_Atlas_Label(aar->orig_label)), aar->side);
      snprintf(madeuplabel, 36*sizeof(char), "%c.%s", aar->side, 
            Clean_Atlas_Label_to_Prefix(Clean_Atlas_Label(aar->orig_label)));
   } else {
      snprintf(madeupname, 400*sizeof(char), "%s.%s",
              atlas->atlas_name,   
              Clean_Atlas_Label_to_Prefix(Clean_Atlas_Label(
                                                   aar->orig_label)));
      snprintf(madeuplabel, 36*sizeof(char), "%s", 
            Clean_Atlas_Label_to_Prefix(Clean_Atlas_Label(aar->orig_label)));
   }

   /* Now form the output mask dataset */

   maskset = EDIT_empty_copy( ATL_DSET(atlas) ) ;
   EDIT_dset_items(  maskset,
                       ADN_prefix    , madeupname ,
                       ADN_datum_all , MRI_byte ,
                       ADN_nvals     , 1 ,
                       ADN_ntt       , 0 ,
                       ADN_func_type , 
                        ISANAT(ATL_DSET(atlas)) ? 
                                       atlas->adh->adset->func_type
                                       : FUNC_FIM_TYPE ,
                       ADN_brick_label_one, madeuplabel,
                       ADN_directory_name , "./" ,
                       ADN_none ) ;

   EDIT_substitute_brick( maskset , 0 , MRI_byte , bmask ) ;  

   /* all done */
   RETURN(maskset);
}

/*!
   Try to locate a region in an atlas.

   "left" and "right" strings are not used in the matching.

   Returns a search struct containing
   array (as->iloc) of integers that is aa->N_regions long
   aa->reg[as->iloc[0]]->orig_label is the best match if as->nmatch is > 0
   aa->reg[as->iloc[1]]->orig_label is the second best match, etc.

   if ur->id > 0 && ur->N_chnks == 0 : An exact search is done based on ur->id

   Returned struct must be freed by the user (Free_Atlas_Search), but can be re-used by
   passing it as the last parameter.
*/
ATLAS_SEARCH * Find_Atlas_Regions(AFNI_ATLAS *aa, AFNI_ATLAS_REGION *ur , ATLAS_SEARCH *reusethis )
{
   int chnk_match[500], bs = 0;
   int k = 0, iu=0, lu=0, lr=0, ir=0, MinAcc=0, ngood = 0;
   ATLAS_SEARCH *as=NULL;

   ENTRY("Find_Atlas_Regions");

   if (!aa || !ur) {
      ERROR_message("NULL input");
      RETURN(as);
   }

   if (reusethis) as = reusethis;
   else {
      as = (ATLAS_SEARCH *)calloc(1, sizeof(ATLAS_SEARCH));
      as->iloc = (int *)calloc(aa->N_regions, sizeof(int));
      as->score = (float *)calloc(aa->N_regions, sizeof(float));
      as->N = aa->N_regions;
      as->nmatch = 0;
   }

   if (as->N < aa->N_regions) {
      ERROR_message("Reused as structure too small for this atlas!");
      RETURN(Free_Atlas_Search(as));
   }

   /* do we have a search by number ? */
   if (ur->id > 0 && ur->N_chnks == 0) {
      /* search by id */
      as->nmatch = 0;
      for (k=0; k<aa->N_regions; ++k) {/* aa->N_regions */
         if (aa->reg[k]->id == ur->id) { /* found */
            as->iloc[as->nmatch] = k;
            ++as->nmatch;
         }
      }
      RETURN(as);
   }


   /* Compare each region to input */
   for (k=0; k<aa->N_regions; ++k) {/* aa->N_regions */
      as->score[k] = 0.0; /* as->score of match for particular region */
      for (iu=0; iu < ur->N_chnks; ++iu) {  /* for each chunk in the user input */
         /* find best chunk match in atlas region, no regard for ordering of chunks */
         lu = strlen(ur->chnks[iu]);
         for (ir=0; ir < aa->reg[k]->N_chnks; ++ir) {
            chnk_match[ir] = 0; /* how well does atlas region chunk ir match with user chunk */
            lr = strlen(aa->reg[k]->chnks[ir]);
            if (strncmp(ur->chnks[iu], aa->reg[k]->chnks[ir], MIN_PAIR( lu, lr)) == 0) {
               /* one of the strings is inside the other */
               if (lu == lr) { /* identical match */
                  chnk_match[ir] = 4;
               } if (lu < lr) { /* user provided subset */
                  chnk_match[ir] = 2;
               } if (lu > lr) { /* user provided superset */
                  chnk_match[ir] = 1;
               }
               /* fprintf(stderr,"User string %s, Region %s: match = %d\n",
                  ur->chnks[iu], aa->reg[k]->chnks[ir],chnk_match[ir]);*/
            }
         }
         /* keep the best match for iu */
         bs = 0;
         for (ir=0; ir < aa->reg[k]->N_chnks; ++ir) {
            if (bs < chnk_match[ir]) bs = chnk_match[ir];
         }
         if (!bs) {
            /* user provided a chunk that matched nothing, penalize!
               (you might want to skip penalty for first chunks,
               if they are l,r, left, or right ...) */
            if (  0 && iu == 0 &&
                  (  strcmp("l",ur->chnks[0]) == 0 ||
                     strcmp("l",ur->chnks[0]) == 0 ||
                     strcmp("l",ur->chnks[0]) == 0 ||
                     strcmp("l",ur->chnks[0]) == 0 ) ) {
               /* clemency please*/
               bs = 0;
            } else { /* big penalty if user provides bad clues */
               bs = -4;
            }
         }

         /* add the as->score contribution for this chunk */
         as->score[k] += (float)bs;
      }
      /* small penalty if number of chunks is larger in label */
      if (aa->reg[k]->N_chnks - ur->N_chnks > 0) {
         as->score[k] -= (aa->reg[k]->N_chnks - ur->N_chnks);
      }
      /* bias by side matching */
      if (SIDE_MATCH(ur->side, aa->reg[k]->side)) {
         ++as->score[k];
      }
      /* fprintf (stderr,"Region %s as->scored %f with %s\n",  aa->reg[k]->orig_label, as->score[k], ur->orig_label);  */
   }

   /* sort the as->scores, largest to smallest */
   {
      int *itmp = NULL;
      itmp = z_iqsort(as->score, aa->N_regions);
      if (itmp) {
         memcpy((void*)as->iloc, (void *)itmp, aa->N_regions*sizeof(int));
         free(itmp); itmp = NULL;
      }else {
         ERROR_message("Error sorting!");
         RETURN(Free_Atlas_Search(as));
      }
   }

   /* show results where at least all chunks had partial match */
   k = 0;
   ngood = 0;
   MinAcc = 2*ur->N_chnks; /* the least acceptable value should be a partial match for user supplied chunk */
   while (as->score[k] >= MinAcc && k < aa->N_regions) {
      /* fprintf (stderr,"Match %d for %s is %s at as->score %f\n", k, ur->orig_label, aa->reg[as->iloc[k]]->orig_label, as->score[k]); */
      ++ngood;
      ++k;
   }

   if (!ngood) {
      /* fprintf (stderr,  "No match for %s\n", ur->orig_label); */
      if (as->score[0] > 0 && as->score[0] > as->score[5]) { /* maybe some useful suggestions */
         /* fprintf (stderr,  "Closest few guesses:\n"); */
         k = 0;
         while (as->score[k] == as->score[0] && k < 5) {
            /* fprintf (stderr,  "   %s, as->score %f\n", aa->reg[as->iloc[0]]->orig_label, as->score[k]); */
            ++k;
         }
      } else {
         /* fprintf (stderr,  "   I don't even have good suggestions.\n"
                           "   Try to be more explicit.\n"); */
      }

      as->nmatch = 0;
   } else {
      if (as->score[0] > as->score[1]) { /* unique best fit */
         /* fprintf (stderr,"Best match for %s is %s (code %d)\n",
                     ur->orig_label, aa->reg[as->iloc[0]]->orig_label, aa->reg[as->iloc[0]]->id); */
         as->nmatch = 1;
      } else {
         k=0;
         /* fprintf (stderr,"Potential match for %s:\n", ur->orig_label); */
         while (as->score[k] == as->score[0] && k<aa->N_regions) {
            /* fprintf (stderr,"         %s (code %d):\n", aa->reg[as->iloc[k]]->orig_label, aa->reg[as->iloc[k]]->id); */
            as->nmatch += 1;
            ++k;
         }
      }
   }


   RETURN(as);
}

char * Atlas_Prob_String(float p)
{
   static char probs[256];

   if (p == -2.0) {
      sprintf(probs," MPM");
   } else if (p == -1.0) {
      sprintf(probs," ---");
   } else {
      sprintf(probs,"%.2f", p);
   }
   return(probs);

}

char * Atlas_Code_String(int c)
{
   static char codes[256];

   if (c == -1) {
      sprintf(codes,"---");
   } else {
      sprintf(codes,"%3d", c);
   }
   return(codes);

}

char * Clean_Atlas_Label( char *lb)
{
   static char lab_buf[256];
   int nlab=0, nn=0;

   ENTRY("Clean_Atlas_Label");

   lab_buf[0] = '\0';

   if (!lb) {
      ERROR_message("NULL input!\n");
      RETURN(lab_buf);
   }

   nlab = strlen(lb);
   if (nlab > 250) {
      ERROR_message("Dset labels too long!\n");
      RETURN(lab_buf);
   }

   nn = nlab-1;
   while (nn >=0 && lb[nn] == '.') --nn;


   nlab = nn;
   nn = 0;
   if (nlab) {
      while (nn<=nlab) {
         lab_buf[nn] = lb[nn];
         ++nn;
      }
      lab_buf[nn] = '\0';
   }

   /* fprintf(stderr,"lbl = %s, clean = %s\n", lb, lab_buf); */
   RETURN(lab_buf);
}

char * Clean_Atlas_Label_to_Prefix( char *lb)
{
   static char lab_buf[256];
   int nn=0, cnt=0, nlab=0, notnum = 0;

   ENTRY("Clean_Atlas_Label_to_Prefix");

   lab_buf[0] = '\0';

   nlab = strlen(lb);
   if (nlab > 250) {
      ERROR_message("Dset labels too long!\n");
      RETURN(lab_buf);
   }

   /* do we have an integer label ? */
   notnum=0;
   nn=0;
   while (lb[nn] != '\0' && !notnum) {
      if (!IS_NUMBER(lb[nn])) notnum = 1;
      ++nn;
   }
   if (!notnum) {
      sprintf(lab_buf,"%d",atoi(lb));
      RETURN(lab_buf);
   }

   /* Not an integer label ... */
   cnt=0;
   for (nn=0; nn<nlab; ++nn) {
      if (!IS_LETTER(lb[nn]) && lb[nn] != '-' && lb[nn] != '_' && lb[nn] != '.') {
         if (cnt==0 || lab_buf[cnt-1] != '_') {
            lab_buf[cnt] = '_';
            ++cnt;
         }
      } else {
         lab_buf[cnt] = lb[nn];
         ++cnt;
      }
   }

   lab_buf[cnt] = '\0';

   RETURN(lab_buf);
}

const char *Space_Code_to_Space_Name (AFNI_STD_SPACES cod)
{
   ENTRY("Space_Code_to_Space_Name");

   switch(cod) {
      case UNKNOWN_SPC:
         RETURN("Unknown");
      case AFNI_TLRC_SPC:
         RETURN("TLRC");
      case MNI_SPC:
         RETURN("MNI");
      case MNI_ANAT_SPC:
         RETURN("MNI_ANAT");
      case NUMBER_OF_SPC:
         RETURN("Flag for number of spaces");
      default:
         RETURN("Willis?");
   }

   RETURN("No way Willis.");
}

const char *Atlas_Code_to_Atlas_Dset_Name (AFNI_ATLAS_CODES cod)
{
   ENTRY("Atlas_Code_to_Atlas_Dset_Name");

   /* this function should only be called when no global niml file exists*/
   if (wami_verb()) WARNING_message("OBSOLETE, do NOT use anymore");
   
   switch(cod) {
      case UNKNOWN_ATLAS:
         RETURN("Unknown ");
      case AFNI_TLRC_ATLAS:
         RETURN(TT_DAEMON_TT_PREFIX);
      case CA_EZ_N27_MPM_ATLAS:
         RETURN(CA_EZ_N27_MPM_TT_PREFIX);
      case CA_EZ_N27_ML_ATLAS:
         RETURN(CA_EZ_N27_ML_TT_PREFIX);
      case CA_EZ_N27_LR_ATLAS:
         RETURN(CA_EZ_N27_LR_TT_PREFIX);
      case CA_EZ_N27_PMAPS_ATLAS:
         RETURN(CA_EZ_N27_PMaps_TT_PREFIX);
      case CUSTOM_ATLAS :
        RETURN(CUSTOM_ATLAS_PREFIX);
      case NUMBER_OF_ATLASES:
         RETURN("Flag for number of atlases");
      default:
         RETURN("Bert?");
   }

   RETURN("No way Bert.");
}

AFNI_ATLAS_CODES Atlas_Dset_Name_to_Atlas_Code (char *dset_name)
{
   int LocalHead = wami_lh();

   ENTRY("Atlas_Dset_Name_to_Atlas_Code");
   
   INFO_message("OBsoLETE, do NOT use anymore");
   
   if (LocalHead)
      INFO_message("Finding code from dataset name >%s<"
                   "Pick from: %s,%s,%s,%s,%s,%s\n", 
                   dset_name, TT_DAEMON_TT_PREFIX, CA_EZ_N27_MPM_TT_PREFIX,
                   CA_EZ_N27_ML_TT_PREFIX, CA_EZ_N27_LR_TT_PREFIX,
                   CA_EZ_N27_PMaps_TT_PREFIX, CUSTOM_ATLAS_PREFIX);
   if(strstr(dset_name, TT_DAEMON_TT_PREFIX)){
      if(LocalHead) 
         INFO_message("%s for AFNI_TLRC_ATLAS %d",dset_name,AFNI_TLRC_ATLAS);
      RETURN(AFNI_TLRC_ATLAS);
   }
   if(strstr(dset_name, CA_EZ_N27_MPM_TT_PREFIX))
      RETURN(CA_EZ_N27_MPM_ATLAS);
   if(strstr(dset_name, CA_EZ_N27_ML_TT_PREFIX))
      RETURN(CA_EZ_N27_ML_ATLAS);
   if(strstr(dset_name, CA_EZ_N27_LR_TT_PREFIX))
      RETURN(CA_EZ_N27_LR_ATLAS);
   if(strstr(dset_name, CA_EZ_N27_PMaps_TT_PREFIX))
      RETURN(CA_EZ_N27_PMAPS_ATLAS);
   if(strstr(dset_name, CUSTOM_ATLAS_PREFIX))
      RETURN(CUSTOM_ATLAS);
   RETURN(UNKNOWN_ATLAS);
}

char *Atlas_Code_to_Atlas_Name (AFNI_ATLAS_CODES cod)
{
   ENTRY("Atlas_Code_to_Atlas_Name");
   int LocalHead = wami_lh();
   
   if (wami_verb()) 
      WARNING_message("Only for old style loading (no niml)."
                      "Don't allow ATLAS_LIST *");
   
   
   if (LocalHead) fprintf(stderr,"code in: %d (%d)\n", cod, AFNI_TLRC_ATLAS);
   switch(cod) {
      case UNKNOWN_ATLAS:
         RETURN("Unknown");
      case AFNI_TLRC_ATLAS:
         RETURN("TT_Daemon");
      case CA_EZ_N27_MPM_ATLAS:
         RETURN("CA_N27_MPM");
      case CA_EZ_N27_ML_ATLAS:
         RETURN("CA_N27_ML");
      case CA_EZ_N27_LR_ATLAS:
         RETURN("CA_N27_LR");
      case CA_EZ_N27_PMAPS_ATLAS:
         RETURN("CA_N27_PM");
      case CUSTOM_ATLAS:
         RETURN(CUSTOM_ATLAS_PREFIX);
      case NUMBER_OF_ATLASES:
         RETURN("Flag for number of atlases");
      default:
         RETURN("Bert?");
   }

   RETURN("No way Bert.");
}


/* This function could be made to return an integer into
the global space list. The 'Code' term should be changed 
to an index  */
AFNI_STD_SPACES Space_Name_to_Space_Code(char *nm) 
{
   ENTRY("Atlas_Space_Name_to_Atlas_Space_Code");
   
        if (!nm || !strcmp(nm,"Unknown")) RETURN(UNKNOWN_SPC);
   else if (!strcmp(nm,"TLRC")) RETURN(AFNI_TLRC_SPC);
   else if (!strcmp(nm,"MNI")) RETURN(MNI_SPC); 
   else if (!strcmp(nm,"MNI_ANAT")) RETURN(MNI_ANAT_SPC); 
   
   RETURN(UNKNOWN_SPC);
}
  
char *Atlas_Name(ATLAS *atl) 
{
   static char aname[10][65];
   static int icall=0;
   
   ENTRY("Atlas_Name");
   ++icall; if (icall > 9) icall = 0;
   aname[icall][0] = '\0';
   
   
   if (atl->atlas_name &&
       atl->atlas_name[0] != '\0') RETURN(atl->atlas_name);
   
   /* nothing to do now but go via old route */
   WARNING_message("Reverting to old name nonesense."
                   " This option should be turned off. Use atlas_name directly");
   strncpy( aname[icall], 
            Atlas_Code_to_Atlas_Name(
               Atlas_Dset_Name_to_Atlas_Code(atl->atlas_dset_name)),
            64);
   
   RETURN(aname[icall]);
}

int is_Coord_Space_Named(ATLAS_COORD ac, char *name) 
{
   if (ac.space_name && !strcmp(ac.space_name,name)) return(1);
   return(0);
}

int set_Coord_Space_Name (ATLAS_COORD *ac, char *name) 
{
   if (!name || strlen(name) > 63) {
      ERROR_message("Bad space name of >>%s<<", STR_PRINT(name));
      return(0);
   }
   strncpy(ac->space_name, name, 64);
   return(1);
}

int is_Atlas_Named(ATLAS *atl, char *name)
{
   if (!strcmp(Atlas_Name(atl),name)) return(1);
   return(0);
}

int is_Dset_Space_Named(THD_3dim_dataset *dset, char *name)
{
   char *spc = THD_get_space(dset);
   if (!spc) return(-1); /* no definition */
   if (!strcmp(spc,name)) return(1);
   return(0);
}


char *Atlas_Code_to_Atlas_Description (AFNI_ATLAS_CODES cod)
{
   ENTRY("Atlas_Code_to_Atlas_Description");
   
   if (wami_verb()) {
      WARNING_message("Only old style (no niml), do not allow ATLAS_LIST here");
   }
   
   switch(cod) {
      case UNKNOWN_ATLAS:
         RETURN("Unknown");
      case AFNI_TLRC_ATLAS:
         RETURN("Talairach-Tournoux Atlas");
      case CA_EZ_N27_MPM_ATLAS:
         RETURN("Cytoarch. Max. Prob. Maps (N27)");
      case CA_EZ_N27_ML_ATLAS:
         RETURN("Macro Labels (N27)");
      case CA_EZ_N27_LR_ATLAS:
         RETURN("Left/Right (N27)");
      case CA_EZ_N27_PMAPS_ATLAS:
         RETURN("Cytoarch. Probabilistic Maps (N27)");
      case CUSTOM_ATLAS:
         RETURN(CUSTOM_ATLAS_PREFIX);
      case NUMBER_OF_ATLASES:
         RETURN("Flag for number of atlases");
      default:
         RETURN("Bert?");
   }

   RETURN("No way Bert.");
}



void
init_custom_atlas()
{
   char *cust_atlas_str;
   int LocalHead = wami_lh();

   ENTRY("init_custom_atlas");

   cust_atlas_str = getenv("AFNI_CUSTOM_ATLAS");   

   if(cust_atlas_str)
      snprintf(CUSTOM_ATLAS_PREFIX, 255*sizeof(char), "%s", cust_atlas_str);
   if(LocalHead)
      INFO_message("CUSTOM_ATLAS_PREFIX = %s", CUSTOM_ATLAS_PREFIX);
   EXRETURN;
}

/*!
   Locate or create a zone of a particular level.
   \param aq (ATLAS_QUERY *)
   \param level (int)
   \return zn (ATLAS_ZONE) a new zone if none was
               found in aq at the proper level. Or
               whatever was found in aq.
*/
ATLAS_ZONE *Get_Atlas_Zone(ATLAS_QUERY *aq, int level)
{
   int ii=0, fnd=0;
   ATLAS_ZONE *zn=NULL;

   ENTRY("Get_Atlas_Zone");

   if (!aq) {
      ERROR_message("NULL atlas query");
      RETURN(zn);
   }

   /* fprintf(stderr,"Looking for zone level %d\n", level); */

   ii = 0;
   while (ii<aq->N_zone) {
      if (aq->zone[ii]->level == level) {
         if (fnd) {
            WARNING_message(  
               "More than one (%d) zone of level %d found in query.\n"
               "Function will ignore duplicates.\n", fnd, level ) ;
         }else {
            /* fprintf(stderr,"Zone with level %d found\n", level);  */
            zn = aq->zone[ii];
         }
         ++fnd;
      }
      ++ii;
   }

   if (!zn) {
      /* fprintf(stderr,"Zone with level %d NOT found\n", level);  */
      zn = (ATLAS_ZONE *)calloc(1, sizeof(ATLAS_ZONE));
      zn->level = level;
      zn->N_label = 0;
      zn->label = NULL;
      zn->code = NULL;
      zn->atname = NULL;
      zn->prob = NULL;
      zn->radius = NULL;
   }

   RETURN(zn);
}

/*!
   Create or Add to an Atlas Zone
   \param zn (ATLAS_ZONE *) If null then create a new one.
                            If a zone is given then add new labels to it
   \param level (int) a classifier of zones, in a way. 
                      In most cases, it is the equivalent of the within 
                      parameter.
   \param label (char *) a label string for this zone
   \param code (int) the integer code for this zone (not added if label is NULL)
   \param prob (float) the probability of that zone being label 
                        (not added if label is NULL)
   \param within (float) radius of label's occurrence
   \return zno (ATLAS_ZONE *) either a new structure (if zn == NULL) 
                  or a modified one (if zn != NULL)

   \sa free with Free_Atlas_Zone
*/
ATLAS_ZONE *Atlas_Zone( ATLAS_ZONE *zn, int level, char *label, 
                        int code, float prob, float within, 
                        char *aname)
{
   ATLAS_ZONE *zno = NULL;

   ENTRY("Atlas_Zone");

   if ( (prob < 0 && prob != -1.0 && prob != -2.0) || prob > 1) {
      ERROR_message( "Probability must be 0<=prob<=1 or -1.0 or -2.0\n"
                     "You sent %f\n", prob);
      RETURN(zno);
   }
   if (within < 0 && within != -1.0 ) {
      ERROR_message( "'Within' must be > 0 or -1.0\n"
                     "You sent %f\n", within);
      RETURN(zno);
   }
   if (!zn) {
      zno = (ATLAS_ZONE *)calloc(1, sizeof(ATLAS_ZONE));
      zno->level = level;
      zno->N_label = 0;
      zno->label = NULL;
      zno->code = NULL;
      zno->atname = NULL;
      zno->prob = NULL;
      zno->radius = NULL;
   } else {
      zno = zn;
      if (zno->level != level) {
         ERROR_message( "When zn is not null\n"
                        "level (%d) must be equal to zn->level (%d)\n", 
                        level, zn->level);
         RETURN(zno);
      }
   }
   if (label) {
      /* add label */
      ++zno->N_label;
      zno->label = (char **)realloc(zno->label, sizeof(char *)*zno->N_label);
      zno->label[zno->N_label-1] = strdup(label);
      zno->code = (int *)realloc(zno->code, sizeof(int)*zno->N_label);
      zno->code[zno->N_label-1] = code;
      zno->atname = (char **)realloc(zno->atname, sizeof(char*)*zno->N_label);
      zno->atname[zno->N_label-1] = nifti_strdup(aname);
      zno->prob = (float *)realloc(zno->prob, sizeof(float)*zno->N_label);
      zno->prob[zno->N_label-1] = prob;
      zno->radius = (float *)realloc(zno->radius, sizeof(float)*zno->N_label);
      zno->radius[zno->N_label-1] = within;
   }

   RETURN(zno);
}

ATLAS_ZONE *Free_Atlas_Zone(ATLAS_ZONE *zn)
{
   int k=0;

   ENTRY("Free_Atlas_Zone");

   if (!zn) RETURN(NULL);

   if (zn->label) {
      for (k=0; k<zn->N_label; ++k) if (zn->label[k]) free(zn->label[k]);
      free(zn->label);
   }
   if (zn->atname) {
      for (k=0; k<zn->N_label; ++k) if (zn->atname[k]) free(zn->atname[k]);
      free(zn->atname);
   }
   free(zn->code);
   free(zn->prob);
   free(zn->radius);
   free(zn);

   RETURN(NULL);
}

void Show_Atlas_Zone(ATLAS_ZONE *zn, ATLAS_LIST *atlas_list)
{
   int k=0;
   char probs[16], codes[16], radiuss[16];

   ENTRY("Show_Atlas_Zone");

   if (!zn) { fprintf(stderr,"NULL zone"); EXRETURN;}

   fprintf(stderr,
            "     level     :   %d\n"
            "     N_label(s):   %d\n",
            zn->level, zn->N_label);
   if (zn->label) {
      for (k=0; k<zn->N_label; ++k) {
         sprintf(probs, "%s", Atlas_Prob_String(zn->prob[k]));
         sprintf(codes, "%s", Atlas_Code_String(zn->code[k]));

         sprintf(radiuss,"%.1f", zn->radius[k]);

         fprintf(stderr,
   "     %d: label=%-32s, prob=%-3s, rad=%-3s, code=%-3s, atlas=%-10s\n",
                  k, Clean_Atlas_Label(zn->label[k]), probs, radiuss, codes, 
                  zn->atname[k]);
      }
   } else {
      fprintf(stderr,"     label (NULL");
   }

   EXRETURN;
}

void Show_Atlas_Query(ATLAS_QUERY *aq, ATLAS_LIST *atlas_list)
{
   int k=0;

   ENTRY("Show_Atlas_Query");

   if (!aq) { fprintf(stderr,"NULL query\n"); EXRETURN;}

   fprintf(stderr,
            "----------------------\n"
            "Atlas_Query: %d zones\n",
            aq->N_zone);
   if (aq->zone) {
      for (k=0; k<aq->N_zone; ++k) {
         fprintf(stderr,"  zone[%d]:\n", k);
         Show_Atlas_Zone(aq->zone[k], atlas_list);
         fprintf(stderr,"\n");
      }
   } else {
      fprintf(stderr,"  zone (NULL)\n");
   }
   fprintf(stderr,
            "----------------------\n");
   EXRETURN;
}

ATLAS_QUERY *Add_To_Atlas_Query(ATLAS_QUERY *aq, ATLAS_ZONE *zn)
{
   int i, fnd=0;
   ATLAS_QUERY *aqo;

   ENTRY("Add_To_Atlas_Query");

   if (!aq) {
      aqo = (ATLAS_QUERY *)calloc(1, sizeof(ATLAS_QUERY));
      aqo->N_zone = 0;
      aqo->zone = NULL;
   }else{
      aqo = aq;
   }

   if (zn) {
      /* make sure this one does not exist already */
      for (i=0; i<aqo->N_zone; ++i) {
         if (aqo->zone[i] == zn) {
            fnd = 1;
            break;
         }
      }
      if (!fnd) {
         /* add zone */
         ++aqo->N_zone;
         aqo->zone = (ATLAS_ZONE **)realloc( aqo->zone, 
                                             sizeof(ATLAS_ZONE*)*aqo->N_zone);
         aqo->zone[aqo->N_zone-1] = zn;
      }
   }
   RETURN(aqo);
}

ATLAS_QUERY *Free_Atlas_Query(ATLAS_QUERY *aq)
{
   int k=0;

   ENTRY("Free_Atlas_Query");

   if (!aq) RETURN(NULL);

   if (aq->zone) {
      for (k=0; k<aq->N_zone; ++k) if (aq->zone[k]) Free_Atlas_Zone(aq->zone[k]);
      free(aq->zone);
   }
   free(aq);

   RETURN(NULL);
}

int Check_Version_Match(THD_3dim_dataset * dset, char *atname)
{
   ATR_int *notecount;
   int num_notes, i, j, mmm ;
   char *chn , *chd, *mt ;
   char *ver=NULL;
   
   ENTRY("Check_Version_Match");
   
   if (!dset) RETURN(0); /* not good */
   
   ver = atlas_version_string(atname); 
   
   if (!ver) RETURN(1); /* no versions here */
   if (!strcmp(atname,"CA_N27_MPM") ||
       !strcmp(atname,"CA_N27_PM")  ||
       !strcmp(atname,"CA_N27_LR") ||
       !strcmp(atname,"CA_N27_ML") ) {   /* CA atlases, good */
     notecount = THD_find_int_atr(dset->dblk, "NOTES_COUNT");
     if( notecount != NULL ){
        num_notes = notecount->in[0] ;
        mmm = 4000 ;
        for (i=1; i<= num_notes; i++) {
           chn = tross_Get_Note( dset , i ) ;
           if( chn != NULL ){
              j = strlen(chn) ; if( j > mmm ) chn[mmm] = '\0' ;
              chd = tross_Get_Notedate(dset,i) ;
              if( chd == NULL ){ chd = AFMALL(char,16) ; strcpy(chd,"no date") ; }
              /* fprintf(stderr,"\n----- NOTE %d [%s] (searching for %s) -----\n%s\n",i,chd, CA_EZ_VERSION_STR, chn ) ; */
              /* search for matching versions */
              mt = strstr(chn, ver);
              free(chn) ; free(chd) ;
              if (mt) {
               RETURN(1); /* excellent */
              }
           }
        }
     }

   }

   RETURN(0); /* not good */
}

static int N_VersionMessage = 0;

static char *VersionMessage(void)
{
   static char verr[1000];
   ENTRY("VersionMessage");
   sprintf( verr, "Mismatch of Anatomy Toolbox Versions.\n"
                  "Version in AFNI is %s and appears\n"
                  "different from version string in atlas' notes.\n"
                  "See whereami -help for more info.\n", CA_EZ_VERSION_STR_HARD);
   RETURN(verr);
}

#ifdef KILLTHIS /* Remove all old sections framed by #ifdef KILLTHIS
                  in the near future.  ZSS May 2011   */ 
int CA_EZ_ML_load_atlas_old(void)
{
   char *epath ;
   char atpref[256];

   ENTRY("CA_EZ_ML_load_atlas_old");

   WARNING_message("Obsolete, use Atlas_With_Trimming(\"CA_EZ_ML\", .) instead");

   if( have_dseCA_EZ_ML_old >= 0 ) 
      RETURN(have_dseCA_EZ_ML_old) ;  /* for later calls */

   have_dseCA_EZ_ML_old = 0 ;  /* don't have it yet */

   /*----- 20 Aug 2001: see if user specified alternate database -----*/

   epath = getenv("AFNI_CA_EZ_N27_ML_ATLAS_DATASET") ;   /* suggested path, if any */
   snprintf(atpref, 255*sizeof(char), "%s+tlrc", CA_EZ_N27_ML_TT_PREFIX);
   dseCA_EZ_ML_old = get_atlas( epath, atpref ) ;  /* try to open it */
   if (!dseCA_EZ_ML_old) { /* try for NIFTI */
      snprintf(atpref, 255*sizeof(char), "%s.nii.gz", CA_EZ_N27_ML_TT_PREFIX);
      dseCA_EZ_ML_old = get_atlas( epath, atpref) ;
   }
   if( dseCA_EZ_ML_old != NULL ){                     /* got it!!! */
      /* check on version */
      if (!Check_Version_Match(dseCA_EZ_ML_old, "CA_N27_ML")) {
         if (!N_VersionMessage) { POPUP_MESSAGE( VersionMessage() ); ++N_VersionMessage; }
         ERROR_message( VersionMessage() );
         /* dump the load */
         /* CA_EZ_ML_purge_atlas();, not good enough will get reloaded elsewhere */
         DSET_delete(dseCA_EZ_ML_old) ;  dseCA_EZ_ML_old = NULL;
         RETURN(0) ;
      }
      have_dseCA_EZ_ML_old = 1; RETURN(1);
   }

   RETURN(0) ; /* got here -> didn't find it */
}

int CA_EZ_LR_load_atlas_old(void)
{
   char *epath ;
   char atpref[256];

   ENTRY("CA_EZ_LR_load_atlas_old");
   
   WARNING_message("Obsolete, use Atlas_With_Trimming(\"CA_EZ_LR\", .) instead");
   
   if( have_dseCA_EZ_LR_old >= 0 ) 
      RETURN(have_dseCA_EZ_LR_old) ;  /* for later calls */

   have_dseCA_EZ_LR_old = 0 ;  /* don't have it yet */

   /*----- 20 Aug 2001: see if user specified alternate database -----*/

   epath = getenv("AFNI_CA_EZ_N27_LR_ATLAS_DATASET") ;   /* suggested path, if any */
   snprintf(atpref, 255*sizeof(char), "%s+tlrc", CA_EZ_N27_LR_TT_PREFIX);
   dseCA_EZ_LR_old = get_atlas( epath, atpref ) ;  /* try to open it */
   if (!dseCA_EZ_LR_old) { /* try for NIFTI */
      snprintf(atpref, 255*sizeof(char), "%s.nii.gz", CA_EZ_N27_LR_TT_PREFIX);
      dseCA_EZ_LR_old = get_atlas( epath, atpref) ;
   }
   if( dseCA_EZ_LR_old != NULL ){                     /* got it!!! */
      /* check on version */
      if (!Check_Version_Match(dseCA_EZ_LR_old, "CA_N27_LR")) {
         if (!N_VersionMessage) { POPUP_MESSAGE( VersionMessage() ); ++N_VersionMessage; }
         ERROR_message(  VersionMessage() );
         /* dump the load */
         /* CA_EZ_LR_purge_atlas();, not good enough will get reloaded elsewhere */
         DSET_delete(dseCA_EZ_LR_old) ; dseCA_EZ_LR_old = NULL;
         RETURN(0) ;
      }
      have_dseCA_EZ_LR_old = 1; RETURN(1);
   }

   RETURN(0) ; /* got here -> didn't find it */
}

int CA_EZ_MPM_load_atlas_old(void)
{
   char *epath ;
   char atpref[256];

   ENTRY("CA_EZ_MPM_load_atlas_old");
   WARNING_message(
      "Obsolete, use Atlas_With_Trimming(\"CA_EZ_MPM\", .) instead");

   if( have_dseCA_EZ_MPM_old >= 0 ) 
      RETURN(have_dseCA_EZ_MPM_old) ;  /* for later calls */

   have_dseCA_EZ_MPM_old = 0 ;  /* don't have it yet */

   /*----- 20 Aug 2001: see if user specified alternate database -----*/

   epath = getenv("AFNI_CA_EZ_N27_MPM_ATLAS_DATASET") ;   /* suggested path, if any */
   snprintf(atpref, 255*sizeof(char), "%s+tlrc", CA_EZ_N27_MPM_TT_PREFIX);
   dseCA_EZ_MPM_old = get_atlas( epath, atpref ) ;  /* try to open it */
   if (!dseCA_EZ_MPM_old) { /* try for NIFTI */
      snprintf(atpref, 255*sizeof(char), "%s.nii.gz", CA_EZ_N27_MPM_TT_PREFIX);
      dseCA_EZ_MPM_old = get_atlas( epath, atpref) ;
   }
   if( dseCA_EZ_MPM_old != NULL ){                     /* got it!!! */
      /* check on version */
      if (!Check_Version_Match(dseCA_EZ_MPM_old, "CA_N27_MPM")) {
         if (!N_VersionMessage) { POPUP_MESSAGE( VersionMessage() ); ++N_VersionMessage; }
         ERROR_message( VersionMessage() );
         /* dump the load */
         /*CA_EZ_MPM_purge_atlas();, not good enough will get reloaded elsewhere */
         DSET_delete(dseCA_EZ_MPM_old) ; dseCA_EZ_MPM_old = NULL;
         RETURN(0) ;
      }
      have_dseCA_EZ_MPM_old = 1; RETURN(1);
   }

   RETURN(0) ; /* got here -> didn't find it */
}


int CA_EZ_PMaps_load_atlas_old(void)
{
   char *epath ;
   char atpref[256];

   ENTRY("CA_EZ_PMaps_load_atlas_old");
   
   WARNING_message("Obsolete, use Atlas_With_Trimming(\"CA_EZ_PM\", .) instead");

   if( have_dseCA_EZ_PMaps_old >= 0 ) 
      RETURN(have_dseCA_EZ_PMaps_old) ;  /* for later calls */

   have_dseCA_EZ_PMaps_old = 0 ;  /* don't have it yet */

   /*----- 20 Aug 2001: see if user specified alternate database -----*/

   epath = getenv("AFNI_CA_EZ_N27_PMAPS_ATLAS_DATASET") ;   /* suggested path, if any */
   snprintf(atpref, 255*sizeof(char), "%s+tlrc", CA_EZ_N27_PMaps_TT_PREFIX) ;
   dseCA_EZ_PMaps_old = get_atlas( epath, atpref ) ;  /* try to open it */
   if (!dseCA_EZ_PMaps_old) { /* try for NIFTI */
      snprintf(atpref, 255*sizeof(char), 
               "%s.nii.gz", CA_EZ_N27_PMaps_TT_PREFIX) ;
      dseCA_EZ_PMaps_old = get_atlas( epath, atpref) ;
   }
   if( dseCA_EZ_PMaps_old != NULL ){                     /* got it!!! */
      /* check on version */
      if (!Check_Version_Match(dseCA_EZ_PMaps_old, "CA_N27_PM")) {
         if (!N_VersionMessage) { POPUP_MESSAGE( VersionMessage() ); ++N_VersionMessage; }
         ERROR_message(  VersionMessage() );
         /* dump the load */
         /* CA_EZ_PMaps_purge_atlas();, not good enough will get reloaded elsewhere */
         DSET_delete(dseCA_EZ_PMaps_old) ; dseCA_EZ_PMaps_old = NULL;
         RETURN(0) ;
      }
      have_dseCA_EZ_PMaps_old = 1; RETURN(1);
   }

   RETURN(0) ; /* got here -> didn't find it */
}
#endif

/* load any atlas (ignoring different environment variables
   for alternate locations) */
THD_3dim_dataset *load_atlas_dset(char *dsetname)
{
   char *epath ;
   char atpref[256];
   THD_3dim_dataset *dset=NULL;
   int LocalHead = wami_lh();

   ENTRY("load_atlas_dset");
   /* suggested path, if any - if not set, then use afni, plugin or current dir*/
   epath = getenv("AFNI_TTATLAS_DATASET") ;
   /* try dsetname first */
   if (!dset) {
      if(LocalHead)
         INFO_message("load_atlas: epath %s, name %s", epath, dsetname);
      dset = get_atlas( epath, dsetname ) ;  /* try to open it */
   }
   if (!dset) { /* try the AFNI format with +tlrc in the name */
      snprintf(atpref, 255*sizeof(char), "%s+tlrc", dsetname);
      if(LocalHead)
         INFO_message("load_atlas: epath %s, name %s", epath, atpref);
      dset = get_atlas( epath, atpref ) ;  /* try to open it */
   }
   if (!dset) { /* try NIFTI naming */
      snprintf(atpref, 255*sizeof(char), "%s.nii.gz", dsetname);
      if(LocalHead)
         INFO_message("load_atlas: epath %s, name %s", epath, atpref);
      dset = get_atlas( epath, atpref) ;
   }
   if (!dset) {
      if(LocalHead)
         INFO_message("load_atlas: no AFNI format found");
   }
   RETURN(dset) ;
}


#define IS_BLANK(c) ( ((c) == ' ' || (c) == '\t' || (c) == '\n' || (c) == '\v' || (c) == '\f' || (c) == '\r') ? 1 : 0 )

char *AddLeftRight(char *name, char lr)
{
   static char namesave[500];

   ENTRY("AddLeftRight");

   if (lr == 'l' || lr == 'L') sprintf(namesave,"Left %s", name);
   else if (lr == 'r' || lr == 'R') sprintf(namesave,"Right %s", name);
   else RETURN(name);

   RETURN(namesave);
}

/* removes one occurence of left or right in name , search is case insensitive*/
char *NoLeftRight (char *name)
{
   char *nolr0=NULL, namesave[500];
   int i;
   ENTRY("NoLeftRight");

   if (!name) RETURN(name);

   snprintf(namesave,499*sizeof(char), "%s", name);

   for (i=0; i<strlen(name); ++i) name[i] = TO_UPPER(name[i]);
   nolr0 = strstr(name, "LEFT");
   /*if (nolr0) fprintf(stderr,"%s\n%s\n", name, nolr0+4); */
   if (!nolr0) { /* left not found, remove right */
      nolr0 = strstr(name, "RIGHT");
     /* if (nolr0) fprintf(stderr,"%s\n%s\n", name, nolr0+5); */
     if (nolr0) {
         nolr0 += 5; /* jump beyond right */
      }
   }else {
      nolr0 += 4; /* jump beyond left */
   }

   /* deblank */
   if (nolr0) {
      while (nolr0[0] != '\0' && IS_BLANK(nolr0[0]))  {
         ++nolr0;
      }
   }

   /* put it back */
   sprintf(name,"%s", namesave);

   if (nolr0) RETURN(nolr0);
   else RETURN(name);
}

/* See also atlas_key_label */
const char *Atlas_Val_Key_to_Val_Name(ATLAS *atlas, int tdval)
{
   int ii, cmax = 600;
   static char tmps[600];

   ENTRY("Atlas_Val_Key_to_Val_Name");

   tmps[0] = '\0';
   if (tdval > atlas->adh->maxkeyval || tdval < atlas->adh->minkeyval) {
      ERROR_message( "integer code %d outside [%d %d]!", 
                     tdval, atlas->adh->minkeyval, atlas->adh->maxkeyval);
      RETURN(NULL);
   }
   if (!atlas->adh->apl2 || !atlas->adh->apl2->at_point) {
      ERROR_message("No list for this atlas, fool!");
      RETURN(NULL);
   }

   if (!atlas->adh->duplicateLRentries) {
      /* quicky */
      if (tdval < MAX_ELM(atlas->adh->apl2)) {
         if (atlas->adh->apl2->at_point[tdval].tdval == tdval) 
               RETURN(Clean_Atlas_Label(atlas->adh->apl2->at_point[tdval].name));
      }
      /* longy */
      for( ii=0 ; ii < MAX_ELM(atlas->adh->apl2) ; ii++ ) {
         if (atlas->adh->apl2->at_point[ii].tdval == tdval) 
               RETURN(Clean_Atlas_Label(atlas->adh->apl2->at_point[ii].name));
      }
   } else {
      int fnd[30], nfnd =0;
      /* search for all, for safety */
      for( ii=0 ; ii < MAX_ELM(atlas->adh->apl2) ; ii++ ) {
         if (atlas->adh->apl2->at_point[ii].tdval == tdval) {
            fnd[nfnd] = ii; ++nfnd;
         }
      }

      if ( nfnd == 0) RETURN(NULL);

      if ( nfnd == 1) {
         RETURN(NoLeftRight(
                Clean_Atlas_Label(atlas->adh->apl2->at_point[fnd[0]].name)));
      }

      if (nfnd > 2) {
         /* too many entries, return them all */
         snprintf(tmps,sizeof(char)*(int)((cmax-6*nfnd)/nfnd), "%s", 
                     Clean_Atlas_Label(atlas->adh->apl2->at_point[fnd[0]].name));
         for( ii=1 ; ii < nfnd; ++ii) {
            snprintf(tmps,sizeof(char)*(int)((cmax-6*nfnd)/nfnd), "-AND-%s", 
                  Clean_Atlas_Label(atlas->adh->apl2->at_point[fnd[ii]].name));
         }
      } else {
         /* get ridd of the LR business */
         if (!strcmp(NoLeftRight(
                   Clean_Atlas_Label(atlas->adh->apl2->at_point[fnd[0]].name)),
                     NoLeftRight(
                   Clean_Atlas_Label(atlas->adh->apl2->at_point[fnd[1]].name)))){
            RETURN(NoLeftRight(
                    Clean_Atlas_Label(atlas->adh->apl2->at_point[fnd[0]].name)));
         }else{
            snprintf(tmps,sizeof(char)*cmax, "%s-AND-%s", 
                     Clean_Atlas_Label(atlas->adh->apl2->at_point[fnd[0]].name), 
                     Clean_Atlas_Label(atlas->adh->apl2->at_point[fnd[1]].name));
            RETURN(tmps);
         }
      }
   }

   RETURN(NULL);
}

ATLAS *get_Atlas_Named(char *atname, ATLAS_LIST *atlas_list)
{
   ATLAS *atl = NULL;
   int i=0;
   
   ENTRY("get_Atlas_Named");
   
   if (!atlas_list && !(atlas_list = get_G_atlas_list())) {
      ERROR_message("I don't have an atlas list");
      RETURN(NULL);
   }
   if (!atname) {
      ERROR_message("NULL name");
      RETURN(NULL);
   }

   for (i=0; i<atlas_list->natlases;++i) {
      if (!strcmp(atname, atlas_list->atlas[i].atlas_name)) {
         RETURN(&(atlas_list->atlas[i]));
      }
   }
   RETURN(NULL);
}

ATLAS_DSET_HOLDER *Free_Atlas_Dset_Holder(ATLAS_DSET_HOLDER *adh) 
{
   if (!adh) return(NULL);
   if (adh->apl2) free_atlas_point_list(adh->apl2);
   if (adh->adset) DSET_delete(adh->adset);
   free(adh);
   return(NULL);
}

int Init_Atlas_Dset_Holder(ATLAS *atlas) 
{
   ATLAS_DSET_HOLDER *adh=NULL;
   
   ENTRY("New_Atlas_Dset_Holder");
   
   if (!atlas) RETURN(0);
   
   if (atlas->adh) {
      ERROR_message("Non NULL ADH this is not allowed here");
      RETURN(0);
   }
   
   /* initialize atlas dataset holder, adh, to null defaults */
   atlas->adh = (ATLAS_DSET_HOLDER *)calloc(1, sizeof(ATLAS_DSET_HOLDER));
   atlas->adh->adset = NULL;
   atlas->adh->params_set = 0;
   atlas->adh->mxlablen = -1;
   atlas->adh->lrmask = NULL;
   atlas->adh->maxkeyval = -1;
   atlas->adh->minkeyval = 1000000;     
   atlas->adh->duplicateLRentries = 0; 
                 /* Are LR labels listed in atlas->adh->apl2->at_point and
                 under the same code? (only case I know of is in TTO_list*/
   atlas->adh->apl2 = NULL;
   atlas->adh->build_lr = 0; 
               /* assume we do *not* need to figure out left,right*/
   atlas->adh->mxlablen = ATLAS_CMAX;
   atlas->adh->probkey = -2;
   
   RETURN(1);
}

ATLAS *Atlas_With_Trimming(char *atname, int LoadLRMask, 
                                       ATLAS_LIST *atlas_list)
{  
   
   int ii, pmap;
   int LocalHead = wami_lh();
   static int n_warn=0;
   ATR_int *pmap_atr;
   ATLAS *atlas=NULL;
   
   ENTRY("Atlas_With_Trimming");
   
   if (!atlas_list && !(atlas_list = get_G_atlas_list())) {
      ERROR_message("Cannot get me an atlas list");
      RETURN(NULL);
   }
   
   /* Get the atlas structure from the list */
   if (!(atlas = get_Atlas_Named(atname, atlas_list))) {
      ERROR_message("Cannot find atlas %s", atname);
      RETURN(NULL);
   }
   
   /* Now get dset if it is missing */
   if (!ATL_DSET(atlas)) {
      if (LocalHead) 
         fprintf(stderr,"Loading %s\n", atname);
      if (!(genx_load_atlas_dset(atlas))) {
         if (wami_verb()) {
             if (!n_warn || wami_verb()>1) {
               WARNING_message(  "Could not read atlas dset: %s \n"
                      "See whereami -help for help on installing atlases. ",
                      ATL_NAME_S(atlas),
          (wami_verb() > 1) ? "":"\nOther similar warnings are now muted\n" );
            ++(n_warn);
            }
         }
         RETURN(NULL);
      }
   } else {
      if (LocalHead) INFO_message("Reusing dset");
      /* reload, in case it was purged */
      for (ii=0; ii<DSET_NVALS(ATL_DSET(atlas)); ++ii) {
         if (DSET_BRICK_IS_PURGED(ATL_DSET(atlas), ii)) {
            DSET_load(ATL_DSET(atlas)) ;
            break;
         }
      }
   }
    
   /* Now the trimming */    
   if (!ATL_ADH_SET(atlas)) {
      if (LocalHead) {
         INFO_message("Filling ADH");
         fprintf(stderr,"Getting NIML attribute segmentation\n");
      }
      if (atlas->adh->apl2) {
         if (wami_verb()) INFO_message("Recreating apl2");
         free_atlas_point_list(atlas->adh->apl2); 
         atlas->adh->apl2 = NULL;
      }
      /* check to see if atlas dataset has NIML attributes for segmentation */
      atlas->adh->apl2 = dset_niml_to_atlas_list(ATL_DSET(atlas));
      if(atlas->adh->apl2 == NULL) {
         if (LocalHead) fprintf(stderr,"No NIML attributes.\n"
                             "Getting hard-coded segmentation\n");

         if(set_adh_old_way(atlas->adh, Atlas_Name(atlas)))
            WARNING_message(  "Could not read atlas dset4 %s \n"
                               "See whereami -help for help on installing "
                               "atlases.\n", atlas->atlas_dset_name );
      } else {
         if (LocalHead) fprintf(stderr,"NIML attributes being used.\n");

         for (ii=0; ii<MAX_ELM(atlas->adh->apl2); ++ii) {
            if(atlas->adh->apl2->at_point[ii].tdval > atlas->adh->maxkeyval)
                atlas->adh->maxkeyval = atlas->adh->apl2->at_point[ii].tdval;
            if(atlas->adh->apl2->at_point[ii].tdval < atlas->adh->minkeyval)
                atlas->adh->minkeyval = atlas->adh->apl2->at_point[ii].tdval;
         }
         atlas->adh->duplicateLRentries = 0; 
         atlas->adh->build_lr = 0;
         if (LocalHead) print_atlas_point_list(atlas->adh->apl2);
         pmap_atr = THD_find_int_atr(atlas->adh->adset->dblk,"ATLAS_PROB_MAP") ;
         if(pmap_atr!=NULL) {
            pmap = pmap_atr->in[0] ;
            if (pmap==1)
               atlas->adh->probkey = 0;
            else
               atlas->adh->probkey = -1;
            if (LocalHead) fprintf(stderr, "probability map %d\n", pmap);
         }

      }

      /* have LR mask ? */
      /* check to see if dataset has to be distinguished 
         left-right based on LR atlas */
      if (atlas->adh->build_lr && LoadLRMask) {
            /* DO NOT ask Atlas_With_Trimming to load LRMask in next call !! */
         ATLAS *atlas_lr = Atlas_With_Trimming("CA_N27_LR", 0, atlas_list);
         if (!atlas_lr) {
            ERROR_message("Could not read LR atlas\n"
                                 "LR decision will be based on coordinates.");
         } else {
            atlas->adh->lrmask = DSET_BRICK_ARRAY(ATL_DSET(atlas_lr),0);
            if (!atlas->adh->lrmask) {
               ERROR_message("Unexpected NULL array.\n"
                             "Proceeding without LR mask");
            }
         }
      }
      
      atlas->adh->params_set = 1;   /* mark as initialized */
   } else {
      if (LocalHead) INFO_message("Reusing ADH");
   }
   
   RETURN(atlas);
}

/*! Fills in the ATLAS structure if it needs filling */ 
int genx_load_atlas_dset(ATLAS *atlas)
{
   int ii, pmap;
   int LocalHead = wami_lh();
   ATR_int *pmap_atr;

   /* Load the dataset */
   if(ATL_DSET(atlas) == NULL) {
      /* initialize holder */
      if (!Init_Atlas_Dset_Holder(atlas)) {
         ERROR_message("Failed to initialize ADH for atlas %s",
                        Atlas_Name(atlas));
         RETURN(0);
      }
      if (LocalHead) 
         fprintf(stderr,"genx loading dset %s\n", atlas->atlas_dset_name);
      atlas->adh->adset = load_atlas_dset(atlas->atlas_dset_name);
      if (ATL_DSET(atlas) == NULL) {
         if (LocalHead) {
            WARNING_message("Could not read atlas dataset: %s \n"
                         "See whereami -help for help on installing atlases.\n",
                          atlas->atlas_dset_name);
         }
         /* For the moment, cleanup and return. */
         if (wami_verb()) { 
            INFO_message("Daniel: Note that each time a query is called,\n "
                   "these functions will attempt to reload all \n"
                   "missing dsets. For efficiency, one might want to prune\n"
                   "the atlas_list from those dsets that fail to load.");
         }
         atlas->adh = Free_Atlas_Dset_Holder(atlas->adh);
         if (wami_verb()) { 
            INFO_message("Daniel: You might want to try harder here based on\n "
                   " atlas->atlas_name perhaps, or by allowing for particular\n "
                   " path environment variables as in TT_load_atlas_old\n");
         }
         RETURN(0);
      }
   } else {
      if (LocalHead) 
         fprintf(stderr,"genx dset %s already loaded\n", atlas->atlas_dset_name);
   }

   RETURN(1);
}

/* purge atlas to save memory */
int purge_atlas(char *atname) {
   ATLAS *atlas=NULL;
   THD_3dim_dataset *dset=NULL;

   ENTRY("purge_atlas");
   
   /* Get the atlas structure from the list */
   if (!(atlas = get_Atlas_Named(atname, NULL))) {
      if (wami_verb()) {
         INFO_message("Cannot find atlas %s for purging", atname);
      }
      RETURN(1);
   }
   if (!(dset=ATL_DSET(atlas))) {
      if (wami_verb()) {
         INFO_message("Atlas %s's dset not loaded", atname);
      }
      RETURN(1);
   }
   
   PURGE_DSET(dset);
   RETURN(1); 
}


ATLAS_POINT_LIST *atlas_point_to_atlas_point_list(ATLAS_POINT * apl, int n_pts)
{
   ATLAS_POINT_LIST *apl2 = NULL;
   int i;
   
   if (!apl) return(NULL);
   
   apl2 = (ATLAS_POINT_LIST *)calloc(1,sizeof(ATLAS_POINT_LIST));
   apl2->n_points = n_pts;
   apl2->at_point = (ATLAS_POINT *)calloc(n_pts, sizeof(ATLAS_POINT));
   for (i=0; i<n_pts; ++i) {
      NI_strncpy(apl2->at_point[i].name,apl[i].name,ATLAS_CMAX);
      NI_strncpy(apl2->at_point[i].dsetpref,apl[i].dsetpref,ATLAS_CMAX);
      
      apl2->at_point[i].tdval = apl[i].tdval;
      apl2->at_point[i].okey = apl[i].okey;
      apl2->at_point[i].tdlev = apl[i].tdlev;
      apl2->at_point[i].xx = apl[i].xx;
      apl2->at_point[i].yy = apl[i].yy;
      apl2->at_point[i].zz = apl[i].zz; 
   }
   return(apl2);
}  

int set_adh_old_way(ATLAS_DSET_HOLDER *adh, char *aname)
{
   int ii;
   int LocalHead = wami_lh();
   ATLAS_POINT_LIST *apl=NULL;
   
   ENTRY("set_adh_old_way");
   
   if (!aname) RETURN(1);   
      /* DO NOT CALL atlas_point_list or you will cause recursion
         with Atlas_With_Trimming */
   if (!(apl = atlas_point_list_old_way(aname))) {
      ERROR_message("Malheur de malheur >%s< nous sommes foutus!", aname);
      RETURN(1);      
   }   
   adh->apl2 = NULL;
    
          if (!strcmp(aname,"CA_N27_MPM")) {
     adh->mxlablen = ATLAS_CMAX;
     adh->probkey = -2;
     adh->apl2 = atlas_point_to_atlas_point_list(apl->at_point, apl->n_points);
     /* Are LR labels listed in adh->apl and under the same code?
       (only case I know of is in TTO_list - this will not be allowed */
     adh->duplicateLRentries = 0; 
   } else if(!strcmp(aname,"CA_N27_ML")) {
     /* Load the MacroLabels */
     adh->mxlablen = ATLAS_CMAX;
     adh->probkey = -1;
     adh->apl2 = atlas_point_to_atlas_point_list(apl->at_point, apl->n_points);
     adh->duplicateLRentries = 0;
   } else if(!strcmp(aname,"CA_N27_LR")) {     
     adh->mxlablen = ATLAS_CMAX;
     adh->probkey = -1;
     adh->apl2 = atlas_point_to_atlas_point_list(apl->at_point, apl->n_points);
     /* Are LR labels listed in adh->apl and under the same code? 
         (only case I know of is in TTO_list*/
     adh->duplicateLRentries = 0; 
   } else if(!strcmp(aname,"CA_N27_PM")) {  
     adh->mxlablen = ATLAS_CMAX;
     adh->maxkeyval = -1; /* not appropriate */
     adh->minkeyval = INT_MAX; /* not appropriate */
     adh->probkey = 0;
     if (wami_verb()) INFO_message("Daniel, why fill apl2 here? Would we store it in such a dset? Should we just include a reference to the atlas from which we should borrow the list?\n I see why you do it this way, and I think that is fine. But we need to be sure this does not flag such a dset as an ROI type dset in AFNI and messup the colorbar, etc.");
     adh->apl2= atlas_point_to_atlas_point_list(apl->at_point, apl->n_points);
               ; /* use cytoarchitectonic list for probability maps*/
     adh->duplicateLRentries = 0;
   } else if(!strcmp(aname,"TT_Daemon")) {   
     adh->mxlablen = ATLAS_CMAX;
     adh->probkey = -1;
     adh->apl2 = atlas_point_to_atlas_point_list(apl->at_point, apl->n_points);
     adh->duplicateLRentries = 1;
   } else {
     RETURN(1); /* no old type atlas like this */
   }

    if(adh->probkey == 0) RETURN(0);
    
    adh->maxkeyval = -1;
    adh->minkeyval = INT_MAX;
    for (ii=0; ii<MAX_ELM(adh->apl2); ++ii) {
       if(adh->apl2->at_point[ii].tdval > adh->maxkeyval)
           adh->maxkeyval = adh->apl2->at_point[ii].tdval;
       if(adh->apl2->at_point[ii].tdval < adh->minkeyval)
           adh->minkeyval = adh->apl2->at_point[ii].tdval;
    }

   RETURN(0);
}


/* return 1 if a key should have a label in the atlas */
byte is_atlas_key_labeled(ATLAS *atlas, int key) {
   if (!key) return(0);
   if (  key < atlas->adh->minkeyval ||
         key > atlas->adh->maxkeyval) return(0);
   else return(1);
}

/* return 1 if atlas has integer key labels, 0 otherwise*/
byte is_integral_atlas(ATLAS *atlas) {
   if (wami_verb()) {
      WARNING_message(
         "Daniel, what is the proper logic here? Check where I am using it ...");
   }
   /* New atlases should have apl2, but not old ones*/
   if (atlas->adh->apl2) return(1);
   return(0);
}

byte is_probabilistic_atlas(ATLAS *atlas) {
   if (wami_verb()) {
      WARNING_message(
         "Not sure that probkey decision is legitimate (%f, %p)\n"
         "What should we do here?",
                  atlas->adh->probkey, atlas->adh->apl2);
   }
   if (atlas->adh->probkey != 0.0) return(0);
   return(1);
}

/* return the label associated with a key, 
   see also Atlas_Val_Key_to_Val_Name */
char *atlas_key_label(ATLAS *atlas, int key) {
   char *klab = NULL;
   int ii;
   if( key != 0 ){            /* find label     */
      for( ii=0 ; ii < MAX_ELM(atlas->adh->apl2) ; ii++ ) { 
         if( key == atlas->adh->apl2->at_point[ii].tdval ) break ;
      }
      if( ii < MAX_ELM(atlas->adh->apl2) )  {          /* always true? */
         klab = atlas->adh->apl2->at_point[ii].name;
         /* 
            if( atcode == AFNI_TLRC_ATLAS) 
               klab = AddLeftRight( NoLeftRight(atlas->adh->apl2->at_point[ii].name),
                                 (ac.x<0.0)?'R':'L');
         */
      }
   }
   return(klab);
}

/* Return the label (and key) of the area corresponding
   to the sub-brick in the probability atlas */
char *prob_atlas_sb_to_label(ATLAS *atlas, int sb, int *key) 
{
   int i, nlab;
   char *lab_buf=NULL; /* no free please */
   
   ENTRY("prob_atlas_sb_to_label");
   
   *key = -1;
   
   if (!atlas->adh->apl2) {
      ERROR_message("Have no apl2");
      RETURN(NULL);
   }
   
   nlab = strlen(atlas->adh->adset->dblk->brick_lab[sb]);
   
   if (nlab > atlas->adh->mxlablen) {
      ERROR_message("Dset labels too long! Max allowed is %d, proceeding...",
                    atlas->adh->mxlablen);
   }
   
   for (i=0; i<atlas->adh->apl2->n_points; ++i) {
      lab_buf = Clean_Atlas_Label(atlas->adh->apl2->at_point[i].dsetpref);
      if ( (nlab == strlen(lab_buf)) && 
            !strcmp(lab_buf, atlas->adh->adset->dblk->brick_lab[sb])) {
         *key = atlas->adh->apl2->at_point[i].tdval;
         if (wami_verb()>1) {
            INFO_message(" Matched %s with %s\n", 
                     atlas->adh->adset->dblk->brick_lab[sb], 
                     atlas->adh->apl2->at_point[i].dsetpref);
         }
         break;
      }
   }
   if (*key >= 0) {
      RETURN(atlas->adh->apl2->at_point[i].name);
   }
   RETURN(NULL);
}

int Atlas_Voxel_Value(ATLAS *atlas, int sb, int ijk) 
{
   byte *ba=NULL;
   short *sa=NULL;
   float *fa=NULL, sbf=1.0;
   int ival = -1;
   
   switch(DSET_BRICK_TYPE(ATL_DSET(atlas), sb)) {
      case MRI_byte:
         ba = (byte *)DSET_ARRAY(ATL_DSET(atlas), sb);
         ival = (int)ba[ijk];
         break;
      case MRI_short:
         sa = (short *)DSET_ARRAY(ATL_DSET(atlas), sb);
         ival = (int)sa[ijk];
         break;
      case MRI_float:
         fa = (float *)DSET_ARRAY(ATL_DSET(atlas), sb);
         sbf = DSET_BRICK_FACTOR(ATL_DSET(atlas), sb); 
         if (sbf == 0.0) sbf = 1.0;
         ival = (int)(fa[ijk]*sbf);
         break;
      default:
         ERROR_message("Bad Atlas dset brick type %d\n",
                        DSET_BRICK_TYPE(ATL_DSET(atlas), sb)); 
         break;
   }
   return(ival);
   
}

/*!
   \brief Returns a whereami query from just one atlas
   \param atlas ATLAS * 
   \param Xrai (float[3]) x,y,z in RAI  
   \param wami append results to this wami
   \return wami the query results 
*/
int whereami_in_atlas(  char *aname, 
                        ATLAS_COORD ac, 
                        ATLAS_QUERY **wamip) 
{
   int nfind, *b_find=NULL, *rr_find=NULL ;
   int ii, kk, ix,jy,kz , nx,ny,nz,nxy ,sb=0;
   int aa,bb,cc , ff,baf,rff, ival=-1;
   char *blab ;
   ATLAS_ZONE *zn = NULL;
   THD_ivec3 ijk ;
   ATLAS *atlas=NULL;
   int LocalHead = wami_lh();
   
   ENTRY("whereami_in_atlas");
   
   if (!aname) {
      ERROR_message("No name");
      RETURN(0);
   }
   if (!wamip ) {
      ERROR_message("Need wamip != NULL");
      RETURN(0);
   }
   if (!(atlas = Atlas_With_Trimming(aname, 1, NULL))) {
      if (LocalHead) ERROR_message("Could not load atlas %s", aname);
      RETURN(0);
   }    
   
   if (LocalHead) {
      INFO_message("Wami on atlas %s (%d %d)\n", 
                atlas->atlas_name, 
                is_integral_atlas(atlas), is_probabilistic_atlas(atlas));
   }
   
   if (strcmp(atlas->atlas_space, ac.space_name)) {
      ERROR_message("Atlas space names mismatch: %s != %s",
                     atlas->atlas_space, ac.space_name);
      RETURN(0);
   }
   
   if (strncmp(ac.orcode, "RAI", 3)) {
      ERROR_message("AC orientation (%s) not RAI",
                     ac.orcode);
      RETURN(0);
   }
   
   
   if (MAX_FIND < 0) {
      Set_Whereami_Max_Find(MAX_FIND);
   }
   
   b_find = (int*)calloc(MAX_FIND, sizeof(int));
   rr_find = (int*)calloc(MAX_FIND, sizeof(int));
   if (!b_find || !rr_find) {
      ERROR_message( "Jimney Crickets!\nFailed to allocate for finds!\n"
                     "MAX_FIND = %d\n", MAX_FIND);
      RETURN(0);
   }

   if (!*wamip) { /* A new query structure, if necessary*/
      if (LocalHead) INFO_message("New wami");
      *wamip = Add_To_Atlas_Query(NULL, NULL);
   }

   if (LocalHead) {
      INFO_message("Coords: %f %f %f (%s, %s):\n", 
                     ac.x, ac.y, ac.z, ac.orcode, ac.space_name);
      print_atlas(atlas,0);
   }
   
   if ((Atlas_Name(atlas))[0] == '\0') {
      ERROR_message("An atlas with no name\n");
      print_atlas(atlas, 0);
      RETURN(0);
   }
   if (LocalHead)
      INFO_message(  "Now whereaming atlas %s, is_integral %d, is_prob %d\n",
                     atlas->atlas_dset_name, 
                     is_integral_atlas(atlas), is_probabilistic_atlas(atlas));

   if (  is_integral_atlas(atlas) && 
         !is_probabilistic_atlas(atlas)) { /* the multi-radius searches */
      for (sb=0; sb < DSET_NVALS(ATL_DSET(atlas)); ++sb) {
         if (LocalHead)
            fprintf(stderr,
               "Processing sub-brick %d of atlas %s\n",
               sb,  Atlas_Name(atlas));
         if (!DSET_BRICK_ARRAY(ATL_DSET(atlas),sb)) { 
            ERROR_message("Unexpected NULL array"); 
            RETURN(0); 
         }
         if (WAMIRAD < 0.0) {
            WAMIRAD = Init_Whereami_Max_Rad();
         }
         if( wamiclust == NULL ){
            wamiclust = MCW_build_mask( 1.0,1.0,1.0 , WAMIRAD ) ;
            if( wamiclust == NULL ) 
               RETURN(0) ;  /* should not happen! */

            for( ii=0 ; ii < wamiclust->num_pt ; ii++ ) /* set radius */
               wamiclust->mag[ii] = 
                  (int)rint(sqrt((double)
                        (wamiclust->i[ii]*wamiclust->i[ii] +
                         wamiclust->j[ii]*wamiclust->j[ii] +
                         wamiclust->k[ii]*wamiclust->k[ii]) )) ;

            MCW_sort_cluster( wamiclust ) ;  /* sort by radius */
         }

         /*-- find locations near the given one that are in the Atlas --*/
         ijk = THD_3dmm_to_3dind( ATL_DSET(atlas) , 
                                  TEMP_FVEC3(ac.x,ac.y,ac.z));
         UNLOAD_IVEC3(ijk,ix,jy,kz) ;                               

         nx = DSET_NX(ATL_DSET(atlas)) ;   /* size of atlas dataset axes */
         ny = DSET_NY(ATL_DSET(atlas)) ;
         nz = DSET_NZ(ATL_DSET(atlas)) ; nxy = nx*ny ;

         nfind = 0 ;

         /*-- check the exact input location --*/
         kk = ix + jy*nx + kz*nxy ;        /* index into brick arrays */
         b_find[0] = Atlas_Voxel_Value(atlas, sb, kk);
         if( is_atlas_key_labeled(atlas, b_find[0] )) {
            rr_find[0] = 0     ;
            if (LocalHead)  
               fprintf(stderr,"Adding b_find[%d]=%d rr_find[%d]=%d\n",
                           nfind, b_find[nfind], nfind, rr_find[nfind]);
            nfind++ ;
         }

         /*-- check locations near it --*/

         for( ii=0 ; ii < wamiclust->num_pt ; ii++ ){

            /* compute index of nearby location, skipping if outside atlas */

            aa = ix + wamiclust->i[ii] ; 
               if( aa < 0 || aa >= nx ) continue ;
            bb = jy + wamiclust->j[ii] ; 
               if( bb < 0 || bb >= ny ) continue ;
            cc = kz + wamiclust->k[ii] ; 
               if( cc < 0 || cc >= nz ) continue ;

            kk  = aa + bb*nx + cc*nxy ;   /* index into bricks */
            baf = Atlas_Voxel_Value(atlas, sb, kk) ; /* Atlas marker there */

            if( baf == 0 )                            continue ;

            for( ff=0 ; ff < nfind ; ff++ ){       /* cast out         */
               if( baf == b_find[ff] ) baf = 0 ;  /* duplicate labels  */
            }

            if (!is_atlas_key_labeled(atlas, baf)) baf = 0;

            if( baf == 0 )                            continue ;

            b_find[nfind] = baf ;  /* save what we found */
            rr_find[nfind] = (int) wamiclust->mag[ii] ;
            if (LocalHead)  
               fprintf(stderr,"Adding b_find[%d]=%d rr_find[%d]=%d\n",
                           nfind, b_find[nfind], nfind, rr_find[nfind]);
            nfind++ ;

            if( nfind == MAX_FIND ) {
              if (!getenv("AFNI_WHEREAMI_NO_WARN")) {
               INFO_message(
         "Potentially more regions could be found than the %d reported.\n"
         "Set the environment variable AFNI_WHEREAMI_MAX_FIND to higher\n"
         "than %d if you desire a larger report.\n"
         "It behooves you to also checkout AFNI_WHEREAMI_MAX_SEARCH_RAD\n"
         "and AFNI_WHEREAMI_NO_WARN. See whereami -help for detail.\n", 
                                 MAX_FIND, MAX_FIND);
              }
              break ;  /* don't find TOO much */
            }
         }

         /*-- bubble-sort what we found, by radius --*/

         if( nfind > 1 ){  /* don't have to sort only 1 result */
           int swap, tmp ;
           do{
              swap=0 ;
              for( ii=1 ; ii < nfind ; ii++ ){
                 if( rr_find[ii-1] > rr_find[ii] ){
                   tmp = rr_find[ii-1]; 
                     rr_find[ii-1] = rr_find[ii]; 
                        rr_find[ii] = tmp;
                   tmp = b_find[ii-1]; 
                     b_find[ii-1] = b_find[ii]; 
                        b_find[ii] = tmp;
                   swap++ ;
                 }
              }
           } while(swap) ;
         }

         /* build query results */
         rff = -1 ;  /* rff = radius of last found label */

         if (LocalHead) INFO_message("   %d findings...\n", nfind);

         for( ff=0 ; ff < nfind ; ff++ ){
            baf = b_find[ff] ; blab = NULL ;
            blab = atlas_key_label(atlas, baf);
            if( blab == NULL && is_atlas_key_labeled(atlas, baf)) {
               WARNING_message(
                  "No label found for code %d in atlas %s\nContinuing...", 
                               baf, Atlas_Name(atlas));
               continue ;  /* no labels? */
            }

            zn = Get_Atlas_Zone (*wamip, (int)rr_find[ff] ); 
                     /* zone levels are based on search radius */
            zn = Atlas_Zone(  zn, zn->level,
                              blab, baf, atlas->adh->probkey, rr_find[ff], 
                              Atlas_Name(atlas));
            if (LocalHead) 
               INFO_message("Adding zone on %s to wami\n", 
                               Atlas_Name(atlas)); 
            *wamip = Add_To_Atlas_Query(*wamip, zn);

            rff = rr_find[ff] ;  /* save for next time around */
         }
      } /* for each sub-brick */
   } else if (is_probabilistic_atlas(atlas)) { /* the PMAPS */
      if (LocalHead)  
         fprintf(stderr,"Processing with %s\n", 
                        atlas->atlas_dset_name);

      /*-- find locations near the given one that are in the Atlas --*/
      ijk = THD_3dmm_to_3dind( ATL_DSET(atlas) , TEMP_FVEC3(ac.x,ac.y,ac.z) ) ; 
      UNLOAD_IVEC3(ijk,ix,jy,kz) ;                               

      nx = DSET_NX(ATL_DSET(atlas)) ;        /* size of atlas dataset axes */
      ny = DSET_NY(ATL_DSET(atlas)) ;
      nz = DSET_NZ(ATL_DSET(atlas)) ; nxy = nx*ny ;
      kk = ix + jy*nx + kz*nxy ;        /* index into brick arrays */

      zn = Get_Atlas_Zone(*wamip, 0);    /* get the zero level zone */
      for (sb=0; sb<DSET_NVALS(ATL_DSET(atlas)); ++sb) {
         if (!DSET_BRICK_ARRAY(ATL_DSET(atlas),sb)) { 
            ERROR_message("Unexpected NULL array"); 
            RETURN(0); 
         }
         ival = Atlas_Voxel_Value(atlas, sb, kk);
         if (LocalHead)  
            fprintf(stderr,"  ++ Sub-brick %d in %s ival=%d\n", 
                           sb, atlas->atlas_dset_name, ival);
         if( ival != 0 ){
            if( atlas->adh->adset->dblk->brick_lab == NULL || 
                atlas->adh->adset->dblk->brick_lab[sb] == NULL) {
               if (LocalHead)  fprintf(stderr,"  ++ No Label!\n");
               if (wami_verb()) {
                  INFO_message("Daniel, this division by 250 is only"
                              "for the Zilles dsets, this should be"
                              "handled via a scaling factor in the dset"
                              "perhaps...Check for it elsewhere.");
               }
               zn = Atlas_Zone(zn, 0, "No Label", -1, 
                              (float)ival/250.0, 0, 
                              Atlas_Name(atlas));
            } else {
               if( atlas->adh->adset->dblk->brick_lab[sb] && 
                   atlas->adh->adset->dblk->brick_lab[sb][0] != '\0' ){
                  blab = prob_atlas_sb_to_label(atlas, sb, &baf); 
                  if (blab) {
                     if (LocalHead) fprintf(stderr," blabing: %s\n", blab);
                     zn = Atlas_Zone(zn, 0, blab, baf , 
                                    (float)ival/250.0, 0, 
                            Atlas_Name(atlas));
                  } else {
                     if (LocalHead) fprintf(stderr," no blabing:\n");
                     zn = Atlas_Zone(  zn, 0, "Unexpected trouble.", 
                                       -1, -1.0, 0, 
                            Atlas_Name(atlas));
                  }
               } else {
                  zn = Atlas_Zone(zn, 0, "Empty Label", -1, 
                                  (float)ival/250.0, 0,
                           Atlas_Name(atlas));
               }
            }
            *wamip = Add_To_Atlas_Query(*wamip, zn);
         }
      }
   } else {
      ERROR_message("dunno what to do for atlas %s\n", 
                     atlas->atlas_dset_name);
      RETURN(0);
   }
   /* Show_Atlas_Query(wami); */
   
   free(b_find); b_find = NULL; free(rr_find); rr_find = NULL;

   RETURN(1);  
}

/*!
   \brief A newer version of whereami_9yards
   \param asl (atlas_space_list *) list of atlas spaces to query
               If null, then function determines all those
               accessible 
*/
int whereami_3rdBase( ATLAS_COORD aci, ATLAS_QUERY **wamip,
                        ATLAS_SPACE_LIST *asli, ATLAS_LIST *aali)
{
   ATLAS_QUERY *wami = NULL;
   ATLAS_COORD ac;
   ATLAS_XFORM_LIST *xfl=NULL;
   ATLAS_SPACE_LIST *asl=get_G_space_list();
   ATLAS *atlas=NULL;
   int *iatl=NULL, ii;
   int N_iatl=0, ia=0;
   float xout=0.0, yout=0.0, zout = 0.0;
   int LocalHead = wami_lh();

   ENTRY("whereami_3rdBase");
   /* initalized ? */
   if (!aali) aali = get_G_atlas_list();
   if (!aali || aali->natlases == 0) {
      ERROR_message("No atlas_alist, or empty one.");
      RETURN(0);
   }
   
   /* find list of atlases whose spaces are reachable from aci.spacename */
   if (LocalHead) {
      print_atlas_list(aali); print_space_list(asl); 
   }
   for (ia=0; ia<aali->natlases; ++ia) {
      xfl = report_xform_chain(aali->atlas[ia].atlas_space, aci.space_name, 0);
      if (xfl) {
         ++N_iatl;
         iatl = (int*)realloc(iatl, N_iatl*sizeof(int));
         iatl[N_iatl-1]=ia;
         free_xform_list(xfl); xfl=NULL; 
      }
   }
   if (LocalHead) {
      INFO_message("Have %d reachable atlases\n", N_iatl);
   }
   if (N_iatl<1) {
      ERROR_message("No reachable atlases from %s\n", aci.space_name);
      RETURN(0);
   }
   
   for (ia=0; ia<N_iatl; ++ia) { /* for each reachable, get query */ 
      atlas = &(aali->atlas[iatl[ia]]);
      /* get xform, and apply it to coords at input */
      if (!(xfl = report_xform_chain(aci.space_name, atlas->atlas_space, 0))) {
         ERROR_message("Should not happen here");
         RETURN(0);
      }
      apply_xform_chain(xfl, aci.x, aci.y, aci.z, &xout, &yout, &zout);
      if (wami_verb() > 1)
         INFO_message(
           "Coords in: %f, %f, %f (%s) -> out: %f, %f, %f (%s - %s)\n",
             aci.x,aci.y,aci.z, aci.space_name, xout,yout,zout, 
             Atlas_Name(atlas),
             atlas->atlas_space);
      
      XYZ_to_AtlasCoord(xout, yout, zout, "RAI", atlas->atlas_space, &ac);

      if (!whereami_in_atlas(Atlas_Name(atlas), ac , &wami)) {
         if (LocalHead) 
            INFO_message("Failed at whereami for %s", Atlas_Name(atlas));
      }
   }
   
   /* sort the query by zone levels, be nice */
   if( wami && wami->N_zone > 1 ){  /* don't have to sort only 1 result */
     int swap;
     ATLAS_ZONE *tmp ;
     do{
        swap=0 ;
        for( ii=1 ; ii < wami->N_zone ; ii++ ){
           if( wami->zone[ii-1]->level > wami->zone[ii]->level ){
             tmp = wami->zone[ii-1];
             wami->zone[ii-1] = wami->zone[ii];
             wami->zone[ii] = tmp;
             swap++ ;
           }
        }
     } while(swap) ;
   }


   if (LocalHead) {
      Show_Atlas_Query(wami,aali);
   }

   *wamip = wami;

   RETURN(1);
}

int whereami_9yards(  ATLAS_COORD aci, ATLAS_QUERY **wamip,
                      ATLAS_LIST *atlas_alist)
{
   int   ii,kk , ix,jy,kz , nx,ny,nz,nxy ,
         aa,bb,cc , ff,baf,rff, iatlas=0, sb = 0 ;
   THD_ivec3 ijk ;
   short *ba = NULL ;
   byte *bba = NULL ;
   float *fba = NULL;
   char *blab ;
   int nfind, *b_find=NULL, *rr_find=NULL ;
   ATLAS_QUERY *wami = NULL;
   ATLAS_ZONE *zn = NULL;
   THD_fvec3 vn3, vo3;
   ATLAS_COORD ac;
   ATLAS *atlas=NULL;
   int LocalHead = wami_lh();
   int dset_kind;
   float fbaf, fbaf_factor;
   static int iwarn = 0;
   
   ENTRY("whereami_9yards");

   if (0 && *wamip) { /* Could be building on other wamis */
      ERROR_message("Send me a null wamip baby\n");
      RETURN(0);
   }

   if (!atlas_alist || atlas_alist->natlases == 0) {
      ERROR_message("Send me a non null or non empty atlaslist\n");
      RETURN(0);
   }

   /* check on coord system (!!have to change coord system depending on atlas) */
   if (wami_verb()) {
      INFO_message("Using the old coord xform method, space name >%s<", 
                aci.space_name);
   }
   if (is_Coord_Space_Named(aci, "MNI_ANAT")) {
      LOAD_FVEC3(vo3, aci.x, aci.y, aci.z);
      vn3 = THD_mnia_to_tta_N27(vo3);
      ac.x = vn3.xyz[0]; ac.y = vn3.xyz[1]; ac.z = vn3.xyz[2];
      set_Coord_Space_Name(&ac, aci.space_name);
   } else if (is_Coord_Space_Named(aci, "MNI")) {
      LOAD_FVEC3(vo3, aci.x, aci.y, aci.z);
      vn3 = THD_mni_to_tta_N27(vo3);
      ac.x = vn3.xyz[0]; ac.y = vn3.xyz[1]; ac.z = vn3.xyz[2];
      set_Coord_Space_Name(&ac, aci.space_name);
   } else if (is_Coord_Space_Named(aci, "TLRC")) {
      ac.x = aci.x; ac.y = aci.y; ac.z = aci.z;
      set_Coord_Space_Name(&ac, aci.space_name);
   } else {
      ERROR_message("Coordinates in bad space %s.", aci.space_name);
      print_atlas_coord(aci);
      RETURN(0);
   }

   if (MAX_FIND < 0) {
      Set_Whereami_Max_Find(MAX_FIND);
   }
   b_find = (int*)calloc(MAX_FIND, sizeof(int));
   rr_find = (int*)calloc(MAX_FIND, sizeof(int));
   if (!b_find || !rr_find) {
      ERROR_message( "Jimney Crickets!\nFailed to allocate for finds!\n"
                     "MAX_FIND = %d\n", MAX_FIND);
      RETURN(0);
   }

   if (!*wamip) { /* A new query structure, if necessary*/
      if (LocalHead) INFO_message("New wami");
      wami = Add_To_Atlas_Query(NULL, NULL);
   }

   if (LocalHead)
      INFO_message("Coords: %f %f %f (%s)\n", ac.x, ac.y, ac.z, ac.space_name);

   for (iatlas=0; iatlas < atlas_alist->natlases; ++iatlas) {/* iatlas loop */
      if (wami_verb())
         INFO_message(  "Now Processing atlas %s (%d)",
                        atlas_alist->atlas[iatlas].atlas_dset_name);

      if (!(atlas = Atlas_With_Trimming(  atlas_alist->atlas[iatlas].atlas_name,
                                          1, atlas_alist))) {
         if (wami_verb()) {
            if (!iwarn || wami_verb() > 1) {
               INFO_message("No atlas dataset %s found for whereami location"
                            "%s",
                       atlas_alist->atlas[iatlas].atlas_name,
                        wami_verb() < 2 ? 
                           "\nWarnings for other atlases will be muted.":"");
               ++iwarn;
            }
         }
         continue;
      }

      dset_kind = (int)DSET_BRICK_TYPE(ATL_DSET(atlas),0) ;

      /* the multi-radius searches - not for probability maps*/
      if(atlas->adh->probkey!=0) { 
            if(dset_kind != MRI_short && dset_kind != MRI_byte ) {
               ERROR_message("Atlas dataset %s may only be byte or short," 
                             "not data type '%s'",
                  DSET_BRIKNAME(ATL_DSET(atlas)) , MRI_TYPE_name[dset_kind] ) ;
               RETURN(0);
            }

         for (sb=0; sb < DSET_NVALS(ATL_DSET(atlas)); ++sb) {
            if (LocalHead)
               fprintf(stderr,"Processing sub-brick %d with %s\n",
                       sb, atlas->atlas_dset_name);
            /* make dataset sub-brick integer - change from previous byte
                to allow values > 255 */
            if(dset_kind == MRI_short) {
               ba = DSET_BRICK_ARRAY(ATL_DSET(atlas),sb); /* short type */
               if (!ba) { ERROR_message("Unexpected NULL array"); RETURN(0); }
            }
            else {
               bba = DSET_BRICK_ARRAY(ATL_DSET(atlas),sb); /* byte array */
               if (!bba) { ERROR_message("Unexpected NULL array"); RETURN(0); }
               if (LocalHead) {
		            fprintf(stderr,"++ have bba = %p, kind = %d\n", 
                     bba, DSET_BRICK_TYPE(ATL_DSET(atlas),sb));
                  fprintf(stderr,"   (byte = %d, short = %d)\n", 
                     MRI_byte, MRI_short);
               }
            }

            if (WAMIRAD < 0.0) {
               WAMIRAD = Init_Whereami_Max_Rad();
            }
            if( wamiclust_CA_EZ == NULL ){
               wamiclust_CA_EZ = MCW_build_mask( 1.0,1.0,1.0 , WAMIRAD ) ;
               if( wamiclust_CA_EZ == NULL ) 
                  RETURN(0) ;  /* should not happen! */

               for( ii=0 ; ii < wamiclust_CA_EZ->num_pt ; ii++ ) /* set radius */
                  wamiclust_CA_EZ->mag[ii] = 
                     (int)rint(sqrt((double)
                           (wamiclust_CA_EZ->i[ii]*wamiclust_CA_EZ->i[ii] +
                            wamiclust_CA_EZ->j[ii]*wamiclust_CA_EZ->j[ii] +
                            wamiclust_CA_EZ->k[ii]*wamiclust_CA_EZ->k[ii]) )) ;

               MCW_sort_cluster( wamiclust_CA_EZ ) ;  /* sort by radius */
            }

            /*-- find locations near the given one that are in the Atlas --*/
            ijk = THD_3dmm_to_3dind( ATL_DSET(atlas) , 
                                     TEMP_FVEC3(ac.x,ac.y,ac.z) ) ;
            UNLOAD_IVEC3(ijk,ix,jy,kz) ;                               

            nx = DSET_NX(ATL_DSET(atlas)) ;    /* size of atlas dataset axes */
            ny = DSET_NY(ATL_DSET(atlas)) ;
            nz = DSET_NZ(ATL_DSET(atlas)) ; nxy = nx*ny ;

            nfind = 0 ;

            /*-- check the exact input location --*/

            kk = ix + jy*nx + kz*nxy ;        /* index into brick arrays */

            if(dset_kind == MRI_short)
              baf = ba[kk];
            else {
              baf = bba[kk];
              if (LocalHead)  fprintf(stderr,
   "Byte value at focus point %d, %d, %d, %f, %f, %f, %d, %d, %d, %d, baf=%d\n", 
                    ix,jy,kz, ac.x, ac.y, ac.z, nx, ny, nz, kk, baf);
            }

            if( baf != 0){
               b_find[0] = baf ;
               rr_find[0] = 0     ;
               if (LocalHead)  
                  fprintf(stderr,"Adding b_find[%d]=%d rr_find[%d]=%d\n",
                              nfind, b_find[nfind], nfind, rr_find[nfind]);
               nfind++ ;
            }
            else{
               if (LocalHead)  fprintf(stderr,
                  "No value at focus point %d, %d, %d, ba=%d\n", ix,jy,kz, baf);
            }
            /*-- check locations near it --*/

            for( ii=0 ; ii < wamiclust_CA_EZ->num_pt ; ii++ ){

               /* compute index of nearby location, skipping if outside atlas */

               aa = ix + wamiclust_CA_EZ->i[ii] ; 
                  if( aa < 0 || aa >= nx ) continue ;
               bb = jy + wamiclust_CA_EZ->j[ii] ; 
                  if( bb < 0 || bb >= ny ) continue ;
               cc = kz + wamiclust_CA_EZ->k[ii] ; 
                  if( cc < 0 || cc >= nz ) continue ;

               kk  = aa + bb*nx + cc*nxy ;   /* index into bricks */

               /* Atlas structure marker there - value at coordinate in atlas */
               if(dset_kind == MRI_short)
                 baf = ba[kk];
               else
                 baf = bba[kk];


               if( baf == 0 )                            continue ;

               for( ff=0 ; ff < nfind ; ff++ ){       /* cast out         */
                  if( baf == b_find[ff] ) baf = 0 ;  /* duplicate labels  */
               }

               if( baf == 0 )                            continue ;

               b_find[nfind] = baf ;  /* save what we found */
               rr_find[nfind] = (int) wamiclust_CA_EZ->mag[ii] ;
               if (LocalHead)  
                  fprintf(stderr,"Adding b_find[%d]=%d rr_find[%d]=%d\n",
                              nfind, b_find[nfind], nfind, rr_find[nfind]);
               nfind++ ;

               if( nfind == MAX_FIND ) {
                 if (wami_verb() || !getenv("AFNI_WHEREAMI_NO_WARN")) {
                  INFO_message(
            "Potentially more regions could be found than the %d reported.\n"
            "Set the environment variable AFNI_WHEREAMI_MAX_FIND to higher\n"
            "than %d if you desire a larger report.\n"
            "It behooves you to also checkout AFNI_WHEREAMI_MAX_SEARCH_RAD\n"
            "and AFNI_WHEREAMI_NO_WARN. See whereami -help for detail.\n", 
                                    MAX_FIND, MAX_FIND);
                 }
                 break ;  /* don't find TOO much */
               }
            }

            /*-- bubble-sort what we found, by radius --*/

            if( nfind > 1 ){  /* don't have to sort only 1 result */
              int swap, tmp ;
              do{
                 swap=0 ;
                 for( ii=1 ; ii < nfind ; ii++ ){
                    if( rr_find[ii-1] > rr_find[ii] ){
                      tmp = rr_find[ii-1]; 
                        rr_find[ii-1] = rr_find[ii]; 
                           rr_find[ii] = tmp;
                      tmp = b_find[ii-1]; 
                        b_find[ii-1] = b_find[ii]; 
                           b_find[ii] = tmp;
                      swap++ ;
                    }
                 }
              } while(swap) ;
            }

            /* build query results */
            rff = -1 ;  /* rff = radius of last found label */

            if (LocalHead) 
               INFO_message("   %d findings on atlas named %s ...\n", 
                            nfind, Atlas_Name(atlas) );

            for( ff=0 ; ff < nfind ; ff++ ){
               baf = b_find[ff] ; blab = NULL ;
               blab = atlas_key_label(atlas, baf);

               if( blab == NULL && 
                   is_atlas_key_labeled(atlas,baf) ) {
                  if (LocalHead)
                     WARNING_message(
                       "No label found for code %d in atlas %s\n"
                       "Continuing...", 
                       baf, atlas->atlas_dset_name);
                  continue ;  /* no labels? */
               }
               /* zone levels are based on search radius */
               if(LocalHead)
                   INFO_message("Getting atlas %s zone %d for finds", 
                                Atlas_Name(atlas), (int)rr_find[ff]);
               zn = Get_Atlas_Zone (wami, (int)rr_find[ff] ); 
               if(LocalHead)
                   INFO_message("Adding zone to query results (%d, %s, %d)",
                                 baf, STR_PRINT(blab), 
                                 is_atlas_key_labeled(atlas,baf));
               zn = Atlas_Zone(  zn, zn->level,
                     blab, baf, (float) atlas->adh->probkey, 
                     rr_find[ff], Atlas_Name(atlas));
               
               wami = Add_To_Atlas_Query(wami, zn);
               rff = rr_find[ff] ;  /* save for next time around */
            }
         } /* for each sub-brick */
      } else { /* the PMAPS */
         if (LocalHead)  fprintf(stderr,
            "Processing with %s for probability maps\n",
            atlas->atlas_dset_name);

         /*-- find locations near the given one that are in the Atlas --*/
         ijk = THD_3dmm_to_3dind( ATL_DSET(atlas) , 
                                  TEMP_FVEC3(ac.x,ac.y,ac.z) ) ; 
         UNLOAD_IVEC3(ijk,ix,jy,kz) ;                               

         nx = DSET_NX(ATL_DSET(atlas)) ;   /* size of atlas dataset axes */
         ny = DSET_NY(ATL_DSET(atlas)) ;
         nz = DSET_NZ(ATL_DSET(atlas)) ; nxy = nx*ny ;
         kk = ix + jy*nx + kz*nxy ;        /* index into brick arrays */

         zn = Get_Atlas_Zone(wami, 0);    /* get the zero level zone */
         for (ii=0; ii<DSET_NVALS(ATL_DSET(atlas)); ++ii) {
            /* make dataset sub-brick integer - 
               change from previous byte to allow values > 255 */
            switch(dset_kind) {
              case MRI_short :
                 ba = DSET_BRICK_ARRAY(ATL_DSET(atlas),ii); /* short type */
                 if (!ba) { ERROR_message("Unexpected NULL array"); RETURN(0); }
                 fbaf = ba[kk];
                 fbaf_factor = 250.0;
                 break;
              case MRI_byte :
                 bba = DSET_BRICK_ARRAY(ATL_DSET(atlas),ii); /* byte array */
                 if (!bba) { ERROR_message("Unexpected NULL array"); RETURN(0); }
                 fbaf = bba[kk];
                 fbaf_factor = 250.0;
                 break;
              case MRI_float :
                 fba = DSET_BRICK_ARRAY(ATL_DSET(atlas),ii); /* float array */
                 if (!fba) { ERROR_message("Unexpected NULL array"); RETURN(0); }
                 fbaf = fba[kk];
                 fbaf_factor = 1.0;  /* just to use as a floating point prob. */
                 break;
              default :
                 ERROR_message("Unexpected data type for probability maps"); 
                 RETURN(0);
            }
            if (LocalHead)  fprintf(stderr,
                              "  ++ Sub-brick %d in %s ba[kk]=%d\n",
                              ii, atlas->atlas_dset_name,
                              (int)fbaf);
            if( fbaf != 0 ){
               if( atlas->adh->adset->dblk->brick_lab == NULL ||
                   atlas->adh->adset->dblk->brick_lab[ii] == NULL) {
                  if (LocalHead)  fprintf(stderr,"  ++ No Label!\n");
                  zn = Atlas_Zone(zn, 0, "No Label", -1, 
                        (float)ba[kk]/250.0, 0, Atlas_Name(atlas));
               } else {
                  if( atlas->adh->adset->dblk->brick_lab[ii] && 
                      atlas->adh->adset->dblk->brick_lab[ii][0] != '\0' ){
                     if (LocalHead)  
                        fprintf(stderr,
                              "  ++ Checking area label against sub-brick.\n");
                     blab = prob_atlas_sb_to_label(atlas, ii, &baf);
                     if (blab) {
                        if (LocalHead) fprintf(stderr," blabing: %s\n", blab);
                        zn = Atlas_Zone(  zn, 0, blab, baf , 
                                          (float)fbaf/250.0, 0, 
                                          Atlas_Name(atlas));
                     } else {
                        if (LocalHead) fprintf(stderr," no blabing:\n");
                        zn = Atlas_Zone(zn, 0, "Unexpected trouble.",
                              -1, -1.0, 0, Atlas_Name(atlas));
                     }
                  } else {
                     zn = Atlas_Zone(zn, 0, "Empty Label", -1,
                             (float)fbaf/fbaf_factor, 0, Atlas_Name(atlas));
                  }
               }
               wami = Add_To_Atlas_Query(wami, zn);
            }
         }

      }
      /* Show_Atlas_Query(wami); */
   } /* iatlas loop */


   /* sort the query by zone levels, be nice */
   if( wami && wami->N_zone > 1 ){  /* don't have to sort only 1 result */
     int swap;
     ATLAS_ZONE *tmp ;
     do{
        swap=0 ;
        for( ii=1 ; ii < wami->N_zone ; ii++ ){
           if( wami->zone[ii-1]->level > wami->zone[ii]->level ){
             tmp = wami->zone[ii-1];
             wami->zone[ii-1] = wami->zone[ii];
             wami->zone[ii] = tmp;
             swap++ ;
           }
        }
     } while(swap) ;
   }


   if (LocalHead) {
      Show_Atlas_Query(wami, atlas_alist);
   }

   *wamip = wami;

   free(b_find); b_find = NULL; free(rr_find); rr_find = NULL;
   RETURN(1);
}

THD_3dim_dataset *THD_3dim_G_from_ROIstring(char *shar) {
   return(THD_3dim_from_ROIstring(shar, get_G_atlas_list()));
}

THD_3dim_dataset *THD_3dim_from_ROIstring(char *shar, ATLAS_LIST *atlas_list)
{
   THD_3dim_dataset *maskset = NULL;
   AFNI_ATLAS_REGION *aar= NULL;
   AFNI_ATLAS *aa = NULL;
   ATLAS_SEARCH *as=NULL;
   char *string=NULL;
   int nbest = 0, codes[3], n_codes;
   int LocalHead = wami_lh();

   ENTRY("THD_3dim_from_ROIstring");

   if (!shar) RETURN(maskset);
   if (strlen(shar) < 3) RETURN(maskset);
   Set_ROI_String_Decode_Verbosity(0); /* must be discreet here */

   if (!(aar = ROI_String_Decode(shar, atlas_list))) {
      if (LocalHead) ERROR_message("ROI string decoding failed.");
      RETURN(maskset);
   }

   if (LocalHead) {
      fprintf( stderr,
               "User seeks the following region in atlas %s:\n", 
               aar->atlas_name);
      Show_Atlas_Region(aar);
   }

   /* is this an OK atlas */
   if (!get_Atlas_Named(aar->atlas_name, atlas_list)) {
      if (LocalHead) ERROR_message("Atlas not found");
      RETURN(maskset);
   }
   if (aar->N_chnks < 1 && aar->id <= 0) {
      if (LocalHead) ERROR_message("bad or empty label");
      RETURN(maskset);
   }
   if (!(aa = Build_Atlas(aar->atlas_name, atlas_list))) {
      if (LocalHead) ERROR_message("Failed to build atlas");
      RETURN(maskset);
   }
   if (wami_verb() > 1) Show_Atlas(aa);
   as = Find_Atlas_Regions(aa,aar, NULL);

   /* analyze the matches,*/
   string = Report_Found_Regions(aa, aar, as, &nbest);
   if (string) {
      if (LocalHead) fprintf(stderr,"%s\n", string);
      free(string); string = NULL;
   } else {
      if (LocalHead) ERROR_message("NULL string returned"); 
            /* something went wrong, although I care not for string ... */
      RETURN(maskset);
   }
   /* Now we know what matches, give me a mask */
   if (nbest) {
      if (nbest > 2) {
         ERROR_message( "More than 2 choices available. I am not used to this.\n"
                        "Please post a message explaining how you generated \n"
                        "this on the AFNI message board");
         RETURN(maskset);
      }
      n_codes = 1;
      codes[0] = aa->reg[as->iloc[0]]->id;
      if (nbest == 2) {
         if (aa->reg[as->iloc[0]]->id != aa->reg[as->iloc[1]]->id) {
            n_codes = 2;
            codes[1] = aa->reg[as->iloc[1]]->id;
         }
      }
      if (!(maskset = Atlas_Region_Mask(aar, codes, 
                                        n_codes, atlas_list))) {
         ERROR_message("Failed to create mask");
         RETURN(maskset);
      }
   }

   if (aar) aar = Free_Atlas_Region(aar);
   if (as) as = Free_Atlas_Search(as);
   if (string) free(string); string = NULL;
   if (aa) aa = Free_Atlas(aa);

   RETURN(maskset);
}

int find_in_names_list(char **nl, int N_nl, char *name) {
   int i = -1;
   
   if (!name || !nl || N_nl < 1) return(i);
   for (i=0; i<N_nl; ++i) {
      if (nl[i] && !strcmp(nl[i],name)) return(i);
   }
   return(-1);
}

char **add_to_names_list(char **nl, int *N_nl, char *name) {
   int i;
   
   if (!name) return(nl);
   
   if (!nl) *N_nl = 0;
   if (find_in_names_list(nl, *N_nl, name) >= 0) return(nl); /* got it already */
   
   /* new one */
   nl = (char **)realloc(nl, (*N_nl+1)*sizeof(char *));
   nl[*N_nl] = nifti_strdup(name);
   *N_nl = *N_nl+1;
   
   return(nl);  
}

char **free_names_list(char **nl, int N_nl) {
   int i;
   if (!nl) return(NULL);
   for (i=0; i<N_nl; ++i) {
      if (nl[i]) free(nl[i]);
   }
   free(nl[i]); 
   return(NULL);
}

int atlas_n_points(char *atname) {
   ATLAS *atlas;
   if (!(atlas = Atlas_With_Trimming (atname, 1, NULL)) || !ATL_ADH_SET(atlas)) {
      if (wami_verb()) 
         ERROR_message("Failed getting atlas for n_points");
      if (wami_verb()) 
         WARNING_message("Old style n_points retrieval for %s", atname);
      if (!strcmp(atname,"TT_Daemon")) { 
         return(TTO_COUNT_HARD);
      } if (!strcmp(atname,"CA_N27_MPM") ||
            !strcmp(atname,"CA_N27_PM") ) { 
         return(CA_EZ_COUNT_HARD);
      } if (!strcmp(atname,"CA_N27_LR")) {
         return(LR_EZ_COUNT_HARD);
      } if (!strcmp(atname,"CA_N27_ML")) {
         return(ML_EZ_COUNT_HARD);
      }
      return(0);
   }
   return(atlas->adh->apl2->n_points);
}

ATLAS_POINT *atlas_points(char *atname) {
   ATLAS *atlas;
   if (!(atlas = Atlas_With_Trimming (atname, 1, NULL)) || !ATL_ADH_SET(atlas)) {
      if (wami_verb()) 
         ERROR_message("Failed getting atlas for atlas_points");
      if (wami_verb()) 
         WARNING_message("Old style atlas_points retrieval for %s", atname);
      if (!strcmp(atname,"TT_Daemon")) { 
         return(TTO_list_HARD);
      } else if ( !strcmp(atname,"CA_N27_MPM") ||
                  !strcmp(atname,"CA_N27_PM") ) { 
         return(CA_EZ_list_HARD);
      } else if (!strcmp(atname,"CA_N27_LR")) { 
         return(LR_EZ_list_HARD);
      } else if (!strcmp(atname,"CA_N27_ML")) { 
         return(ML_EZ_list_HARD);
      }
      return(NULL);
   }
   return(atlas->adh->apl2->at_point);
}

ATLAS_POINT_LIST *atlas_point_list_old_way(char *atname) 
{
   static ATLAS_POINT_LIST apl[1];
   
   if (wami_verb()) 
         WARNING_message("Old style atlas_point_list_old_way for %s", atname);
         
   if (!strcmp(atname,"TT_Daemon")) { 
      apl->at_point = TTO_list_HARD;
      apl->n_points = TTO_COUNT_HARD;
      return(apl);
   } else if ( !strcmp(atname,"CA_N27_MPM")||
               !strcmp(atname,"CA_N27_PM") ) { 
      apl->at_point = CA_EZ_list_HARD;
      apl->n_points = CA_EZ_COUNT_HARD;
      return(apl);
   }  else if (!strcmp(atname,"CA_N27_LR")) { 
      apl->at_point = LR_EZ_list_HARD;
      apl->n_points = LR_EZ_COUNT_HARD;
      return(apl);
   }  else if (!strcmp(atname,"CA_N27_ML")) { 
      apl->at_point = ML_EZ_list_HARD;
      apl->n_points = ML_EZ_COUNT_HARD;
      return(apl);
   }
   return(NULL);
}

ATLAS_POINT_LIST *atlas_point_list(char *atname) 
{
   ATLAS *atlas;
   
   if (!(atlas = Atlas_With_Trimming (atname, 1, NULL)) || !ATL_ADH_SET(atlas)) {
      if (wami_verb()) 
         ERROR_message("Failed getting atlas for atlas_point_list\n");
      return(atlas_point_list_old_way(atname));
   }
   return(atlas->adh->apl2);
}

char *atlas_version_string(char *atname) {
   ATLAS *atlas;
   
   if (wami_verb()) {
      WARNING_message(
         "Daniel: I don't know how to get at this in the modern way.\n"
         "We need to revisit version and reference options...");
   }
      
   if (1 || !(atlas = Atlas_With_Trimming(atname, 1, NULL))) {
      if (wami_verb()) 
         ERROR_message("Failed getting atlas for atlas_version_string");
      if (!strcmp(atname,"CA_N27_MPM") ||
          !strcmp(atname,"CA_N27_PM")  ||
          !strcmp(atname,"CA_N27_LR") ||
          !strcmp(atname,"CA_N27_ML")) { 
         if (wami_verb()) 
            WARNING_message("Old style retrieval of version string for %s", 
                           atname);
         return(CA_EZ_VERSION_STR_HARD);
      }
   }
   
   return(NULL);
}

char **atlas_reference_string_list(char *atname, int *N_refs) {
   ATLAS *atlas;
   char **slist=NULL;
   int i = 0;
   
   *N_refs = 0;
   if (wami_verb()) {
      WARNING_message(
         "Daniel: I don't know how to get at this in the modern way.\n"
         "We need to revisit version and reference options...");
   }
   
   if (1 || !(atlas = Atlas_With_Trimming(atname, 1, NULL))) {
      if (wami_verb()) 
         ERROR_message("Failed getting atlas for atlas_reference_string_list");
      if (!strcmp(atname,"CA_N27_MPM") ||
          !strcmp(atname,"CA_N27_PM")  ||
          !strcmp(atname,"CA_N27_LR") ||
          !strcmp(atname,"CA_N27_ML")) { 
         if (wami_verb()) 
            WARNING_message("Old style retrieval of reference string for %s",
                              atname);
         i = 0; 
         while (CA_EZ_REF_STR_HARD[i][0]!='\0') {
            slist = add_to_names_list(slist, 
                              N_refs, CA_EZ_REF_STR_HARD[i]);
            ++i;
         }
         return(slist);
      }
   }
   
   return(NULL);
}

char **atlas_chooser_formatted_labels(char *atname) {
   char **at_labels=NULL;
   ATLAS_POINT_LIST *apl=NULL;
   ATLAS *atlas;
   int ii;
   
   if (!(apl = atlas_point_list(atname))) {
      if (wami_verb()) {
         ERROR_message("Failed getting atlas point list for %s", atname); 
      }
      return(NULL);
   }
   at_labels = (char **) calloc(apl->n_points, sizeof(char*));
   for( ii=0 ; ii < apl->n_points ; ii++ ){
      at_labels[ii] = (char *) malloc( sizeof(char) * TTO_LMAX ) ;
      sprintf( at_labels[ii] , TTO_FORMAT , apl->at_point[ii].name ,
         apl->at_point[ii].xx , apl->at_point[ii].yy , apl->at_point[ii].zz ) ;
   }
      
   return(at_labels);
}

/* End ZSS: Additions for Eickhoff and Zilles Cytoarchitectonic maps */

