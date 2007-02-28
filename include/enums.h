/*
 * Copyright (C) 2003 by Dave J. Andruczyk <djandruczyk at yahoo dot com>
 *
 * Linux Megasquirt tuning software
 * 
 * 
 * This software comes under the GPL (GNU Public License)
 * You may freely copy,distribute, etc. this as long as all the source code
 * is made available for FREE.
 * 
 * No warranty is made or implied. You use this program at your own risk.
 */

#ifndef __ENUMS_H__
#define __ENUMS_H__

/* Serial data comes in and handled by the following possibilities.
 * dataio.c uses these to determine which course of action to take...
 */
typedef enum
{
	REALTIME_VARS=0,
	VE_BLOCK,
	RAW_MEMORY_DUMP,
	C_TEST,
	GET_ERROR,
	NULL_HANDLER,
}InputHandler;

/* Regular Buttons */
typedef enum
{
	START_REALTIME = 0x20,
	STOP_REALTIME,
	START_PLAYBACK,
	STOP_PLAYBACK,
	READ_VE_CONST,
	READ_RAW_MEMORY,
	BURN_MS_FLASH,
	CHECK_ECU_COMMS,
	INTERROGATE_ECU,
	OFFLINE_MODE,
	SELECT_DLOG_EXP,
	SELECT_DLOG_IMP,
	SELECT_FIRMWARE_LOAD,
	ENTER_SENSOR_INFO,
	DOWNLOAD_FIRMWARE,
	DLOG_SELECT_DEFAULTS,
	DLOG_SELECT_ALL,
	DLOG_DESELECT_ALL,
	DLOG_DUMP_INTERNAL,
	CLOSE_LOGFILE,
	START_DATALOGGING,
	STOP_DATALOGGING,
	EXPORT_VETABLE,
	IMPORT_VETABLE,
	REBOOT_GETERR,
	REVERT_TO_BACKUP,
	BACKUP_ALL,
	RESTORE_ALL,
	SELECT_PARAMS,
	REQ_FUEL_POPUP,
	RESCALE_TABLE,
	REQFUEL_RESCALE_TABLE,
}StdButton;

/* Toggle/Radio buttons */
typedef enum
{
	TOOLTIPS_STATE=0x50,
	TRACKING_FOCUS,
        FAHRENHEIT,
        CELSIUS,
	COMMA,
	TAB,
	USE_ALT_IAT,
	USE_ALT_CLT,
	REALTIME_VIEW,
	PLAYBACK_VIEW,
	HEX_VIEW,
	BINARY_VIEW,
	DECIMAL_VIEW,
	OFFLINE_FIRMWARE_CHOICE,
	START_TOOTHMON_LOGGER,
	START_TRIGMON_LOGGER,
}ToggleButton;

	
/* spinbuttons... */
typedef enum
{
	REQ_FUEL_DISP=0x80,
	REQ_FUEL_CYLS,
	REQ_FUEL_RATED_INJ_FLOW,
	REQ_FUEL_RATED_PRESSURE,
	REQ_FUEL_ACTUAL_PRESSURE,
	REQ_FUEL_AFR,
	LOCKED_REQ_FUEL,
	REQ_FUEL_1,
	REQ_FUEL_2,
	SER_INTERVAL_DELAY,
	SET_SER_PORT,
	NUM_SQUIRTS_1,
	NUM_SQUIRTS_2,
	NUM_CYLINDERS_1,
	NUM_CYLINDERS_2,
	NUM_INJECTORS_1,
	NUM_INJECTORS_2,
	TRIGGER_ANGLE,
	ODDFIRE_ANGLE,
	LOGVIEW_ZOOM,
	DEBUG_LEVEL,
	GENERIC,
	MAP_SENSOR_TYPE,
	ALT_SIMUL,
}SpinButton;

/* Runtime Status flags */
typedef enum 
{       
	STAT_CONNECTED = 0, 
        STAT_CRANKING, 
        STAT_RUNNING, 
        STAT_WARMUP, 
        STAT_AS_ENRICH, 
        STAT_ACCEL, 
        STAT_DECEL,
}RuntimeStatus;

