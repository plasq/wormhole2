/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
   WormHole.r
   ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#if !defined(TARGET_REZ_MAC_PPC)
 #define TARGET_REZ_MAC_PPC 1
#endif

#if !defined(TARGET_REZ_MAC_X86)
 #define TARGET_REZ_MAC_X86 1
#endif

#include "apConfiguration.h"

#include <AudioUnit/AudioUnit.r>
#include <AudioUnit/AudioUnitCarbonView.r>

// Note that resource IDs must be spaced 2 apart for the 'STR ' name and description

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#define RES_ID				11000
#define COMP_TYPE			kAudioUnitType_Effect
#define COMP_SUBTYPE	kapIdentifier
#define COMP_MANUF		kapManuID	 
#define VERSION				kapVersion
#define NAME					"plasq.com: Wormhole2"
#define DESCRIPTION		"Hyperspace Travel For Audio"
#define ENTRY_POINT		"WormHole2Entry"

#include "AUResources.r"

// ____________________________________________________________________________

#define RES_ID				1000
#define COMP_TYPE			kAudioUnitCarbonViewComponentType
#define COMP_SUBTYPE	kapIdentifier
#define COMP_MANUF		kapManuID
#define VERSION				kapVersion
#define NAME					"plasq.com: Wormhole2View"
#define DESCRIPTION		"the Editor"
#define ENTRY_POINT		"WormHoleEditorEntry"

#include "AUResources.r"