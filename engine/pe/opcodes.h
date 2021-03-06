// PEscript VM opcodes

#define OPCODEMAX  512
#define OPCODEMASK 0x1ff

enum {
	PEVM_Return,
	PEVM_CallVRM,
	PEVM_Printstr,
	PEVM_Printint,
	PEVM_PrintXint,
	PEVM_PrintCR,
	PEVM_ClearScreen,
	PEVM_ClearLine,
	PEVM_Callfunc,
	PEVM_CallfuncS,
	PEVM_Let_iei,
	PEVM_Let_ieipi,
	PEVM_Let_pes,
	PEVM_Let_seU,
	PEVM_Add,
	PEVM_Goto,
	PEVM_If_iei,
	PEVM_If_oics,
	PEVM_If_oeo,
	PEVM_If_oEo,
	PEVM_If_ses,
	PEVM_If_o,
	PEVM_If_no,
	PEVM_If_i,
	PEVM_If_ni,
	PEVM_If_p,
	PEVM_If_np,
	PEVM_ClearArrayI,
	PEVM_ClearArrayS,
	PEVM_ClearArrayO,
	PEVM_For,
	PEVM_Searchin,
	PEVM_Listafter,
	PEVM_SearchWorld,
	PEVM_StopSearch,
	PEVM_PlayMusic,
	PEVM_StopMusic,
	PEVM_IsPlaying,
	PEVM_PlaySound,
	PEVM_ObjSound,
	PEVM_Create,
	PEVM_CreateS,
	PEVM_Destroy,
	PEVM_SetDarkness,
	PEVM_If_on,
	PEVM_If_non,
	PEVM_If_tn,
	PEVM_If_ntn,
	PEVM_SetFlag,
	PEVM_GetFlag,
	PEVM_GetEngineFlag,
	PEVM_SetPFlag,
	PEVM_GetPFlag,
	PEVM_SetUFlag,
	PEVM_GetUFlag,
	PEVM_If_Uflag,
	PEVM_If_nUflag,
	PEVM_SetLocal,
	PEVM_GetLocal,
	PEVM_If_onLocal,
	PEVM_If_nonLocal,
	PEVM_ResetFlag,
	PEVM_MoveObject,
	PEVM_PushObject,
	PEVM_XferObject,
	PEVM_ShowObject,
	PEVM_GotoXY,
	PEVM_PrintXYi,
	PEVM_PrintXYs,
	PEVM_PrintXYx,
	PEVM_PrintXYcr,
	PEVM_TextColour,
	PEVM_TextColourDefault,
	PEVM_TextFont,
	PEVM_GetTextColour,
	PEVM_GetTextFont,
	PEVM_SetSequenceN,
	PEVM_SetSequenceS,
	PEVM_Lightning,
	PEVM_Earthquake,
	PEVM_Bestname,
	PEVM_GetObject,
	PEVM_GetSolidObject,
	PEVM_GetFirstObject,
	PEVM_GetTile,
	PEVM_GetTileCost,
	PEVM_GetObjectBelow,
	PEVM_GetBridge,
	PEVM_ChangeObject,
	PEVM_ChangeObjectS,
	PEVM_ReplaceObject,
	PEVM_ReplaceObjectS,
	PEVM_SetDir,
	PEVM_ForceDir,
	PEVM_RedrawMap,
	PEVM_RedrawText,
	PEVM_MoveToPocket,
	PEVM_XferToPocket,
	PEVM_MoveFromPocket,
	PEVM_ForceFromPocket,
	PEVM_Spill,
	PEVM_SpillXY,
	PEVM_MoveContents,
	PEVM_Empty,
	PEVM_FadeOut,
	PEVM_FadeIn,
	PEVM_MoveToTop,
	PEVM_MoveToFloor,
	PEVM_InsertAfter,
	PEVM_GetLOS,
	PEVM_GetDistance,
	PEVM_CheckHurt,
	PEVM_IfSolid,
	PEVM_IfVisible,
	PEVM_WeighObject,
	PEVM_GetBulk,
	PEVM_InputInt,
	PEVM_IfInPocket,
	PEVM_IfNInPocket,
	PEVM_ReSyncEverything,
	PEVM_TalkTo1,
	PEVM_TalkTo2,
	PEVM_Random,
	PEVM_GetYN,
	PEVM_GetYNN,
	PEVM_Restart,
	PEVM_ScrTileN,
	PEVM_ScrTileS,
	PEVM_ResetTileN,
	PEVM_ResetTileS,
	PEVM_While1,
	PEVM_While2,
	PEVM_Break,
	PEVM_GetKey,
	PEVM_GetKey_quiet,
	PEVM_GetKeyAscii,
	PEVM_GetKeyAscii_quiet,
	PEVM_FlushKeys,
	PEVM_DoAct,
	PEVM_DoActS,
	PEVM_DoActTo,
	PEVM_DoActSTo,
	PEVM_NextAct,
	PEVM_NextActS,
	PEVM_NextActTo,
	PEVM_NextActSTo,
	PEVM_InsAct,
	PEVM_InsActS,
	PEVM_InsActTo,
	PEVM_InsActSTo,
	PEVM_StopAct,
	PEVM_ResumeAct,
	PEVM_GetContainer,
	PEVM_SetLeader,
	PEVM_SetMember,
	PEVM_AddMember,
	PEVM_DelMember,
	PEVM_MoveTowards8,
	PEVM_MoveTowards4,
	PEVM_WaitForAnimation,
	PEVM_WaitFor,
	PEVM_If_oonscreen,
	PEVM_If_not_oonscreen,
	PEVM_GiveQty,
	PEVM_TakeQty,
	PEVM_MoveQty,
	PEVM_CountQty,
	PEVM_CopySchedule,
	PEVM_CopySpeech,
	PEVM_AllOnscreen,
	PEVM_AllAround,
	PEVM_CheckTime,
	PEVM_FindNear,
	PEVM_FindNearby,
	PEVM_FindNearbyFlag,
	PEVM_FindTag,
	PEVM_FastTag,
	PEVM_MakeTagList,
	PEVM_FindNext,
	PEVM_SetLight,
	PEVM_SetLight_single,
	PEVM_MoveTag,
	PEVM_UpdateTag,
	PEVM_GetDataSSS,
	PEVM_GetDataSSI,
	PEVM_GetDataSIS,
	PEVM_GetDataSII,
	PEVM_GetDataISS,
	PEVM_GetDataISI,
	PEVM_GetDataIIS,
	PEVM_GetDataIII,
	PEVM_GetDataKeysII,
	PEVM_GetDataKeysIS,
	PEVM_GetDataKeysSI,
	PEVM_GetDataKeysSS,
	PEVM_GetDecor,
	PEVM_DelDecor,
	PEVM_SearchContainer,
	PEVM_ChangeMap1,
	PEVM_ChangeMap2,
	PEVM_GetMapNo,
	PEVM_PicScroll,
	PEVM_SetPanel,
	PEVM_FindPathMarker,
	PEVM_ReSolid,
	PEVM_SetPName,
	PEVM_DelProp,
	PEVM_QUpdate,

