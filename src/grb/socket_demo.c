/*	socket_demo.c -- to demostrate a socket connection for a GCN site
 *
 *	USAGE:
 *		socket_demo  <port_num>
 *
 *			Where "port_num" is the port number for your site.
 *
 *	DESCRIPTION:
 *		INTRO:  This standalone C program shows all the neccessary parts
 *		that a site would use to become a GCN site connected via
 *		the Internet socket method.  These parts can be cut and spliced
 *		into your site instrument-control software  or
 *		this program can be used as the basis of your instrument's control
 *		program (there is a place noted in the code where to put your
 *		instrument-specific code).
 *
 *		CLIENT-SERVER:  The site (in this case the computer/control_program
 *		for the instrument that will be making the follow-up observations)
 *		is acting as a socket "server".  The GCN machine here at GSFC
 *		acts as a socket "client".  This is a reversal of the what might be
 *		considered the common sense notion of who is "serving" who,
 *		but it is necessary for the case of multiple sites.  The GCN machine
 *		needs to be able to handle the case when one or more of
 *		the GCN sites "goes off-line".  To do this GCN needs to be
 *		the "client".  If your "server" is running, the GCN "client"
 *		will connect to it and send I'm_alive packets and the various
 *		burst position packet types enabled for your site.  The GCN program
 *		makes continuous periodic attempts to connect to all socket sites
 *		listed in the GCN "sites.cfg" file.
 *
 *		PACKET TYPES:  This version of the program has been set up to handle all
 *		the types of packets that the GCN system produces.  However, each site
 *		chooses which sources of GRB, X-ray Transient, and far-UV Transient
 *		information it wants sent to it.  In this demo program, those unwanted
 *		types can easily be deleted.
 *
 *		IMALIVE CHECKING:  This program reports if the Imalive packets arrive
 *		at shorter or longer than the nominal 60-sec intervals.  It also reports
 *		to the operator via e-mail when no Imalive packets have been received
 *		for more than 600 seconds.  At least one Imalive received is required
 *		to start the 600-sec watchdog clock (ie it does not report
 *		from program-start-up).  It also sends e-mail when the Imalive packets resume.
 *
 *		PORT NUMBERS:  The port number is unique for each site and must be chosen &
 *		agreed to by both ends before any activity can take place.  A block of acceptable
 *		port numbers was choosen so as to not conflict with any known commercial
 *		Internet socket activity.
 *
 *		CONFIG SETUP:  Also, before any connections are possible,
 *		a small amount of data identifying the site (name, Long, Lat,
 *		filter criteria, IPN, port, etc) must be added to the "sites.cfg" file
 *		at the GCN end.
 *
 *	OPERATIONAL CONSIDERATIONS:
 *		RESTARTING TOO QUICKLY:  It is a common phenomenon when running
 *		this program that when it is terminated (for whatever reason)
 *		and restarted too quickly, that it will complain about
 *		"server bind()" errors.  It varies from site to site, but too quickly
 *		seems to be within ~30 sec.  So if you wait at least 60 sec
 *		before restarting this program, then you should avoid this problem.
 *		Basically, it is a result of the operating system on your machine
 *		"remembering" for a few tens of seconds who/what was assigned to each port.
 *		It is possible to adjust this time period, but it is op-sys/machine
 *		dependent and I thought it better to leave it out of this
 *		"general purpose demo program".
 *
 *		ONLY ONE PROGRAM:  Another scenario which produces a situation
 *		where connection is not made comes from having two socket_demo
 *		programs running simultaneously.  This can easily happen
 *		when running socket_demo in the background, and then invoking
 *		it a second time.  It can also happen in a much more subtle and
 *		irksome way.  If the socket_demo gets "wedged", halted, paused, or
 *		some other similar defunct type mode, then it will often not show up
 *		in the process table.  Not seeing it present in the process table
 *		will cause many people to conclude that it is not present, and
 *		then they will restart it.  But it is in fact still present -- at least
 *		to the point of still holding on to the socket connection and port number.
 *		Thus it will be competing for packets with the second invokation, and
 *		both will loose.  It looks to the user that the second program
 *		can never connect.  This can be very perplexing to the user/operator.
 *		The solution to this is making sure you use all the appropriate options
 *		to the system utility that tells you about entries (active AND defunct)
 *		in the process table.  Once you see that the defunct copy is still
 *		in the table, then you can use a "kill with extreme prejudice" command
 *		(if using UNIX: kill -9 <pid>) to totally remove it from the process list.
 *
 *		STANDARDOUT COPY:  This program duplicates almost all of its output
 *		to the logfile also to the standardout.  This duplication to the stdout
 *		allows the first-time/interactive user to "see" what is going on with the
 *		operations of the program.  After a while, you will probably want
 *		to invoke the program with the stdout redirected to /dev/null and
 *		run it in the background:
 *			socket_demo 5000 >& /dev/null &
 *
 *		UNPACKING EXAMPLES:  The various pr_*() routines are included
 *		just to show how to unpack the various items within the packet.
 *		However, not all the items within the packets are unpacked
 *		in these example routines.  They unpack all the major components
 *		in the packet, but there are a few minor items not unpacked
 *		(they are left as an exercise to the reader ;-)).
 *		Once you understand these examples, you will probably want
 *		to delete them from your actual instrument_control_program.
 *
 *		BYTE SWAPPING:  The unpacking of the lbuf packet array is machine dependent.
 *		Byte and/or word swapping my be required if the ordering
 *		on your machine is different than the order on a Sun4.
 *
 *		NET PACKING FORMAT:  The storage/usage of the IP Number and the Port
 *		are also platform-dependent.  On your site's platform, you may need to make
 *		calls to htonl() and htons() to handles these dependencies.
 *
 *		EMAIL ADDRESS CHANGES:  Also, don't forget to change the e-mail address
 *		that is hardwired into the e-mail command near the end
 *		of the e_mail_alert() routine from my address to your address(es)
 *		(assuming you still want this feature).
 *
 *		ERRORS:  All the "bad situations" within the operations of this demo program
 *		are marked with print statements with the prefix "ERR:" to the error
 *		messsage.  This prefix allows the user quickly search the logfile
 *		for instances of bad activity.
 *
 *	FILES:
 *		socket_demo.log	// A logfile of all packets received & program actions.
 *						// This file is created, opened, and written to
 *						// by the socket_demo program.  It provides a
 *						// permanent record of what packets were received.
 *		e_me_tmp		// A temp-file to produce the body of the e-mail.
 *
 *	BUGS:
 *		This program generates two error messages about the separation &
 *		arrival times of the Imalive packets when first started and at midnight.
 *		It was felt that the code to fix this would clutter up the basic
 *		structure and therefore the readability of this demo program.
 *
 *	LIST OF ROUTINES:
 *		brokenpipe()           // Wrap things up if the socket connection breaks
 *		main()                 // The main processing routine
 *		server()               // Set up the server end of a connection (ie the customer end)
 *		serr()                 // Specialized error/status reporting routine
 *		pr_imalive()           // Print the contents of received Imalive packets
 *		pr_packet()            // Contents of TEST/Original/Final/MAXBC/LOCBURST pkts
 *		pr_kill()              // Print the contents of KILL packets
 *		pr_alexis()            // Print the contents of ALEXIS packets
 *		pr_xte_pca()           // Print the contents of XTE-PCA packets
 *		pr_xte_asm()           // Print the contents of XTE-ASM packets
 *		pr_xte_asm_trans()     // Print the contents of XTE-ASM_TRANS packets
 *		pr_ipn_raw()           // Print the contents of IPN Raw packets
 *		pr_ipn_seg()           // Print the contents of IPN Segment packets
 *		pr_ipn_pos()           // Print the contents of IPN Position packets
 *		pr_sax_wfc()           // Print the contents of SAX-WFC packets
 *		pr_hete()              // Print the contents of HETE packets
 *		pr_grb_cntrpart()      // Print the contents of GRB Counterpart packets
 *		pr_integral_point()    // Print the contents of INTEGRAL POINTDIR packets
 *		pr_integral_spiacs()   // Print the contents of INTEGRAL SPI ACS packets
 *		pr_integral_w_r_o()    // Print the contents of INTEGRAL WAKEUP/REFINED/OFFLINE pkts
 *		pr_milagro()           // Print the contents of MILAGRO packets
 *		pr_swift_bat_alert()   // Print the contents of Swift-BAT ALERT pkts
 *		pr_swift_bat_pos_ack() // Print the contents of Swift-BAT Position_Ack pkts
 *		pr_swift_bat_pos_nack()// Print the contents of Swift-BAT Position_Nack pkts
 *		pr_swift_fom_2obs()    // Print the contents of Swift-FOM 2Observe pkts
 *		pr_swift_sc_2slew()    // Print the contents of Swift-SC 2Slew pkts
 *		pr_swift_xrt_pos_ack() // Print the contents of Swift-XRT Position_Ack pkts
 *		pr_swift_xrt_pos_nack()// Print the contents of Swift-XRT Position_Nack pkts
 *		pr_swift_xrt_spec()    // Print the contents of Swift-XRT Spectrum pkts
 *		pr_swift_xrt_image()   // Print the contents of Swift-XRT Image pkts
 *		pr_swift_xrt_lc()      // Print the contents of Swift-XRT LightCurve pkts
 *		pr_swift_uvot_image()  // Print the contents of Swift-UVOT Image pkts
 *		pr_swift_uvot_slist()  // Print the contents of Swift-UVOT SrcList pkts
 *		pr_swift_uvot_pos()    // Print the contents of Swift-UVOT Position_Ack pkts
 *		pr_swift_point()       // Print the contents of SWIFT POINTDIR packets
 *		pr_agile_pos_ack()     // Print the contents of SuperAGILE Position pkts (W/P/R)
 *		pr_agile_point()       // Print the contents of AGILE POINTDIR packets
 *		pr_fermi_gbm_flt_pos() // Print the contents of FERMI-GBM Flt Position pkts
 *		pr_fermi_gbm_gnd_pos() // Print the contents of FERMI-GBM Gnd Position pkts
 *		pr_fermi_lat_pos()     // Print the contents of FERMI-LAT Position pkts
 *		pr_fermi_point()       // Print the contents of FERMI POINTDIR packets
 *		chk_imalive()          // Check and report an absence of Imalive packets rcvd
 *		e_mail_alert()         // Send e-mail when no Imalive packets
 *		hete_same()            // Are the four corners the same position?
 *
 *	COMPILATION:
 *		For LINUX platforms: 
 *			cc -DLINUXPC socket_demo.c -lm -o socket_demo
 *		or for non-LINUX platforms: 
 *			cc socket_demo.c -lm -o socket_demo
 *			It has been a while since this program was compiled and run
 *			on a non-LINUX platform; buyer beware.
 *
 *	MISC NOTES:
 *		1) This file looks best with a tabstop=4 setting.
 *
 *		2) This code was purposefully written in a brute-force style rather
 *		than in a theoretical computer-science style, so that it is easier
 *		to read no matter what the experience level of the end-user.
 *		Also, by not using the full set of features within the C-language,
 *		it is easier to transliterate to other languages (eg Fortran).
 *		All the macro defines are directly included in this C source-code
 *		file (instead of using #include statements) so that _all_ the
 *		relavent code is in a _single_ file.  This makes public distribution
 *		much easier.
 *
 *		3) Inside the e_mail_alert() routine there is a command to send an email
 *		to the "opertor(s)".  It is currently set up
 *		with the email addresses for the GCN Operators (myself).
 *		When you set this up for your own testing/operations,
 *		change this hardwired address to your own address(es).
 *
 *  AUTHOR:
 *		Scott Barthelmy				NASA-GSFC					Code 661.0
 *
 *	MODS:
 *		Fixed the last_here & last_imalive mistake.				12Feb95
 *		Added the NO_IMALIVE_PACKETS e-mailing capability.		29Jul96
 *		Added TYPE_{GRB_FINAL,HUNTS_SRC,ALEXIS_SRC,XTE_PCA_ALERT,XTE_PCA_SRC,
 *		HETE_SRC,IPN_SRC} packet handling.						11May97
 *		Added pr_kill().										23Jul97
 *		Added the TYPE_SAX_WFC_SRC handling.					01Oct97
 *		Added the XTE_ASM handling.								27Oct97
 *		Changed XTE_STATUS to OBS_STATUS to indicate its cross-spacecraft
 *		functionality/content.									15Nov97
 *		Added the XTE_ASM_TRANS handling.						13Jan99
 *		Changed IPN_SRC to IPN_SEG_SRC.							18Dec99
 *		Added the 5 HETE packets handling.						10Feb01
 *		A couple minor tweaks with pr_hete().					17Mar01
 *		Changed "demo.log" to "socket_demo.log".				18Mar01
 *		Added the new HETE Validity flag bits.					04Jun01
 *		Updated a few HETE flag bit definitions.				08Jun01
 *		Change the HETE types (S/C_ prefix).					22Jun01
 *		Added the new HETE Probably trigger flag bits.			22Aug01
 *		Added conditional compilation for LINUXPC sockets.		01Jan02
 *		Added the HETE LightCurve_Sig/Noise (aka Virtue) &
 *		Image_Sig/Noise (aka  Goodness) and S/C Longitude masks.07Apr02
 *		Added the GRB_CNTRPART stuff.							15Apr02
 *		Used ioctl for Linux I/O and swapped buffer before using
 *		in local program - send back the packet unswapped.
 *		Added nbuf as the unswapped buffer, since lbuf is used throughout
 *		the program.											17Jun02  TBS
 *		Added the HETE S/C_LONGitude.
 *		Added the not-any-other-HETE-type-flags, then Possible_GRB case.
 *		Commented out the SAX stuff (the s/c is no longer operating).
 *		Added the INTEGRAL handling.							14Oct02
 *		Fixed lbuf-->nbuf typo in KILL test in LINUX section
 *		(after the read()).										12Mar03
 *		Added the MILAGRO handling.								15Sep04
 *		Finished the Swift stuff.								26Oct04
 *		Updated the flag-bit and comments definitions for about half
 *		of the swift-based packets.
 *		Added the BAT_POS_TEST and BAT_TRANS types.				12Feb05
 *		A few improvements/additions to the comments of a few Swift-based pkts.
 *																26Feb05
 *		Added the missing-pkts comments in BAT_LC, XRT_SP/_IM.	13Mar05
 *		Fixed/finished the Swift_POINTDIR type.					23Mar05
 *		Fixed/added some comment fields in the Swift types.		26Apr05
 *		Fixed/added some more comment fields in the Swift types.07May05
 *		Added some more comment fields in the Swift types.		07Jul05
 *		Changed the FChart & DBurst to SrcList & Image.			18Aug05
 *		Added/fixed some comments in Swift BAT/XRT/UVOT types.	29May06
 *		Converted the 2 in-line sections of code to brokenpipe().
 *																02Nov06
 *		Added BAT_SUBTHESHOLD and BAT_SLEW_POS handling.
 *		Added/started the AGILE and FERMI handling.				30Jun08
 *		Added Lon,Lat to the Swift POINTDIR output.
 *		Added the IPN_RAW & IPN_POS unpacking.
 *		Finished the AGILE handling.							07Sep08
 *		Change all forms of naming GLAST to FERMI.
 *		Finished the Fermi-GBM handling.						09nov08
 *		Added the 2 lines for #define TYPE_SWIFT_TOO_FOM and
 *		TYPE_SWIFT_TOO_SC_SLEW  handling.						09nov08
 *		Deleted mistake CosmicRay_Hit in Swift-XRT_Image.
 *		Added missing BrightStar & NGC flag exractions for several
 *		Swift packet types.										15nov08
 *
 */


// Please note that most of these include_files are the ones
// for SunOS Version 4.1.3 1 and LINUX.
// On other machines with different operating systems (and even different
// versions of SunOS), these include_files may have different names.
// If, when you compile socket_demo.c on your local platform, you find
// that one or more of these files are "not found", then you will have
// to explore your system's include_files to find the contents that are missing
// during the compilation process.  Unfortunately, there are too many
// combinations of op_sys's for me to incorporate the appropriate 
// include_files.
#include <stdio.h>              // Standard i/o header file
#include <sys/types.h>          // File typedefs header file
#include <sys/socket.h>         // Socket structure header file
#include <signal.h>
#include <unistd.h>
#include <math.h>
#ifndef LINUXPC
#include <sys/filio.h>
#include <string.h>
#endif
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <netdb.h>
#include <time.h>
#include <sys/time.h>
#include <errno.h>
#include <strings.h>

char	*version = "socket_demo     Ver: 3.37   15 Nov 08";

#define	TRUE	1
#define	FALSE	0


// Indices into the socket packet array of 40 longs.  The naming and
// definition of these 40 is based on the contents for the first packet
// defined (the grb_coords packets; type=1).  The packing for the other packet
// types reuses some of these 40, but they have different meaning/content.
#define PKT_TYPE      0		// Packet type (see below)
#define PKT_SERNUM    1		// Packet serial number (1 to 4billion)
#define PKT_HOP_CNT   2		// Packet hop counter
#define PKT_SOD       3		// Packet Sec-of-Day [centi-secs] (sssss.ss*100)
#define BURST_TRIG    4		// BATSE Trigger Number
#define BURST_TJD     5		// Truncated Julian Day
#define BURST_SOD     6		// Sec-of-Day [centi-secs] (sssss.ss*100)
#define BURST_RA      7		// RA  [centi-deg] (0.0 to 359.99 *100)
#define BURST_DEC     8		// Dec [centi-deg] (-90.0 to +90.0 *100)
#define BURST_INTEN   9		// Burst intensity/fluence [cnts]
#define BURST_PEAK   10		// Burst intensity [cnts]
#define BURST_ERROR  11		// Position uncertainty [deg] (radius)
#define SC_AZ        12		// Burst SC Az [centi-deg] (0.0 to 359.99 *100)
#define SC_EL        13		// Burst SC El [centi-deg] (-90.0 to +90.0 *100)
#define SC_X_RA      14		// SC X-axis RA [centi-deg] (0.0 to 359.99 *100)
#define SC_X_DEC     15		// SC X-axis Dec [centi-deg] (-90.0 to +90.0 *100)
#define SC_Z_RA      16		// SC Z-axis RA [centi-deg] (0.0 to 359.99 *100)
#define SC_Z_DEC     17		// SC Z-axis Dec [centi-deg] (-90.0 to +90.0 *100)
#define TRIGGER_ID   18		// Trigger type identification flags
#define MISC         19		// Misc indicators
#define E_SC_AZ      20		// Earth's center in SC Az
#define E_SC_EL      21		// Earth's center in SC El
#define SC_RADIUS    22		// Orbital radius of the GRO SC
#define BURST_T_PEAK 23		// Peak intensity time [centi-sec] (sssss.sss*100)
#define PKT_SPARE23  23		// Beginning of spare section
#define PKT_SPARE38  38		// End of spare section
#define PKT_TERM     39		// Packet termination char
// NOTE:  Some of the fields above are used in different ways
// for the various different packet Types.
// You should see the socket_packet_definition_document for the details 
// for each field within each packet type.  The various pr_*() routines
// can also be of use in determining the packing contents & quantization.
// The socket packet definition document is located at:
// http://gcn.gsfc.nasa.gov/gcn/sock_pkt_def_doc.html

#define SIZ_PKT      40		// Number of longwords in socket packet

// Masks for extractions for the GRO-BATSE-based TRIGGER_ID in the packet:
#define SUSP_GRB            0x00000001	// Suspected GRB
#define DEF_GRB             0x00000002	// Definitely a GRB
#define NEAR_SUN            0x00000004	// Coords is near the Sun (<15deg)
#define SOFT_SF             0x00000008	// Spectrum is soft, h_ratio > 2.0
#define SUSP_PE             0x00000010	// Suspected Particle Event
#define DEF_PE              0x00000020	// Definitely a Particle Event
#define X_X_X_X_XXX         0x00000040	// spare
#define DEF_UNK             0x00000080	// Definitely an Unknown
#define EARTHWARD           0x00000100	// Location towards Earth center
#define SOFT_FLAG           0x00000200	// Small hardness ratio (>1.5)
#define NEAR_SAA            0x00000400	// S/c is near/in the SAA region
#define DEF_SAA             0x00000800	// Definitely an SAA region
#define SUSP_SF             0x00001000	// Suspected Solar Flare
#define DEF_SF              0x00002000	// Definitely a Solar Flare
#define OP_FLAG             0x00004000	// Op-dets flag set
#define DEF_NOT_GRB         0x00008000	// Definitely not a GRB event
#define ISO_PE              0x00010000	// Datelowe Iso param is small (PE)
#define ISO_GRB             0x00020000	// D-Iso param is large (GRB/SF)
#define NEG_H_RATIO         0x00040000	// Negative h_ratio
#define NEG_ISO_BC          0x00080000	// Negative iso_part[1] or iso_part[2]
#define NOT_SOFT            0x00100000	// Not soft flag, GRB or PE
#define HI_ISO_RATIO        0x00200000	// Hi C3/C2 D-Iso ratio
#define LOW_INTEN           0x00400000	// Inten too small to be a real GRB

// Masks for extractions for the GRO-BATSE-based MISC in the packet:
#define TM_IND_MASK         0x00000001	// TM Indicator mask
#define BAD_CALC_MASK       0x00000002	// Singular matrix (bad calculation)
#define C_FOV_MASK          0x00000004	// COMPTEL FOV indicator mask
#define E_FOV_MASK          0x00000008	// EGRET FOV indicator mask
#define O_FOV_MASK          0x00000010	// OSSE FOV indicator mask
#define PROG_LEV_MASK       0x000000E0	// Program Algorithm Level mask
#define NOTICE_TYPE_MASK    0x00000300	// Notification Type mask
#define A_ASD_MASK          0x00000400	// ALEXIS Anti-Solar Dir ind mask
#define IPN_PEAK_MASK       0x00000800	// Passed the IPN Peak Inten thresh
#define IPN_FLUE_MASK       0x00001000	// Passed the IPN Fluence thresh
#define XTE_CRITERIA        0x00002000	// Meets XTE-PCA follow-up criteria
#define OBS_STATUS          0x00004000	// Won't/will obs & Didn't/did see
//#define SPARE2_MASK       0x00007800	// spare bits
#define NO_TRIG_MASK        0x00008000	// BATSE is not in a triggerable mode
#define VER_MINOR_MASK      0x0FFF0000	// Program Minor Version Number mask
#define VER_MAJOR_MASK      0xF0000000	// Program Major Version Number mask


// The HETE-based packet word definitions:
// The definitions for 0-8 are the same as in the BATSE-based packet.
#define H_TRIG_FLAGS	9	// See trigger flag bits below
#define H_GAMMA_CTS_S	10	// cts/s defined by gamma, can be 0
#define H_WXM_S_N 		11	// Trigger score [S/N]
#define H_SXC_CTS_S		12	// cts/s defined by SXC, can be 0
#define H_GAMMA_TSCALE	13	// Timescale in milliseconds
#define H_WXM_TSCALE	14	// Timescale in milliseconds
#define H_POINTING		15	// RA/DEC in upper/lower word [deg]
#define H_WXM_CNR1_RA	16	// 1st corner of WXM error box
#define H_WXM_CNR1_DEC	17	// 1st corner of WXM error box
#define H_WXM_CNR2_RA	18	// 2nd corner of WXM error box
#define H_WXM_CNR2_DEC	19	// 2nd corner of WXM error box
#define H_WXM_CNR3_RA	20	// 3rd corner of WXM error box
#define H_WXM_CNR3_DEC	21	// 3rd corner of WXM error box
#define H_WXM_CNR4_RA	22	// 4th corner of WXM error box
#define H_WXM_CNR4_DEC	23	// 4th corner of WXM error box
#define H_WXM_ERRORS	24	// Arcsec, stat high, sys low
#define H_WXM_DIM_NSIG	25	// Dim [arcsec] << 16 | significance
#define H_SXC_CNR1_RA	26	// 1st corner of SXC error box
#define H_SXC_CNR1_DEC	27	// 1st corner of SXC error box
#define H_SXC_CNR2_RA	28	// 2nd corner of SXC error box
#define H_SXC_CNR2_DEC	29	// 2nd corner of SXC error box
#define H_SXC_CNR3_RA	30	// 3rd corner of SXC error box
#define H_SXC_CNR3_DEC	31	// 3rd corner of SXC error box
#define H_SXC_CNR4_RA	32	// 4th corner of SXC error box
#define H_SXC_CNR4_DEC	33	// 4th corner of SXC error box
#define H_SXC_ERRORS	34	// Arcsec, stat high, sys low
#define H_SXC_DIM_NSIG	35	// Dim [arcsec] << 16 | significance
#define H_POS_FLAGS		36	// Collection of various info bits
#define H_VALIDITY		37	// Tells whether burst is valid

// HETE word and bit mask and shift definitions:
#define H_MAXDIM_MASK	0xffff0000	// Dimension in upper bits
#define H_MAXDIM_SHIFT	16			// Dimension in upper bits
#define H_NSIG_MASK		0x0000ffff	// Significance in lower bits
#define H_NSIG_SHIFT	0			// Significance in lower bits
#define H_P_DEC_MASK	0x0000ffff	// Dec in lower word
#define H_P_DEC_SHIFT	0			// Dec in lower word
#define H_P_RA_MASK		0xffff0000	// RA in upper word
#define H_P_RA_SHIFT	16			// RA in upper word
#define H_SEQNUM_MASK	0xffff0000	// Message number in low bits
#define H_SEQNUM_SHIFT	16			// Message number in low bits
#define H_STATERR_MASK	0xffff0000	// Statistical error in upper bits
#define H_STATERR_SHIFT	16			// Statistical error in upper bits
#define H_SYSERR_MASK	0x0000ffff	// Systematic error in lower bits
#define H_SYSERR_SHIFT	0			// Systematic error in lower bits
#define H_TRIGNUM_MASK	0x0000ffff	// Trigger number in low bits
#define H_TRIGNUM_SHIFT	0			// Trigger number in low bits
#define H_WHO_TRIG_MASK	0x00000007	// Gamma, XG, or SXC
#define H_X_LC_SN		0xff000000	// WXM X LC (aka Virtue) value in slot 38
#define H_Y_LC_SN		0x00ff0000	// WXM Y LC (aka Virtue) value in slot 38
#define H_X_IMAGE_SN	0x0000ff00	// WXM X (aka Goodness) value in slot 38
#define H_Y_IMAGE_SN	0x000000ff	// WXM Y (aka Goodness) value in slot 38

// HETE H_TRIG_FLAGS bit_flags definitions:
#define H_GAMMA_TRIG	0x00000001	// Did FREGATE trigger? 1=yes
#define H_WXM_TRIG		0x00000002	// Did WXM trigger?
#define H_SXC_TRIG		0x00000004	// Did SXC trigger?
#define H_ART_TRIG		0x00000008	// Artificial trigger
#define H_POSS_GRB		0x00000010	// Possible GRB
#define H_DEF_GRB 		0x00000020	// Definite GRB
#define H_DEF_NOT_GRB	0x00000040	// Definitely NOT a GRB
#define H_NEAR_SAA		0x00000080	// S/c is in or near the SAA
#define H_POSS_SGR		0x00000100	// Possible SGR
#define H_DEF_SGR 		0x00000200	// Definite SGR
#define H_POSS_XRB		0x00000400	// Possible XRB
#define H_DEF_XRB 		0x00000800	// Definite XRB
#define H_GAMMA_DATA	0x00001000	// FREGATE (gamma) data in this message
#define H_WXM_DATA		0x00002000	// WXM data in this message
#define H_SXC_DATA		0x00004000	// SXC data in this message
#define H_OPT_DATA		0x00008000	// OPT (s/c ACS) data in this message
#define H_WXM_POS 		0x00010000	// WXM position in this message
#define H_SXC_POS 		0x00020000	// SXC position in this message
#define H_TRIG_spare1	0x000C0000	// spare1
#define H_USE_TRIG_SN	0x00400000	// Use H_WXM_S_N
#define H_TRIG_spare2	0x00800000	// spare2
#define H_SXC_EN_TRIG	0x01000000	// Triggerred in the 1.5-12 keV band
#define H_LOW_EN_TRIG	0x02000000	// Triggerred in the 2-30 keV band
#define H_MID_EN_TRIG	0x04000000	// Triggerred in the 6-120 keV band
#define H_HI_EN_TRIG	0x08000000	// Triggerred in the 25-400 keV band
#define H_PROB_XRB		0x10000000	// Probable XRB
#define H_PROB_SGR		0x20000000	// Probable SGR
#define H_PROB_GRB		0x40000000	// Probable GRB
#define H_TRIG_spare3	0x80000000	// spare3

