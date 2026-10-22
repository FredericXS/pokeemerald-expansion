@ Fixed destinations only. The selected starter is staged before any deposit.
Generations_OnJohtoMap::
	special Generations_OnJohtoMapLoaded
	setrespawn HEAL_LOCATION_NEW_BARK_TOWN
	end

Generations_EventScript_ToJohto::
	lock
	faceplayer
	msgbox Generations_Text_Depart, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, Generations_EventScript_Cancel
	msgbox Generations_Text_Chikorita, MSGBOX_YESNO
	setvar VAR_0x8004, SPECIES_CHIKORITA
	goto_if_eq VAR_RESULT, YES, Generations_EventScript_Depart
	msgbox Generations_Text_Cyndaquil, MSGBOX_YESNO
	setvar VAR_0x8004, SPECIES_CYNDAQUIL
	goto_if_eq VAR_RESULT, YES, Generations_EventScript_Depart
	msgbox Generations_Text_Totodile, MSGBOX_YESNO
	setvar VAR_0x8004, SPECIES_TOTODILE
	goto_if_eq VAR_RESULT, NO, Generations_EventScript_Cancel
Generations_EventScript_Depart::
	call Generations_EventScript_CommitJohto
	goto_if_eq VAR_RESULT, GENERATIONS_TRAVEL_PC_FULL, Generations_EventScript_Full
	goto_if_eq VAR_RESULT, GENERATIONS_TRAVEL_UNSAFE_PARTY, Generations_EventScript_Unsafe
	goto_if_ne VAR_RESULT, GENERATIONS_TRAVEL_OK, Generations_EventScript_Locked
	msgbox Generations_Text_Arrive, MSGBOX_DEFAULT
	warp MAP_NEW_BARK_TOWN_ELMS_LAB, 6, 10
	waitstate
	releaseall
	end

@ Also exercised by tests through the real script interpreter.
Generations_EventScript_CommitJohto::
	special Generations_TravelToJohto
	return

Generations_EventScript_ToKanto::
	lock
	faceplayer
	msgbox Generations_Text_Return, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, Generations_EventScript_Cancel
	special Generations_TravelToKanto
	goto_if_eq VAR_RESULT, FALSE, Generations_EventScript_Locked
	warp MAP_PALLET_TOWN, 10, 11
	waitstate
	releaseall
	end

Generations_EventScript_Full::
	msgbox Generations_Text_Full, MSGBOX_DEFAULT
	goto Generations_EventScript_Cancel
Generations_EventScript_Unsafe::
	msgbox Generations_Text_Unsafe, MSGBOX_DEFAULT
	goto Generations_EventScript_Cancel
Generations_EventScript_Locked::
	msgbox Generations_Text_Locked, MSGBOX_DEFAULT
Generations_EventScript_Cancel::
	release
	end

Generations_EventScript_TestProgress::
	lock
	faceplayer
#if GENERATIONS_TEST_ROM
	msgbox Generations_Text_Shortcut, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, NO, Generations_EventScript_Cancel
	special Generations_TestProgress
	msgbox Generations_Text_ShortcutDone, MSGBOX_DEFAULT
#else
	msgbox Generations_Text_NoShortcut, MSGBOX_DEFAULT
#endif
	goto Generations_EventScript_Cancel

Generations_EventScript_PC::
	lockall
	special Generations_CheckPC
	goto_if_eq VAR_RESULT, FALSE, Generations_EventScript_PCLocked
	releaseall
	goto EventScript_PC
Generations_EventScript_PCLocked::
	msgbox Generations_Text_PCLocked, MSGBOX_DEFAULT
	releaseall
	end

Generations_EventScript_Elm::
	lock
	faceplayer
	msgbox Generations_Text_Elm, MSGBOX_DEFAULT
	giveitem ITEM_POKE_BALL, 5
	goto Generations_EventScript_Cancel
Generations_EventScript_Nurse::
	lock
	faceplayer
	call EventScript_PkmnCenterNurse_Frlg
	release
	end
