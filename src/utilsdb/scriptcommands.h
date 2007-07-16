#ifndef __RTS2_SCRIPTCOMMANDS__
#define __RTS2_SCRIPTCOMMANDS__

/**
 * This file list commands which can be introduced to script.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */

#define COMMAND_EXPOSURE	"E"
#define COMMAND_DARK		"D"
#define COMMAND_FILTER		"F"
#define COMMAND_FOCUSING	"O"
#define COMMAND_CHANGE		"C"
#define COMMAND_BINNING		"BIN"
#define COMMAND_BOX		"BOX"
#define COMMAND_CENTER		"CENTER"
// must be paired with COMMAND_CHANGE
#define COMMAND_WAIT		"W"
#define COMMAND_ACQUIRE		"A"
#define COMMAND_WAIT_ACQUIRE	"Aw"
#define COMMAND_MIRROR_MOVE	"M"
#define COMMAND_PHOTOMETER	"P"
#define COMMAND_STAR_SEARCH	"star"
#define COMMAND_PHOT_SEARCH	"PS"
#define COMMAND_BLOCK_WAITSIG   "block_waitsig"
#define COMMAND_GUIDING		"guiding"
#define COMMAND_BLOCK_ACQ	"ifacq"
#define COMMAND_BLOCK_ELSE	"else"
#define COMMAND_BLOCK_FOR	"for"
#define COMMAND_WAITFOR		"waitfor"

// hex pattern
#define COMMAND_HEX		"hex"
// 5x5 pattern
#define COMMAND_FXF		"fxf"

// HAM acqusition - only on FRAM telescope
#define COMMAND_HAM		"HAM"

// signal handling..
#define COMMAND_SEND_SIGNAL	"SS"
#define COMMAND_WAIT_SIGNAL	"SW"

// target operations
#define COMMAND_TARGET_DISABLE	"tardisable"
#define COMMAND_TAR_TEMP_DISAB	"tempdisable"
#define COMMAND_TARGET_BOOST	"tarboost"

#endif /* !__RTS2_SCRIPTCOMMANDS__ */