// HETE POS_FLAGS bitmap definitions:
#define H_WXM_SXC_SAME	0x00000001	// Positions are consistent
#define H_WXM_SXC_DIFF	0x00000002	// Positions are inconsistent
#define H_WXM_LOW_SIG	0x00000004	// NSIG below a threshold
#define H_SXC_LOW_SIG	0x00000008	// NSIG below a threshold
#define H_GAMMA_REFINED	0x00000010	// Gamma refined since FINAL
#define H_WXM_REFINED	0x00000020	// WXM refined since FINAL
#define H_SXC_REFINED	0x00000040	// SXC refined since FINAL
#define H_POS_spare		0x00FFFFB0	// spare
#define H_SC_LONG		0xFF000000	// S/C Longitude

// HETE VALIDITY flag bit masks:
#define H_BURST_VALID	0x00000001	// Burst declared valid
#define H_BURST_INVALID	0x00000002	// Burst declared invalid
#define H_VAL0x4_spare	0x00000004	// spare
#define H_VAL0x8_spare	0x00000008	// spare
#define H_EMERGE_TRIG	0x00000010	// Emersion trigger
#define H_KNOWN_XRS		0x00000020	// Known X-ray source
#define H_NO_POSITION	0x00000040	// No WXM or SXC position
#define H_VAL0x80_spare	0x00000080	// spare
#define H_OPS_ERROR		0x00000100	// Bad burst: s/c & inst ops error
#define H_PARTICLES		0x00000200	// Bad burst: particles
#define H_BAD_FLT_LOC	0x00000400	// Bad burst: bad flight location
#define H_BAD_GND_LOC	0x00000800	// Bad burst: bad ground location
#define H_RISING_BACK	0x00001000	// Bad burst: rising background
#define H_POISSON_TRIG	0x00002000	// Bad burst: poisson fluctuation trigger
#define H_OUTSIDE_FOV	0x00004000	// Good burst: but outside WSX/SXC FOV
#define H_IPN_CROSSING	0x00008000	// Good burst: IPN crossing match
#define H_VALID_spare	0x1FFFC000	// spare
#define H_EMAIL_METHOD	0x20000000	// email import method(=1, socket=0)
#define H_INTERNAL_TEST	0x40000000	// HETE_OPS GndAna test/debug loop
#define H_NOT_A_BURST	0x80000000	// GndAna shows this trigger to be non-GRB

// INTEGRAL word and bit mask and shift definitions:
#define I_TRIGNUM_MASK	0x0000ffff	// Trigger number in low bits
#define I_TRIGNUM_SHIFT	0			// Trigger number in low bits
#define I_SEQNUM_MASK	0xffff0000	// Message number in low bits
#define I_SEQNUM_SHIFT	16			// Message number in low bits

// The Swift-based packet word definitions:
#define BURST_URL     22            // Location of the start of the URL string in the pkt

// SWIFT word and bit mask and shift definitions:
#define S_TRIGNUM_MASK	0x00ffffff	// Trigger number in low bits
#define S_TRIGNUM_SHIFT	0			// Trigger number in low bits
#define S_SEGNUM_MASK	0x000000ff	// Observation Segment number in upper bits
#define S_SEGNUM_SHIFT	24			// Observation Segment number in upper bits


// The GCN defined packet types (missing numbers are gcn-internal use only):
// Types that are commented out are no longer available (eg GRO de-orbit).
#define TYPE_UNDEF               0  // This packet type is undefined
//#define TYPE_GRB_COORDS        1  // BATSE-Original Trigger coords packet
#define TYPE_TEST_COORDS         2  // Test coords packet
#define TYPE_IM_ALIVE            3  // I'm_alive packet
#define TYPE_KILL_SOCKET         4  // Kill a socket connection packet
//#define TYPE_MAXBC            11  // MAXC1/BC packet
#define TYPE_BRAD_COORDS        21  // Special Test coords packet for BRADFORD
//#define TYPE_GRB_FINAL        22  // BATSE-Final coords packet
//#define TYPE_HUNTS_SRC        24  // Huntsville LOCBURST GRB coords packet
//#define TYPE_ALEXIS_SRC       25  // ALEXIS Transient coords packet
#define TYPE_XTE_PCA_ALERT      26  // XTE-PCA ToO Scheduled packet
#define TYPE_XTE_PCA_SRC        27  // XTE-PCA GRB coords packet
#define TYPE_XTE_ASM_ALERT      28  // XTE-ASM Alert packet
#define TYPE_XTE_ASM_SRC        29  // XTE-ASM GRB coords packet
//#define TYPE_COMPTEL_SRC      30  // COMPTEL GRB coords packet
#define TYPE_IPN_RAW_SRC        31  // IPN_RAW GRB time-only or annulus packet
#define TYPE_IPN_SEG_SRC        32  // IPN_SEGment GRB annulus segment coords pkt
//#define TYPE_SAX_WFC_ALERT    33  // SAX-WFC Alert packet		// SAX no longer available
//#define TYPE_SAX_WFC_SRC      34  // SAX-WFC GRB coords packet
//#define TYPE_SAX_NFI_ALERT    35  // SAX-NFI Alert packet
//#define TYPE_SAX_NFI_SRC      36  // SAX-NFI GRB coords packet
#define TYPE_XTE_ASM_TRANS      37  // XTE-ASM TRANSIENT coords packet
#define TYPE_spare_SRC          38  // spare
#define TYPE_IPN_POS_SRC        39  // IPN_POSition coords packet
#define TYPE_HETE_ALERT_SRC     40  // HETE S/C_Alert packet    // HETE no longer available
#define TYPE_HETE_UPDATE_SRC    41  // HETE S/C_Update packet
#define TYPE_HETE_FINAL_SRC     42  // HETE S/C_Last packet
#define TYPE_HETE_GNDANA_SRC    43  // HETE Ground Analysis packet
#define TYPE_HETE_TEST          44  // HETE Test packet
#define TYPE_GRB_CNTRPART_SRC   45  // GRB Counterpart coords packet
#define TYPE_SWIFT_TOO_FOM      46  // SWIFT TOO-form of the FOM packet
#define TYPE_SWIFT_TOO_SC_SLEW  47  // SWIFT TOO-form of the SC_SLEW packet
#define TYPE_INTEGRAL_POINTDIR_SRC  51  // INTEGRAL Pointing Dir packet
#define TYPE_INTEGRAL_SPIACS_SRC    52  // INTEGRAL SPIACS packet
#define TYPE_INTEGRAL_WAKEUP_SRC    53  // INTEGRAL Wakeup packet
#define TYPE_INTEGRAL_REFINED_SRC   54  // INTEGRAL Refined packet
#define TYPE_INTEGRAL_OFFLINE_SRC   55  // INTEGRAL Offline packet
#define TYPE_SNEWS_POS_SRC          57  // SNEWS Position/Test message
//#define TYPE_MILAGRO_POS_SRC      58  // MILAGRO Position message
#define TYPE_KONUS_LC_SRC           59  // KONUS Lightcurve message
#define TYPE_SWIFT_BAT_GRB_ALERT_SRC     60  // SWIFT BAT GRB ALERT message
#define TYPE_SWIFT_BAT_GRB_POS_ACK_SRC   61  // SWIFT BAT GRB Position Acknowledge message
#define TYPE_SWIFT_BAT_GRB_POS_NACK_SRC  62  // SWIFT BAT GRB Position NOT Acknowledge message
#define TYPE_SWIFT_BAT_GRB_LC_SRC        63  // SWIFT BAT GRB Lightcurve message
#define TYPE_SWIFT_SCALEDMAP_SRC         64  // SWIFT BAT Scaled Map message
#define TYPE_SWIFT_FOM_2OBSAT_SRC        65  // SWIFT BAT FOM to Observe message
#define TYPE_SWIFT_FOSC_2OBSAT_SRC       66  // SWIFT BAT S/C to Slew message
#define TYPE_SWIFT_XRT_POSITION_SRC      67  // SWIFT XRT Position message
#define TYPE_SWIFT_XRT_SPECTRUM_SRC      68  // SWIFT XRT Spectrum message
#define TYPE_SWIFT_XRT_IMAGE_SRC         69  // SWIFT XRT Image message (aka postage stamp)
#define TYPE_SWIFT_XRT_LC_SRC            70  // SWIFT XRT Lightcurve message (aka Prompt)
#define TYPE_SWIFT_XRT_CENTROID_SRC      71  // SWIFT XRT Position NOT Ack message (Centroid Error)
#define TYPE_SWIFT_UVOT_IMAGE_SRC        72  // SWIFT UVOT Image message (aka DarkBurst, aka GeNie)
#define TYPE_SWIFT_UVOT_SLIST_SRC        73  // SWIFT UVOT Source List message (aka FindChart)
#define TYPE_SWIFT_FULL_DATA_INIT_SRC    74  // SWIFT Full Data Set Initial message
#define TYPE_SWIFT_FULL_DATA_UPDATE_SRC  75  // SWIFT Full Data Set Updated message
#define TYPE_SWIFT_BAT_GRB_LC_PROC_SRC   76  // SWIFT BAT GRB Lightcurve processed message
#define TYPE_SWIFT_XRT_SPECTRUM_PROC_SRC 77  // SWIFT XRT Spectrum processed message
#define TYPE_SWIFT_XRT_IMAGE_PROC_SRC    78  // SWIFT XRT Image processed message
#define TYPE_SWIFT_UVOT_IMAGE_PROC_SRC   79  // SWIFT UVOT Image processed mesg (aka DarkBurst, aka GeNie)
#define TYPE_SWIFT_UVOT_SLIST_PROC_SRC   80  // SWIFT UVOT Source List processed message (aka FindChart)
#define TYPE_SWIFT_UVOT_POS_SRC          81  // SWIFT UVOT Position message
#define TYPE_SWIFT_BAT_GRB_POS_TEST      82  // SWIFT BAT GRB Position Test message
#define TYPE_SWIFT_POINTDIR_SRC          83  // SWIFT Pointing Direction message
#define TYPE_SWIFT_BAT_TRANS             84  // SWIFT BAT Transient Position message
#define TYPE_SWIFT_UVOT_POS_NACK_SRC     89  // SWIFT UVOT Position Nack message
//#define TYPE_SWIFT_TOO                 97  // SWIFT TOO non-public message (this will never exist)
#define TYPE_SWIFT_BAT_SUB_THRESHOLD_SRC 98  // SWIFT BAT Sub-Threshold Position message
#define TYPE_SWIFT_BAT_SLEW_POS_SRC      99  // SWIFT BAT SLEW Burst/Transient Position (the Harvard BATSS thing)
#define TYPE_AGILE_GRB_WAKEUP            100  // AGILE GRB Wake-Up Position message
#define TYPE_AGILE_GRB_PROMPT            101  // AGILE GRB Prompt Position message
#define TYPE_AGILE_GRB_REFINED           102  // AGILE GRB Refined Position message
#define TYPE_spare103_106                103  // values 103 thru 106 are not yet assigned
#define TYPE_AGILE_POINTDIR              107  // AGILE Pointing Direction
#define TYPE_AGILE_TRANS                 108  // AGILE Transient Position message
#define TYPE_AGILE_GRB_POS_TEST          109  // AGILE GRB Position Test message
#define TYPE_FERMI_GBM_ALERT             110  // FERMI GBM Trigger Alert message
#define TYPE_FERMI_GBM_FLT_POS           111  // FERMI GBM Flt-cal Position message
#define TYPE_FERMI_GBM_GND_POS           112  // FERMI GBM Gnd-cal Position message
#define TYPE_FERMI_GBM_LC                113  // FERMI GBM GRB Lightcurve message
#define TYPE_spare114_117                117  // values 114 thru 117 are not yet assigned
#define TYPE_FERMI_GBM_TRANS             118  // FERMI GBM Transient Position mesg
#define TYPE_FERMI_GBM_POS_TEST          119  // FERMI GBM GRB Position Test message
#define TYPE_FERMI_LAT_POS_INI           120  // FERMI LAT Position Initial message
#define TYPE_FERMI_LAT_POS_UPD           121  // FERMI LAT Position Update message
#define TYPE_FERMI_LAT_POS_DIAG          122  // FERMI LAT Position Diagnostic message
#define TYPE_FERMI_LAT_TRANS             123  // FERMI LAT Transient Position mesg (eg AGN, Solar Flare)
#define TYPE_FERMI_LAT_POS_TEST          124  // FERMI LAT Position Test message
#define TYPE_FERMI_OBS_REQ               125  // FERMI Observe_Request message
#define TYPE_FERMI_SC_SLEW               126  // FERMI SC_Slew message
#define TYPE_FERMI_LAT_GND_REF           127  // FERMI LAT Ground-analysis Refined Pos message
#define TYPE_FERMI_LAT_GND_TRIG          128  // FERMI LAT Ground-analysis Trigger Pos message
#define TYPE_FERMI_POINTDIR              129  // FERMI Pointing Direction


// A few global variables for the socket_demo program:
int		inetsd;				// Socket descriptor
FILE	*lg;				// Logfile stream pointer
int		errno;				// Error return code number
time_t	tloc;				// Seconds since machine epoch
long	here_sod;			// Machine GMT converted to sec-of-day
long	last_here_sod;		// When the last imalive packet arrived
double	last_imalive_sod;	// SOD of the previous imalive packet

#define FIND_SXC	0		// Used in hete_same()
#define FIND_WXM	1		// Ditto; check the corners of a WXM box

/*---------------------------------------------------------------------------*/
void
brokenpipe()
{
printf(    "brokenpipe(): If this is the last line, then the shutdown() to GCN hung.\n");
fprintf(lg,"brokenpipe(): If this is the last line, then the shutdown() to GCN hung.\n");
if(shutdown(inetsd,2) == -1)
	{
	serr(0,"ERR: brokenpipe(): The shutdown did NOT work.\n");
	printf(    "ERR: brokenpipe(): The shutdown did NTO work. errno=%d\n",errno);
	fprintf(lg,"ERR: brokenpipe(): The shutdown did NTO work. errno=%d\n",errno);
	}
else
	{
	printf(    "brokenpipe(): The shutdown worked OK.\n");
	fprintf(lg,"brokenpipe(): The shutdown worked OK.\n");
	}
printf(    "brokenpipe(): After the shutdown call; the shutdown didn't hang.\n");
fprintf(lg,"brokenpipe(): After the shutdown call; the shutdown didn't hang.\n");
if(close(inetsd))
	{
	perror("brokenpipe(), close(): ");
	printf(    "ERR: close() problem in brokenpipe(), errno=%d\n",errno);
	fprintf(lg,"ERR: close() problem in brokenpipe(), errno=%d\n",errno);
	}
inetsd = -1;
}