typedef enum
{
	VE_EXPORT = 0xb0,
	VE_IMPORT,
	DATALOG_INT_DUMP,
	DATALOG_EXPORT,
	DATALOG_IMPORT,
	FIRMWARE_LOAD,
	FULL_BACKUP,
	FULL_RESTORE,
}FileIoType;


typedef enum
{	
	RED=0xc0,
	BLACK,
}GuiColor;

typedef enum
{
	HEADER=0xd0,
	PAGE,
	RANGE,
	TABLE,
}ImportParserFunc;

typedef enum
{
	VEX_EVEME=0xe0,
	VEX_USER_REV,
	VEX_USER_COMMENT,
	VEX_DATE,
	VEX_TIME,
	VEX_RPM_RANGE,
	VEX_LOAD_RANGE,
	VEX_NONE,
}ImportParserArg;

typedef enum
{
	FONT=0x100,
	TRACE,
	GRATICULE,
	HIGHLIGHT,
	TTM_AXIS,
	TTM_TRACE,
}GcType;

typedef enum
{	/* up to 32 Capability flags.... */
	/* No capabilities == Standard B&G code with no modifications */
	STANDARD	= 1<<0,
	DUALTABLE	= 1<<1,
	MSNS_E		= 1<<2,
}Capability;

typedef enum
{
	MTX=0x110,
	MT_CLASSIC,
	MT_FULL,
	MT_RAW,
}LogType;

typedef enum
{
	NO_DEBUG 	= 0,
	INTERROGATOR 	= 1<<0,
	OPENGL		= 1<<1,
	CONVERSIONS	= 1<<2,
	SERIAL_RD	= 1<<3,
	SERIAL_WR	= 1<<4,
	IO_PROCESS	= 1<<5,
	THREADS		= 1<<6,
	REQ_FUEL	= 1<<7,
	TABLOADER	= 1<<8,
	KEYPARSER	= 1<<9,
	RTMLOADER	= 1<<10,
	COMPLEX_EXPR	= 1<<11,
	CRITICAL	= 1<<30,
}Dbg_Class;

typedef enum guint
{
	INTERROGATOR_SHIFT	= 0,
	OPENGL_SHIFT		= 1,
	CONVERSIONS_SHIFT	= 2,
	SERIAL_RD_SHIFT		= 3,
	SERIAL_WR_SHIFT		= 4,
	IO_PROCESS_SHIFT	= 5,
	THREADS_SHIFT		= 6,
	REQ_FUEL_SHIFT		= 7,
	TABLOADER_SHIFT		= 8,
	KEYPARSER_SHIFT		= 9,
	RTMLOADER_SHIFT		= 10,
	COMPLEX_EXPR_SHIFT	= 11,
	CRITICAL_SHIFT		= 30,
}Dbg_Shift;

typedef enum
{
        VNUM = 0x120,
        TEXTVER,
        SIG,
}StoreType;

typedef enum
{
	BURN_CMD = 0x130,
	READ_CMD,
	WRITE_CMD,
	NULL_CMD,
	COMMS_TEST,
	INTERROGATION,
	OPEN_SERIAL,
	CLOSE_SERIAL,
}CmdType;

typedef enum
{
	IO_REALTIME_READ=0x140,
	IO_INTERROGATE_ECU,
	IO_COMMS_TEST,
	IO_READ_VE_CONST,
	IO_READ_RAW_MEMORY,
	IO_BURN_MS_FLASH,
	IO_WRITE_DATA,
	IO_UPDATE_VE_CONST,
	IO_LOAD_REALTIME_MAP,
	IO_LOAD_GUI_TABS,
	IO_REBOOT_GET_ERROR,
	IO_GET_BOOT_PROMPT,
	IO_BOOT_READ_ERROR,
	IO_JUST_BOOT,
	IO_CLEAN_REBOOT,
	IO_OPEN_SERIAL,
	IO_CLOSE_SERIAL,
	IO_READ_TRIGMON_PAGE,
	IO_READ_TOOTHMON_PAGE,
}Io_Command;