Generations_EventScript_Chikorita::
	msgbox Generations_Text_BallChikorita, MSGBOX_NPC
	end
Generations_EventScript_Cyndaquil::
	msgbox Generations_Text_BallCyndaquil, MSGBOX_NPC
	end
Generations_EventScript_Totodile::
	msgbox Generations_Text_BallTotodile, MSGBOX_NPC
	end
Generations_EventScript_TownSign::
	msgbox Generations_Text_Town, MSGBOX_SIGN
	end
Generations_EventScript_RouteSign::
	msgbox Generations_Text_Route, MSGBOX_SIGN
	end
Generations_EventScript_RouteGuide::
	msgbox Generations_Text_Guide, MSGBOX_NPC
	end

Generations_Text_Depart::
	.string "JOHTO expedition! First visit stores\n"
	.string "your WHOLE team in the PC.\p"
	.string "Reserve ELM's starter before leaving.\n"
	.string "Later visits keep your current team.\p"
	.string "PC and return travel unlock after\n"
	.string "four JOHTO badges. Continue?$"
Generations_Text_Chikorita::
	.string "Reserve CHIKORITA, the Grass-type?\n"
	.string "It will join you at level 5.$"
Generations_Text_Cyndaquil::
	.string "Reserve CYNDAQUIL, the Fire-type?\n"
	.string "It will join you at level 5.$"
Generations_Text_Totodile::
	.string "Reserve TOTODILE, the Water-type?\n"
	.string "Say NO to cancel the trip.$"
Generations_Text_Arrive::
	.string "Ready! On a first visit, your old\n"
	.string "team is safely boxed and ELM's\l"
	.string "reserved partner is with you.$"
Generations_Text_Return::
	.string "Return to PALLET TOWN with your\n"
	.string "current team? Four JOHTO badges\l"
	.string "are needed for this passage.$"
Generations_Text_Full::
	.string "Not enough free PC slots for your\n"
	.string "entire team. Nothing was moved.\p"
	.string "Free enough slots, then try again.$"
Generations_Text_Unsafe::
	.string "Cannot transfer this team safely.\n"
	.string "Remove MAIL and unfuse POKéMON.\p"
	.string "A damaged or empty team cannot\n"
	.string "travel. Nothing was moved.$"
Generations_Text_Locked::
	.string "Passage unavailable. Become KANTO\n"
	.string "CHAMPION before your first trip.\p"
	.string "Return requires four JOHTO badges.\n"
	.string "If transfer failed, nothing moved.$"
Generations_Text_Shortcut::
	.string "TEST ROM progression shortcut.\p"
	.string "In KANTO: eight badges + CHAMPION.\n"
	.string "In JOHTO: first four badges.\p"
	.string "This changes your saved progress.\n"
	.string "Apply the shortcut?$"
Generations_Text_ShortcutDone::
	.string "Progress updated. In PALLET TOWN,\n"
	.string "speak to the sailor near the sign.\p"
	.string "In JOHTO, PC and return travel\n"
	.string "are now available.$"
Generations_Text_NoShortcut::
	.string "Progress shortcuts are available\n"
	.string "only in the progression TEST ROM.$"
Generations_Text_PCLocked::
	.string "This expedition's PC unlocks after\n"
	.string "four JOHTO badges. Your old team\l"
	.string "is kept safely until then.$"
Generations_Text_Elm::
	.string "ELM: Welcome to NEW BARK TOWN!\n"
	.string "Your reserved partner is with you.\p"
	.string "The western path is ROUTE 29.\n"
	.string "Try catching a SENTRET there!\p"
	.string "Take five POKé BALLS. Come back\n"
	.string "for more while testing.$"
Generations_Text_BallChikorita::
	.string "CHIKORITA's reservation display.\n"
	.string "Choose your partner before boarding.\l"
	.string "One JOHTO starter per adventure.$"
Generations_Text_BallCyndaquil::
	.string "CYNDAQUIL's reservation display.\n"
	.string "Choose your partner before boarding.\l"
	.string "One JOHTO starter per adventure.$"