/*---------------------------------------------------------------------------*/
main(argc, argv)
int				argc;			// Number of cmdline arguments
char			*argv[];		// Pointers to all the cmdline arguments
{
int				port;			// Port number of the socket to connect to
int				bytes;			// Number of bytes read from the socket
char			hostname[64];	// Machine host name
long			lbuf[SIZ_PKT];	// The packet data buffer
int				bad_type_cnt=0;	// Count number of bad pkt types received
#ifdef LINUXPC
int				i;				// General loop variable
static long		nbuf[SIZ_PKT];	// Swap space for different endian platforms
short			*sp;			// Ptr to a short; used for the swapping
short			pl, ph;			// Low part & high part
int				siz_lbuf;		// Variable form of the size of the lbuf
#endif
struct sigvec	vec;			// Signal vector structure
struct tm 		*t;				// Pointer to time struct of machine GMT

#ifdef LINUXPC
siz_lbuf = SIZ_PKT*sizeof(lbuf[0]);
#endif

if(argc != 2)
	{
	printf("USAGE:  socket_demo  <port_num>\n");
	exit(1);
	}

port = atoi(argv[1]);	// Get the cmdline value for the port number

if((lg = fopen("socket_demo.log", "a")) == NULL)	// Open for appending
	{
	printf("Failed to open logfile.  Exiting.\n");
	exit(2);
	}
fprintf(lg,"\n===================== New Session ========================\n");
fprintf(lg,"%s\n",version);
fprintf(lg,"port=%d\n",port);

// The following sets up the signal catcher that is used to detect when
// the socket has been broken.
vec.sv_handler = brokenpipe;
vec.sv_mask = sigmask(SIGPIPE);
vec.sv_flags = 0;
if(sigvec(SIGPIPE, &vec, (struct sigvec *) NULL) == -1)
	serr(0, "Sigvec did not work for brokenpipe.\n");

bzero((char *)lbuf,sizeof(lbuf));	// Clears the buffer
#ifdef LINUXPC
bzero((char *)nbuf,sizeof(nbuf));	// Clears the buffer
#endif
// Calls the subroutine to initiate the socket connection:
//   hostname is not needed for an INET connection,
//   port number is defined/initialized above, and
//   AF_INET is for an inet connection.
// The return value is the sock descriptor.
inetsd = server(hostname, port, AF_INET);


// An infinite loop for reading the socket, echoing the packet back
// to the GCN originator, and handling the reconnection if there is a break.
// Also in this loop is your site/instrument-specific code (if you want).
while(1)
	{
	// This printf is just a "busy light" diagnostic.
	// printf("while(1): inetsd=%d, bytes=%d, errno=%d, bad_type_cnt=%d\n",
	//							inetsd,bytes,errno,bad_type_cnt);

	time((time_t *)&tloc);

	if(inetsd <= 0)
		{
		// If the sock descriptor is zero or less, the socket connection has been
		// broken and the call to server() will try to re-establish the socket.
		inetsd = server(hostname, port, AF_INET);
		}
	else
		{
		// If socket is connected, read the socket for packet; if the socket
		// does not have a packet, continue to loop.
#		ifdef LINUXPC
		// Byte and/or word swapping may be needed on your particular
		// platform.  These packets were generated on a Sun4, so if the
		// byte ordering for long-integer storage is different than Sun4
		// on your platform, you will need to rearrange the bytes in lbuf
		// accordingly.  Insert that code here.

		// Read into nbuf, swap into lbuf.
		if((bytes=read(inetsd,(char *)nbuf,sizeof(nbuf))) > 0)
			{
			// Immediately echo back the packet so GCN can monitor:
			// (1) the actual receipt by the site, and
			// (2) the roundtrip travel times.
			// Everything except KILL's get echo-ed back.
			if(nbuf[PKT_TYPE] != TYPE_KILL_SOCKET)
				write(inetsd,(char *)nbuf,sizeof(nbuf)); 
			swab(nbuf,lbuf,siz_lbuf);
			sp = (short *)lbuf;
			for(i=0;i<SIZ_PKT;i++)
				{
				pl = sp[2*i];
				ph = sp[2*i + 1];
				sp[2*i] = ph;
				sp[2*i + 1] = pl;
				}
#		else
		if((bytes=read(inetsd,(char *)lbuf,sizeof(lbuf))) > 0)
			{
			// Immediately echo back the packet so GCN can monitor:
			// (1) the actual receipt by the site, and
			// (2) the roundtrip travel times.
			// Everything except KILL's get echo-ed back.
			if(lbuf[PKT_TYPE] != TYPE_KILL_SOCKET)
				write(inetsd,(char *)lbuf,sizeof(lbuf)); 
#		endif
			t = gmtime((time_t *)&tloc);
			here_sod = t->tm_hour*3600 + t->tm_min*60 + t->tm_sec;

			switch(lbuf[PKT_TYPE])
				{
				case TYPE_IM_ALIVE:
					pr_imalive(lbuf,stdout);// Dump pkt contents to the screen
					pr_imalive(lbuf,lg);	// Dump pkt contents to the logfile
					last_here_sod = here_sod;
					last_imalive_sod = lbuf[PKT_SOD]/100.0;
					chk_imalive(1,tloc);	// Pass the time of latest imalive received
					break;
				//case TYPE_GRB_COORDS:		// BATSE-Original (no longer available)
				//case TYPE_MAXBC:			// BATSE-MAXBC    (no longer available)
				//case TYPE_GRB_FINAL:		// BATSE-Final    (no longer available)
				//case TYPE_HUNTS_SRC:		// BATSE-LOCBURST (no longer available)
				case TYPE_TEST_COORDS:		// Test Coordinates
				case TYPE_BRAD_COORDS:		// Special Bradford Test Coords
					pr_packet(lbuf,stdout);	// Dump pkt contents to the screen
					pr_packet(lbuf,lg);		// Dump pkt contents to the logfile
					break;
				//case TYPE_ALEXIS_SRC:		// ALEXIS extreme-UV transients (no longer available)
				//	pr_alexis(lbuf,stdout);
				//	pr_alexis(lbuf,lg);
				//	break;
				case TYPE_XTE_PCA_ALERT:
				case TYPE_XTE_PCA_SRC:
					pr_xte_pca(lbuf,stdout);
					pr_xte_pca(lbuf,lg);
					break;
				//case TYPE_XTE_ASM_ALERT:	// Not implimented yet
				case TYPE_XTE_ASM_SRC:
					pr_xte_asm(lbuf,stdout);
					pr_xte_asm(lbuf,lg);
					break;
				case TYPE_XTE_ASM_TRANS:
					pr_xte_asm_trans(lbuf,stdout);
					pr_xte_asm_trans(lbuf,lg);
					break;
				case TYPE_IPN_RAW_SRC:
					pr_ipn_raw(lbuf,stdout);
					pr_ipn_raw(lbuf,lg);
					break;
				case TYPE_IPN_SEG_SRC:		// Not implimented for sockets yet
					pr_ipn_seg(lbuf,stdout);
					pr_ipn_seg(lbuf,lg);
					break;
				case TYPE_IPN_POS_SRC:
					pr_ipn_pos(lbuf,stdout);
					pr_ipn_pos(lbuf,lg);
					break;
				//case TYPE_SAX_WFC_ALERT:	// BeppoSAX     (no longer available)
				//case TYPE_SAX_WFC_SRC:
				//	pr_sax_wfc(lbuf,stdout);
				//	pr_sax_wfc(lbuf,lg);
				//	break;
				//case TYPE_SAX_NFI_SRC:
				//	pr_sax_nfi(lbuf,stdout);
				//	pr_sax_nfi(lbuf,lg);
				//	break;
				//case TYPE_HETE_ALERT_SRC:	// HETE  (no longer available)
				//case TYPE_HETE_UPDATE_SRC:
				//case TYPE_HETE_FINAL_SRC:
				//case TYPE_HETE_GNDANA_SRC:
				case TYPE_HETE_TEST:		// The Test form is still available
					pr_hete(lbuf,stdout);
					pr_hete(lbuf,lg);
					break;
				case TYPE_GRB_CNTRPART_SRC:
					pr_grb_cntrpart(lbuf,stdout);
					pr_grb_cntrpart(lbuf,lg);
					break;
				case TYPE_INTEGRAL_POINTDIR_SRC:
					pr_integral_point(lbuf,stdout);
					pr_integral_point(lbuf,lg);
					break;
				case TYPE_INTEGRAL_SPIACS_SRC:
					pr_integral_spiacs(lbuf,stdout);
					pr_integral_spiacs(lbuf,lg);
					break;
				case TYPE_INTEGRAL_WAKEUP_SRC:
				case TYPE_INTEGRAL_REFINED_SRC:
				case TYPE_INTEGRAL_OFFLINE_SRC:
					pr_integral_w_r_o(lbuf,stdout);
					pr_integral_w_r_o(lbuf,lg);
					break;
				//case TYPE_MILAGRO_POS_SRC:           // 58  // No longer available
				//	pr_milagro(lbuf,stdout);
				//	pr_milagro(lbuf,lg);
				//	break;
				case TYPE_SWIFT_BAT_GRB_ALERT_SRC:     // 60  // SWIFT BAT GRB ALERT message
					pr_swift_bat_alert(lbuf,stdout);
					pr_swift_bat_alert(lbuf,lg);
					break;
				case TYPE_SWIFT_BAT_GRB_POS_ACK_SRC:   // 61  // SWIFT BAT GRB Pos Acknowledge mesg
				case TYPE_SWIFT_BAT_GRB_POS_TEST:      // 82  // SWIFT BAT GRB Test Position message
				case TYPE_SWIFT_BAT_TRANS:             // 84  // SWIFT BAT Transient Position mesg
				case TYPE_SWIFT_BAT_SUB_THRESHOLD_SRC: // 98  // SWIFT BAT Sub-Threshold Position mesg
					pr_swift_bat_pos_ack(lbuf,stdout);
					pr_swift_bat_pos_ack(lbuf,lg);
					break;
				case TYPE_SWIFT_BAT_GRB_POS_NACK_SRC:  // 62  // SWIFT BAT GRB Position NOT Ack mesg
					pr_swift_bat_pos_nack(lbuf,stdout);
					pr_swift_bat_pos_nack(lbuf,lg);
					break;
				case TYPE_SWIFT_BAT_GRB_LC_SRC:        // 63  // SWIFT BAT GRB Lightcurve message
					pr_swift_bat_lc(lbuf,stdout);
					pr_swift_bat_lc(lbuf,lg);
					break;
				case TYPE_SWIFT_SCALEDMAP_SRC:         // 64  // SWIFT BAT Scaled Map message
					printf(    "WARN: Should not be able to get a BAT Scaled Map.\n");
					fprintf(lg,"WARN: Should not be able to get a BAT Scaled Map.\n");
					break;
				case TYPE_SWIFT_FOM_2OBSAT_SRC:        // 65  // SWIFT BAT FOM to Observe message
				case TYPE_SWIFT_TOO_FOM:               // 46  // SWIFT TOO FOM message
					pr_swift_fom_2obs(lbuf,stdout);
					pr_swift_fom_2obs(lbuf,lg);
					break;
				case TYPE_SWIFT_FOSC_2OBSAT_SRC:       // 66  // SWIFT BAT S/C to Slew message
				case TYPE_SWIFT_TOO_SC_SLEW:           // 47  // SWIFT TOO SC_SLEW message
					pr_swift_sc_2slew(lbuf,stdout);
					pr_swift_sc_2slew(lbuf,lg);
					break;
				case TYPE_SWIFT_XRT_POSITION_SRC:      // 67  // SWIFT XRT Position message
					pr_swift_xrt_pos_ack(lbuf,stdout);
					pr_swift_xrt_pos_ack(lbuf,lg);
					break;
				case TYPE_SWIFT_XRT_SPECTRUM_SRC:      // 68  // SWIFT XRT Spectrum message
					pr_swift_xrt_spec(lbuf,stdout);
					pr_swift_xrt_spec(lbuf,lg);
					break;
				case TYPE_SWIFT_XRT_IMAGE_SRC:         // 69  // SWIFT XRT Image message
					pr_swift_xrt_image(lbuf,stdout);
					pr_swift_xrt_image(lbuf,lg);
					break;
				case TYPE_SWIFT_XRT_LC_SRC:            // 70  // SWIFT XRT Lightcurve mesg (aka Prompt)
					pr_swift_xrt_lc(lbuf,stdout);
					pr_swift_xrt_lc(lbuf,lg);
					break;
				case TYPE_SWIFT_XRT_CENTROID_SRC:      // 71  // SWIFT XRT Pos mesg (Centroid Error )
					pr_swift_xrt_pos_nack(lbuf,stdout);
					pr_swift_xrt_pos_nack(lbuf,lg);
					break;
				case TYPE_SWIFT_UVOT_IMAGE_SRC:        // 72  // SWIFT UVOT Image mesg (aka Neighbor)
					pr_swift_uvot_image(lbuf,stdout);
					pr_swift_uvot_image(lbuf,lg);
					break;
				case TYPE_SWIFT_UVOT_SLIST_SRC:        // 73  // SWIFT UVOT Source List message
					pr_swift_uvot_slist(lbuf,stdout);
					pr_swift_uvot_slist(lbuf,lg);
					break;

				// The 'processed' Swift versions (type=76-80) are essentially identical
				// to the 'raw' Swift versions, 63,68,69,72,73, respectively,
				// so they will not be duplicated here in this switch().

				case TYPE_SWIFT_UVOT_POS_SRC:          // 81  // SWIFT UVOT Position message
					pr_swift_uvot_pos(lbuf,stdout);
					pr_swift_uvot_pos(lbuf,lg);
					break;
				case TYPE_SWIFT_POINTDIR_SRC:          // 83  // Swift Pointing Direction
					pr_swift_point(lbuf,stdout);
					pr_swift_point(lbuf,lg);
					break;

				case TYPE_AGILE_GRB_WAKEUP:             // 100  // SuperAGILE GRB Wakeup Position
				case TYPE_AGILE_GRB_PROMPT:             // 101
				case TYPE_AGILE_GRB_REFINED:            // 102
				case TYPE_AGILE_TRANS:                  // 108
				case TYPE_AGILE_GRB_POS_TEST:           // 109
					pr_agile_pos_ack(lbuf,stdout);
					pr_agile_pos_ack(lbuf,lg);
					break;
				case TYPE_AGILE_POINTDIR:               // 107  // AGILE Pointing Direction
					pr_agile_point(lbuf,stdout);
					pr_agile_point(lbuf,lg);
					break;

				case TYPE_FERMI_GBM_ALERT:              // 110  // FERMI GBM Alert packet
					pr_fermi_gbm_alert(lbuf,stdout);
					pr_fermi_gbm_alert(lbuf,lg);
					break;
				case TYPE_FERMI_GBM_FLT_POS:            // 111  // FERMI GBM Flt-calc Position pkt
				case TYPE_FERMI_GBM_TRANS:              // 118  // FERMI GBM Transient Position pkt
				case TYPE_FERMI_GBM_POS_TEST:           // 119  // FERMI GBM Position Test packet
					pr_fermi_gbm_flt_pos(lbuf,stdout);
					pr_fermi_gbm_flt_pos(lbuf,lg);
					break;
				case TYPE_FERMI_GBM_GND_POS:            // 112  // FERMI GBM Gnd-calc Position pkt
					pr_fermi_gbm_gnd_pos(lbuf,stdout);
					pr_fermi_gbm_gnd_pos(lbuf,lg);
					break;
				case TYPE_FERMI_GBM_LC:                 // 113  // FERMI Lightcurve packet
					// NOOP for now
					break;
				case TYPE_FERMI_LAT_POS_INI:            // 120  // FERMI LAT Position Initial packet
				case TYPE_FERMI_LAT_POS_UPD:            // 121  // FERMI LAT Position Update packet
				case TYPE_FERMI_LAT_POS_DIAG:           // 122  // FERMI LAT Position Diagnostic packet
				case TYPE_FERMI_LAT_TRANS:              // 123  // FERMI LAT Transient Position pkt (eg AGN, Solar Flare)
				case TYPE_FERMI_LAT_POS_TEST:           // 124  // FERMI LAT Position Test pkt (I guess like the INI only)
				case TYPE_FERMI_LAT_GND_REF:            // 127  // FERMI LAT Ground-analysis Refined Pos pkt
				case TYPE_FERMI_LAT_GND_TRIG:           // 128  // FERMI LAT Ground-analysis Trigger Pos pkt
					pr_fermi_lat_pos(lbuf,stdout);
					pr_fermi_lat_pos(lbuf,lg);
					break;
				case TYPE_FERMI_POINTDIR:               // 129  // FERMI Pointing Direction
					pr_fermi_point(lbuf,stdout);
					pr_fermi_point(lbuf,lg);
					break;

				case TYPE_KILL_SOCKET:		// Signal to break connection
					printf(    "Got a KILL socket packet.\n");
					fprintf(lg,"Got a KILL socket packet.\n");
					pr_kill(lbuf,stdout);
					pr_kill(lbuf,lg);
					brokenpipe();
					break;
				default:					// Illegal packet type received
					bad_type_cnt++;
					printf(    "ERR: Unrecognized pkt type(=%d).\n",lbuf[PKT_TYPE]);
					fprintf(lg,"ERR: Unrecognized pkt type(=%d).\n",lbuf[PKT_TYPE]);
					break;
				}
			}
		else if((bytes == 0) && (errno != EWOULDBLOCK))
			{								// The connection is broken
			printf(    "bytes==0 && errno!=EWOULDBLOCK (errno=%d)\n",errno);
			fprintf(lg,"bytes==0 && errno!=EWOULDBLOCK (errno=%d)\n",errno);
			brokenpipe();
			}
		}

	chk_imalive(0,tloc);	// See if the time limit has been exceeded

	// This sleep() is ONLY for this demo program.  It should NOT be included
	// in your site's application code as it most definitely adds up to 1 sec
	// to the response time to the socket/packet servicing.
	// Replace it with your instrument-specific code if you choose to use
	// this demo program as the basis for your operations program.
	sleep(1);				// Give the CPU a rest
	}
}

/*--------------------------------------------------------------------------*/
int
server(hostname, port, type)
char                *hostname; // Machine host name
int                 port;      // Port number
unsigned short      type;      // Type of socket connection
{
int                 sock;      // The connected sock descriptor
int                 sd=-1;     // The offerred sock descriptor
int                 temp;      // Dummy variable
int                 saddrlen;  // Socket address length + 2
char                on=1;      // Flag for nonblocking I/O
struct sockaddr     saddr;     // Socket structure for UNIX
struct sockaddr     *psaddr;   // Pointer to sin
struct sockaddr_in  sin;       // Socket structure for inet


temp = 0;
// If the socket is for inet connection, then set up the sin structure.
if(type == AF_INET)
	{
	bzero((char *) &sin, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = htonl(INADDR_ANY);
#	ifdef LINUXPC
	sin.sin_port = htons(port);
#	else
	sin.sin_port = (unsigned short)port;
#	endif
	psaddr = (struct sockaddr *) &sin;
	saddrlen = sizeof(sin);
	}
else
	{
	// If the socket is for UNIX connection, then set up the saddr structure.
	saddr.sa_family = AF_UNIX;
	strcpy(saddr.sa_data, hostname);
	psaddr = &saddr;
	saddrlen = strlen(saddr.sa_data) + 2;
	}
 
printf("server(): type=%d  (int)type=%d\n",type,(int)type);  // Debug

// Initiate the socket connection.
if((sd = socket((int)type, SOCK_STREAM, IPPROTO_TCP)) < 0)
	{
	printf("server(): socket() errno=%d.\n",errno);
	return(serr(sd,"server(): socket."));
	}

// Bind the name to the socket.
if(bind(sd, psaddr, saddrlen) == -1)
	{
	printf("server(): bind() errno=%d.\n",errno);
	return(serr(sd,"server(): bind."));
	}

// Listen for the socket connection from the GCN machine.
if(listen(sd, 5))
	{
	printf("server(): listen() errno=%d.\n",errno);
	return(serr(sd,"server(): listen."));
	}

// Accept the socket connection from the GCN machine (the client).
if((sock = accept(sd, &saddr, &temp)) < 0)
	{
	printf("server(): accept() errno=%d.\n",errno);
	return(serr(sd,"server(): accept."));
	}
 
if(close(sd))
	{
	printf("server(): close() errno=%d.\n",errno);
	}

// Make the connection nonblocking I/O.
if(ioctl(sock, FIONBIO, &on) < 0)
	{
	printf("server(): ioctl() errno=%d.\n",errno);
	return(serr(sock,"server(): ioctl."));
	}

serr(0,"server(): the server is up.");
return(sock);
}

/*---------------------------------------------------------------------------*/
/* The error routine that prints the status of your socket connection. */
int
serr(fds, call)
int      fds;
char     *call;
{
if(fds > 0)
	if(close(fds))
		{
		perror("serr(), close(): ");
		printf("close() problem in serr(), errno=%d.\n",errno);
		}
printf(    "%s\n",call);
fprintf(lg,"%s\n",call);
return(-1);
}

/*---------------------------------------------------------------------------*/
pr_imalive(lbuf,s)               /* print the contents of the imalive packet */
long	*lbuf;	/* Ptr to the newly arrived packet to print out */
FILE	*s;		/* Stream to print it to */
{
fprintf(s,"PKT INFO:    Received: LT %s",ctime((time_t *)&tloc));
fprintf(s,"   Type= %d     SN  = %d\n", lbuf[PKT_TYPE], lbuf[PKT_SERNUM]);
fprintf(s,"   TM_present: %s   Triggerable: %s   PKT_SOD= %.2f    delta=%5.2f\n",
									(lbuf[MISC] & TM_IND_MASK) ? "Yes":"No ",
									(lbuf[MISC] & NO_TRIG_MASK) ? "No ":"Yes",
									lbuf[PKT_SOD]/100.0,
									here_sod - lbuf[PKT_SOD]/100.0);
if(((lbuf[PKT_SOD]/100.0) - last_imalive_sod) > 62.0)
	fprintf(s,"ERR: Imalive packets generated at greater than 60sec intervals(=%.1f).\n",
									(lbuf[PKT_SOD]/100.0) - last_imalive_sod);
else if(((lbuf[PKT_SOD]/100.0) - last_imalive_sod) < 58.0)
	fprintf(s,"ERR: Imalive packets generated at less than 60sec intervals(=%.1f).\n",
									(lbuf[PKT_SOD]/100.0) - last_imalive_sod);
if((here_sod - last_here_sod) > 62.0)
	fprintf(s,"ERR: Imalive packets arrived at greater than 60sec intervals(=%d).\n",
													here_sod - last_here_sod);
else if((here_sod - last_here_sod) < 58.0)
	fprintf(s,"ERR: Imalive packets arrived at less than 60sec intervals(=%d).\n",
													here_sod - last_here_sod);
fflush(s);	/* This flush is optional -- it's useful for debugging */
}

/*---------------------------------------------------------------------------*/
pr_packet(lbuf,s)     /* print the contents of the BATSE-based or test packet*/
long		*lbuf;	/* Ptr to the newly arrived packet to print out */
FILE		*s;		/* Stream to print it to */
{
int			i;		/* Loop var */
static char	*note_type[4] = {"Original","n/a","n/a","Final"};

fprintf(s,"PKT INFO:    Received: LT %s",ctime((time_t *)&tloc));
fprintf(s,"   Type= %d     SN  = %d\n", lbuf[PKT_TYPE], lbuf[PKT_SERNUM]);
fprintf(s,"   Hop_cnt= %d\n", lbuf[PKT_HOP_CNT]);
fprintf(s,"   PKT_SOD=  %.2f [sec]   delta=%5.2f\n",
					lbuf[PKT_SOD]/100.0, here_sod - lbuf[PKT_SOD]/100.0);
fprintf(s,"   Trig#=  %d\n", lbuf[BURST_TRIG]);
fprintf(s,"   TJD=    %d\n", lbuf[BURST_TJD]);
fprintf(s,"   SOD=  %.2f [sec]   delta=%5.2f\n",
					lbuf[BURST_SOD]/100.0, here_sod - lbuf[BURST_SOD]/100.0);
fprintf(s,"   RA=    %7.2f [deg]\n", lbuf[BURST_RA]/100.0);
fprintf(s,"   Dec=   %7.2f [deg]\n", lbuf[BURST_DEC]/100.0);
fprintf(s,"   Inten=  %d     Peak=%d\n", lbuf[BURST_INTEN],lbuf[BURST_PEAK]);
fprintf(s,"   Error= %6.1f [deg]\n", lbuf[BURST_ERROR]/100.0);
fprintf(s,"   SC_Az= %7.2f [deg]\n", lbuf[SC_AZ]/100.0);
fprintf(s,"   SC_El= %7.2f [deg]\n", lbuf[SC_EL]/100.0);
fprintf(s,"   SC_X_RA=  %7.2f [deg]\n", lbuf[SC_X_RA]/100.0);
fprintf(s,"   SC_X_Dec= %7.2f [deg]\n", lbuf[SC_X_DEC]/100.0);
fprintf(s,"   SC_Z_RA=  %7.2f [deg]\n", lbuf[SC_Z_RA]/100.0);
fprintf(s,"   SC_Z_Dec= %7.2f [deg]\n", lbuf[SC_Z_DEC]/100.0);
fprintf(s,"   Trig_id= %08x\n", lbuf[TRIGGER_ID]);
fprintf(s,"   Misc= %08x\n", lbuf[MISC]);
fprintf(s,"   E_SC_Az=  %7.2f [deg]\n", lbuf[E_SC_AZ]/100.0);
fprintf(s,"   E_SC_El=  %7.2f [deg]\n", lbuf[E_SC_EL]/100.0);
fprintf(s,"   SC_Radius=%d [km]\n", lbuf[SC_RADIUS]);
fprintf(s,"   Spares:");
for(i=PKT_SPARE23; i<=PKT_SPARE38; i++)
	fprintf(s," %x",lbuf[i]);
fprintf(s,"\n");
fprintf(s,"   Pkt_term: %02x %02x %02x %02x\n",
										*((char *)(&lbuf[PKT_TERM]) + 0),
										*((char *)(&lbuf[PKT_TERM]) + 1),
										*((char *)(&lbuf[PKT_TERM]) + 2),
										*((char *)(&lbuf[PKT_TERM]) + 3));

fprintf(s,"   TM_present: %s\n", (lbuf[MISC] & TM_IND_MASK) ? "Yes" : "No ");
fprintf(s,"   NoticeType: %s\n",note_type[(lbuf[MISC] & NOTICE_TYPE_MASK) >>8]);
fprintf(s,"   Prog_level: %d\n", (lbuf[MISC] & PROG_LEV_MASK) >> 5);
fprintf(s,"   Prog_version: %d.%02d\n",
									((lbuf[MISC] & VER_MAJOR_MASK) >> 28)&0xF,
									(lbuf[MISC] & VER_MINOR_MASK) >> 16);
fflush(s);	/* This flush is optional -- it's useful for debugging */
}

/*---------------------------------------------------------------------------*/
pr_kill(lbuf,s)                     /* print the contents of the KILL packet */
long	*lbuf;	/* Ptr to the newly arrived packet to print out */
FILE	*s;		/* Stream to print it to */
{
fprintf(s,"PKT INFO:    Received: LT %s",ctime((time_t *)&tloc));
fprintf(s,"   Type= %d     SN  = %d\n", lbuf[PKT_TYPE], lbuf[PKT_SERNUM]);
fprintf(s,"   Hop_cnt= %d\n", lbuf[PKT_HOP_CNT]);
fprintf(s,"   PKT_SOD=  %.2f [sec]\n", lbuf[PKT_SOD]/100.0);
}

/*---------------------------------------------------------------------------*/
pr_alexis(lbuf,s)                 /* print the contents of the ALEXIS packet */
long	*lbuf;	/* Ptr to the newly arrived packet to print out */
FILE	*s;		/* Stream to print it to */
{
int			i;	/* Loop var */
static char	tele[7][16]={"n/a", "1A, 93eV","1B, 70eV","2A, 93eV","2B, 66eV","3A, 70eV","3B, 66eV"};

fprintf(s,"PKT INFO:    Received: LT %s",ctime((time_t *)&tloc));
fprintf(s,"   Type= %d     SN  = %d\n", lbuf[PKT_TYPE], lbuf[PKT_SERNUM]);
fprintf(s,"   Hop_cnt= %d\n", lbuf[PKT_HOP_CNT]);
fprintf(s,"   PKT_SOD=  %.2f [sec]   delta=%5.2f\n",
					lbuf[PKT_SOD]/100.0, here_sod - lbuf[PKT_SOD]/100.0);
fprintf(s,"   Ssn=    %d\n", lbuf[BURST_TRIG]);
fprintf(s,"   TJD=    %d\n", lbuf[BURST_TJD]);
fprintf(s,"   SOD=  %.2f [sec]   delta=%5.2f\n",
					lbuf[BURST_SOD]/100.0, here_sod - lbuf[BURST_SOD]/100.0);
fprintf(s,"   RA=    %7.2f [deg]\n", lbuf[BURST_RA]/100.0);
fprintf(s,"   Dec=   %7.2f [deg]\n", lbuf[BURST_DEC]/100.0);
fprintf(s,"   Error= %6.1f [deg]\n", lbuf[BURST_ERROR]/100.0);
fprintf(s,"   Alpha= %.2f\n", lbuf[SC_RADIUS]/100.0);
fprintf(s,"   Trig_id= %08x\n", lbuf[TRIGGER_ID]);
fprintf(s,"   Misc= %08x\n", lbuf[MISC]);
fprintf(s,"   Map_duration= %d [hrs]\n",lbuf[BURST_T_PEAK]);
fprintf(s,"   Telescope(=%d): %s\n",lbuf[24],tele[lbuf[24]]);
fprintf(s,"   Spares:");
for(i=PKT_SPARE23; i<=PKT_SPARE38; i++)
	fprintf(s," %x",lbuf[i]);
fprintf(s,"\n");
fprintf(s,"   Pkt_term: %02x %02x %02x %02x\n",
										*((char *)(&lbuf[PKT_TERM]) + 0),
										*((char *)(&lbuf[PKT_TERM]) + 1),
										*((char *)(&lbuf[PKT_TERM]) + 2),
										*((char *)(&lbuf[PKT_TERM]) + 3));

fprintf(s,"   TM_present: %s\n", (lbuf[MISC] & TM_IND_MASK) ? "Yes" : "No ");
fprintf(s,"   Prog_level: %d\n", (lbuf[MISC] & PROG_LEV_MASK) >> 5);
fprintf(s,"   Prog_version: %d.%02d\n",((lbuf[MISC] & VER_MAJOR_MASK)>>28)&0xF,
									    (lbuf[MISC] & VER_MINOR_MASK) >> 16);
}

/*---------------------------------------------------------------------------*/
pr_xte_pca(lbuf,s)               /* print the contents of the XTE-PCA packet */
long	*lbuf;	/* Ptr to the newly arrived packet to print out */
FILE	*s;		/* Stream to print it to */
{
fprintf(s,"PKT INFO:    Received: LT %s",ctime((time_t *)&tloc));
fprintf(s,"   Type= %d     SN  = %d\n", lbuf[PKT_TYPE], lbuf[PKT_SERNUM]);
fprintf(s,"   Hop_cnt= %d\n", lbuf[PKT_HOP_CNT]);
fprintf(s,"   PKT_SOD=  %.2f [sec]   delta=%5.2f\n",
					lbuf[PKT_SOD]/100.0, here_sod - lbuf[PKT_SOD]/100.0);
fprintf(s,"   Trig#=  %d\n", lbuf[BURST_TRIG]);
fprintf(s,"   TJD=    %d\n", lbuf[BURST_TJD]);
fprintf(s,"   SOD=  %.2f [sec]   delta=%5.2f\n",
					lbuf[BURST_SOD]/100.0, here_sod - lbuf[BURST_SOD]/100.0);
if(lbuf[MISC] & OBS_STATUS)
	{
	fprintf(s,"   RA=    %7.3f [deg]\n", lbuf[BURST_RA]/10000.0);
	fprintf(s,"   Dec=   %7.3f [deg]\n", lbuf[BURST_DEC]/10000.0);
	fprintf(s,"   Inten=  %d [mCrab]\n", lbuf[BURST_INTEN]);
	fprintf(s,"   Error= %6.2f [deg]\n", lbuf[BURST_ERROR]/10000.0);
	fprintf(s,"   Trig_id= %08x\n", lbuf[TRIGGER_ID]);
	fprintf(s,"   OBS_START_DATE= %d TJD\n", lbuf[24]);
	fprintf(s,"   OBS_START_SOD=  %d UT\n", lbuf[25]);
	}
else{
	fprintf(s,"RXTE-PCA could not localize this GRB.\n");
	}
fprintf(s,"   Prog_level: %d\n", (lbuf[MISC] & PROG_LEV_MASK) >> 5);
}

/*---------------------------------------------------------------------------*/
pr_xte_asm(lbuf,s)               /* print the contents of the XTE-ASM packet */
long	*lbuf;	/* Ptr to the newly arrived packet to print out */
FILE	*s;		/* Stream to print it to */
{
fprintf(s,"PKT INFO:    Received: LT %s",ctime((time_t *)&tloc));
fprintf(s,"   Type= %d     SN  = %d\n", lbuf[PKT_TYPE], lbuf[PKT_SERNUM]);
fprintf(s,"   Hop_cnt= %d\n", lbuf[PKT_HOP_CNT]);
fprintf(s,"   PKT_SOD=  %.2f [sec]   delta=%5.2f\n",
					lbuf[PKT_SOD]/100.0, here_sod - lbuf[PKT_SOD]/100.0);
fprintf(s,"   Trig#=  %d\n", lbuf[BURST_TRIG]);
fprintf(s,"   TJD=    %d\n", lbuf[BURST_TJD]);
fprintf(s,"   SOD=  %.2f [sec]   delta=%5.2f\n",
					lbuf[BURST_SOD]/100.0, here_sod - lbuf[BURST_SOD]/100.0);
if(lbuf[MISC] & OBS_STATUS)
	{
	fprintf(s,"   RA=    %7.3f [deg]\n", lbuf[BURST_RA]/10000.0);
	fprintf(s,"   Dec=   %7.3f [deg]\n", lbuf[BURST_DEC]/10000.0);
	fprintf(s,"   Inten=  %d [mCrab]\n", lbuf[BURST_INTEN]);
	fprintf(s,"   Error= %6.2f [deg]\n", lbuf[BURST_ERROR]/10000.0);
	fprintf(s,"   Trig_id= %08x\n", lbuf[TRIGGER_ID]);
	fprintf(s,"   OBS_START_DATE= %d TJD\n", lbuf[24]);
	fprintf(s,"   OBS_START_SOD=  %d UT\n", lbuf[25]);
	}
else{
	fprintf(s,"RXTE-ASM could not localize this GRB.\n");
	}
fprintf(s,"   Prog_level: %d\n", (lbuf[MISC] & PROG_LEV_MASK) >> 5);
}

/*---------------------------------------------------------------------------*/
pr_xte_asm_trans(lbuf,s)   /* print the contents of the XTE-ASM_TRANS packet */
long	*lbuf;	/* Ptr to the newly arrived packet to print out */
FILE	*s;		/* Stream to print it to */
{
fprintf(s,"PKT INFO:    Received: LT %s",ctime((time_t *)&tloc));
fprintf(s,"   Type= %d     SN  = %d\n", lbuf[PKT_TYPE], lbuf[PKT_SERNUM]);
fprintf(s,"   Hop_cnt= %d\n", lbuf[PKT_HOP_CNT]);
fprintf(s,"   PKT_SOD=  %.2f [sec]   delta=%5.2f\n",
					lbuf[PKT_SOD]/100.0, here_sod - lbuf[PKT_SOD]/100.0);
fprintf(s,"   RefNum= %d\n", lbuf[BURST_TRIG]);
fprintf(s,"   TJD=    %d\n", lbuf[BURST_TJD]);
fprintf(s,"   SOD=  %.2f [sec]   delta=%5.2f\n",
					lbuf[BURST_SOD]/100.0, here_sod - lbuf[BURST_SOD]/100.0);
fprintf(s,"   RA=    %7.3f [deg]\n", lbuf[BURST_RA]/10000.0);
fprintf(s,"   Dec=   %7.3f [deg]\n", lbuf[BURST_DEC]/10000.0);
fprintf(s,"   Inten=  %.1f [mCrab]\n", lbuf[BURST_INTEN]/100.0);
fprintf(s,"   Error= %6.2f [deg]\n", lbuf[BURST_ERROR]/10000.0);
fprintf(s,"   T_Since=  %d [sec]\n", lbuf[13]);
fprintf(s,"   ChiSq1=   %.2f\n", lbuf[14]/100.0);
fprintf(s,"   ChiSq2=   %.2f\n", lbuf[15]/100.0);
fprintf(s,"   SigNoise1=   %.2f\n", lbuf[16]/100.0);
fprintf(s,"   SigNoise2=   %.2f\n", lbuf[17]/100.0);
/*fprintf(s,"   Trig_ID=      %8x\n", lbuf[TRIGGER_ID]);*/
fprintf(s,"   RA_Crnr1=    %7.3f [deg]\n", lbuf[24]/10000.0);
fprintf(s,"   Dec_Crnr1=   %7.3f [deg]\n", lbuf[25]/10000.0);
fprintf(s,"   RA_Crnr2=    %7.3f [deg]\n", lbuf[26]/10000.0);
fprintf(s,"   Dec_Crnr2=   %7.3f [deg]\n", lbuf[27]/10000.0);
fprintf(s,"   RA_Crnr3=    %7.3f [deg]\n", lbuf[28]/10000.0);
fprintf(s,"   Dec_Crnr3=   %7.3f [deg]\n", lbuf[29]/10000.0);
fprintf(s,"   RA_Crnr4=    %7.3f [deg]\n", lbuf[30]/10000.0);
fprintf(s,"   Dec_Crnr4=   %7.3f [deg]\n", lbuf[31]/10000.0);
if(lbuf[MISC] | OBS_STATUS)
	fprintf(s,"   ASM_TRANSIENT Notices are all Cross_box type.\n");
else
	{
	fprintf(s,"   LineLength=  %7.3f [deg]\n", lbuf[32]/10000.0);
	fprintf(s,"   LineWidth=   %7.3f [deg]\n", lbuf[33]/10000.0);
	fprintf(s,"   PosAngle=    %7.3f [deg]\n", lbuf[34]/10000.0);
	}
fprintf(s,"   Prog_level: %d\n", (lbuf[MISC] & PROG_LEV_MASK) >> 5);
}

/*---------------------------------------------------------------------------*/
pr_ipn_raw(lbuf,s)               /* print the contents of the IPN RAW packet */
long	*lbuf;	/* Ptr to the newly arrived packet to print out */
FILE	*s;		/* Stream to print it to */
{
fprintf(s,"PKT INFO:    Received: LT %s",ctime((time_t *)&tloc));
fprintf(s,"   Type= %d   SN= %d,  IPN Raw\n",
					lbuf[PKT_TYPE], lbuf[PKT_SERNUM]);
fprintf(s,"   Hop_cnt=         %d\n", lbuf[PKT_HOP_CNT]);
fprintf(s,"   PKT_SOD=         %.2f [sec]   delta=%5.2f\n",
						lbuf[PKT_SOD]/100.0, here_sod - lbuf[PKT_SOD]/100.0);
fprintf(s,"   TJD=             %d\n", lbuf[BURST_TJD]);
fprintf(s,"   SOD=             %.2f [sec]   delta=%5.2f\n",
					lbuf[BURST_SOD]/100.0, here_sod - lbuf[BURST_SOD]/100.0);
fprintf(s,"   IPN_CENTER_RA:   %8.4fd  (J2000)\n", lbuf[BURST_RA]/10000.0);
fprintf(s,"   IPN_CENTER_DEC:  %+8.4fd  (J2000)\n",lbuf[BURST_DEC]/10000.0);
fprintf(s,"   IPN_RADIUS:      %+8.4fd\n",lbuf[9]/10000.0);
fprintf(s,"   IPN_WIDTH:       %+8.4fd\n",lbuf[11]/10000.0);
if(lbuf[TRIGGER_ID]&0x1)
fprintf(s,"   IPN_WINDOW:      %.2f [sec]\n",lbuf[10]/100.0);
else
fprintf(s,"   IPN_WINDOW:      n/a [sec]\n");
fprintf(s,"   EVENT_DUR:       %.2f [sec]\n",lbuf[14]/100.0);

if(lbuf[TRIGGER_ID] & 0x1)
    fprintf(s,"This notice contains only a timestamp -- the RA,Dec,Radius,Width fields are undefined.");
if(lbuf[TRIGGER_ID] & (0x1<<2))
    fprintf(s,"This is definitely a GRB.");
if(lbuf[TRIGGER_ID] & (0x1<<3))
    fprintf(s,"This is probably a GRB.");
if(lbuf[TRIGGER_ID] & (0x1<<4))
    fprintf(s,"This is possibly a GRB.");
if(lbuf[TRIGGER_ID] & (0x1<<5))
    fprintf(s,"This is definitely not a GRB.");
if(lbuf[MISC] & 0x1)
    fprintf(s,"No further information will be available on this event from this mission-instrument.");
if(lbuf[MISC] & (0x1<<1))
    fprintf(s,"Further information might be available on this event from this mission-instrument.");
if(lbuf[MISC] & (0x1<<2))
    fprintf(s,"Further information will definitely be available on this event from this mission-instrument.");
if(lbuf[MISC] & (0x1<<3))
    fprintf(s,"This is an Update notice -- this notice superceeds the previous notice(s).");
if(lbuf[MISC] & (0x1<<10))
    fprintf(s,"The Ulysses mission contributed to this notice.");
if(lbuf[MISC] & (0x1<<11))
    fprintf(s,"The Odyssey-HEND instrument contributed to this notice.");
if(lbuf[MISC] & (0x1<<12))
    fprintf(s,"The Odyssey-GRS instrument contributed to this notice.");
if(lbuf[MISC] & (0x1<<13))
    fprintf(s,"The RHESSI mission contributed to this notice.");
if(lbuf[MISC] & (0x1<<14))
    fprintf(s,"The KONUS-Wind instrument contributed to this notice.");
if(lbuf[MISC] & (0x1<<15))
    fprintf(s,"The Swift-BAT instrument contributed to this notice.");
if(lbuf[MISC] & (0x1<<16))
    fprintf(s,"The MESSENGER-GRNS instrument contributed to this notice.");
if(lbuf[MISC] & (0x1<<17))
    fprintf(s,"The Suzaku-WAM instrument contributed to this notice.");
if(lbuf[MISC] & (0x1<<18))
    fprintf(s,"The INTEGRAL-SPIACS instrument contributed to this notice.");
if(lbuf[MISC] & (0x1<<19))
    fprintf(s,"The SuperAGILE instrument contributed to this notice.");
if(lbuf[MISC] & (0x1<<20))
    fprintf(s,"The AGILE-GRID instrument contributed to this notice.");
if(lbuf[MISC] & (0x1<<21))
    fprintf(s,"The AGILE-MCAL instrument contributed to this notice.");
if(lbuf[MISC] & (0x1<<22))
    fprintf(s,"The Fermi-GBM instrument contributed to this notice.");
if(lbuf[MISC] & (0x1<<23))
    fprintf(s,"The Fermi-LAT instrument contributed to this notice.");
if(lbuf[TRIGGER_ID] & (0x1<<30))
    fprintf(s,"THIS IS A TEST. REPEAT THIS IS A TEST.");
}

/*---------------------------------------------------------------------------*/
pr_ipn_seg(lbuf,s)           /* print the contents of the IPN SEGMENT packet */
long	*lbuf;	/* Ptr to the newly arrived packet to print out */
FILE	*s;		/* Stream to print it to */
{
fprintf(s,"IPN_SEG exists, but the printout routine doesn't exist yet.\n");
}

/*---------------------------------------------------------------------------*/
pr_ipn_pos(lbuf,s)          /* print the contents of the IPN POSITION packet */
long	*lbuf;	/* Ptr to the newly arrived packet to print out */
FILE	*s;		/* Stream to print it to */
{
fprintf(s,"PKT INFO:    Received: LT %s",ctime((time_t *)&tloc));
fprintf(s,"   Type= %d   SN= %d,  IPN Raw\n",
					lbuf[PKT_TYPE], lbuf[PKT_SERNUM]);
fprintf(s,"   Hop_cnt=         %d\n", lbuf[PKT_HOP_CNT]);
fprintf(s,"   PKT_SOD=         %.2f [sec]   delta=%5.2f\n",
						lbuf[PKT_SOD]/100.0, here_sod - lbuf[PKT_SOD]/100.0);
fprintf(s,"   TJD=             %d\n", lbuf[BURST_TJD]);
fprintf(s,"   SOD=             %.2f [sec]   delta=%5.2f\n",
					lbuf[BURST_SOD]/100.0, here_sod - lbuf[BURST_SOD]/100.0);
fprintf(s,"   IPN_RA:        %8.4fd  (J2000)\n", lbuf[BURST_RA]/10000.0);
fprintf(s,"   IPN_DEC:       %+8.4fd  (J2000)\n",lbuf[BURST_DEC]/10000.0);
fprintf(s,"   ERROR1_RA1:    %8.4fd  (J2000)\n", (float)lbuf[23]/10000.0);
fprintf(s,"   ERROR1_DEC1:   %8.4fd  (J2000)\n", (float)lbuf[24]/10000.0);
fprintf(s,"   ERROR1_RA2:    %8.4fd  (J2000)\n", (float)lbuf[25]/10000.0);
fprintf(s,"   ERROR1_DEC2:   %8.4fd  (J2000)\n", (float)lbuf[26]/10000.0);
fprintf(s,"   ERROR1_RA3:    %8.4fd  (J2000)\n", (float)lbuf[27]/10000.0);
fprintf(s,"   ERROR1_DEC3:   %8.4fd  (J2000)\n", (float)lbuf[28]/10000.0);
fprintf(s,"   ERROR1_RA4:    %8.4fd  (J2000)\n", (float)lbuf[29]/10000.0);
fprintf(s,"   ERROR1_DEC4:   %8.4fd  (J2000)\n", (float)lbuf[30]/10000.0);
if(((lbuf[MISC] & PROG_LEV_MASK) >> 5) == 1)		// A double box ipn notice
  {
fprintf(s,"   IPN2_RA:       %8.4fd  (J2000)\n", (float)lbuf[16]/10000.0);
fprintf(s,"   IPN2_DEC:      %+8.4fd  (J2000)\n",(float)lbuf[17]/10000.0);
fprintf(s,"   ERROR2_RA1:    %8.4fd  (J2000)\n", (float)lbuf[31]/10000.0);
fprintf(s,"   ERROR2_DEC1:   %8.4fd  (J2000)\n", (float)lbuf[32]/10000.0);
fprintf(s,"   ERROR2_RA2:    %8.4fd  (J2000)\n", (float)lbuf[33]/10000.0);
fprintf(s,"   ERROR2_DEC2:   %8.4fd  (J2000)\n", (float)lbuf[34]/10000.0);
fprintf(s,"   ERROR2_RA3:    %8.4fd  (J2000)\n", (float)lbuf[35]/10000.0);
fprintf(s,"   ERROR2_DEC3:   %8.4fd  (J2000)\n", (float)lbuf[36]/10000.0);
fprintf(s,"   ERROR2_RA4:    %8.4fd  (J2000)\n", (float)lbuf[37]/10000.0);
fprintf(s,"   ERROR2_DEC4:   %8.4fd  (J2000)\n", (float)lbuf[38]/10000.0);
  }
fprintf(s,"   ERROR:         %.2f [arcmin radius of circumscribing circle, 3 sigma]\n",
												60.0*lbuf[BURST_ERROR]/10000.0);
fprintf(s,"   BOX_AREA:      %.0f [sq.arcmin]\n", lbuf[21]/(10000.0));

if(lbuf[BURST_PEAK]*1e-9 == -1)
fprintf(s,"   PEAK_FLUX:     unknown [erg/cm2/sec]");
else
fprintf(s,"   PEAK_FLUX:     %.1e [erg/cm2/sec in 25-100 keV]", lbuf[BURST_PEAK]*1e-9);
if(lbuf[12])
	fprintf(s," in a %.2f sec interval", lbuf[12]/10000.0);
fprintf(s,"\n");

if(lbuf[BURST_INTEN]*1e-9 == -1)
fprintf(s,"   FLUENCE:       unknown [erg/cm2]\n");
else
fprintf(s,"   FLUENCE:       %.1e [erg/cm2 in 25-100 keV]\n", lbuf[BURST_INTEN]*1e-9);

if(lbuf[22]/100.0 == -1.0)
fprintf(s,"   DURATION:      unknown [sec]\n");
else
fprintf(s,"   DURATION:      %.1f [sec]\n", lbuf[22]/100.0);

}

#ifdef SAX_NO_LONGER_AVAILABLE
/*---------------------------------------------------------------------------*/
pr_sax_wfc(lbuf,s)               /* print the contents of a SAX-WFC packet */
long	*lbuf;	/* Ptr to the newly arrived packet to print out */
FILE	*s;		/* Stream to print it to */
{
fprintf(s,"PKT INFO:    Received: LT %s",ctime((time_t *)&tloc));
fprintf(s,"   Type= %d     SN  = %d\n", lbuf[PKT_TYPE], lbuf[PKT_SERNUM]);
fprintf(s,"   Hop_cnt= %d\n", lbuf[PKT_HOP_CNT]);
fprintf(s,"   PKT_SOD=  %.2f [sec]   delta=%5.2f\n",
					lbuf[PKT_SOD]/100.0, here_sod - lbuf[PKT_SOD]/100.0);
fprintf(s,"   Trig#=  %d\n", lbuf[BURST_TRIG]);
fprintf(s,"   TJD=    %d\n", lbuf[BURST_TJD]);
fprintf(s,"   SOD=  %.2f [sec]   delta=%5.2f\n",
					lbuf[BURST_SOD]/100.0, here_sod - lbuf[BURST_SOD]/100.0);
fprintf(s,"   RA=    %7.3f [deg]\n", lbuf[BURST_RA]/10000.0);
fprintf(s,"   Dec=   %7.3f [deg]\n", lbuf[BURST_DEC]/10000.0);
fprintf(s,"   Inten=  %d [mCrab]\n", lbuf[BURST_INTEN]);
fprintf(s,"   Error= %6.2f [deg]\n", lbuf[BURST_ERROR]/10000.0);
fprintf(s,"   Trig_id= %08x\n", lbuf[TRIGGER_ID]);
fprintf(s,"   Prog_level: %d\n", (lbuf[MISC] & PROG_LEV_MASK) >> 5);
}

/*---------------------------------------------------------------------------*/
pr_sax_nfi(lbuf,s)               /* print the contents of a SAX-NFI packet */
long	*lbuf;	/* Ptr to the newly arrived packet to print out */
FILE	*s;		/* Stream to print it to */
{
fprintf(s,"SAX-NFI exists, but the printout routine doesn't exist yet.\n");
fprintf(s,"The NFI packet is very similar to the WFC packet.\n");
}
#endif

/*---------------------------------------------------------------------------*/
/* Please note that not all of the many flag bits are extracted and printed
 * in this example routine.  The important ones are shown (plus all the
 * numerical fields).  Please refer to the sock_pkt_def_doc.html on the
 * GCN web site for the full details of the contents of these flag bits.     */
pr_hete(lbuf,s)               /* print the contents of the HETE-based packet */
long	*lbuf;	/* Ptr to the newly arrived packet to print out */
FILE	*s;		/* Stream to print it to */
{
unsigned short	max_arcsec, overall_goodness;
int				wxm_x_lc_sn, wxm_y_lc_sn, wxm_x_img_sn, wxm_y_img_sn;

fprintf(s,"PKT INFO:    Received: LT %s",ctime((time_t *)&tloc));
fprintf(s,"   Type= %d,  ", lbuf[PKT_TYPE]);
switch(lbuf[PKT_TYPE])
	{
	//case TYPE_HETE_ALERT_SRC:		printf("S/C_Alert");	break;
	//case TYPE_HETE_UPDATE_SRC:	printf("S/C_Update");	break;
	//case TYPE_HETE_FINAL_SRC:		printf("S/C_Last");		break;
	//case TYPE_HETE_GNDANA_SRC:	printf("GndAna");		break;
	case TYPE_HETE_TEST:		printf("Test");		break;
	default:					printf("Illegal");	break;
	}
fprintf(s,"   SN= %d\n", lbuf[PKT_SERNUM]);
fprintf(s,"   Hop_cnt=    %d\n", lbuf[PKT_HOP_CNT]);
fprintf(s,"   PKT_SOD=     %.2f [sec]   delta=%5.2f\n",
					lbuf[PKT_SOD]/100.0, here_sod - lbuf[PKT_SOD]/100.0);
fprintf(s,"   Trig#=     %d\n",(lbuf[BURST_TRIG] & H_TRIGNUM_MASK) >>H_TRIGNUM_SHIFT);
fprintf(s,"   Seq#=      %d\n",(lbuf[BURST_TRIG] & H_SEQNUM_MASK)  >>H_SEQNUM_SHIFT);
fprintf(s,"   BURST_TJD=    %d\n", lbuf[BURST_TJD]);
fprintf(s,"   BURST_SOD=  %.2f [sec]   delta=%5.2f\n",
					lbuf[BURST_SOD]/100.0, here_sod - lbuf[BURST_SOD]/100.0);

fprintf(s,"   TRIGGER_SOURCE: ");
if(lbuf[H_TRIG_FLAGS] & H_SXC_EN_TRIG) 
	fprintf(s,"Trigger on the 1.5-12 keV band.\n");
else if(lbuf[H_TRIG_FLAGS] & H_LOW_EN_TRIG) 
	fprintf(s,"Trigger on the 2-30 keV band.\n");
else if(lbuf[H_TRIG_FLAGS] & H_MID_EN_TRIG) 
	fprintf(s,"Trigger on the 6-120 keV band.\n");
else if(lbuf[H_TRIG_FLAGS] & H_HI_EN_TRIG) 
	fprintf(s,"Trigger on the 25-400 keV band.\n");
else
	fprintf(s,"Error: No trigger source!\n");

fprintf(s,"   GAMMA_CTS_S:   %d [cnts/s] ", lbuf[H_GAMMA_CTS_S]);
fprintf(s,"on a %.3f sec timescale\n", (float)lbuf[H_GAMMA_TSCALE]/1000.0);
fprintf(s,"   WXM_SIG/NOISE:  %d [sig/noise]  ",lbuf[H_WXM_S_N]);
fprintf(s,"on a %.3f sec timescale\n", (float)lbuf[H_WXM_TSCALE]/1000.0);
if(lbuf[H_TRIG_FLAGS] & H_WXM_DATA)		/* Check for XG data and parse */
	fprintf(s,",  Data present.\n");
else
	fprintf(s,",  Data NOT present.\n");
fprintf(s,"   SXC_CTS_S:     %d\n", lbuf[H_SXC_CTS_S]);


if(lbuf[H_TRIG_FLAGS] & H_OPT_DATA)
	fprintf(s,"   S/C orientation is available.\n");
else
	fprintf(s,"   S/C orientation is NOT available.\n");
fprintf(s,"   SC_-Z_RA:    %4d [deg]\n",(unsigned short)(lbuf[H_POINTING] >> 16));
fprintf(s,"   SC_-Z_DEC:   %4d [deg]\n",(short)(lbuf[H_POINTING] & 0xffff));
fprintf(s,"   SC_LONG:     %3d [deg East]\n",
							2*(((lbuf[H_POS_FLAGS] & H_SC_LONG) >> 24) &0xFF)); 

fprintf(s,"   BURST_RA:    %7.3fd  (current)\n", lbuf[BURST_RA]/10000.0);
fprintf(s,"   BURST_DEC:   %+7.3fd  (current)\n", lbuf[BURST_DEC]/10000.0);

if(lbuf[H_TRIG_FLAGS] & H_WXM_POS)	/* Flag says that WXM pos is available */
	fprintf(s,"   WXM position is available.\n");
else
	fprintf(s,"   WXM position is NOT available.\n");
fprintf(s,"   WXM_CORNER1:  %8.4f %8.4f [deg] (J2000)\n",
										(float)lbuf[H_WXM_CNR1_RA]/10000.0,
										(float)lbuf[H_WXM_CNR1_DEC]/10000.0);
fprintf(s,"   WXM_CORNER2:  %8.4f %8.4f [deg] (J2000)\n",
										(float)lbuf[H_WXM_CNR2_RA]/10000.0,
										(float)lbuf[H_WXM_CNR2_DEC]/10000.0);
fprintf(s,"   WXM_CORNER3:  %8.4f %8.4f [deg] (J2000)\n",
										(float)lbuf[H_WXM_CNR3_RA]/10000.0,
										(float)lbuf[H_WXM_CNR3_DEC]/10000.0);
fprintf(s,"   WXM_CORNER4:  %8.4f %8.4f [deg] (J2000)\n",
										(float)lbuf[H_WXM_CNR4_RA]/10000.0,
										(float)lbuf[H_WXM_CNR4_DEC]/10000.0);

max_arcsec = lbuf[H_WXM_DIM_NSIG] >> 16;
overall_goodness = lbuf[H_WXM_DIM_NSIG] & 0xffff;
fprintf(s,"   WXM_MAX_SIZE:  %.2f [arcmin]\n", (float)max_arcsec/60.0);
fprintf(s,"   WXM_LOC_SN:    %d\n", overall_goodness);
wxm_x_lc_sn  = ((lbuf[38] & H_X_LC_SN) >> 24) & 0xFF;
wxm_y_lc_sn  = ((lbuf[38] & H_Y_LC_SN) >> 16) & 0xFF;
wxm_x_img_sn = ((lbuf[38] & H_X_IMAGE_SN) >> 8) & 0xFF;
wxm_y_img_sn =  (lbuf[38] & H_Y_IMAGE_SN);
fprintf(s,"   WXM_IMAGE_SN:    X= %.1f Y= %.1f [sig/noise]\n",
										wxm_x_img_sn/10.0, wxm_y_img_sn/10.0);
fprintf(s,"   WXM_LC_SN:       X= %.1f Y= %.1f [sig/noise]\n",
										wxm_x_lc_sn/10.0, wxm_y_lc_sn/10.0);

if(lbuf[H_TRIG_FLAGS] & H_SXC_POS)	/* Flag says that SXC pos is available */
	fprintf(s,"   SXC position is available.\n");
else
	fprintf(s,"   SXC position is NOT available.\n");
fprintf(s,"   SXC_CORNER1:   %8.4f %8.4f [deg] (J2000)\n",
										(float)lbuf[H_SXC_CNR1_RA]/10000.0,
										(float)lbuf[H_SXC_CNR1_DEC]/10000.0);
fprintf(s,"   SXC_CORNER2:   %8.4f %8.4f [deg] (J2000)\n",
										(float)lbuf[H_SXC_CNR2_RA]/10000.0,
										(float)lbuf[H_SXC_CNR2_DEC]/10000.0);
fprintf(s,"   SXC_CORNER3:   %8.4f %8.4f [deg] (J2000)\n",
										(float)lbuf[H_SXC_CNR3_RA]/10000.0,
										(float)lbuf[H_SXC_CNR3_DEC]/10000.0);
fprintf(s,"   SXC_CORNER4:   %8.4f %8.4f [deg] (J2000)\n",
										(float)lbuf[H_SXC_CNR4_RA]/10000.0,
										(float)lbuf[H_SXC_CNR4_DEC]/10000.0);

	max_arcsec = lbuf[H_SXC_DIM_NSIG] >> 16;
	overall_goodness = lbuf[H_SXC_DIM_NSIG] & 0xffff;
	fprintf(s,"   SXC_MAX_SIZE:   %.2f [arcmin]\n", (float)max_arcsec/60.0);
	fprintf(s,"   SXC_LOC_SN:   %d\n", overall_goodness);

if(lbuf[H_TRIG_FLAGS] & H_ART_TRIG) 
	fprintf(s,"   ARTIFICIAL BURST TRIGGER!\n");
if(!(lbuf[H_TRIG_FLAGS] & H_OPT_DATA))
	fprintf(s,"   No s/c ACS pointing info available yet.\n");
if(lbuf[H_TRIG_FLAGS] & H_DEF_GRB)
	fprintf(s,"   Definite GRB.\n");
if(lbuf[H_TRIG_FLAGS] & H_PROB_GRB)
	fprintf(s,"   Probable GRB.\n");
if(lbuf[H_TRIG_FLAGS] & H_POSS_GRB)
	fprintf(s,"   Possible GRB.\n");
if(lbuf[H_TRIG_FLAGS] & H_DEF_NOT_GRB)
	fprintf(s,"   Definitely not a GRB.\n");
if(lbuf[H_TRIG_FLAGS] & H_DEF_SGR)
	fprintf(s,"   Definite SGR.\n");
if(lbuf[H_TRIG_FLAGS] & H_PROB_SGR)
	fprintf(s,"   Probable SGR.\n");
if(lbuf[H_TRIG_FLAGS] & H_POSS_SGR)
	fprintf(s,"   Possible SGR.\n");
if(lbuf[H_TRIG_FLAGS] & H_DEF_XRB)
	fprintf(s,"   Definite XRB.\n");
if(lbuf[H_TRIG_FLAGS] & H_PROB_XRB)
	fprintf(s,"   Probable XRB.\n");
if(lbuf[H_TRIG_FLAGS] & H_POSS_XRB)
	fprintf(s,"   Possible XRB.\n");

if(!(lbuf[H_VALIDITY] & H_EMAIL_METHOD))
 if(!(lbuf[H_TRIG_FLAGS] & H_DEF_GRB)    &&
   !(lbuf[H_TRIG_FLAGS] & H_PROB_GRB)    &&
   !(lbuf[H_TRIG_FLAGS] & H_POSS_GRB)    &&
   !(lbuf[H_TRIG_FLAGS] & H_DEF_NOT_GRB) &&
   !(lbuf[H_TRIG_FLAGS] & H_DEF_SGR)     &&
   !(lbuf[H_TRIG_FLAGS] & H_PROB_SGR)    &&
   !(lbuf[H_TRIG_FLAGS] & H_POSS_SGR)    &&
   !(lbuf[H_TRIG_FLAGS] & H_DEF_XRB)     &&
   !(lbuf[H_TRIG_FLAGS] & H_PROB_XRB)    &&
   !(lbuf[H_TRIG_FLAGS] & H_POSS_XRB)       )
	fprintf(s,"      Possible GRB.\n");

if(lbuf[H_TRIG_FLAGS] & H_NEAR_SAA) 
	fprintf(s,"   S/c is in or near the SAA.\n");

if(lbuf[H_TRIG_FLAGS] & H_WXM_POS)/* Flag says that SXC position is available */
	if(hete_same(lbuf,FIND_WXM))
		fprintf(s,"   WXM error box is circular; not rectangular.\n");
if(lbuf[H_TRIG_FLAGS] & H_SXC_POS)/* Flag says that SXC position is available */
	if(hete_same(lbuf,FIND_SXC))
		fprintf(s,"   SXC error box is circular; not rectangular.\n");

if(lbuf[H_POS_FLAGS] & H_WXM_SXC_SAME) 
	fprintf(s,"   The WXM & SXC positions are consistant; overlapping error boxes.\n");
if(lbuf[H_POS_FLAGS] & H_WXM_SXC_DIFF) 
	fprintf(s,"   The WXM & SXC positions are NOT consistant; non-overlapping error boxes.\n");
if(lbuf[H_POS_FLAGS] & H_WXM_LOW_SIG) 
	fprintf(s,"   WXM S/N is less than a reasonable value.\n");
if(lbuf[H_POS_FLAGS] & H_SXC_LOW_SIG) 
	fprintf(s,"   SXC S/N is less than a reasonable value.\n");


if((lbuf[PKT_TYPE] == TYPE_HETE_UPDATE_SRC) ||
   (lbuf[PKT_TYPE] == TYPE_HETE_FINAL_SRC)  ||
   (lbuf[PKT_TYPE] == TYPE_HETE_GNDANA_SRC)   )
{
if(lbuf[H_VALIDITY] & H_EMERGE_TRIG) 
	fprintf(s,"   This trigger is the result of Earth Limb emersion.\n");
if(lbuf[H_VALIDITY] & H_KNOWN_XRS) 
	fprintf(s,"   This position matches a known X-ray source.\n");
if( (((lbuf[H_TRIG_FLAGS] & H_WXM_POS) == 0) &&
	 ((lbuf[H_TRIG_FLAGS] & H_SXC_POS) == 0)   ) ||
    (lbuf[H_VALIDITY] & H_NO_POSITION)             )
	fprintf(s,"   There is no position known for this trigger at this time.\n");
}

if((lbuf[PKT_TYPE] == TYPE_HETE_FINAL_SRC) ||
   (lbuf[PKT_TYPE] == TYPE_HETE_GNDANA_SRC)  )
{
if(lbuf[H_VALIDITY] & H_BURST_VALID) 
	fprintf(s,"   Burst_Validity flag is true.\n");
if(lbuf[H_VALIDITY] & H_BURST_INVALID) 
	fprintf(s,"   Burst_Invalidity flag is true.\n");
}
if(lbuf[PKT_TYPE] == TYPE_HETE_GNDANA_SRC)
{
if(lbuf[H_POS_FLAGS] & H_GAMMA_REFINED) 
	fprintf(s,"   FREGATE data refined since S/C_Last Notice.\n");
if(lbuf[H_POS_FLAGS] & H_WXM_REFINED) 
	fprintf(s,"   WXM data refined since S/C_Last Notice.\n");
if(lbuf[H_POS_FLAGS] & H_SXC_REFINED) 
	fprintf(s,"   SXC data refined since S/C_Last Notice.\n");


if(lbuf[H_VALIDITY] & H_OPS_ERROR) 
	fprintf(s,"   Invalid trigger due to operational error.\n");
if(lbuf[H_VALIDITY] & H_PARTICLES) 
	fprintf(s,"   Particle event.\n");
if(lbuf[H_VALIDITY] & H_BAD_FLT_LOC) 
	fprintf(s,"   Invalid localization by flight code.\n");
if(lbuf[H_VALIDITY] & H_BAD_GND_LOC) 
	fprintf(s,"   Invalid localization by ground analysis code.\n");
if(lbuf[H_VALIDITY] & H_RISING_BACK) 
	fprintf(s,"   Trigger caused by rising background levels.\n");
if(lbuf[H_VALIDITY] & H_POISSON_TRIG) 
	fprintf(s,"   Background fluctuation exceeded trigger level.\n");
if(lbuf[H_VALIDITY] & H_OUTSIDE_FOV) 
	fprintf(s,"   Burst was outside WXM and SXC fields-of-view.\n");
if(lbuf[H_VALIDITY] & H_IPN_CROSSING) 
	fprintf(s,"   Burst coordinates consistent with IPN annulus.\n");
if(lbuf[H_VALIDITY] & H_NOT_A_BURST) 
	fprintf(s,"   Ground & human analysis have determined this to be a non-GRB.\n");
}

if(lbuf[PKT_TYPE] == TYPE_HETE_TEST)
	{
	fprintf(s,"\n");
	fprintf(s,"   This is a TEST Notice.\n");
	fprintf(s,"   And for this TEST Notice, most of the flags\n");
	fprintf(s,"   have been turned on to show most of the fields.\n");
	}
}

/*---------------------------------------------------------------------------*/
pr_grb_cntrpart(lbuf,s)  /* print the contents of the GRB CouNTeRPART packet */
long		*lbuf;	/* Ptr to the newly arrived packet to print out */
FILE		*s;		/* Stream to print it to */
{
fprintf(s,"PKT INFO:    Received: LT %s",ctime((time_t *)&tloc));
fprintf(s,"   Type= %d     SN= %d\n", lbuf[PKT_TYPE], lbuf[PKT_SERNUM]);
fprintf(s,"   Hop_cnt= %d\n", lbuf[PKT_HOP_CNT]);
fprintf(s,"   PKT_SOD=  %.2f [sec]   delta=%5.2f\n",
					lbuf[PKT_SOD]/100.0, here_sod - lbuf[PKT_SOD]/100.0);
fprintf(s,"   Circ#=  %d\n", lbuf[BURST_TRIG]);
fprintf(s,"   GRB_TJD=    %d\n", lbuf[BURST_TJD]);
fprintf(s,"   GRB_SOD=  %.2f [sec]   delta=%5.2f\n",
					lbuf[BURST_SOD]/100.0, here_sod - lbuf[BURST_SOD]/100.0);
fprintf(s,"   CntrPart_RA=    %7.2f [deg]\n", lbuf[BURST_RA]/10000.0);
fprintf(s,"   CntrPart_Dec=   %7.2f [deg]\n", lbuf[BURST_DEC]/10000.0);
fprintf(s,"   CntrPart_Inten=  %d\n", lbuf[BURST_INTEN]);
fprintf(s,"   CntrPart_Error= %6.1f [deg]\n", lbuf[BURST_ERROR]/10000.0);
fprintf(s,"   Filter= %d\n", lbuf[12]);
fprintf(s,"   Seeing= %.2f [arcsec]\n", lbuf[13]/100.0);
fprintf(s,"   Obs_TJD=    %d\n", lbuf[14]);
fprintf(s,"   Obs_SOD=  %.2f [sec]\n", lbuf[15]/100.0);
fprintf(s,"   Obs_Dur=  %.2f [sec]\n", lbuf[16]/100.0);
/* I need to finish this part: */
/*fprintf(s,"   Flags= %08x\n", lbuf[18]);
fprintf(s,"   Misc=  %08x\n", lbuf[19]);
fprintf(s,"   Tele=  %d\n", lbuf[??]);
fprintf(s,"   Obs_Filter=  %d\n", lbuf[??]);
fprintf(s,"   Red_Shift=  %.3f\n", lbuf[??]/10000.0); */
}

/*---------------------------------------------------------------------------*/
pr_integral_point(lbuf,s)/* print the contents of a INTEGRAL POINTDIR packet */
long		*lbuf;	/* Ptr to the newly arrived packet to print out */
FILE		*s;		/* Stream to print it to */
{
fprintf(s,"PKT INFO:    Received: LT %s",ctime((time_t *)&tloc));
fprintf(s,"   Type= %d   SN= %d,  Pointing Direction\n",
					lbuf[PKT_TYPE], lbuf[PKT_SERNUM]);
fprintf(s,"   Hop_cnt= %d\n", lbuf[PKT_HOP_CNT]);
fprintf(s,"   PKT_SOD=  %.2f [sec]   delta=%5.2f\n",
					lbuf[PKT_SOD]/100.0, here_sod - lbuf[PKT_SOD]/100.0);
fprintf(s,"   Trig#=  %d\n", lbuf[BURST_TRIG]);
fprintf(s,"   Next_Point_RA:  %8.4fd (J2000)\n", lbuf[14]/10000.0);
fprintf(s,"   Next_Point_Dec: %+8.4fd (J2000)\n",lbuf[15]/10000.0);
fprintf(s,"   Slew_Time:      %.2f SOD UT\n", lbuf[6]/100.0);
fprintf(s,"   Slew_Date:      %5d TJD\n", lbuf[5]);
fprintf(s,"   Test/MPos= 0x%08x\n", lbuf[12]);
fprintf(s,"   S/C_att=   0x%08x\n", lbuf[19]);
}

/*---------------------------------------------------------------------------*/
pr_integral_spiacs(lbuf,s) /* print the contents of a INTEGRAL SPIACS packet */
long		*lbuf;	/* Ptr to the newly arrived packet to print out */
FILE		*s;		/* Stream to print it to */
{
int	id, seqnum;

fprintf(s,"PKT INFO:    Received: LT %s",ctime((time_t *)&tloc));
fprintf(s,"   Type= %d   SN= %d, SPI ACS\n", lbuf[PKT_TYPE], lbuf[PKT_SERNUM]);
fprintf(s,"   Hop_cnt= %d\n", lbuf[PKT_HOP_CNT]);
fprintf(s,"   PKT_SOD=  %.2f [sec]   delta=%5.2f\n",
					lbuf[PKT_SOD]/100.0, here_sod - lbuf[PKT_SOD]/100.0);
id =     (lbuf[BURST_TRIG] & I_TRIGNUM_MASK) >> I_TRIGNUM_SHIFT;
seqnum = (lbuf[BURST_TRIG] & I_SEQNUM_MASK)  >> I_SEQNUM_SHIFT;
fprintf(s,"   Trig#=  %d,  Subnum= %d\n",id,seqnum);
fprintf(s,"   TJD=    %d\n", lbuf[BURST_TJD]);
fprintf(s,"   SOD=  %.2f [sec]   delta=%5.2f\n",
					lbuf[BURST_SOD]/100.0, here_sod - lbuf[BURST_SOD]/100.0);
fprintf(s,"   Intensity= %.2f [sigma]\n", lbuf[BURST_INTEN]/100.0);
fprintf(s,"   TimeScale= %.2f [sigma]\n", lbuf[13]/10000.0);
fprintf(s,"   TimeError= %.2f [sigma]\n", lbuf[16]/10000.0);
fprintf(s,"   Det_Flags= 0x%08x\n", lbuf[9]);
fprintf(s,"   Test/MPos= 0x%08x\n", lbuf[12]);
fprintf(s,"   S/C_att=   0x%08x\n", lbuf[19]);
}

/*---------------------------------------------------------------------------*/
pr_integral_w_r_o(lbuf,s)/* print the contents of a INTEGRAL_WAKEUP/REFINED/OFFLINE packet */
long		*lbuf;	/* Ptr to the newly arrived packet to print out */
FILE		*s;		/* Stream to print it to */
{
int	id, seqnum;

fprintf(s,"PKT INFO:    Received: LT %s",ctime((time_t *)&tloc));
fprintf(s,"   Type= %d   SN= %d,   ", lbuf[PKT_TYPE], lbuf[PKT_SERNUM]);
switch(lbuf[PKT_TYPE])
	{
	case TYPE_INTEGRAL_WAKEUP_SRC:		fprintf(s,"Wakeup\n");			break;
	case TYPE_INTEGRAL_REFINED_SRC:		fprintf(s,"Refined\n");			break;
	case TYPE_INTEGRAL_OFFLINE_SRC:		fprintf(s,"Offline\n");			break;
	default:							fprintf(s,"Illegal\n");			break;
	}
fprintf(s,"   Hop_cnt= %d\n", lbuf[PKT_HOP_CNT]);
fprintf(s,"   PKT_SOD=  %.2f [sec]   delta=%5.2f\n",
					lbuf[PKT_SOD]/100.0, here_sod - lbuf[PKT_SOD]/100.0);
id =     (lbuf[BURST_TRIG] & I_TRIGNUM_MASK) >> I_TRIGNUM_SHIFT;
seqnum = (lbuf[BURST_TRIG] & I_SEQNUM_MASK)  >> I_SEQNUM_SHIFT;
fprintf(s,"   Trig#=  %d,  Subnum= %d\n",id,seqnum);
fprintf(s,"   TJD=    %d\n", lbuf[BURST_TJD]);
fprintf(s,"   SOD=  %.2f [sec]   delta=%5.2f\n",
					lbuf[BURST_SOD]/100.0, here_sod - lbuf[BURST_SOD]/100.0);
fprintf(s,"   RA=    %7.2f [deg]\n", lbuf[BURST_RA]/10000.0);
fprintf(s,"   Dec=   %7.2f [deg]\n", lbuf[BURST_DEC]/10000.0);
fprintf(s,"   Error= %.2f [arcmin, radius, statistical only]\n",
											(float)lbuf[BURST_ERROR]/60.0);
fprintf(s,"   Inten= %.2f [sigma]\n", lbuf[BURST_INTEN]/100.0);
fprintf(s,"   SC_RA= %6.2f [deg] (J2000)\n", lbuf[14]/10000.0);
fprintf(s,"   SC_DEC=%6.2f [deg] (J2000)\n", lbuf[15]/10000.0);
fprintf(s,"   TimeScale= %.2f [sigma]\n", lbuf[13]/10000.0);
fprintf(s,"   TimeError= %.2f [sigma]\n", lbuf[16]/10000.0);
// I'll give you more on these flags when the IBAS people define them:
fprintf(s,"   Det_Flags= 0x%08x\n", lbuf[9]);
fprintf(s,"   Test/MPos= 0x%08x\n", lbuf[12]);
fprintf(s,"   S/C_att=   0x%08x\n", lbuf[19]);
}

/*---------------------------------------------------------------------------*/
pr_milagro(lbuf,s)/* print the contents of a MILAGRO Position packet */
long		*lbuf;	/* Ptr to the newly arrived packet to print out */
FILE		*s;		/* Stream to print it to */
{
fprintf(s,"PKT INFO:    Received: LT %s",ctime((time_t *)&tloc));
fprintf(s,"   Type= %d   SN= %d,   ", lbuf[PKT_TYPE], lbuf[PKT_SERNUM]);
switch(lbuf[MISC] & 0x0F)
	{
	case 1:		fprintf(s,"Short\n");			break;
	case 2:		fprintf(s,"Medium(TBD)\n");		break;
	case 3:		fprintf(s,"Long(TBD)\n");		break;
	default:	fprintf(s,"Illegal\n");			break;
	}
fprintf(s,"   Hop_cnt= %d\n", lbuf[PKT_HOP_CNT]);
fprintf(s,"   PKT_SOD=  %.2f [sec]   delta=%5.2f\n",
					lbuf[PKT_SOD]/100.0, here_sod - lbuf[PKT_SOD]/100.0);
fprintf(s,"   Trig#=  %d,  Subnum= %d\n",lbuf[BURST_TRIG],(lbuf[MISC] >> 24) & 0xFF);
fprintf(s,"   TJD=    %d\n", lbuf[BURST_TJD]);
fprintf(s,"   SOD=  %.2f [sec]   delta=%5.2f\n",
					lbuf[BURST_SOD]/100.0, here_sod - lbuf[BURST_SOD]/100.0);
fprintf(s,"   RA=    %7.2f [deg]\n", lbuf[BURST_RA]/10000.0);
fprintf(s,"   Dec=   %7.2f [deg]\n", lbuf[BURST_DEC]/10000.0);
fprintf(s,"   Error= %.2f [deg, radius, statistical only]\n",
											(float)lbuf[BURST_ERROR]/10000.0);
fprintf(s,"   Inten= %d [events]\n", lbuf[BURST_INTEN]);
fprintf(s,"   Bkg=   %.2f [events]\n", lbuf[10]/10000.0);
fprintf(s,"   Duration=%.2f [sec]\n", lbuf[13]/100.0);
fprintf(s,"   Signif=%6.2e\n",pow((double)10.0,(double)-1.0*lbuf[12]/100.0));
fprintf(s,"   Annual_rate= %.2f [estimated occurances/year]\n", lbuf[14]/100.0);
fprintf(s,"   Zenith= %.2f [deg]\n", lbuf[15]/100.0);
if(lbuf[TRIGGER_ID] & 0x00000001)
	fprintf(s,"   Possible GRB.\n");
if(lbuf[TRIGGER_ID] & 0x00000002)
	fprintf(s,"   Definite GRB.\n");
if(lbuf[TRIGGER_ID] & (0x01 << 15))
	fprintf(s,"   Definately not a GRB.\n");
}

/*---------------------------------------------------------------------------*/
// The SWIFT_BAT_ALERT is currently not being sent to the TDRSS transmitter,
// so there is never any to receive and distribute through GCN.
// This is because the original motivation for it no longer exists.
// The ALERT type was created to come out at T_zero to "warm up" the transmitter
// and get the end-to-end bit-n-frame-sync's locked by the time the BAT_POS type
// became available ~20sec later (range: T+7-40sec, ave=20sec).
// But Swift is operating the TDRSS transmitter in a 24/7 mode, so it is always "warm".
pr_swift_bat_alert(lbuf,s) /* print the contents of a Swift-BAT Alert packet */
long		*lbuf;	/* Ptr to the newly arrived packet to print out */
FILE		*s;		/* Stream to print it to */
{
int	id, segnum;

fprintf(s,"PKT INFO:    Received: LT %s",ctime((time_t *)&tloc));
fprintf(s,"   Type= %d   SN= %d    Swift-BAT Alert\n",lbuf[PKT_TYPE],lbuf[PKT_SERNUM]);
fprintf(s,"   Hop_cnt=   %d\n", lbuf[PKT_HOP_CNT]);
fprintf(s,"   PKT_SOD=   %.2f [sec]   delta=%.2f [sec]\n",
					lbuf[PKT_SOD]/100.0, here_sod - lbuf[PKT_SOD]/100.0);
id =     (lbuf[BURST_TRIG] >> S_TRIGNUM_SHIFT) & S_TRIGNUM_MASK;
segnum = (lbuf[BURST_TRIG] >> S_SEGNUM_SHIFT) & S_SEGNUM_MASK;
fprintf(s,"   Trig#=     %d,  Segnum= %d\n", id, segnum);
fprintf(s,"   TJD=       %d\n", lbuf[BURST_TJD]);
fprintf(s,"   SOD=       %.2f [sec]   delta=%.2f [sec]\n",
					lbuf[BURST_SOD]/100.0, here_sod - lbuf[BURST_SOD]/100.0);
fprintf(s,"   Trig_Index=%d\n", lbuf[17]);
fprintf(s,"   Signif=    %.2f [sigma]\n", lbuf[21]/100.0);
if(lbuf[MISC] & 0x40000000)
	fprintf(s,"   This packet was ground-reprocessed from flight-data.\n");
if(lbuf[MISC] & 0x80000000)
	fprintf(s,"   This Notice was received with 1 (or more) CRC errors -- values are suspect.\n");
}

/*---------------------------------------------------------------------------*/
pr_swift_bat_pos_ack(lbuf,s)/* print the contents of a Swift-BAT Pos_Ack packet */
long		*lbuf;	/* Ptr to the newly arrived packet to print out */
FILE		*s;		/* Stream to print it to */
{
int i;
int id, segnum;

fprintf(s,"PKT INFO:    Received: LT %s",ctime((time_t *)&tloc));
if(lbuf[PKT_TYPE] == TYPE_SWIFT_BAT_GRB_POS_ACK_SRC)
fprintf(s,"   Type= %d   SN= %d    Swift-BAT Pos_Ack\n",lbuf[PKT_TYPE],lbuf[PKT_SERNUM]);
else if(lbuf[PKT_TYPE] == TYPE_SWIFT_BAT_TRANS)
fprintf(s,"   Type= %d   SN= %d    Swift-BAT Transient Position\n",lbuf[PKT_TYPE],lbuf[PKT_SERNUM]);
else if(lbuf[PKT_TYPE] == TYPE_SWIFT_BAT_GRB_POS_TEST)
fprintf(s,"   Type= %d   SN= %d    Swift-BAT Test Position\n",lbuf[PKT_TYPE],lbuf[PKT_SERNUM]);
else if(lbuf[PKT_TYPE] == TYPE_SWIFT_BAT_SUB_THRESHOLD_SRC)
fprintf(s,"   Type= %d   SN= %d    Swift-BAT SubThreshold Position\n",lbuf[PKT_TYPE],lbuf[PKT_SERNUM]);
else
fprintf(s,"   Type= %d   SN= %d    Illegal type!\n",lbuf[PKT_TYPE],lbuf[PKT_SERNUM]);
fprintf(s,"   Hop_cnt= %d\n", lbuf[PKT_HOP_CNT]);
fprintf(s,"   PKT_SOD= %.2f [sec]   delta=%.2f [sec]\n",
					lbuf[PKT_SOD]/100.0, here_sod - lbuf[PKT_SOD]/100.0);
id =     (lbuf[BURST_TRIG] >> S_TRIGNUM_SHIFT) & S_TRIGNUM_MASK;
segnum = (lbuf[BURST_TRIG] >> S_SEGNUM_SHIFT) & S_SEGNUM_MASK;
fprintf(s,"   Trig#=   %d,  Segnum= %d\n",id,segnum);
fprintf(s,"   TJD=     %d\n", lbuf[BURST_TJD]);
fprintf(s,"   SOD=     %.2f [sec]   delta=%.2f [sec]\n",
					lbuf[BURST_SOD]/100.0, here_sod - lbuf[BURST_SOD]/100.0);
fprintf(s,"   RA=      %7.2f [deg] (J2000)\n", lbuf[BURST_RA]/10000.0);
fprintf(s,"   Dec=     %7.2f [deg] (J2000)\n", lbuf[BURST_DEC]/10000.0);
fprintf(s,"   Error=   %.2f [arcmin, radius, statistical only]\n",
											(float)60.0*lbuf[BURST_ERROR]/10000.0);
fprintf(s,"   Inten=   %d [cnts]\n", lbuf[BURST_INTEN]);
fprintf(s,"   Peak=    %d [cnts]\n", lbuf[BURST_PEAK]);
fprintf(s,"   Int_Time=%.3f [sec]\n", lbuf[14]/250.0); // convert 4msec_ticks to sec
fprintf(s,"   Phi=     %6.2f [deg]\n", lbuf[12]/100.0);
fprintf(s,"   Theta=   %6.2f [deg]\n", lbuf[13]/100.0);
fprintf(s,"   Trig_Index=   %d\n", lbuf[17]);
fprintf(s,"   Soln_Status=  0x%x\n", lbuf[18]);
fprintf(s,"   Rate_Signif=  %.2f [sigma]\n", lbuf[21]/100.0); // signif of peak in FFT image
fprintf(s,"   Image_Signif= %.2f [sigma]\n", lbuf[20]/100.0); // signif of the rate trigger
fprintf(s,"   Bkg_Inten=    %d [cnts]\n", lbuf[22]);
fprintf(s,"   Bkg_Time=     %.2f [sec]   delta=%.2f [sec]\n",
				(double)lbuf[23]/100.0, lbuf[BURST_SOD]/100.0 - lbuf[23]/100.0);
fprintf(s,"   Bkg_Dur=      %.2f [sec]\n",lbuf[24]/100.0);  // Still whole seconds
fprintf(s,"   Merit_Vals= ");
for(i=0;i<10;i++) fprintf(s," %d ",*(((char *)(lbuf+36))+i));
fprintf(s,"\n");
if(lbuf[MISC] & 0x10000000)
	{
	fprintf(s,"\n");
	fprintf(s,"   Since all the BAT_Pos_Ack & BAT_LC & BAT_ScaledMap messages were missing in the real-time TDRSS stream,\n");
	fprintf(s,"   this Notice was reconstructed from the (delayed) Malindi-pass ground-generated BAT_Pos SERS message.\n");
	fprintf(s,"   As such, many of the fields above are undefined -- they are not available in the limited SERS messages.\n");
	fprintf(s,    "However, the GRB_RA/DEC, GRB_DATE/TIME, IMAGE_SIGNIF, and a few other fields are defined and valid.\n");
	fprintf(s,"\n");
	}
if(lbuf[MISC] & 0x20000000)
	{
	fprintf(s,"\n");
	fprintf(s,"   Since the real-time TDRSS version of the BAT_Pos_Ack message was missing in the TDRSS stream,\n");
	fprintf(s,"   this Notice was reconstructed from pieces of a LightCurve or ScaledMap message.\n");
	fprintf(s,"   As such, some of the fields above are undefined -- they are not available in the LC or SM messages.\n");
	fprintf(s,"\n");
	}
if(lbuf[MISC] & 0x01000000)
	{
	fprintf(s,"   This Notice was delayed by more than 60 sec past the end of the trigger integration interval;\n");
	fprintf(s,"   probably due to it occurring during a Malindi downlink session.\n");
	}
if(id < 99999)
	{
	fprintf(s,"   This Notice is generated as a result of an uploaded TOO -- this is not a normal BAT-detected burst.\n");
	fprintf(s,"   As a result of this TOO, only the GRB_RA/_DEC, SUN, MOON, and GAL_/ECL_COORDS fields will be valid!\n");
	}
if(lbuf[MISC] & 0x00800000)
	{
	fprintf(s,"   This Notice was originally generated with the GRB_DATE & GRB_TIME fields set to zero,\n");
	fprintf(s,"   so those fields are now filled using the spacecraft system clock (not the normal burst trigger_time).\n");
	fprintf(s,"   This is usually the result of an uploaded GRB_TOO -- this is not a normal BAT-detected burst.\n");
	}
if(lbuf[TRIGGER_ID] & 0x00000010)
	{
	fprintf(s,"   This is an image trigger.    (The RATE_SIGNIF & BKG_{INTEN, TIME, DUR} are undefined.)\n");
	}
else
	fprintf(s,"   This is a rate trigger.\n");
if(lbuf[TRIGGER_ID] & 0x00000001)
	fprintf(s,"   A point_source was found.\n");
else
	fprintf(s,"   A point_source was not found.\n");
if(lbuf[TRIGGER_ID] & 0x00000008)
	fprintf(s,"   This matches a source in the on-board catalog.\n");
else
	fprintf(s,"   This does not match any source in the on-board catalog.\n");
if(lbuf[TRIGGER_ID] & 0x00000100)
	fprintf(s,"   This matches a source in the ground catalog.\n");
else
	fprintf(s,"   This does not match any source in the ground catalog.\n");
if((lbuf[TRIGGER_ID] & 0x00000002)  && ((lbuf[TRIGGER_ID] & 0x00000020) == 0) && !(lbuf[TRIGGER_ID] & 0x00000100))
	fprintf(s,"   This is a GRB.\n");
else if(id < 99999)
	fprintf(s,"   This is a TOO.\n");
else
	fprintf(s,"   This is not a GRB.\n");
if(lbuf[TRIGGER_ID] & 0x00000004)
	fprintf(s,"   This is an interesting source.\n");
if(lbuf[TRIGGER_ID] & 0x00000040)
	fprintf(s,"   This trigger occurred during a high background rate interval.\n");
if(lbuf[TRIGGER_ID] & 0x00000080)
	fprintf(s,"   Since the IMAGE_SIGNIF is less than 7 sigma, this is a questionable detection.\n");
if(lbuf[TRIGGER_ID] & 0x00000200)
	fprintf(s,"   Since the Bkg_rate is greater than the foregnd_rate (exiting SAA), this is a questionable detection.\n");
if(lbuf[TRIGGER_ID] & 0x00000400)
    fprintf(s,"   This trigger occured while the StarTracker had lost lock, so it is probably bogus.\n");
if(lbuf[TRIGGER_ID] & 0x00000020)
	fprintf(s,"   This is a RETRACTION of a previous notice that identified this as a GRB -- it is NOT a GRB.\n");
if(lbuf[MISC] & 0x00002000)
  fprintf(s,"   There is a bright star near this position.\n");
if(lbuf[MISC] & 0x00004000)
  {
  fprintf(s,"   The Swift-BAT position is inside an NGC galaxy.\n");
  fprintf(s,"   There is a 0.05%% probability that this would happen by chance coincidence.\n");
  }
if(lbuf[MISC] & 0x00008000)
  {
  fprintf(s,"   An NGC galaxy is inside the Swift-BAT position error circle.\n");
  fprintf(s,"   There is a 2e5%% probability that an NGC galaxy would happen by chance coincidence.\n");
  }
if(lbuf[MISC] & 0x40000000)
	fprintf(s,"   This Notice was ground-reprocessed from flight-data.\n");
if(lbuf[MISC] & 0x80000000)
	fprintf(s,"   This Notice was received with 1 (or more) CRC errors -- values are suspect.\n");
}

/*---------------------------------------------------------------------------*/
pr_swift_bat_pos_nack(lbuf,s)/* print the contents of a Swift-BAT Pos_Nack packet */
long		*lbuf;	/* Ptr to the newly arrived packet to print out */
FILE		*s;		/* Stream to print it to */
{
int i;
int	id, segnum;

fprintf(s,"PKT INFO:    Received: LT %s",ctime((time_t *)&tloc));
fprintf(s,"   Type= %d   SN= %d    Swift-BAT Pos_Nack\n",lbuf[PKT_TYPE],lbuf[PKT_SERNUM]);
fprintf(s,"   Hop_cnt= %d\n", lbuf[PKT_HOP_CNT]);
fprintf(s,"   PKT_SOD= %.2f [sec]   delta=%.2f [sec]\n",
					lbuf[PKT_SOD]/100.0, here_sod - lbuf[PKT_SOD]/100.0);
id =     (lbuf[BURST_TRIG] >> S_TRIGNUM_SHIFT) & S_TRIGNUM_MASK;
segnum = (lbuf[BURST_TRIG] >> S_SEGNUM_SHIFT) & S_SEGNUM_MASK;
fprintf(s,"   Trig#=   %d,  Segnum= %d\n",id,segnum);
fprintf(s,"   TJD=     %d\n", lbuf[BURST_TJD]);
fprintf(s,"   SOD=     %.2f [sec]   delta=%.2f [sec]\n",
					lbuf[BURST_SOD]/100.0, here_sod - lbuf[BURST_SOD]/100.0);
fprintf(s,"   Trig_Index=   %d\n", lbuf[17]);
fprintf(s,"   Soln_Status=  0x%x\n", lbuf[18]);
//fprintf(s,"   Rate_Signif=  %.2f [sigma]\n", lbuf[21]/100.0);  // Always zero's
//fprintf(s,"   Image_Signif= %.2f [sigma]\n", lbuf[20]/100.0);
//fprintf(s,"   Bkg_Inten=    %d [cnts]\n", lbuf[22]);
//fprintf(s,"   Bkg_Time=     %.2f [sec]\n",(double)lbuf[23]/100.0);
//fprintf(s,"   Bkg_Dur=      %.2f [sec]\n",lbuf[24]/100);
fprintf(s,"   Merit_Vals= ");
for(i=0;i<10;i++) fprintf(s," %d ",*(((char *)(lbuf+36))+i));
fprintf(s,"\n");
if(lbuf[TRIGGER_ID] & 0x00000001)
	{
	fprintf(s,"   A point_source was found.\n");
	fprintf(s,"   ERR: a BAT_POS_NACK with the Pt-Src flag bit set!\n");
	}
else
	fprintf(s,"   A point_source was not found.\n");
if(lbuf[MISC] & 0x80000000)
	fprintf(s,"   This Notice was received with 1 (or more) CRC errors -- values are suspect.\n");
}

/*---------------------------------------------------------------------------*/
pr_swift_bat_lc(lbuf,s)/* print the contents of a Swift-BAT LightCurve packet */
long		*lbuf;	/* Ptr to the newly arrived packet to print out */
FILE		*s;		/* Stream to print it to */
{
int	id, segnum;

fprintf(s,"PKT INFO:    Received: LT %s",ctime((time_t *)&tloc));
fprintf(s,"   Type= %d   SN= %d    Swift-BAT LightCurve\n",lbuf[PKT_TYPE],lbuf[PKT_SERNUM]);
fprintf(s,"   Hop_cnt=    %d\n", lbuf[PKT_HOP_CNT]);
fprintf(s,"   PKT_SOD=    %.2f [sec]   delta=%.2f [sec]\n",
					lbuf[PKT_SOD]/100.0, here_sod - lbuf[PKT_SOD]/100.0);
id =     (lbuf[BURST_TRIG] >> S_TRIGNUM_SHIFT) & S_TRIGNUM_MASK;
segnum = (lbuf[BURST_TRIG] >> S_SEGNUM_SHIFT) & S_SEGNUM_MASK;
fprintf(s,"   Trig#=      %d,  Segnum= %d\n",id,segnum);
fprintf(s,"   TJD=        %d\n", lbuf[BURST_TJD]);
fprintf(s,"   SOD=        %.2f [sec]   delta=%.2f [sec]\n",
					lbuf[BURST_SOD]/100.0, here_sod - lbuf[BURST_SOD]/100.0);
fprintf(s,"   RA=         %7.2f [deg] (J2000)\n", lbuf[BURST_RA]/10000.0);
fprintf(s,"   Dec=        %7.2f [deg] (J2000)\n", lbuf[BURST_DEC]/10000.0);
fprintf(s,"   Trig_Index= %d\n", lbuf[17]);
fprintf(s,"   Phi=        %6.2f [deg]\n", lbuf[12]/100.0);
fprintf(s,"   Theta=      %6.2f [deg]\n", lbuf[13]/100.0);
fprintf(s,"   Delta_T=    %.2f [sec]\n", lbuf[14]/100.0);  // of the 3rd/final packet
fprintf(s,"   URL=        %s\n", (char *)(&lbuf[BURST_URL]));
if(lbuf[MISC] & 0x10000000)
	{
	fprintf(s,"   This lightcurve contains more than 50 illegal values -- probably an empty lightcurve.\n");
	fprintf(s,"   So the FITS file will have empty/missing rows -- possibly all rows will be missing.\n");
	}
if(lbuf[MISC] & 0x20000000)
	fprintf(s,"   This notice was forced out after watchdog timer expiring -- most likely due to missing packet(s).\n");
if(lbuf[MISC] & 0x08000000)
	fprintf(s,"   The 1st packet (of 3) was missing in the lightcurve data stream.\n");
if(lbuf[MISC] & 0x04000000)
	fprintf(s,"   The 2nd packet (of 3) was missing in the lightcurve data stream.\n");
if(lbuf[MISC] & 0x02000000)
	fprintf(s,"   The 3rd packet (of 3) was missing in the lightcurve data stream.\n");
if(lbuf[MISC] & 0x40000000)
	fprintf(s,"   This Notice was ground-reprocessed from flight-data.\n");
if(lbuf[MISC] & 0x80000000)
	fprintf(s,"   This Notice was received with 1 (or more) CRC errors -- values are suspect.\n");
if(lbuf[TRIGGER_ID] & 0x00000020)
	fprintf(s,"   This is a RETRACTION of a previous notice that identified this as a GRB -- it is NOT a GRB.\n");
if(lbuf[MISC] & (0x1<<24))  // Only if this got a copy of BAT_POS_ACK soln_status
	{                       // will the following be valid/defined.
	fprintf(s,"\n");
	fprintf(s,"   The next comments were copied from the BAT_POS Notice:\n");
	if(lbuf[TRIGGER_ID] & 0x00000010)
		fprintf(s,"   This is an image trigger.\n");
	else
		fprintf(s,"   This is a rate trigger.\n");
	if(lbuf[TRIGGER_ID] & 0x00000001)
		fprintf(s,"   A point_source was found.\n");
	else
		fprintf(s,"   A point_source was not found.\n");
	if(lbuf[TRIGGER_ID] & 0x00000008)
		fprintf(s,"   This matches a source in the on-board catalog.\n");
	else
		fprintf(s,"   This does not match any source in the on-board catalog.\n");
	if(lbuf[TRIGGER_ID] & 0x00000100)
		fprintf(s,"   This matches a source in the ground catalog.\n");
	else
		fprintf(s,"   This does not match any source in the ground catalog.\n");
	if((lbuf[TRIGGER_ID] & 0x00000002) && ((lbuf[TRIGGER_ID] & 0x00000020) == 0) && ((lbuf[TRIGGER_ID] & 0x00000100) == 0))
		fprintf(s,"   This is a GRB.\n");   // grb  &&  !def_not_grb                &&     !gnd_catalog
	else
		fprintf(s,"   This is not a GRB.\n");
	if(lbuf[TRIGGER_ID] & 0x00000040)
		fprintf(s,"   This trigger occurred during a high background rate interval.\n");
	if(lbuf[TRIGGER_ID] & 0x00000080)
		fprintf(s,"   Since the IMAGE_SIGNIF is less than 7 sigma, this is a questionable detection.\n");
	if(lbuf[TRIGGER_ID] & 0x00000200)
		fprintf(s,"   Since the foreground interval count rate is less than the bkg interval (SAA exit), this is a questionable detection.\n");
	if(lbuf[TRIGGER_ID] & 0x00000400)
		fprintf(s,"   This trigger occured while the StarTracker had lost lock, so it is probably bogus.\n");
	if(lbuf[TRIGGER_ID] & 0x00000004)
		fprintf(s,"   This is an interesting source.\n");
	}
if(lbuf[MISC] & 0x00002000)
  fprintf(s,"   There is a bright star near this position.\n");
if(lbuf[MISC] & 0x00004000)
  {
  fprintf(s,"   The Swift-BAT position is inside an NGC galaxy.\n");
  fprintf(s,"   There is a 0.05%% probability that this would happen by chance coincidence.\n");
  }
if(lbuf[MISC] & 0x00008000)
  {
  fprintf(s,"   An NGC galaxy is inside the Swift-BAT position error circle.\n");
  fprintf(s,"   There is a 2e5%% probability that an NGC galaxy would happen by chance coincidence.\n");
  }
}

/*---------------------------------------------------------------------------*/
pr_swift_fom_2obs(lbuf,s)/* print the contents of a Swift-FOM 2Observe packet */
long		*lbuf;	/* Ptr to the newly arrived packet to print out */
FILE		*s;		/* Stream to print it to */
{
int	id, segnum;

fprintf(s,"PKT INFO:    Received: LT %s",ctime((time_t *)&tloc));
fprintf(s,"   Type= %d   SN= %d    Swift FOM2Observe\n",lbuf[PKT_TYPE],lbuf[PKT_SERNUM]);
fprintf(s,"   Hop_cnt=     %d\n", lbuf[PKT_HOP_CNT]);
fprintf(s,"   PKT_SOD=     %.2f [sec]   delta=%.2f [sec]\n",
					lbuf[PKT_SOD]/100.0, here_sod - lbuf[PKT_SOD]/100.0);
id =     (lbuf[BURST_TRIG] >> S_TRIGNUM_SHIFT) & S_TRIGNUM_MASK;
segnum = (lbuf[BURST_TRIG] >> S_SEGNUM_SHIFT) & S_SEGNUM_MASK;
fprintf(s,"   Trig#=       %d,  Segnum= %d\n",id,segnum);
fprintf(s,"   TJD=         %d\n", lbuf[BURST_TJD]);
fprintf(s,"   SOD=         %.2f [sec]   delta=%.2f [sec]\n",
					lbuf[BURST_SOD]/100.0, here_sod - lbuf[BURST_SOD]/100.0);
fprintf(s,"   RA=         %7.2f [deg] (J2000)\n", lbuf[BURST_RA]/10000.0);
fprintf(s,"   Dec=        %7.2f [deg] (J2000)\n", lbuf[BURST_DEC]/10000.0);
fprintf(s,"   Trig_Index=  %d\n", lbuf[17]);
fprintf(s,"   Rate_Signif= %.2f [sigma]\n", lbuf[21]/100.0);
fprintf(s,"   Flags=       0x%x\n", lbuf[18]);
fprintf(s,"   Merit=       %.2f\n", lbuf[38]/100.0);
if(id < 100000)
	{
	fprintf(s,"   This Notice is generated as a result of an uploaded TOO -- this is not a normal BAT-detected burst.\n");
	fprintf(s,"   As a result of this TOO, the TRIG# and Rate_Signif fields will not be valid!\n");
	fprintf(s,"   And sometimes the GRB_DATE/TIME fields are not valid!\n");
	}
if((id > 100000) && (segnum > 0))
	{
	fprintf(s,"   This Notice is generated as a result of an uploaded TOO on an old GRB -- this is not a newly detected burst.\n");
	fprintf(s,"   As a result of this TOO, the TRIG# and Rate_Signif fields will not be valid!\n");
	fprintf(s,"   And sometimes the GRB_DATE/TIME fields are not valid!\n");
	}
else if(lbuf[MISC] & 0x20000000)
	fprintf(s,"   Because no BAT_Position Notice was received, this FOM Notice was forced out after watchdog expiration.\n");
if(lbuf[18] & 0x1)
	{
	fprintf(s,"   This was of sufficient merit to become the new Automated Target;\n");
	fprintf(s,"   but if the Current PPT has a greater merit, then we will not request a slew.\n");
	}
else
	{
	fprintf(s,"   This was NOT of sufficient merit to become the new Automated Target.\n");
	fprintf(s,"   FOM will NOT request the s/c to slew.\n");
	}
if(lbuf[18] & 0x2)
	{
	fprintf(s,"   FOM will request the s/c to slew.\n");
	fprintf(s,"   The S/C has yet to make its decision about safe to slew or not.\n");
	}
else
	fprintf(s,"   FOM will NOT request to observe.\n");
if(lbuf[MISC] & 0x80000000)
	fprintf(s,"   This Notice was received with 1 (or more) CRC errors -- values are suspect.\n");
if(lbuf[16] & 0x00000020)
	fprintf(s,"   This is a RETRACTION of a previous notice that identified this as a GRB -- it is NOT a GRB.\n");
if(lbuf[MISC] & (0x1<<24))  // Only if this got a copy of BAT_POS_ACK soln_status
	{                       // will the following be valid/defined.
	fprintf(s,"\n");
	fprintf(s,"   The next comments were copied from the BAT_POS Notice:\n");
	if(lbuf[16] & 0x00000010)
		fprintf(s,"   This is an image trigger.\n");
	else
		fprintf(s,"   This is a rate trigger.\n");
	if(lbuf[16] & 0x00000001)
		fprintf(s,"   A point_source was found.\n");
	else
		fprintf(s,"   A point_source was not found.\n");
	if(lbuf[16] & 0x00000008)
		fprintf(s,"   This matches a source in the on-board catalog.\n");
	else
		fprintf(s,"   This does not match any source in the on-board catalog.\n");
	if(lbuf[16] & 0x00000100)
		fprintf(s,"   This matches a source in the ground catalog.\n");
	else
		fprintf(s,"   This does not match any source in the ground catalog.\n");
	if((lbuf[16] & 0x00000002) && ((lbuf[16] & 0x00000020) == 0) && ((lbuf[16] & 0x00000100) == 0))
		fprintf(s,"   This is a GRB.\n");   // grb  &&  !def_not_grb                &&     !gnd_catalog
	else
		fprintf(s,"   This is not a GRB.\n");
	if(lbuf[16] & 0x00000040)
		fprintf(s,"   This trigger occurred during a high background rate interval.\n");
	if(lbuf[16] & 0x00000080)
		fprintf(s,"   Since the IMAGE_SIGNIF is less than 7 sigma, this is a questionable detection.\n");
	if(lbuf[16] & 0x00000800)
		fprintf(s,"   The IMAGE_SIGNIF is VERY LOW, this is almost certainly not real.\n");
	if(lbuf[16] & 0x00000200)
		fprintf(s,"   Since the foreground interval count rate is less than the bkg interval (SAA exit), this is a questionable detection.\n");
	if(lbuf[16] & 0x00000400)
		fprintf(s,"   This trigger occured while the StarTracker had lost lock, so it is probably bogus.\n");
	if(lbuf[16] & 0x00000004)
		fprintf(s,"   This is an interesting source.\n");
	}
if(lbuf[MISC] & 0x00002000)
  fprintf(s,"   There is a bright star near this position.\n");
if(lbuf[MISC] & 0x00004000)
  {
  fprintf(s,"   The Swift-BAT position is inside an NGC galaxy.\n");
  fprintf(s,"   There is a 0.05%% probability that this would happen by chance coincidence.\n");
  }
if(lbuf[MISC] & 0x00008000)
  {
  fprintf(s,"   An NGC galaxy is inside the Swift-BAT position error circle.\n");
  fprintf(s,"   There is a 2e5%% probability that an NGC galaxy would happen by chance coincidence.\n");
  }
}

/*---------------------------------------------------------------------------*/
pr_swift_sc_2slew(lbuf,s)  /* print the contents of a Swift-FOM 2Slew packet */
long		*lbuf;	/* Ptr to the newly arrived packet to print out */
FILE		*s;		/* Stream to print it to */
{
int	id, segnum;

fprintf(s,"PKT INFO:    Received: LT %s",ctime((time_t *)&tloc));
fprintf(s,"   Type= %d   SN= %d    Swift SC2Slew\n",lbuf[PKT_TYPE],lbuf[PKT_SERNUM]);
fprintf(s,"   Hop_cnt=    %d\n", lbuf[PKT_HOP_CNT]);
fprintf(s,"   PKT_SOD=    %.2f [sec]   delta=%.2f [sec]\n",
					lbuf[PKT_SOD]/100.0, here_sod - lbuf[PKT_SOD]/100.0);
id =     (lbuf[BURST_TRIG] >> S_TRIGNUM_SHIFT) & S_TRIGNUM_MASK;
segnum = (lbuf[BURST_TRIG] >> S_SEGNUM_SHIFT) & S_SEGNUM_MASK;
fprintf(s,"   Trig#=      %d,  Segnum= %d\n",id,segnum);
fprintf(s,"   TJD=        %d\n", lbuf[BURST_TJD]);
fprintf(s,"   SOD=        %.2f [sec]   delta=%.2f [sec]\n",
					lbuf[BURST_SOD]/100.0, here_sod - lbuf[BURST_SOD]/100.0);
fprintf(s,"   RA=       %9.4f [deg] (J2000)\n", lbuf[BURST_RA]/10000.0);
fprintf(s,"   Dec=      %9.4f [deg] (J2000)\n", lbuf[BURST_DEC]/10000.0);
fprintf(s,"   Roll=     %9.4f [deg] (J2000)\n", lbuf[9]/10000.0);
fprintf(s,"   Trig_Index= %d\n", lbuf[17]);
fprintf(s,"   Rate_Signif=%.2f [sigma]\n", lbuf[21]/100.0);
fprintf(s,"   Slew_Query= %d\n", lbuf[18]);
fprintf(s,"   Wait_Time=  %.2f [sec]\n", lbuf[13]/100.0);
fprintf(s,"   Obs_Time=   %.2f [sec]\n", lbuf[14]/100.0);
fprintf(s,"   Inst_Modes: BAT: %d   XRT: %d   UVOT: %d\n",lbuf[22],lbuf[23],lbuf[24]);
fprintf(s,"   Merit=      %.2f\n", lbuf[38]/100.0);
if(id < 100000)
	{
	fprintf(s,"   This Notice is generated as a result of an uploaded TOO -- this is not a normal BAT-detected burst.\n");
	fprintf(s,"   As a result of this TOO, the TRIG# and Rate_Signif fields will not be valid!\n");
	fprintf(s,"   And sometimes the GRB_DATE/TIME fields are not valid!\n");
	}
if((id > 100000) && (segnum > 0))
	{
	fprintf(s,"   This Notice is generated as a result of an uploaded TOO on an old GRB -- this is not a newly detected burst.\n");
	fprintf(s,"   As a result of this TOO, the TRIG# and Rate_Signif fields will not be valid!\n");
	fprintf(s,"   And sometimes the GRB_DATE/TIME fields are not valid!\n");
	}
if(lbuf[18] == 0)                                          // The SC reply
	{
	if(lbuf[13] == 0)
	fprintf(s,"   SC will immediately slew to this target.\n");
	else
	fprintf(s,"   SC will not immediately slew to this target; see WAIT_TIME.\n");
	}
else if(lbuf[18] == 1)
	{
	fprintf(s,"   SC will NOT slew to this target; Sun constraint.\n");
	fprintf(s,"   There maybe a delayed slew or no slew at all; see WAIT_TIME.\n");
	}
else if(lbuf[18] == 2)
	{
	fprintf(s,"   SC will NOT slew to this target; Earth_limb constraint.\n");
	fprintf(s,"   There maybe a delayed slew or no slew at all; see WAIT_TIME.\n");
	}
else if(lbuf[18] == 3)
	{
	fprintf(s,"   SC will NOT slew to this target; Moon constraint.\n");
	fprintf(s,"   There maybe a delayed slew or no slew at all; see WAIT_TIME.\n");
	}
else if(lbuf[18] == 4)
	{
	fprintf(s,"   SC will NOT slew to this target; Ram_vector constraint.\n");
	fprintf(s,"   There maybe a delayed slew or no slew at all; see WAIT_TIME.\n");
	}
else if(lbuf[18] == 5)
	fprintf(s,"   SC will NOT slew to this target; Invalid slew_request parameter value.\n");
else
	fprintf(s,"   Unknown reply_code.\n");
if(lbuf[MISC] & 0x80000000)
	fprintf(s,"   This Notice was received with 1 (or more) CRC errors -- values are suspect.\n");
if(lbuf[16] & 0x00000020)
	fprintf(s,"   This is a RETRACTION of a previous notice that identified this as a GRB -- it is NOT a GRB.\n");
if(lbuf[MISC] & (0x1<<24))  // Only if this got a copy of BAT_POS_ACK soln_status
	{                       // will the following be valid/defined.
	fprintf(s,"\n");
	fprintf(s,"   The next comments were copied from the BAT_POS Notice:\n");
	if(lbuf[16] & 0x00000010)
		fprintf(s,"   This is an image trigger.\n");
	else
		fprintf(s,"   This is a rate trigger.\n");
	if(lbuf[16] & 0x00000001)
		fprintf(s,"   A point_source was found.\n");
	else
		fprintf(s,"   A point_source was not found.\n");
	if(lbuf[16] & 0x00000008)
		fprintf(s,"   This matches a source in the on-board catalog.\n");
	else
		fprintf(s,"   This does not match any source in the on-board catalog.\n");
	if(lbuf[16] & 0x00000100)
		fprintf(s,"   This matches a source in the ground catalog.\n");
	else
		fprintf(s,"   This does not match any source in the ground catalog.\n");
	if((lbuf[16] & 0x00000002) && ((lbuf[16] & 0x00000020) == 0) && ((lbuf[16] & 0x00000100) == 0))
		fprintf(s,"   This is a GRB.\n");   // grb  &&  !def_not_grb                &&     !gnd_catalog
	else
		fprintf(s,"   This is not a GRB.\n");
	if(lbuf[16] & 0x00000040)
		fprintf(s,"   This trigger occurred during a high background rate interval.\n");
	if(lbuf[16] & 0x00000080)
		fprintf(s,"   Since the IMAGE_SIGNIF is less than 7 sigma, this is a questionable detection.\n");
	if(lbuf[16] & 0x00000200)
		fprintf(s,"   Since the foreground interval count rate is less than the bkg interval (SAA exit), this is a questionable detection.\n");
	if(lbuf[16] & 0x00000400)
		fprintf(s,"   This trigger occured while the StarTracker had lost lock, so it is probably bogus.\n");
	if(lbuf[16] & 0x00000004)
		fprintf(s,"   This is an interesting source.\n");
	}
if(lbuf[MISC] & 0x00002000)
  fprintf(s,"   There is a bright star near this position.\n");
if(lbuf[MISC] & 0x00004000)
  {
  fprintf(s,"   The Swift-BAT position is inside an NGC galaxy.\n");
  fprintf(s,"   There is a 0.05%% probability that this would happen by chance coincidence.\n");
  }
if(lbuf[MISC] & 0x00008000)
  {
  fprintf(s,"   An NGC galaxy is inside the Swift-BAT position error circle.\n");
  fprintf(s,"   There is a 2e5%% probability that an NGC galaxy would happen by chance coincidence.\n");
  }
}

/*---------------------------------------------------------------------------*/
pr_swift_xrt_pos_ack(lbuf,s)/* print the contents of a Swift-XRT Pos_Ack packet */
long		*lbuf;	/* Ptr to the newly arrived packet to print out */
FILE		*s;		/* Stream to print it to */
{
int	id, segnum;

fprintf(s,"PKT INFO:    Received: LT %s",ctime((time_t *)&tloc));
fprintf(s,"   Type= %d   SN= %d    Swift-XRT Pos_Ack\n",lbuf[PKT_TYPE],lbuf[PKT_SERNUM]);
fprintf(s,"   Hop_cnt=  %d\n", lbuf[PKT_HOP_CNT]);
fprintf(s,"   PKT_SOD=  %.2f [sec]   delta=%.2f [sec]\n",
					lbuf[PKT_SOD]/100.0, here_sod - lbuf[PKT_SOD]/100.0);
id =     (lbuf[BURST_TRIG] >> S_TRIGNUM_SHIFT) & S_TRIGNUM_MASK;
segnum = (lbuf[BURST_TRIG] >> S_SEGNUM_SHIFT) & S_SEGNUM_MASK;
fprintf(s,"   Trig#=    %d,  Segnum= %d\n",id,segnum);
fprintf(s,"   Start_TJD=%d\n", lbuf[BURST_TJD]);
fprintf(s,"   Start_SOD=%.2f [sec]   delta=%.2f [sec]\n",
					lbuf[BURST_SOD]/100.0, here_sod - lbuf[BURST_SOD]/100.0);
fprintf(s,"   RA=       %9.4f [deg] (J2000)\n", lbuf[BURST_RA]/10000.0);
fprintf(s,"   Dec=      %9.4f [deg] (J2000)\n", lbuf[BURST_DEC]/10000.0);
fprintf(s,"   Error=    %.1f [arcsec, radius, statistical only]\n",
											(float)3600.0*lbuf[BURST_ERROR]/10000.0);
fprintf(s,"   Flux=     %.2f [arb]\n", lbuf[BURST_INTEN]/100.0);  //this are arbitrary units
fprintf(s,"   Signif=   %.2f [sigma]\n", lbuf[21]/100.0); // sqrt(num_pixels_w_photons)
fprintf(s,"   TAM[0]=   %7.2f\n", lbuf[12]/100.0);
fprintf(s,"   TAM[1]=   %7.2f\n", lbuf[13]/100.0);
fprintf(s,"   TAM[2]=   %7.2f\n", lbuf[14]/100.0);
fprintf(s,"   TAM[3]=   %7.2f\n", lbuf[15]/100.0);
fprintf(s,"   Amplifier=%d\n", (lbuf[17]>>8) & 0xFF);
fprintf(s,"   Waveform= %d\n",  lbuf[17] & 0xFF);
if(lbuf[MISC] & 0x10000000)
	{
	fprintf(s,"   This Notice was reconstructed from the (delayed) Malindi-pass ground-generated XRT_Pos SERS message.\n");
	fprintf(s,"   As such, many of the fields above are undefined -- they are not available in the limited SERS messages.\n");
	fprintf(s,"   However, the GRB_RA/DEC, IMG_START_DATE/TIME, GRB_INTEN, and a few other fields are defined and valid.\n");
	}
if(id < 100000)
	{
	fprintf(s,"   This Notice is generated as a result of an uploaded TOO -- this is not a normal BAT-detected burst.\n");
	fprintf(s,"   And the IMG_START_DATE/TIME fields are the current epoch -- not the original detection.\n");
	}
if((id > 100000) && (segnum > 0))
	{
	fprintf(s,"   This Notice is generated as a result of an uploaded TOO on an old GRB -- this is not a newly detected burst.\n");
	fprintf(s,"   And the IMG_START_DATE/TIME fields are the current epoch -- not the original detection.\n");
	}
if(lbuf[TRIGGER_ID] & 0x00000020)
	fprintf(s,"   This is a RETRACTION of a previous notice that identified this as a GRB -- it is NOT a GRB.\n");
if(lbuf[TRIGGER_ID] & 0x1)
	{
	fprintf(s,"   The object found at this position is either a very bright burst or a cosmic ray hit.\n");
	fprintf(s,"   Examine the XRT Image to differentiate (CRs are much more compact; see examples at:\n");
	fprintf(s,"   http://www.swift.psu.edu/xrt/XRT_Postage_Stamp_Image_Photo_Gallery.htm ).\n");
	}
if(lbuf[TRIGGER_ID] & 0x1)
	fprintf(s,"   The 'object' found for this position is very likely a cosmic ray hit in the CCD; not a burst afterglow.\n");
if(lbuf[MISC] & 0x00002000)
  fprintf(s,"   There is a bright star near this position.\n");
if(lbuf[MISC] & 0x00004000)
  {
  fprintf(s,"   The Swift-XRT position is inside an NGC galaxy.\n");
  fprintf(s,"   There is a 0.05%% probability that this would happen by chance coincidence.\n");
  }
if(lbuf[MISC] & 0x00008000)
  {
  fprintf(s,"   An NGC galaxy is inside the Swift-XRT position error circle.\n");
  fprintf(s,"   There is a 2e5%% probability that an NGC galaxy would happen by chance coincidence.\n");
  }
if(lbuf[MISC] & 0x40000000)
	fprintf(s,"   This is a ground-generated Notice -- not flight-generated.\n");
if(lbuf[MISC] & 0x80000000)
	fprintf(s,"   This Notice was received with 1 (or more) CRC errors -- values are suspect.\n");
}

/*---------------------------------------------------------------------------*/
pr_swift_xrt_pos_nack(lbuf,s)/* print the contents of a Swift-XRT Pos_Nack packet */
long		*lbuf;	/* Ptr to the newly arrived packet to print out */
FILE		*s;		/* Stream to print it to */
{
int	id, segnum;

fprintf(s,"PKT INFO:    Received: LT %s",ctime((time_t *)&tloc));
fprintf(s,"   Type= %d   SN= %d    Swift-XRT Pos_Nack\n",lbuf[PKT_TYPE],lbuf[PKT_SERNUM]);
fprintf(s,"   Hop_cnt=  %d\n", lbuf[PKT_HOP_CNT]);
fprintf(s,"   PKT_SOD=  %.2f [sec]   delta=%.2f [sec]\n",
								lbuf[PKT_SOD]/100.0, here_sod - lbuf[PKT_SOD]/100.0);
id =     (lbuf[BURST_TRIG] >> S_TRIGNUM_SHIFT) & S_TRIGNUM_MASK;
segnum = (lbuf[BURST_TRIG] >> S_SEGNUM_SHIFT) & S_SEGNUM_MASK;
fprintf(s,"   Trig#=    %d,  Segnum= %d\n",id,segnum);
fprintf(s,"   Start_TJD=%d\n", lbuf[BURST_TJD]);
fprintf(s,"   Start_SOD=%.2f [sec]   delta=%.2f [sec]\n",
					lbuf[BURST_SOD]/100.0, here_sod - lbuf[BURST_SOD]/100.0);
fprintf(s,"   Bore_RA=  %9.4f [deg] (J2000)\n", lbuf[BURST_RA]/10000.0);
fprintf(s,"   Bore_Dec= %9.4f [deg] (J2000)\n", lbuf[BURST_DEC]/10000.0);
fprintf(s,"   Counts=   %d    Min_needed= %d\n",lbuf[BURST_INTEN],lbuf[10]);
fprintf(s,"   StdD=     %.2f   Max_StdDev_for_Good=%.2f  [arcsec]\n",
								3600.0*lbuf[11]/10000.0,3600.0*lbuf[12]/10000.0);
fprintf(s,"   Ph2_iter= %d    Max_iter_allowed= %d\n",lbuf[16],lbuf[17]);
fprintf(s,"   Err_Code= %d\n", lbuf[18]);
fprintf(s,"           = 1 = No source found in the image.\n");
fprintf(s,"           = 2 = Algorithm did not converge; too many interations.\n");
fprintf(s,"           = 3 = Standard deviation too large.\n");
fprintf(s,"           = 0xFFFF = General error.\n");
if(lbuf[18] == 1)
	fprintf(s,"   No source found in the image.\n");
else if(lbuf[18] == 2)
	fprintf(s,"   Algorithm did not converge; too many interations.\n");
else if(lbuf[18] == 3)
	fprintf(s,"   Standard deviation too large.\n");
else
	fprintf(s,"   UNRECOGNIZED ERROR CODE.\n");
if(lbuf[MISC] & 0x80000000)
	fprintf(s,"   This Notice was received with 1 (or more) CRC errors -- values are suspect.\n");
}

/*---------------------------------------------------------------------------*/
pr_swift_xrt_spec(lbuf,s)/* print the contents of a Swift-XRT Spectrum packet */
long		*lbuf;	/* Ptr to the newly arrived packet to print out */
FILE		*s;		/* Stream to print it to */
{
int	  id, segnum;
float delta;

fprintf(s,"PKT INFO:    Received: LT %s",ctime((time_t *)&tloc));
fprintf(s,"   Type= %d   SN= %d    Swift-XRT Spectrum\n",lbuf[PKT_TYPE],lbuf[PKT_SERNUM]);
fprintf(s,"   Hop_cnt=    %d\n", lbuf[PKT_HOP_CNT]);
fprintf(s,"   PKT_SOD=    %.2f [sec]   delta=%.2f [sec]\n",
					lbuf[PKT_SOD]/100.0, here_sod - lbuf[PKT_SOD]/100.0);
id =     (lbuf[BURST_TRIG] >> S_TRIGNUM_SHIFT) & S_TRIGNUM_MASK;
segnum = (lbuf[BURST_TRIG] >> S_SEGNUM_SHIFT) & S_SEGNUM_MASK;
fprintf(s,"   Trig#=      %d,  Segnum= %d\n",id,segnum);
fprintf(s,"   Bore_RA=    %9.4f [deg] (J2000)\n", lbuf[BURST_RA]/10000.0);
fprintf(s,"   Bore_Dec=   %9.4f [deg] (J2000)\n", lbuf[BURST_DEC]/10000.0);
fprintf(s,"   Start_TJD=  %d\n", lbuf[BURST_TJD]);
fprintf(s,"   Start_SOD=  %.2f [sec]\n", lbuf[BURST_SOD]/100.0);
fprintf(s,"   Stop_TJD=   %d\n", lbuf[10]);
fprintf(s,"   Stop_SOD=   %.2f [sec]\n", lbuf[11]/100.0);
delta = lbuf[BURST_SOD]/100.0 - lbuf[11]/100.0;
if(delta < 0.0) delta += 86400.0;  // Handle UT_midnight rollover
fprintf(s,"   Stop_Start= %.2f [sec]\n", delta);
fprintf(s,"   LiveTime=   %.2f [sec]\n", lbuf[9]/100.0);
fprintf(s,"   Mode=       %d\n", lbuf[12]);
fprintf(s,"   Waveform=   %d\n", lbuf[13]);
fprintf(s,"   Bias=       %d\n", lbuf[14]);
fprintf(s,"   Term_cond=  %d\n", lbuf[21] & 0x07);
fprintf(s,"   URL=        %s\n", (char *)(&lbuf[BURST_URL]));
if(id < 100000)
	{
	fprintf(s,"   This Notice is generated as a result of an uploaded TOO -- this is not a normal BAT-detected burst.\n");
	fprintf(s,"   And the SPEC_START/STOP_DATE/TIME fields are the current epoch -- not the original detection.\n");
	}
if((id > 100000) && (segnum > 0))
	{
	fprintf(s,"   This Notice is generated as a result of an uploaded TOO on an old GRB -- this is not a newly detected burst.\n");
	fprintf(s,"   And the SPEC_START/STOP_DATE/TIME fields are the current epoch -- not the original detection.\n");
	}
if((lbuf[21] & 0x07) == 1)
	fprintf(s,"   This spectrum accumulation was terminated by the 'Time' condition.\n");
else if((lbuf[21] & 0x07) == 2)
	fprintf(s,"   This spectrum accumulation was terminated by the 'Snapshot End' condition.\n");
else if((lbuf[21] & 0x07) == 3)
	fprintf(s,"   This spectrum accumulation was terminated by the 'SAA Entry' condition.\n");
else if((lbuf[21] & 0x07) == 4)
	fprintf(s,"   This spectrum accumulation was terminated by the 'LRPD-to-WT Transition' condition.\n");
else if((lbuf[21] & 0x07) == 5)
	fprintf(s,"   This spectrum accumulation was terminated by the 'WT-to-LRorPC Transition' condition.\n");
else
	fprintf(s,"   WARNING: This spectrum accumulation has an unrecognized termination condition.\n");
if(lbuf[MISC] & 0x08000000)
	fprintf(s,"   The 1st packet (of 3) was missing in the spectrum data stream.\n");
if(lbuf[MISC] & 0x04000000)
	fprintf(s,"   The 2nd packet (of 3) was missing in the spectrum data stream.\n");
if(lbuf[MISC] & 0x02000000)
	fprintf(s,"   The 3rd packet (of 3) was missing in the spectrum data stream.\n");
if(lbuf[MISC] & 0x40000000)
	fprintf(s,"   This is a ground-generated Notice -- not flight-generated.\n");
if(lbuf[MISC] & 0x20000000)
	fprintf(s,"   This notice was forced out after watchdog timer expiring -- most likely due to missing packet(s).\n");
if(lbuf[MISC] & 0x80000000)
	fprintf(s,"   This Notice was received with 1 (or more) CRC errors -- values are suspect.\n");
}

/*---------------------------------------------------------------------------*/
pr_swift_xrt_image(lbuf,s) /* print the contents of a Swift-XRT Image packet */
long		*lbuf;	/* Ptr to the newly arrived packet to print out */
FILE		*s;		/* Stream to print it to */
{
int	id, segnum;

fprintf(s,"PKT INFO:    Received: LT %s",ctime((time_t *)&tloc));
fprintf(s,"   Type= %d   SN= %d    Swift-XRT Image\n",lbuf[PKT_TYPE],lbuf[PKT_SERNUM]);
fprintf(s,"   Hop_cnt=    %d\n", lbuf[PKT_HOP_CNT]);
fprintf(s,"   PKT_SOD=    %.2f [sec]   delta=%.2f [sec]\n",
					lbuf[PKT_SOD]/100.0, here_sod - lbuf[PKT_SOD]/100.0);
id =     (lbuf[BURST_TRIG] >> S_TRIGNUM_SHIFT) & S_TRIGNUM_MASK;
segnum = (lbuf[BURST_TRIG] >> S_SEGNUM_SHIFT) & S_SEGNUM_MASK;
fprintf(s,"   Trig#=      %d,  Segnum= %d\n",id,segnum);
fprintf(s,"   Start_TJD=  %d\n", lbuf[BURST_TJD]);
fprintf(s,"   Start_SOD=  %.2f [sec]   delta=%.2f [sec]\n",
					lbuf[BURST_SOD]/100.0, here_sod - lbuf[BURST_SOD]/100.0);
fprintf(s,"   RA=         %9.4f [deg] (J2000)\n", lbuf[BURST_RA]/10000.0);
fprintf(s,"   Dec=        %9.4f [deg] (J2000)\n", lbuf[BURST_DEC]/10000.0);
fprintf(s,"   Error=      %.1f [arcsec]\n",3600.0*lbuf[11]/10000.0);
fprintf(s,"   Counts=     %d\n",lbuf[BURST_INTEN]);
fprintf(s,"   Centroid_X= %.2f\n",lbuf[12]/100.0);
fprintf(s,"   Centroid_Y= %.2f\n",lbuf[13]/100.0);
fprintf(s,"   Raw_X=      %d\n",lbuf[14]);
fprintf(s,"   Raw_Y=      %d\n",lbuf[15]);
fprintf(s,"   Roll=       %.4f [deg]\n",lbuf[16]/10000.0);
fprintf(s,"   ExpoTime=   %.2f [sec]\n",lbuf[18]/100.0);
fprintf(s,"   Gain=       %d\n",(lbuf[17] >> 16) & 0xFF);
fprintf(s,"   Mode=       %d\n",(lbuf[17] >>  8) & 0xFF);
fprintf(s,"   Waveform=   %d\n",(lbuf[17]      ) & 0xFF);
fprintf(s,"   GRB_Pos_in_XRT_Y= %.4f\n",lbuf[20]/100.0);
fprintf(s,"   GRB_Pos_in_XRT_Z= %.4f\n",lbuf[21]/100.0);
fprintf(s,"   URL=        %s\n", (char *)(&lbuf[BURST_URL]));
if(id < 100000)
	{
	fprintf(s,"   This Notice is generated as a result of an uploaded TOO -- this is not a normal BAT-detected burst.\n");
	fprintf(s,"   And the IMG_START/STOP_DATE/TIME fields are the current epoch -- not the original detection.\n");
	}
if((id > 100000) && (segnum > 0))
	{
	fprintf(s,"   This Notice is generated as a result of an uploaded TOO on an old GRB -- this is not a newly detected burst.\n");
	fprintf(s,"   And the IMG_START/STOP_DATE/TIME fields are the current epoch -- not the original detection.\n");
	}
if(lbuf[MISC] & 0x00002000)
  fprintf(s,"   There is a bright star near this position.\n");
if(lbuf[MISC] & 0x00004000)
  {
  fprintf(s,"   The Swift-XRT position is inside an NGC galaxy.\n");
  fprintf(s,"   There is a 0.05%% probability that this would happen by chance coincidence.\n");
  }
if(lbuf[MISC] & 0x00008000)
  {
  fprintf(s,"   An NGC galaxy is inside the Swift-XRT position error circle.\n");
  fprintf(s,"   There is a 2e5%% probability that an NGC galaxy would happen by chance coincidence.\n");
  }
if(lbuf[MISC] & 0x08000000)
	fprintf(s,"   The 1st packet (of 3) was missing in the image data stream.\n");
if(lbuf[MISC] & 0x04000000)
	fprintf(s,"   The 2nd packet (of 3) was missing in the image data stream.\n");
if(lbuf[MISC] & 0x02000000)
	fprintf(s,"   The 3rd packet (of 3) was missing in the image data stream.\n");
if(lbuf[MISC] & 0x40000000)
	fprintf(s,"   This is a ground-generated Notice -- not flight-generated.\n");
if(lbuf[MISC] & 0x20000000)
	fprintf(s,"   This notice was forced out after watchdog timer expiring -- most likely due to missing packet(s).\n");
if(lbuf[MISC] & 0x80000000)
	fprintf(s,"   This Notice was received with 1 (or more) CRC errors -- values are suspect.\n");
}

/*---------------------------------------------------------------------------*/
pr_swift_xrt_lc(lbuf,s)/* print the contents of a Swift-XRT LightCurve packet */
long		*lbuf;	/* Ptr to the newly arrived packet to print out */
FILE		*s;		/* Stream to print it to */
{
int   id, segnum;
float delta;

fprintf(s,"PKT INFO:    Received: LT %s",ctime((time_t *)&tloc));
fprintf(s,"   Type= %d   SN= %d    Swift-XRT LightCurve\n",lbuf[PKT_TYPE],lbuf[PKT_SERNUM]);
fprintf(s,"   Hop_cnt=    %d\n", lbuf[PKT_HOP_CNT]);
fprintf(s,"   PKT_SOD=    %.2f [sec]   delta=%.2f [sec]\n",
					lbuf[PKT_SOD]/100.0, here_sod - lbuf[PKT_SOD]/100.0);
id =     (lbuf[BURST_TRIG] >> S_TRIGNUM_SHIFT) & S_TRIGNUM_MASK;
segnum = (lbuf[BURST_TRIG] >> S_SEGNUM_SHIFT) & S_SEGNUM_MASK;
fprintf(s,"   Trig#=      %d,  Segnum= %d\n",id,segnum);
fprintf(s,"   Bore_RA=    %9.4f [deg] (J2000)\n", lbuf[BURST_RA]/10000.0);
fprintf(s,"   Bore_Dec=   %9.4f [deg] (J2000)\n", lbuf[BURST_DEC]/10000.0);
fprintf(s,"   Start_TJD=  %d\n", lbuf[BURST_TJD]);
fprintf(s,"   Start_SOD=  %.2f [sec]\n",lbuf[BURST_SOD]/100.0);
fprintf(s,"   Stop_TJD=   %d\n", lbuf[10]);
fprintf(s,"   Stop_SOD=   %.2f [sec]\n", lbuf[11]/100.0);
delta = lbuf[BURST_SOD]/100.0 - lbuf[11]/100.0;
if(delta < 0.0) delta += 86400.0;  // Handle UT_midnight rollover
fprintf(s,"   Stop_Start= %.2f [sec]\n", delta);
fprintf(s,"   N_Bins=     %d\n", lbuf[20]);
fprintf(s,"   Term_cond=  %d\n", lbuf[21]);
fprintf(s,"            =  0 = Normal (the full 100 bins).\n");
fprintf(s,"            =  1 = Terminated by time (1-99 bins).\n");
fprintf(s,"            =  2 = Terminated by Snapshot (1-99 bins).\n");
fprintf(s,"            =  3 = Terminated by entering the SAA (1-99 bins).\n");
fprintf(s,"   URL=        %s\n", (char *)(&lbuf[BURST_URL]));
if(id < 100000)
	{
	fprintf(s,"   This Notice is generated as a result of an uploaded TOO -- this is not a normal BAT-detected burst.\n");
	fprintf(s,"   And the LC_START/STOP_DATE/TIME fields are the current epoch -- not the original detection.\n");
	}
if((id > 100000) && (segnum > 0))
	{
	fprintf(s,"   This Notice is generated as a result of an uploaded TOO on an old GRB -- this is not a newly detected burst.\n");
	fprintf(s,"   And the LC_START/STOP_DATE/TIME fields are the current epoch -- not the original detection.\n");
	}
if(lbuf[21] == 0)
	fprintf(s,"   This Lightcurve was terminated normally.\n");
else if(lbuf[21] == 1)
	fprintf(s,"   This Lightcurve was terminated by the 'Time' condition.\n");
else if(lbuf[21] == 2)
	fprintf(s,"   This Lightcurve was terminated by the 'Snapshot End' condition.\n");
else if(lbuf[21] == 3)
	fprintf(s,"   This Lightcurve was terminated by the 'SAA Entry' condition.\n");
else if(lbuf[21] == 4)
	fprintf(s,"   This Lightcurve accumulation was terminated by the 'LRPD-to-WT Transition' condition.\n");
else if(lbuf[21] == 5)
	fprintf(s,"   This Lightcurve accumulation was terminated by the 'WT-to-LRorPC Transition' condition.\n");
else
	fprintf(s,"   WARNING: This Lightcurve has an unrecognized termination condition.\n");
if(lbuf[MISC] & 0x80000000)
	fprintf(s,"   This Notice was received with 1 (or more) CRC errors -- values are suspect.\n");
}

/*---------------------------------------------------------------------------*/
pr_swift_uvot_image(lbuf,s)/* print the contents of a Swift-UVOT Image packet */
long		*lbuf;	/* Ptr to the newly arrived packet to print out */
FILE		*s;		/* Stream to print it to */
{
int	id, segnum;

fprintf(s,"PKT INFO:    Received: LT %s",ctime((time_t *)&tloc));
fprintf(s,"   Type= %d   SN= %d    Swift-UVOT Image\n",lbuf[PKT_TYPE],lbuf[PKT_SERNUM]);
fprintf(s,"   Hop_cnt=  %d\n", lbuf[PKT_HOP_CNT]);
fprintf(s,"   PKT_SOD=  %.2f [sec]   delta=%.2f [sec]\n",
					lbuf[PKT_SOD]/100.0, here_sod - lbuf[PKT_SOD]/100.0);
id =     (lbuf[BURST_TRIG] >> S_TRIGNUM_SHIFT) & S_TRIGNUM_MASK;
segnum = (lbuf[BURST_TRIG] >> S_SEGNUM_SHIFT) & S_SEGNUM_MASK;
fprintf(s,"   Trig#=    %d,  Segnum= %d\n",id,segnum);
fprintf(s,"   Start_TJD=%d\n", lbuf[BURST_TJD]);
fprintf(s,"   Start_SOD=%.2f [sec]   delta=%.2f [sec]\n",
					lbuf[BURST_SOD]/100.0, here_sod - lbuf[BURST_SOD]/100.0);
fprintf(s,"   RA=       %9.4f [deg] (J2000)\n", lbuf[BURST_RA]/10000.0);
fprintf(s,"   Dec=      %9.4f [deg] (J2000)\n", lbuf[BURST_DEC]/10000.0);
fprintf(s,"   Roll=     %.4f [deg]\n", lbuf[9]/10000.0);
fprintf(s,"   filter=   %d\n", lbuf[10]);
fprintf(s,"   expo_id=  %d\n", lbuf[11]);
fprintf(s,"   x_offset= %d\n", lbuf[12]);
fprintf(s,"   y_offset= %d\n", lbuf[13]);
fprintf(s,"   width=    %d\n", lbuf[14]);
fprintf(s,"   height=   %d\n", lbuf[15]);
fprintf(s,"   x_grb_pos=%d\n", lbuf[16] & 0xFFFF);
fprintf(s,"   y_grb_pos=%d\n", (lbuf[16] >> 16) & 0xFFFF);
fprintf(s,"   n_frames= %d\n", lbuf[17]);
fprintf(s,"   biining=  %d [index]\n", lbuf[MISC] & 0xF);
fprintf(s,"   swl=      %d\n", lbuf[18] & 0x000F);
fprintf(s,"   lwl=      %d\n", (lbuf[18] >> 16) & 0x000F);
fprintf(s,"   URL=      %s\n", (char *)(&lbuf[BURST_URL]));
if(id < 100000)
	{
	fprintf(s,"   This Notice is generated as a result of an uploaded TOO -- this is not a normal BAT-detected burst.\n");
	fprintf(s,"   And the IMG_START/STOP_DATE/TIME fields are the current epoch -- not the original detection.\n");
	}
if((id > 100000) && (segnum > 0))
	{
	fprintf(s,"   This Notice is generated as a result of an uploaded TOO on an old GRB -- this is not a newly detected burst.\n");
	fprintf(s,"   And the IMG_START/STOP_DATE/TIME fields are the current epoch -- not the original detection.\n");
	}
if(lbuf[MISC] & 0x10000000)
	fprintf(s,"   The source of the GRB Position came from the XRT Position command.\n");
else
	fprintf(s,"   The source of the GRB Position came from the Window Position in the Mode command.\n");
if((lbuf[MISC] & 0x0000000F) == 0)
	fprintf(s,"   The image has 1x1 binning (ie no compression).\n");
else if((lbuf[MISC] & 0x0000000F) == 1)
	fprintf(s,"   The image has 2x2 binning (compression).\n");
else if((lbuf[MISC] & 0x0000000F) == 2)
	fprintf(s,"   The image has 4x4 binning (compression).\n");
else if((lbuf[MISC] & 0x0000000F) == 3)
	fprintf(s,"   The image has 8x8 binning (compression).\n");
else if((lbuf[MISC] & 0x0000000F) == 4)
	fprintf(s,"   The image has 16x16 binning (compression).\n");
else if((lbuf[MISC] & 0x0000000F) == 5)
	fprintf(s,"   The image has 32x32 binning (compression).\n");
else if((lbuf[MISC] & 0x0000000F) == 6)
	fprintf(s,"   The image has 64x64 binning (compression).\n");
if(lbuf[MISC] & 0x40000000)
	fprintf(s,"   This is a ground-generated Notice -- not flight-generated.\n");
if(lbuf[MISC] & 0x80000000)
	fprintf(s,"   This Notice was received with 1 (or more) CRC errors -- values are suspect.\n");
}

/*---------------------------------------------------------------------------*/
pr_swift_uvot_slist(lbuf,s)/* print the contents of a Swift-UVOT SrcList packet */
long		*lbuf;	/* Ptr to the newly arrived packet to print out */
FILE		*s;		/* Stream to print it to */
{
int	id, segnum;

fprintf(s,"PKT INFO:    Received: LT %s",ctime((time_t *)&tloc));
fprintf(s,"   Type= %d   SN= %d    Swift-UVOT SrcList\n",lbuf[PKT_TYPE],lbuf[PKT_SERNUM]);
fprintf(s,"   Hop_cnt=  %d\n", lbuf[PKT_HOP_CNT]);
fprintf(s,"   PKT_SOD=  %.2f [sec]   delta=%.2f [sec]\n",
					lbuf[PKT_SOD]/100.0, here_sod - lbuf[PKT_SOD]/100.0);
id =     (lbuf[BURST_TRIG] >> S_TRIGNUM_SHIFT) & S_TRIGNUM_MASK;
segnum = (lbuf[BURST_TRIG] >> S_SEGNUM_SHIFT) & S_SEGNUM_MASK;
fprintf(s,"   Trig#=    %d,  Segnum= %d\n",id,segnum);
fprintf(s,"   Start_TJD=%d\n", lbuf[BURST_TJD]);
fprintf(s,"   Start_SOD=%.2f [sec]   delta=%.2f [sec]\n",
					lbuf[BURST_SOD]/100.0, here_sod - lbuf[BURST_SOD]/100.0);
fprintf(s,"   RA=       %9.4f [deg] (J2000)\n", lbuf[BURST_RA]/10000.0);
fprintf(s,"   Dec=      %9.4f [deg] (J2000)\n", lbuf[BURST_DEC]/10000.0);
fprintf(s,"   Roll=     %7.4f [deg]\n", lbuf[9]/10000.0);
fprintf(s,"   Filter=   %d\n", lbuf[10]);
fprintf(s,"   bkg_mean= %d\n", lbuf[11]);
fprintf(s,"   x_max=    %d\n", lbuf[12]);
fprintf(s,"   y_max=    %d\n", lbuf[13]);
fprintf(s,"   n_star=   %d\n", lbuf[14]);
fprintf(s,"   x_offset= %d\n", lbuf[15]);
fprintf(s,"   y_offset= %d\n", lbuf[16]);
fprintf(s,"   det_thresh=   %d\n", lbuf[17]);
fprintf(s,"   photo_thresh= %d\n", lbuf[18]);
fprintf(s,"   URL=      %s\n", (char *)(&lbuf[BURST_URL]));
if(id < 100000)
	{
	fprintf(s,"   This Notice is generated as a result of an uploaded TOO -- this is not a normal BAT-detected burst.\n");
	fprintf(s,"   And the IMG_START/STOP_DATE/TIME fields are the current epoch -- not the original detection.\n");
	}
if((id > 100000) && (segnum > 0))
	{
	fprintf(s,"   This Notice is generated as a result of an uploaded TOO on an old GRB -- this is not a newly detected burst.\n");
	fprintf(s,"   And the IMG_START/STOP_DATE/TIME fields are the current epoch -- not the original detection.\n");
	}
if(lbuf[MISC] & 0x40000000)
	fprintf(s,"   This is a ground-generated Notice -- not flight-generated.\n");
if(lbuf[MISC] & 0x80000000)
	fprintf(s,"   This Notice was received with 1 (or more) CRC errors -- values are suspect.\n");
}

/*---------------------------------------------------------------------------*/
pr_swift_uvot_pos(lbuf,s)   /* print the contents of a Swift-UVOT Pos packet */
long		*lbuf;	/* Ptr to the newly arrived packet to print out */
FILE		*s;		/* Stream to print it to */
{
int	id, segnum;

fprintf(s,"PKT INFO:    Received: LT %s",ctime((time_t *)&tloc));
fprintf(s,"   Type= %d   SN= %d    Swift-UVOT Pos_Ack\n",lbuf[PKT_TYPE],lbuf[PKT_SERNUM]);
fprintf(s,"   Hop_cnt= %d\n", lbuf[PKT_HOP_CNT]);
fprintf(s,"   PKT_SOD= %.2f [sec]   delta=%.2f [sec]\n",
					lbuf[PKT_SOD]/100.0, here_sod - lbuf[PKT_SOD]/100.0);
id =     (lbuf[BURST_TRIG] >> S_TRIGNUM_SHIFT) & S_TRIGNUM_MASK;
segnum = (lbuf[BURST_TRIG] >> S_SEGNUM_SHIFT) & S_SEGNUM_MASK;
fprintf(s,"   Trig#=   %d,  Segnum= %d\n",id,segnum);
fprintf(s,"   TJD=     %d\n", lbuf[BURST_TJD]);
fprintf(s,"   SOD=     %.2f [sec]   delta=%.2f [sec]\n",
					lbuf[BURST_SOD]/100.0, here_sod - lbuf[BURST_SOD]/100.0);
fprintf(s,"   RA=      %9.4f [deg] (J2000)\n", lbuf[BURST_RA]/10000.0);
fprintf(s,"   Dec=     %9.4f [deg] (J2000)\n", lbuf[BURST_DEC]/10000.0);
fprintf(s,"   Error=   %.1f [arcsec, radius, statistical only]\n",
											(float)3600.0*lbuf[BURST_ERROR]/10000.0);
fprintf(s,"   Mag=     %.2f\n", lbuf[BURST_INTEN]/100.0);  //confirm this!!!!!!!!!!!!!!!
if(lbuf[MISC] & 0x40000000)
	fprintf(s,"   This is a ground-generated Notice -- not flight-generated.\n");
else
	fprintf(s,"   ERR: Bit not set -- this UVOT_Pos Notice can only ever be generated on the ground.\n");
if(lbuf[TRIGGER_ID] & 0x00000020)
	fprintf(s,"   This is a RETRACTION of a previous notice that identified this as a burst afterglow -- it is NOT a GRB.\n");
if(lbuf[MISC] & 0x80000000)
	fprintf(s,"   This Notice was received with 1 (or more) CRC errors -- values are suspect.\n");
}

/*---------------------------------------------------------------------------*/
pr_swift_point(lbuf,s)      /* print the contents of a SWIFT POINTDIR packet */
long		*lbuf;	// Ptr to the newly arrived packet to print out
FILE		*s;		// Stream to print it to
{
fprintf(s,"PKT INFO:    Received: LT %s",ctime((time_t *)&tloc));
fprintf(s,"   Type= %d   SN= %d,  Pointing Direction\n",
					lbuf[PKT_TYPE], lbuf[PKT_SERNUM]);
fprintf(s,"   Hop_cnt= %d\n", lbuf[PKT_HOP_CNT]);
fprintf(s,"   PKT_SOD=  %.2f [sec]   delta=%5.2f\n",
					lbuf[PKT_SOD]/100.0, here_sod - lbuf[PKT_SOD]/100.0);
fprintf(s,"   Next_Point_RA=   %8.4fd (J2000)\n", lbuf[7]/10000.0);
fprintf(s,"   Next_Point_Dec=  %+8.4fd (J2000)\n",lbuf[8]/10000.0);
fprintf(s,"   Next_Point_Roll= %+8.4fd (J2000)\n",lbuf[9]/10000.0);
fprintf(s,"   Slew_Time=       %.2f SOD UT\n", lbuf[6]/100.0);
fprintf(s,"   Slew_Date=       %5d TJD\n", lbuf[5]);
fprintf(s,"   TgtName=         %s\n", (char *)(&lbuf[BURST_URL]));
fprintf(s,"   ObsTime=         %.2f [sec]\n", lbuf[14]/100.0);
fprintf(s,"   Merit=           %.2f\n", lbuf[15]/100.0);
fprintf(s,"   S/C_Lon,Lat=     %.2f, %.2f [deg] (at the PKT_SOD time)\n",
									(lbuf[16]&0xFFFF)/100.0, (lbuf[16]>>16)/100.0);

if(lbuf[MISC] & 0x40000000)
	fprintf(s,"   This is a ground-generated Notice -- not flight-generated.\n");
}

/*---------------------------------------------------------------------------*/
pr_agile_pos_ack(lbuf,s)/* print the contents of a SuperAGILE Pos_Ack packet */
long		*lbuf;	// Ptr to the newly arrived packet to print out
FILE		*s;		// Stream to print it to
{
fprintf(s,"PKT INFO:    Received: LT %s",ctime((time_t *)&tloc));
if(lbuf[PKT_TYPE] == TYPE_AGILE_GRB_WAKEUP)
fprintf(s,"   Type= %d   SN= %d    SuperAGILE GRB Wakeup Position\n",lbuf[PKT_TYPE],lbuf[PKT_SERNUM]);
else if(lbuf[PKT_TYPE] == TYPE_AGILE_GRB_PROMPT)
fprintf(s,"   Type= %d   SN= %d    SuperAGILE GRB Ground Position\n",lbuf[PKT_TYPE],lbuf[PKT_SERNUM]);
else if(lbuf[PKT_TYPE] == TYPE_AGILE_GRB_REFINED)
fprintf(s,"   Type= %d   SN= %d    SuperAGILE GRB Refined Position\n",lbuf[PKT_TYPE],lbuf[PKT_SERNUM]);
else if(lbuf[PKT_TYPE] == TYPE_AGILE_TRANS)
fprintf(s,"   Type= %d   SN= %d    SuperAGILE Transient Position\n",lbuf[PKT_TYPE],lbuf[PKT_SERNUM]);
else if(lbuf[PKT_TYPE] == TYPE_AGILE_GRB_POS_TEST)
fprintf(s,"   Type= %d   SN= %d    SuperAGILE Test Position\n",lbuf[PKT_TYPE],lbuf[PKT_SERNUM]);
else
fprintf(s,"   Type= %d   SN= %d    Illegal type!\n",lbuf[PKT_TYPE],lbuf[PKT_SERNUM]);
fprintf(s,"   Hop_cnt= %d\n", lbuf[PKT_HOP_CNT]);
fprintf(s,"   PKT_SOD= %.2f [sec]   delta=%.2f [sec]\n",
						lbuf[PKT_SOD]/100.0, here_sod - lbuf[PKT_SOD]/100.0);
fprintf(s,"   Trig#=   %d\n",lbuf[BURST_TRIG]);
fprintf(s,"   TJD=     %d\n", lbuf[BURST_TJD]);
fprintf(s,"   SOD=     %.2f [sec]   delta=%.2f [sec]\n",
						lbuf[BURST_SOD]/100.0, here_sod - lbuf[BURST_SOD]/100.0);
fprintf(s,"   RA=      %9.4f [deg] (J2000)\n", lbuf[BURST_RA]/10000.0);
fprintf(s,"   Dec=     %9.4f [deg] (J2000)\n", lbuf[BURST_DEC]/10000.0);
fprintf(s,"   Error=   %.1f [arcmin, radius, statistical plus systematic]\n",
											(float)60.0*lbuf[BURST_ERROR]/10000.0);
fprintf(s,"   INTEN=   %.3f   %.3f [X & Y cnts]\n", lbuf[BURST_INTEN]/1000.0, lbuf[BURST_PEAK]/1000.0);
fprintf(s,"   SIGNIF=  %.2f  %.2f [sigma]\n",(lbuf[20] & 0x0000FFFF)/100.0,((lbuf[20] & 0xFFFF0000)>>16)/100.0);

if(lbuf[TRIGGER_ID] & 0x00000002)
  fprintf(s,"   This is a GRB.\n");
if(lbuf[TRIGGER_ID] & 0x00002000)
  fprintf(s,"   There is a bright star near this position.\n");
if(lbuf[MISC] & 0x00004000)
  {
  fprintf(s,"   The SuperAGILE position is inside an NGC galaxy.\n");
  fprintf(s,"   There is a 0.05%% probability that this would happen by chance coincidence.\n");
  }
if(lbuf[MISC] & 0x00008000)
  {
  fprintf(s,"   An NGC galaxy is inside the SuperAGILE position error circle.\n");
  fprintf(s,"   There is a 2e5%% probability that an NGC galaxy would happen by chance coincidence.\n");
  }
if(lbuf[MISC] & 0x40000000)
	fprintf(s,"   This is a ground-generated Notice -- not flight-generated.\n");
}

/*---------------------------------------------------------------------------*/
pr_agile_point(lbuf,s)      /* print the contents of a AGILE POINTDIR packet */
long		*lbuf;	// Ptr to the newly arrived packet to print out
FILE		*s;		// Stream to print it to
{
fprintf(s,"PKT INFO:    Received: LT %s",ctime((time_t *)&tloc));
fprintf(s,"   Type= %d   SN= %d,  AGILE Pointing Direction\n",
					lbuf[PKT_TYPE], lbuf[PKT_SERNUM]);
fprintf(s,"   Hop_cnt=         %d\n", lbuf[PKT_HOP_CNT]);
fprintf(s,"   PKT_SOD=         %.2f [sec]   delta=%5.2f\n",
						lbuf[PKT_SOD]/100.0, here_sod - lbuf[PKT_SOD]/100.0);
fprintf(s,"   CURR_POINT_RA=   %7.2fd (J2000),\n", lbuf[7]/10000.0);
fprintf(s,"   CURR_POINT_DEC=  %+7.2fd  (J2000),\n",lbuf[8]/10000.0);
fprintf(s,"   START_DATE=      %5d TJD\n", lbuf[5]);
fprintf(s,"   START_TIME=      %.2f SOD UT\n", lbuf[6]/100.0);
fprintf(s,"   END_DATE=        %5d TJD\n", lbuf[12]);
fprintf(s,"   END_TIME=        %.2f SOD UT\n", lbuf[13]/100.0);

if(lbuf[MISC] & 0x40000000)
	fprintf(s,"   This is a ground-generated Notice -- not flight-generated.\n");
}

/*---------------------------------------------------------------------------*/
pr_fermi_gbm_alert(lbuf,s)    /* print the contents of a FERMI-GBM Alert pkt */
long		*lbuf;	// Ptr to the newly arrived packet to print out
FILE		*s;		// Stream to print it to
{
int i;

fprintf(s,"PKT INFO:    Received: LT %s",ctime((time_t *)&tloc));
fprintf(s,"   Type= %d   SN= %d    FERMI-GBM Alert\n",lbuf[PKT_TYPE],lbuf[PKT_SERNUM]);
fprintf(s,"   Hop_cnt=       %d\n", lbuf[PKT_HOP_CNT]);
fprintf(s,"   PKT_SOD=       %.2f [sec]   delta=%.2f [sec]\n",
					lbuf[PKT_SOD]/100.0, here_sod - lbuf[PKT_SOD]/100.0);

fprintf(s,"   RECORD_NUM=    %d\n",lbuf[20]);
fprintf(s,"   TRIGGER_NUM=   %d\n",lbuf[BURST_TRIG]);
fprintf(s,"   TJD=           %d\n", lbuf[BURST_TJD]);
fprintf(s,"   SOD=           %.2f [sec]   delta=%.2f [sec]\n",
						lbuf[BURST_SOD]/100.0, here_sod - lbuf[BURST_SOD]/100.0);
fprintf(s,"   TRIGGER_DUR=   %.3f [sec]\n",lbuf[14]/1000.0);
 {
int eli, eui;
// According to the GBM Team, there is some chance that these values will change over time.
char *elnai[9] = { "4","11","23","47", "98","291","585",">774",    "illegal value"};
char *eunai[9] = {"11","23","47","98","291","585","774","infinity","illegal value"};
char *elbgo[9] = {"129","383", "831","1602","2912","5104", "9185",">16179",  "illegal value"};
char *eubgo[9] = {"383","831","1602","2912","5104","9185","16179","infinity","illegal value"};
eli = lbuf[15];
eui = lbuf[16];             
if(eli<0) eli=8;            
if(eli>7) eli=8;
if(eui<0) eui=8;            
if(eui>7) eui=8;            
fprintf(s,"   E_RANGE=       %d-%d [chan]   %s-%s [keV]\n",lbuf[15],lbuf[16],elnai[eli],eunai[eui]);
 }                          
fprintf(s,"   ALGORITHM=     %d\n",lbuf[9]);
// There are four clusters of three detectors each, one 3 detector  
// cluster for each of the four corners of the spacecraft: 4 groups of 3.
fprintf(s,"   DETECTORS=     ");
for(i=0;i<14;i++) {if(((i%3)==0)&&(i!=0)) fprintf(s," ");  fprintf(s,"%1d,",((lbuf[26] & (0x1<<i)) ? 1:0) );}
fprintf(s,"\n");
fprintf(s,"   SC_LONG=       %.2f [deg]\n",lbuf[22]/60.0);
fprintf(s,"   SC_LAT=        %.2f [deg]\n",lbuf[23]/60.0);

if(lbuf[MISC] & 0x40000000)
	fprintf(s,"   This is a ground-generated Notice -- not flight-generated.\n");
}

/*---------------------------------------------------------------------------*/
pr_fermi_gbm_flt_pos(lbuf,s)/* print the contents of a FERMI-GBM Flt Pos pkt */
long		*lbuf;	// Ptr to the newly arrived packet to print out
FILE		*s;		// Stream to print it to
{
int i;
int indx;
static char     *gbm_most_likely[14] = {"Error", "Unreliable location", "Local Particles", "Below horizon",
                                        "GRB", "Generic SGR", "Generic Transient", "Distant Particles",
                                        "Solar Flare", "Cyg X-1", "SGR_1806n20", "GRO_J0422_32", "unrecognized value", "n/a"};
/*                                      DOCUMENTATION:
                                        //  Classes 0 to 3 are "single classes": the probabilities of these
                                        //  will be either 0 or 1, never an in between value.
                                        0  ERROR                - ????????????precise wording needed!!!!!!
                                        1  UNRELIABLE_LOCATION  - ????????????precise wording needed!!!!!!
                                        2  LOCAL_PARTICLES      - Local particles, equal rates in opposite detectors
                                        3  BELOW_HORIZON        - Distant particles, assumed to come from the horizon
                                        //  The remaining classes have probabilities calculated
                                        //  from Bayes Theorem: any value from 0.0 to 1.0 is possible.
                                        4  GRB                  - For bursts with good localizations (the desired case)
                                        5  GENERIC_SGR          - This is any SGR (except 1806-20)
                                        6  GENERIC_TRANSIENT    - Any astrophysical transient not included elsewhere
                                        7  DISTANT_PARTICLES    - ????????????precise wording needed!!!!!!
                                        8  SOLAR_FLARE          - This is a Solar Flare event
                                        9  CYG_X1               - This trigger was caused by a CygX1 fluctuation
                                        10 SGR_1806_20          - This trigger came from SGR 1806-20
                                        11 GROJ_0422_32         - This trigger came from GRO J0422-32
                                        */


fprintf(s,"PKT INFO:    Received: LT %s",ctime((time_t *)&tloc));
if(lbuf[PKT_TYPE] == TYPE_FERMI_GBM_FLT_POS)
fprintf(s,"   Type= %d   SN= %d    FERMI-GBM Flt Position\n",lbuf[PKT_TYPE],lbuf[PKT_SERNUM]);
else if(lbuf[PKT_TYPE] == TYPE_FERMI_GBM_TRANS)
fprintf(s,"   Type= %d   SN= %d    FERMI-GBM Transient Position\n",lbuf[PKT_TYPE],lbuf[PKT_SERNUM]);
else if(lbuf[PKT_TYPE] == TYPE_FERMI_GBM_POS_TEST)
fprintf(s,"   Type= %d   SN= %d    FERMI-GBM Test Position\n",lbuf[PKT_TYPE],lbuf[PKT_SERNUM]);
else
fprintf(s,"   Type= %d   SN= %d    Illegal type!\n",lbuf[PKT_TYPE],lbuf[PKT_SERNUM]);
fprintf(s,"   Hop_cnt= %d\n", lbuf[PKT_HOP_CNT]);
fprintf(s,"   PKT_SOD= %.2f [sec]   delta=%.2f [sec]\n",
								lbuf[PKT_SOD]/100.0, here_sod - lbuf[PKT_SOD]/100.0);
fprintf(s,"   RECORD_NUM=      %d\n",lbuf[20]);
fprintf(s,"   TRIGGER_NUM=     %d\n",lbuf[BURST_TRIG]);
fprintf(s,"   TJD=             %d\n", lbuf[BURST_TJD]);
fprintf(s,"   SOD=             %.2f [sec]   delta=%.2f [sec]\n",
								lbuf[BURST_SOD]/100.0, here_sod - lbuf[BURST_SOD]/100.0);
fprintf(s,"   RA=              %9.4f [deg] (J2000)\n", lbuf[BURST_RA]/10000.0);
fprintf(s,"   Dec=             %9.4f [deg] (J2000)\n", lbuf[BURST_DEC]/10000.0);
fprintf(s,"   ERROR=           %.2f [deg radius, statistical only]\n",(float)lbuf[BURST_ERROR]/10000.0);
fprintf(s,"   INTEN=           %d [cnts/sec]\n", lbuf[BURST_INTEN]);
fprintf(s,"   DATA_SIGNIF=     %.2f [sigma]\n",lbuf[21]/100.0);
fprintf(s,"   INTEG_TIME=      %.3f [sec]\n",lbuf[14]/1000.0); // [msec] --> [sec]
fprintf(s,"   PHI=             %6.2f [deg]\n", lbuf[12]/100.0);
fprintf(s,"   THETA=           %6.2f [deg]\n", lbuf[13]/100.0);
fprintf(s,"   DATA_TIME_SCALE= %.4f [sec]\n",lbuf[17]/1000.0);
fprintf(s,"   HARD_RATIO=      %.2f\n",lbuf[25]/100.0);
fprintf(s,"   LOC_ALGORITHM=   %d (version number of)\n",lbuf[22]);
indx = lbuf[23]&0xFFFF;
if((indx < 0) || (indx > 11)) indx = 12;
fprintf(s,"   MOST_LIKELY=     %3.0f%%  %s\n",100.0*((lbuf[23]>>16)&0xFFFF)/255.0,gbm_most_likely[indx]);
indx = lbuf[24]&0xFFFF;
if((indx == 0) && (((lbuf[23]>>16)&0xFFFF) == 255))  indx = 13;
else if((indx < 1) || (indx > 11)) indx = 12;
if((((lbuf[23]>>16)&0xFFFF))==0)        // If MostLikely is 0, then 2ndMost is "n/a"
fprintf(s,"   2nd_MOST_LIKELY= %3d%%  %s\n",0,gbm_most_likely[13]);
else
fprintf(s,"   2nd_MOST_LIKELY= %3.0f%%  %s\n",100.0*((lbuf[24]>>16)&0xFFFF)/255.0,gbm_most_likely[indx]);
if(lbuf[MISC] & (0x1<<24))      // If the Dets flag bits have been carried-over, then print them out
 {
fprintf(s,"   DETECTORS=       ");
for(i=0;i<14;i++) {if(((i%3)==0)&&(i!=0)) fprintf(s," ");  fprintf(s,"%1d,",((lbuf[26] & (0x1<<i)) ? 1:0) );}
fprintf(s,"\n");
fprintf(s,"   SC_LONG=         %.2f [deg]\n",lbuf[37]/60.0);
fprintf(s,"   SC_LAT=          %.2f [deg]\n",lbuf[38]/60.0);
 }
if(lbuf[TRIGGER_ID] & 0x00000020)
	fprintf(s,"   This is a RETRACTION of a previous notice that identified this as a GRB -- it is NOT a GRB.\n");
if(lbuf[MISC] & (0x1 << 24))
     fprintf(s,"The Dets, Lon, and Lat fields have been filled in (carried over from the Alert).\n");
if(lbuf[MISC] & 0x00000800)
     fprintf(s,"WARNING: This Notice was received with 1 (or more) critical values out of range.\n");
if(lbuf[MISC] & 0x40000000)		// Almost never happen -- a person would have to gen an "import" Flt.
	fprintf(s,"   This is a ground-generated Notice -- not flight-generated.\n");
}

/*---------------------------------------------------------------------------*/
pr_fermi_gbm_gnd_pos(lbuf,s)/* print the contents of a FERMI-GBM Gnd Pos pkt */
long		*lbuf;	// Ptr to the newly arrived packet to print out
FILE		*s;		// Stream to print it to
{
fprintf(s,"PKT INFO:    Received: LT %s",ctime((time_t *)&tloc));
if(lbuf[PKT_TYPE] == TYPE_FERMI_GBM_GND_POS)
fprintf(s,"   Type= %d   SN= %d    FERMI-GBM Gnd Position\n",lbuf[PKT_TYPE],lbuf[PKT_SERNUM]);
else
fprintf(s,"   Type= %d   SN= %d    Illegal type!\n",lbuf[PKT_TYPE],lbuf[PKT_SERNUM]);
fprintf(s,"   Hop_cnt= %d\n", lbuf[PKT_HOP_CNT]);
fprintf(s,"   PKT_SOD= %.2f [sec]   delta=%.2f [sec]\n",
								lbuf[PKT_SOD]/100.0, here_sod - lbuf[PKT_SOD]/100.0);
fprintf(s,"   RECORD_NUM=      %d\n",lbuf[20]);
fprintf(s,"   TRIGGER_NUM=     %d\n",lbuf[BURST_TRIG]);
fprintf(s,"   TJD=             %d\n", lbuf[BURST_TJD]);
fprintf(s,"   SOD=             %.2f [sec]   delta=%.2f [sec]\n",
								lbuf[BURST_SOD]/100.0, here_sod - lbuf[BURST_SOD]/100.0);
fprintf(s,"   RA=              %9.4f [deg] (J2000)\n", lbuf[BURST_RA]/10000.0);
fprintf(s,"   Dec=             %9.4f [deg] (J2000)\n", lbuf[BURST_DEC]/10000.0);
fprintf(s,"   ERROR=           %.2f [deg radius, statistical only]\n",
                                            (float)lbuf[BURST_ERROR]/10000.0);
fprintf(s,"   DATA_SIGNIF=     %.2f [sigma]\n",lbuf[21]/10.0);
fprintf(s,"   DATA_INTERVAL=   %.3f [sec]\n",lbuf[14]/1000.0);   // [msec] --> [sec]
fprintf(s,"   PHI=             %6.2f [deg]\n", lbuf[12]/100.0);
fprintf(s,"   THETA=           %6.2f [deg]\n", lbuf[13]/100.0);
fprintf(s,"   E_RANGE=         %.3f - %.3f [keV]\n",lbuf[26]/1000.0,lbuf[27]/1000.0);
fprintf(s,"   LOC_ALGORITHM=   %d (Gnd S/W Version number)\n",lbuf[22]);

if(lbuf[TRIGGER_ID] & 0x00000020)
	fprintf(s,"   This is a RETRACTION of a previous notice that identified this as a GRB -- it is NOT a GRB.\n");
if(lbuf[MISC] & 0x40000000)
	fprintf(s,"   This is a ground-generated Notice -- not flight-generated.\n");
}

/*---------------------------------------------------------------------------*/
pr_fermi_lat_pos(lbuf,s)     /* print the contents of a FERMI-LAT Pos packet */
long		*lbuf;	// Ptr to the newly arrived packet to print out
FILE		*s;		// Stream to print it to
{
fprintf(s,"PKT INFO:    Received: LT %s",ctime((time_t *)&tloc));
if(lbuf[PKT_TYPE] == TYPE_FERMI_LAT_POS_INI)
fprintf(s,"   Type= %d   SN= %d    FERMI-LAT Initial Position\n",lbuf[PKT_TYPE],lbuf[PKT_SERNUM]);
else if(lbuf[PKT_TYPE] == TYPE_FERMI_LAT_POS_UPD)
fprintf(s,"   Type= %d   SN= %d    FERMI-LAT Updated Position\n",lbuf[PKT_TYPE],lbuf[PKT_SERNUM]);
else if(lbuf[PKT_TYPE] == TYPE_FERMI_LAT_POS_DIAG)
fprintf(s,"   Type= %d   SN= %d    FERMI-LAT Diagnostic Position\n",lbuf[PKT_TYPE],lbuf[PKT_SERNUM]);
else if(lbuf[PKT_TYPE] == TYPE_FERMI_LAT_TRANS)
fprintf(s,"   Type= %d   SN= %d    FERMI-LAT Transient Position\n",lbuf[PKT_TYPE],lbuf[PKT_SERNUM]);
else if(lbuf[PKT_TYPE] == TYPE_FERMI_LAT_POS_TEST)
fprintf(s,"   Type= %d   SN= %d    FERMI-LAT Test Position\n",lbuf[PKT_TYPE],lbuf[PKT_SERNUM]);
else if(lbuf[PKT_TYPE] == TYPE_FERMI_LAT_GND_REF)
fprintf(s,"   Type= %d   SN= %d    FERMI-LAT Ground Refined Position\n",lbuf[PKT_TYPE],lbuf[PKT_SERNUM]);
else if(lbuf[PKT_TYPE] == TYPE_FERMI_LAT_GND_TRIG)
fprintf(s,"   Type= %d   SN= %d    FERMI-LAT Ground Trigger Position\n",lbuf[PKT_TYPE],lbuf[PKT_SERNUM]);
else
fprintf(s,"   Type= %d   SN= %d    Illegal type!\n",lbuf[PKT_TYPE],lbuf[PKT_SERNUM]);
fprintf(s,"   Hop_cnt= %d\n", lbuf[PKT_HOP_CNT]);
fprintf(s,"   PKT_SOD= %.2f [sec]   delta=%.2f [sec]\n",
							lbuf[PKT_SOD]/100.0, here_sod - lbuf[PKT_SOD]/100.0);

//more stuff...............
}

/*---------------------------------------------------------------------------*/
pr_fermi_point(lbuf,s)      /* print the contents of a FERMI POINTDIR packet */
long		*lbuf;	// Ptr to the newly arrived packet to print out
FILE		*s;		// Stream to print it to
{
fprintf(s,"PKT INFO:    Received: LT %s",ctime((time_t *)&tloc));
fprintf(s,"   Type= %d   SN= %d,  FERMI Pointing Direction\n",
					lbuf[PKT_TYPE], lbuf[PKT_SERNUM]);
fprintf(s,"   Hop_cnt=         %d\n", lbuf[PKT_HOP_CNT]);
fprintf(s,"   PKT_SOD=         %.2f [sec]   delta=%5.2f\n",
						lbuf[PKT_SOD]/100.0, here_sod - lbuf[PKT_SOD]/100.0);

//more stuff...............
}


/*---------------------------------------------------------------------------*/
/* This technique of trying to "notice" that imalives have not arrived
 * for a long period of time works only some of the time.  It depends on what
 * caused the interruption.  Some forms of broken socket connections hang
 * in the "elseif" shutdown() system call, and so the program never actually
 * gets to the point of calling this routine to check the time and
 * send a message.
 */
chk_imalive(which,ctloc)
int			which;		// Is this an imalive timestamp or delta check call
time_t		ctloc;		// Imalive timestamp or current time [sec since epoch]
{
static time_t	last_ctloc;			// Timestamp of last imalive received
static int		imalive_state = 0;	// State of checking engine
char			buf[32];			// Tmp place for date string modifications
time_t			nowtime;			// The Sun machine time this instant
char			*ctime();			// Convert the sec-since-epoch to ASCII

switch(imalive_state)
	{
	case 0:
		// Do something ONLY when we get that first Imalive timestamp.
		if(which)					/* Ctloc is time of most recent imalive */
			{
			last_ctloc = ctloc;
			imalive_state = 1;		/* Go to testing deltas */
			}
		break;
	case 1:
		// Now that we have an Imalive timestamp, check it against the
		// current time, and update the timestamp when new ones come in.
		if(which)					/* Ctloc is time of most recent imalive */
			last_ctloc = ctloc;
		else if((ctloc - last_ctloc) > 600)
			{
			time(&nowtime);			/* Get system time for mesg timestamp */
			strcpy(buf,ctime((time_t *)&nowtime));
			buf[24] = '\0';			/* Overwrite the newline with a null */
			printf(    "ERR: %s ESDT: No Imalive packets for >600sec.\n",buf);
			fprintf(lg,"ERR: %s ESDT: No Imalive packets for >600sec.\n",buf);
			e_mail_alert(1);
			imalive_state = 2;
			}
		break;
	case 2:
		// Waiting for a resumption of imalives so we can go back to testing
		// and e-mailing if they should stop for a 2nd (n-th) time.
		if(which)					/* Ctloc is time of the resumed imalive */
			{
			last_ctloc = ctloc;
			time(&nowtime);			/* Get system time for mesg timestamp */
			strcpy(buf,ctime((time_t *)&nowtime));
			buf[24] = '\0';			/* Overwrite the newline with a null */
			printf(    "OK: %s ESDT: Imalive packets have resumed.\n",buf);
			fprintf(lg,"OK: %s ESDT: Imalive packets have resumed.\n",buf);
			e_mail_alert(0);
			imalive_state = 1;		/* Go back to testing */
			}
		break;
	default:
		time(&nowtime);				/* Get system time for mesg timestamp */
		strcpy(buf,ctime((time_t *)&nowtime));
		buf[24] = '\0';				/* Overwrite the newline with a null */
		printf(    "ERR: %s ESDT: Bad imalive_state(=%d)\n",imalive_state,buf);
		fprintf(lg,"ERR: %s ESDT: Bad imalive_state(=%d)\n",imalive_state,buf);
		imalive_state = 0;			/* Reset to the initial state */
		break;
	}
}

/*---------------------------------------------------------------------------*/
e_mail_alert(which)             /* send an E_MAIL ALERT if no pkts in 600sec */
int		which;				/* End or re-starting of imalives */
{
int		rtn;				/* Return value from the system() call */
FILE	*etmp;				/* The temp email scratch file stream pointer */
time_t	nowtime;			/* The Sun machine time this instant */
char	*ctime();			/* Convert the sec-since-epoch to ASCII */
char	buf[32];			/* Tmp place for date string modifications */
static char	cmd_str[256];	/* Place to build the system() cmd string */

time(&nowtime);				/* Get the system time for the notice timestamp */
strcpy(buf,ctime((time_t *)&nowtime));
buf[24] = '\0';				/* Overwrite the newline with a null */

if((etmp=fopen("e_me_tmp", "w")) == NULL)
	{
	printf(    "ERR: %s ESDT: Can't open scratch e_mail_alert file.\n",buf);
	fprintf(lg,"ERR: %s ESDT: Can't open scratch e_mail_alert file.\n",buf);
	return;
	}

if(which)
	fprintf(etmp,"TITLE:        socket_demo GCN NO PACKETS ALERT\n");
else
	fprintf(etmp,"TITLE:        socket_demo GCN PACKETS RESUMED\n");
fprintf(etmp,"NOTICE_DATE:  %s ESDT\n", buf);

if(fclose(etmp))			/* Close the email file so the filesys is updated */
	{
	printf(    "ERR: %s ESDT: Problem closing scratch email_me file.\n",buf);
	fprintf(lg,"ERR: %s ESDT: Problem closing scratch email_me file.\n",buf);
	}

cmd_str[0] = 0;				/* Clear string buffer before building new string */
if(which)
	strcat(cmd_str,"mail -s BACO_NO_PKT_ALERT ");
else
	strcat(cmd_str,"mail -s BACO_PKT_RESUMED ");
// NOTICE TO GCN SOCKET SITES:  Please don't forget to change this
// e-mail address to your address(es).
strcat(cmd_str,"scott@lheamail");
strcat(cmd_str," < ");
strcat(cmd_str,"e_me_tmp");
strcat(cmd_str," &");
printf(    "%s\n",cmd_str);
fprintf(lg,"%s\n",cmd_str);
rtn = system(cmd_str);
printf(    "%sSystem() rtn=%d.\n",rtn?"ERR: ":"",rtn);
fprintf(lg,"%sSystem() rtn=%d.\n",rtn?"ERR: ":"",rtn);
}

/*---------------------------------------------------------------------------*/
int
hete_same(hbuf,find_which)
long	*hbuf;				/* Ptr to 40-long packet buffer with HETE contents*/
int		find_which;			/* Which instrument box to find the center of */
{
double	cra[4], cdec[4];	/* Generic 4 corners locations */
int		same = 0;			/* Are the 4 corners the same? */

switch(find_which)
  {
  case FIND_SXC:
	cra[0]  = (double)hbuf[H_SXC_CNR1_RA]/10000.0;	/* All 4 in J2000 */
	cra[1]  = (double)hbuf[H_SXC_CNR2_RA]/10000.0;
	cra[2]  = (double)hbuf[H_SXC_CNR3_RA]/10000.0;
	cra[3]  = (double)hbuf[H_SXC_CNR4_RA]/10000.0;
	cdec[0] = (double)hbuf[H_SXC_CNR1_DEC]/10000.0;	/* All 4 in J2000 */
	cdec[1] = (double)hbuf[H_SXC_CNR2_DEC]/10000.0;
	cdec[2] = (double)hbuf[H_SXC_CNR3_DEC]/10000.0;
	cdec[3] = (double)hbuf[H_SXC_CNR4_DEC]/10000.0;
	break;

  case FIND_WXM:
	cra[0]  = (double)hbuf[H_WXM_CNR1_RA]/10000.0;	/* All 4 in J2000 */
	cra[1]  = (double)hbuf[H_WXM_CNR2_RA]/10000.0;
	cra[2]  = (double)hbuf[H_WXM_CNR3_RA]/10000.0;
	cra[3]  = (double)hbuf[H_WXM_CNR4_RA]/10000.0;
	cdec[0] = (double)hbuf[H_WXM_CNR1_DEC]/10000.0;	/* All 4 in J2000 */
	cdec[1] = (double)hbuf[H_WXM_CNR2_DEC]/10000.0;
	cdec[2] = (double)hbuf[H_WXM_CNR3_DEC]/10000.0;
	cdec[3] = (double)hbuf[H_WXM_CNR4_DEC]/10000.0;
	break;

  default:
	printf(    "ERR: hete_same(): Shouldn't be able to get here; find_which=%d\n",
																find_which);
	fprintf(lg,"ERR: hete_same(): Shouldn't be able to get here; find_which=%d\n",
																find_which);
	return(0);
	break;
  }
if( (cra[0] == cra[1]) && (cra[0] == cra[2]) && (cra[0] == cra[3]) &&
    (cdec[0] == cdec[1]) && (cdec[0] == cdec[2]) && (cdec[0] == cdec[3]))
	return(1);
else
	return(0);
}