typedef enum
{
	UPD_REALTIME = 0x160,
	UPD_LOGBAR,
	UPD_LOGVIEWER,
	UPD_WIDGET,
	UPD_DATALOGGER,
	UPD_VE_CONST,
	UPD_READ_VE_CONST,
	UPD_ENABLE_THREE_D_BUTTONS,
	UPD_RAW_MEMORY,
	UPD_SET_STORE_RED,
	UPD_SET_STORE_BLACK,
	UPD_LOAD_GUI_TABS,
	UPD_LOAD_REALTIME_MAP,
	UPD_LOAD_RT_STATUS,
	UPD_LOAD_RT_SLIDERS,
	UPD_LOAD_RT_TEXT,
	UPD_POPULATE_DLOGGER,
	UPD_COMMS_STATUS,
	UPD_WRITE_STATUS,
	UPD_REENABLE_INTERROGATE_BUTTON,
	UPD_REENABLE_GET_DATA_BUTTONS,
	UPD_START_REALTIME,
	UPD_START_STATUSCOUNTS,
	UPD_GET_BOOT_PROMPT,
	UPD_REBOOT_GET_ERROR,
	UPD_JUST_BOOT,
	UPD_FORCE_UPDATE,
	UPD_RUN_FUNCTION,
	UPD_TRIGTOOTHMON,
	UPD_INITIALIZE_DASH,
}UpdateFunction;

typedef enum
{
	MTX_INT = 0x190,
	MTX_ENUM,
	MTX_BOOL,
	MTX_STRING,
}DataType;

typedef enum
{
	ABOUT_TAB=0x1a0,
	GENERAL_TAB,
	COMMS_TAB,
	ENG_VITALS_TAB,
	CONSTANTS_TAB,
	DT_PARAMS_TAB,
	IGNITON_TAB,
	RUNTIME_TAB,
	ENRICHMENTS_TAB,
	TUNING_TAB,
	TOOLS_TAB,
	RAW_MEM_TAB,
	WARMUP_WIZ_TAB,
	VETABLES_TAB,
	SPARKTABLES_TAB,
	AFRTABLES_TAB,
	BOOSTTABLES_TAB,
	ROTARYTABLES_TAB,
	DATALOGGING_TAB,
	LOGVIEWER_TAB,
	VE3D_VIEWER_TAB,
	ERROR_STATUS_TAB,
}TabIdent;

typedef enum
{
	VE_EMB_BIT=0x1c0,
	VE_VAR,
	RAW_VAR,
}ComplexExprType;

typedef enum
{
	UPLOAD=0x1c8,
	DOWNLOAD,
	RTV,
	GAUGE,
}ConvType;

typedef enum
{
	MTX_HEX=0x1d0,
	MTX_DECIMAL,
}Base;

typedef enum
{
	MTX_ENTRY=0x1e0,
	MTX_TITLE,
	MTX_LABEL,
}WidgetType;

typedef enum
{
	MTX_PROGRESS=0x200,
	MTX_RANGE,
}SliderType;


typedef enum
{
	RTV_TICKLER=0x210,
	LV_PLAYBACK_TICKLER,
	TOOTHMON_TICKLER,
	TRIGMON_TICKLER,
}TicklerType;

typedef enum
{
	ALPHA_N=0x220,
	SPEED_DENSITY,
	MAF,
	SD_AN_HYBRID,
	MAF_AN_HYBRID,
	SD_MAF_HYBRID,
}Algorithm;

typedef enum
{
	VEX_IMPORT=0x230,
	VEX_EXPORT,
	ECU_BACKUP,
	ECU_RESTORE,
}FioAction;

typedef enum
{
	MTX_SINGLE_WRITE=0x240,
	MTX_CHUNK_WRITE,
}WriteMode;

#endif