	PEVM_AddRange,
	PEVM_GridRange,
	PEVM_AddButton,
	PEVM_PushButton,
	PEVM_RightClick,
	PEVM_FlushMouse,
	PEVM_RangePointer,
	PEVM_TextButton,
	PEVM_MovePartyToObj,
	PEVM_MovePartyFromObj,
	PEVM_SaveScreen,
	PEVM_RestoreScreen,
	PEVM_fxGetXY,
	PEVM_fxColour,
	PEVM_fxAlpha,
	PEVM_fxAlphaMode,
	PEVM_fxRandom,
	PEVM_fxLine,
	PEVM_fxPoint,
	PEVM_fxRect,
	PEVM_fxPoly,
	PEVM_fxOrbit,
	PEVM_fxSprite,
	PEVM_fxCorona,
	PEVM_fxBlackCorona,
	PEVM_fxAnimSprite,
	PEVM_dofx,
	PEVM_dofxS,
	PEVM_dofxOver,
	PEVM_dofxOverS,
	PEVM_stopfx,
	
	PEVM_CreateConsole,
	PEVM_DestroyConsole,
	PEVM_ClearConsole,
	PEVM_ConsoleColour,
	PEVM_ConsoleAddLine,
	PEVM_ConsoleAddLineWrap,
	PEVM_DrawConsole,
	PEVM_ScrollConsole,
	PEVM_FindString,
	PEVM_FindTitle,
	PEVM_AddJournal,
	PEVM_AddJournal2,
	PEVM_EndJournal,
	PEVM_DrawJournal,
	PEVM_DrawJournalDay,
	PEVM_DrawJournalTasks,
	PEVM_GetJournalDays,
	PEVM_GetJournalDay,
	PEVM_SetConsoleLine,
	PEVM_SetConsolePercent,
	PEVM_SetConsoleSelect,
	PEVM_SetConsoleSelectS,
	PEVM_SetConsoleSelectU,
	PEVM_GetConsoleLine,
	PEVM_GetConsolePercent,
	PEVM_GetConsoleSelect,
	PEVM_GetConsoleSelectU,
	PEVM_GetConsoleLines,
	PEVM_GetConsoleRows,
	
	PEVM_CreatePicklist,
	PEVM_DestroyPicklist,
	PEVM_PicklistColour,
	PEVM_PicklistPoll,
	PEVM_PicklistAddLine,
	PEVM_PicklistAddSave,
	PEVM_PicklistItem,
	PEVM_PicklistId,
	PEVM_PicklistText,
	PEVM_SetPicklistPercent,
	PEVM_GetPicklistPercent,
	
	PEVM_setstr,
	PEVM_addstr,
	PEVM_addstrn,
	PEVM_strlen,
	PEVM_strlen2,
	PEVM_strgetpos,
	PEVM_strgetpos2,
	PEVM_strsetpos,
	PEVM_strgetval,

	PEVM_getvolume,
	PEVM_setvolume,
	PEVM_LastSaveSlot,
	PEVM_SaveGame,
	PEVM_LoadGame,
	PEVM_GetDate,
	PEVM_IfKey,
	PEVM_IfKeyPressed,

	PEVM_Printaddr,
	PEVM_PrintLog,
	PEVM_Assert,
	PEVM_LastOp,
	
	// Internal functions
	PEVM_GetFuncP=OPCODEMAX-4,
	PEVM_Dump=OPCODEMAX-3,
	PEVM_NOP=OPCODEMAX-2,
	PEVM_Dofus=OPCODEMAX-1
	};

	
#define UP 0
#define DOWN 1
#define LEFT 2
#define RIGHT 3

#define CHAR_U 0
#define CHAR_D 1
#define CHAR_L 2
#define CHAR_R 3


// Tint bitmasks
#define TINT_RED 1
#define TINT_GREEN 2
#define TINT_BLUE 4
#define TINT_YELLOW 3
#define TINT_PURPLE 5
#define TINT_MAGENTA 5
#define TINT_CYAN 6
#define TINT_LIGHTBLUE 6
#define TINT_WHITE 7

extern int pe_result_ok;