Generations_Text_BallTotodile::
	.string "TOTODILE's reservation display.\n"
	.string "Choose your partner before boarding.\l"
	.string "One JOHTO starter per adventure.$"
Generations_Text_Town::
	.string "NEW BARK TOWN - JOHTO expedition\n"
	.string "West: ROUTE 29 to CHERRYGROVE\p"
	.string "East house: healing and PC\n"
	.string "Northwest: ELM's laboratory$"
Generations_Text_Route::
	.string "ROUTE 29 - NEW BARK test area\n"
	.string "The rest of JOHTO is not open yet.$"
Generations_Text_Guide::
	.string "SENTRET and HOOTHOOT live here!\n"
	.string "Heal at the east house in town.\p"
	.string "The scientist in NEW BARK provides\n"
	.string "the TEST ROM's four-badge shortcut.$"

Generations_EventScript_Mom::
    lock
    faceplayer
    msgbox Generations_Text_HomeHeal, MSGBOX_DEFAULT
    call Common_EventScript_OutOfCenterPartyHeal
    msgbox Generations_Text_HomeRested, MSGBOX_DEFAULT
    release
    end
Generations_EventScript_AfterWhiteout::
    lockall
    special HealPlayerParty
    msgbox Generations_Text_HomeRested, MSGBOX_DEFAULT
    releaseall
    end
Generations_EventScript_TownResident::
    msgbox Generations_Text_HGSSNewBark, MSGBOX_NPC
    end
Generations_EventScript_HouseResident::
    msgbox Generations_Text_House, MSGBOX_NPC
    end
Generations_EventScript_ResearchNotes::
    msgbox Generations_Text_Research, MSGBOX_SIGN
    end
Generations_EventScript_LabSign::
    msgbox Generations_Text_Lab, MSGBOX_SIGN
    end
Generations_EventScript_RivalHouseSign::
    msgbox Generations_Text_RivalHouse, MSGBOX_SIGN
    end
Generations_EventScript_RouteWestGuide::
    msgbox Generations_Text_WestBoundary, MSGBOX_NPC
    end
Generations_EventScript_WestBoundary::
    msgbox Generations_Text_WestBoundary, MSGBOX_SIGN
    end
Generations_EventScript_Gatekeeper::
    msgbox Generations_Text_Route46, MSGBOX_NPC
    end
Generations_EventScript_Route46Sign::
    msgbox Generations_Text_Route46, MSGBOX_SIGN
    end
Generations_Text_HomeHeal::
    .string "Welcome home! Let your POKéMON\n"
    .string "rest here before you explore JOHTO.$"
Generations_Text_HomeRested::
    .string "Your POKéMON are feeling better.\n"
    .string "Take care on ROUTE 29!$"
Generations_Text_HGSSNewBark::
    .string "NEW BARK TOWN, where winds of a\n"
    .string "new beginning blow.\p"
    .string "ROUTE 29 lies west. The eastern\n"
    .string "shore leads toward ROUTE 27.$"
Generations_Text_House::
    .string "The houses and ELM's lab have been\n"
    .string "rebuilt for this JOHTO expedition.\p"
    .string "Travel west through the ledges and\n"
    .string "grass of ROUTE 29.$"
Generations_Text_Research::
    .string "ELM's research: POKéMON, their\n"
    .string "habitats and their evolution.\p"
    .string "The study is reached by the stairs\n"
    .string "outside the laboratory.$"
Generations_Text_Lab::
    .string "ELM POKéMON LAB\n"
    .string "Research on POKéMON and evolution.$"
Generations_Text_RivalHouse::
    .string "NEW BARK TOWN\n"
    .string "A neighbor's house.$"
Generations_Text_WestBoundary::
    .string "CHERRYGROVE CITY lies farther west.\p"
    .string "That area is still being built.\n"
    .string "You can explore ROUTE 29 here.$"
Generations_Text_Route46::
    .string "ROUTE 46 is north of this gate.\p"
    .string "Construction has closed that exit.\n"
    .string "The south door returns to ROUTE 29.$"
