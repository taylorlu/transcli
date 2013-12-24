/* $Header: //depot/licensing/ddplus/ddp_enc/dev/20130325_Zeus_v1.3/ddp_enc/include/pro/enc_api_params.h#1 $ */

/***************************************************************************\
*
*	This program is protected under international and U.S. copyright laws
*	as an unpublished work.  Do not copy.
*
*	Copyright 2011 by Dolby Laboratories Inc. All rights reserved.
*
*	Module:		Dolby Digital Plus Encoder API Module
*
*	File:		enc_api_params.h
*
\***************************************************************************/

/**
 *  @file enc_api_params.h
 *
 *  @brief Professional Encoder API params header file.
 *
 * Part of the Professional Encoder API Module.
 */

#ifndef ENC_API_PARAMS_H
#define ENC_API_PARAMS_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/*! \brief Fixed encoder channel ordering for PCM buffer descriptors in ddpi_enc_process_ip structure */
typedef enum
{
	DDPI_ENC_CHAN_L=0,    
	DDPI_ENC_CHAN_R,
	DDPI_ENC_CHAN_C,
	DDPI_ENC_CHAN_LFE,
	DDPI_ENC_CHAN_LS,
	DDPI_ENC_CHAN_RS,
	DDPI_ENC_CHAN_EXT1,
	DDPI_ENC_CHAN_EXT2,
	DDPI_ENC_CHAN_DMIX1,
	DDPI_ENC_CHAN_DMIX2,
	DDPI_ENC_CHAN_DMIX3,
	DDPI_ENC_MAXCHANS               /*!< Maximum number of PCM input channels per indep pgm    */
} DDPI_ENC_CHANS;

/*! \brief External DRC data */
typedef struct ddpi_enc_external_drc_s
{
	int         compr;                          /*!< Compression word (sign extended in 32-bit word) */
	int         dynrng[6];                      /*!< Dynamic range words for each audio block (sign extended in 32-bit word)  */
} ddpi_enc_external_drc;

/*! \brief External ADDBSI data */
typedef struct ddpi_enc_external_addbsi_s
{
  int addbsil;								    /*!< byte-length of addbsi data - 1 */
  int bytecount;								/*!< size of addbsi array */
  unsigned char *p_addbsi;						/*!< pointer to addbsi data */							
} ddpi_enc_external_addbsi;

/*! \brief LICKEY data */
typedef struct ddpi_enc_external_lickey_s
{
	int enflicdatasize;									/*!< size of the key:no of chars */
	char *p_enflicdata;									/*!< pointer to the key */							
} ddpi_enc_external_lickey;

/*! \brief External AUXDATA data */
typedef struct ddpi_enc_external_auxdata_s
{
  int bitcount;									/*!< number of bits passed in bitdata array */
  int bytecount;								/*!< size of auxdata array */
  unsigned char *p_auxdata;						/*!< pointer to auxdata array */							
} ddpi_enc_external_auxdata;

/*! \brief Encoder parameter IDs, passed to #ddpi_enc_getparam or #ddpi_enc_setparam */

/* Core encoder parameters */
#define DDPI_ENC_PARAMID_ENCODERMODE       (1)  /*!< encoder mode: see DDPI_ENC_MODE              */
#define DDPI_ENC_PARAMID_DATARATE          (3)  /*!< datarate in kbps                             */
#define DDPI_ENC_PARAMID_ACMOD             (4)  /*!< audio coding mode: see DDPI_ENC_ACMOD        */
#define DDPI_ENC_PARAMID_LFE               (5)  /*!< if 1, low frequency effects channel enabled  */
#define DDPI_ENC_PARAMID_AUTOSETUP_PROF    (6)  /*!< content profile: see DDPI_ENC_AUTOSETUP_PROF */

/* Pre-processing filters */
#define DDPI_ENC_PARAMID_PH90_FILT         (7)  /*!< if 1, phase 90 filter enabled                */
#define DDPI_ENC_PARAMID_SURATTN           (8)  /*!< if 1, surround attenuation enabled           */
#define DDPI_ENC_PARAMID_DC_FILT           (10) /*!< if 1, DC blocking highpass filter enabled    */
#define DDPI_ENC_PARAMID_LFE_FILT          (11) /*!< if 1, LFE lowpass filter enabled             */

/* Dynamic range compression (DRC) profiles */
#define DDPI_ENC_PARAMID_DYNRNGPROF        (13) /*!< dynrng profile: see DDPI_ENC_DRCPROF        */
#define DDPI_ENC_PARAMID_DYNRNG2PROF       (14) /*!< dynrng2 profile: see DDPI_ENC_DRCPROF       */
#define DDPI_ENC_PARAMID_COMPRPROF         (15) /*!< compr profile: see DDPI_ENC_DRCPROF         */
#define DDPI_ENC_PARAMID_COMPR2PROF        (16) /*!< compr2 profile: see DDPI_ENC_DRCPROF        */
#define DDPI_ENC_PARAMID_GBLDRCPROF        (17) /*!< global DRC profile, is overriden by other DRC profiles: see DDPI_ENC_DRCPROF */

/* Bitstream information (BSI) parameters */
#define DDPI_ENC_PARAMID_DIALNORM          (18) /*!< dialog normalization word */
#define DDPI_ENC_PARAMID_DIALNORM2         (19) /*!< dialog normalization word for ch2 (1+1 mode) */
#define DDPI_ENC_PARAMID_DMIXMOD           (20) /*!< stereo downmix mode: see DDPI_ENC_DMIXMOD */
#define DDPI_ENC_PARAMID_CMIXLEV           (21) /*!< center downmix level: see DDPI_ENC_CMIXLEV */
#define DDPI_ENC_PARAMID_SURMIXLEV         (22) /*!< surround downmix level: see DDPI_ENC_SURMIXLEV */
#define DDPI_ENC_PARAMID_LTRTCMIXLEV       (23) /*!< Lt/Rt center downmix level: see DDPI_ENC_LTRTCMIXLEV */
#define DDPI_ENC_PARAMID_LTRTSURMIXLEV     (24) /*!< Lt/Rt surround downmix level: see DDPI_ENC_LTRTSURMIXLEV */
#define DDPI_ENC_PARAMID_LOROCMIXLEV       (25) /*!< Lo/Ro center downmix level: see DDPI_ENC_LOROCMIXLEV */
#define DDPI_ENC_PARAMID_LOROSURMIXLEV     (26) /*!< Lo/Ro surround downmix level: see DDPI_ENC_LOROSURMIXLEV */
#define DDPI_ENC_PARAMID_DSUREXMOD         (27) /*!< Surround EX mode flag: see DDPI_ENC_DSUREXMOD */
#define DDPI_ENC_PARAMID_DHEADPHONMOD      (28) /*!< Dolby Headphone encoded flag: see DDPI_ENC_DHEADPHONMOD */
#define DDPI_ENC_PARAMID_ADCONVTYP         (29) /*!< A/D converter type: see DDPI_ENC_ADCONVTYP */
#define DDPI_ENC_PARAMID_MIXLEVEL          (31) /*!< audio production mixing level with 80dB offset (e.g. 0=80dB, 20=100dB) */
#define DDPI_ENC_PARAMID_MIXLEVEL2         (32) /*!< same as DDPI_ENC_PARAMID_MIXLEVEL, for ch2 (1+1 mode) */
#define DDPI_ENC_PARAMID_COPYRIGHTB        (33) /*!< copyright bit: see DDPI_ENC_COPYRIGHTB */
#define DDPI_ENC_PARAMID_ORIGBS            (34) /*!< original bitstream flag: see DDPI_ENC_ORIGBS */
#define DDPI_ENC_PARAMID_BSMOD             (35) /*!< bitstream mode: see DDPI_ENC_BSMOD */
#define DDPI_ENC_PARAMID_ROOMTYP           (36) /*!< room type: see DDPI_ENC_ROOMTYP */
#define DDPI_ENC_PARAMID_ROOMTYP2          (37) /*!< same as DDPI_ENC_PARAMID_ROOMTYP, for ch2 (1+1 mode) */
#define DDPI_ENC_PARAMID_DSURMOD           (38) /*!< Dolby surround mode: see DDPI_ENC_DSURMOD */
#define DDPI_ENC_PARAMID_AUDPRODINFOFLAG   (39) /*!< if 1, send audio production info */
#define DDPI_ENC_PARAMID_AUDPRODINFO2FLAG  (40) /*!< if 1, send audio production info 2*/

/* Dolby Digital Plus specific */
#define DDPI_ENC_PARAMID_SUBSTREAMID       (42) /*!< substream identification */
#define DDPI_ENC_PARAMID_PGMSCL            (43) /*!< program scale factor: see DDPI_ENC_SCLFACTOR */
#define DDPI_ENC_PARAMID_EXTPGMSCL         (45) /*!< external program scale factor: see DDPI_ENC_SCLFACTOR */
#define DDPI_ENC_PARAMID_ENFENABL          (47) /*!< if 1, enable encinfo transmission */
#define DDPI_ENC_PARAMID_ENFUSERDATA       (48) /*!< 16-bit unsigned: encinfo user data */
#define DDPI_ENC_PARAMID_ENFDATE           (49) /*!< 32-bit unsigned: encinfo timestamp (UTC) */

/* Secondary audio */
#define DDPI_ENC_PARAMID_PANMEAN           (54) /*!< pan mean direction index: angle = index x 1.5 degrees */
#define DDPI_ENC_PARAMID_EXTPGMSCL_L       (55) /*!< left channel level scale factor: see DDPI_ENC_EXTPGMSCLFACTOR */
#define DDPI_ENC_PARAMID_EXTPGMSCL_C       (56) /*!< center channel level scale factor: see DDPI_ENC_EXTPGMSCLFACTOR */
#define DDPI_ENC_PARAMID_EXTPGMSCL_R       (57) /*!< right channel level scale factor: see DDPI_ENC_EXTPGMSCLFACTOR */
#define DDPI_ENC_PARAMID_EXTPGMSCL_LS      (58) /*!< left surround channel scale factor: see DDPI_ENC_EXTPGMSCLFACTOR */
#define DDPI_ENC_PARAMID_EXTPGMSCL_RS      (59) /*!< right surround channel scale factor: see DDPI_ENC_EXTPGMSCLFACTOR */
#define DDPI_ENC_PARAMID_EXTPGMSCL_LFE     (60) /*!< LFE channel scale factor: see DDPI_ENC_EXTPGMSCLFACTOR */
#define DDPI_ENC_PARAMID_EXTPGMSCL_AUX1    (61) /*!< aux 1 channel scale factor: see DDPI_ENC_EXTPGMSCLFACTOR */
#define DDPI_ENC_PARAMID_EXTPGMSCL_AUX2    (62) /*!< aux 2 channel scale factor: see DDPI_ENC_EXTPGMSCLFACTOR */
#define DDPI_ENC_PARAMID_DMIXSCL           (63) /*!< primary audio downmix scaling: see DDPI_ENC_EXTPGMSCLFACTOR */

/* Timecode */
#define DDPI_ENC_PARAMID_TC_ENABL          (64) /*!< if 1, enable timecode generation */
#define DDPI_ENC_PARAMID_TC_FRCODE         (65) /*!< timecode frame rate code: see DDPI_ENC_TC_FRMRATECODE */
#define DDPI_ENC_PARAMID_TC_ISDROPFRM      (66) /*!< drop-frame flag */
#define DDPI_ENC_PARAMID_TC_OFFSET         (67) /*!< timecode sample offset */

/* Misc */
#define DDPI_ENC_PARAMID_FRAMESPERFRAMESET (68) /*!< if unsupported, use DDPI_ENC_DEF_FRAMESPERFRAMESET */
#define DDPI_ENC_PARAMID_CURRFRAME         (69) /*!< current frame number (for timecode and debug data) */
#define DDPI_ENC_PARAMID_CONFIGQUEUE       (70) /*!< config file support (internal use only) */
/* input channel configuration */
#define DDPI_ENC_PARAMID_INPUTCFG		   (105)  /*!< audio coding mode for input source: see DDPI_ENC_ACMOD */
#define DDPI_ENC_PARAMID_DMXINPUT		   (106) /*!< indicate if downmix input is present: 1: present 0: absent */

#define DDPI_ENC_PARAMID_LICKEYTYPE        (216)  /*!< For licensing key type */
#define DDPI_ENC_PARAMID_LICKEYDATA        (217)  /*!< For licensing key data */

/*! \brief Encoder data IDs, passed to #ddpi_enc_getdata or #ddpi_enc_setdata */

#define DDPI_ENC_DATAID_EXT_DRC            (1)
	/*!< External DRC data.  When set, this parameter causes
	     the encoder to override the internally-generated
	     DRC words and replace them with the provided DRC data.
	     The p_val void pointer to #ddpi_enc_getdata or 
	     #ddpi_enc_setdata must be an array of pointers to 
	     ddpi_enc_external_drc structures.  The length of the 
	     array must match the number of frames per frameset
	     (1 by default, 6 maximum) */

#define DDPI_ENC_DATAID_EXT_DRC2           (2)
	/*!< External DRC data for the second dual mono program, if present.  
	     This parameter is only valid if the audio coding mode has already 
	     been set to DDPI_ENC_ACMOD11.  When set, this parameter causes
	     the encoder to override the internally-generated
	     DRC words for the second dual mono program and replace them with 
	     the provided DRC data.  The p_val void pointer to #ddpi_enc_getdata 
		 or #ddpi_enc_setdata must be an array of pointers to 
	     ddpi_enc_external_drc structures.  The length of the 
	     array must match the number of frames per frameset
	     (1 by default, 6 maximum) */

#define DDPI_ENC_DATAID_ADDBSI             (3)
	/*!< Addbsi data. The p_val void pointer to #ddpi_enc_getdata 
		 or #ddpi_enc_setdata will be interpreted as an array of
		 unsigned chars */	

#define DDPI_ENC_DATAID_AUXDATA            (4)
	/*!< Auxillary data. The p_val void pointer to #ddpi_enc_getdata 
		 or #ddpi_enc_setdata will be interpreted as an array of
		 unsigned chars */

/*! \brief Encoder mode options for DDPI_ENC_PARAM_ENCODERMODE param */
typedef enum
{
	DDPI_ENC_MODE_DDP=0,            /*!< Mode 0:  Dolby Digital Plus (bsid 16)                          */
	DDPI_ENC_MODE_DD,               /*!< Mode 1:  Dolby Digital (bsid 6)                                */
	DDPI_ENC_MODE_RESERVED_1,       /*!< Mode 2:  HDDVD (bsid 16) - no longer supported                 */
	DDPI_ENC_MODE_BLURAY_PRIMARY,   /*!< Mode 3:  BD-compliant primary stream (I0: bsid 6, D0: bsid 16) */
	DDPI_ENC_MODE_BLURAY_SECONDARY, /*!< Mode 4:  BD-compliant secondary audio stream (bsid 16)         */
	DDPI_ENC_MODE_DDP_CONSUMER,     /*!< Mode 5:  Dolby Digital Plus Consumer Mode (bsid 12)            */
	DDPI_ENC_MODE_DD_CONSUMER,      /*!< Mode 6:  Dolby Digital Consumer Mode (bsid 4)                  */
	DDPI_ENC_MODE_DD_LEGACY,        /*!< Mode 7:  Dolby Digital Legacy Mode (bsid 8)                    */
	DDPI_ENC_MODE_DDP_LC,           /*!< Mode 8:  Low Complexity Dolby Digital Plus (bsid 12)           */
	DDPI_ENC_MODE_DD_LC,            /*!< Mode 9:  Low Complexity Dolby Digital (bsid 4)                 */
	DDPI_ENC_MODE_COUNT
} DDPI_ENC_MODE;

/*! \brief Audio coding mode options for DDPI_ENC_PARAMID_ACMOD param */
typedef enum
{							  /*!< Channels:  L R C LFE Ls Rs x1  x2  | dmix1 dmix2 dmix3		*/	
	DDPI_ENC_ACMOD11=0,       /*!< Acmod 0:   L R 												*/
	DDPI_ENC_ACMOD10,         /*!< Acmod 1:       C 											*/
	DDPI_ENC_ACMOD20,         /*!< Acmod 2:   L R 												*/
	DDPI_ENC_ACMOD30,         /*!< Acmod 3:   L R C LFE 										*/
	DDPI_ENC_ACMOD21,         /*!< Acmod 4:   L R   LFE Ls 										*/
	DDPI_ENC_ACMOD31,         /*!< Acmod 5:   L R C LFE Ls 										*/
	DDPI_ENC_ACMOD22,         /*!< Acmod 6:   L R   LFE Ls Rs 									*/
	DDPI_ENC_ACMOD32,         /*!< Acmod 7:   L R C LFE Ls Rs									*/
	DDPI_ENC_CHCFG8,          /*!< Ch cfg 8:  L R C LFE       Cvh     | C						*/
	DDPI_ENC_CHCFG9,          /*!< Ch cfg 9:  L R   LFE Ls Rs Ts      | Ls    Rs				*/
	DDPI_ENC_CHCFG10,         /*!< Ch cfg 10: L R C LFE Ls Rs Ts      | Ls    Rs				*/
	DDPI_ENC_CHCFG11,         /*!< Ch cfg 11: L R C LFE Ls Rs Cvh     | C						*/
	DDPI_ENC_CHCFG12,         /*!< Ch cfg 12: L R C LFE       Lc  Rc  | L     R     C			*/
	DDPI_ENC_CHCFG13,         /*!< Ch cfg 13: L R   LFE Ls Rs Lw  Rw  | L     R					*/
	DDPI_ENC_CHCFG14,         /*!< Ch cfg 14: L R   LFE Ls Rs Lvh Rvh | L     R					*/
	DDPI_ENC_CHCFG15,         /*!< Ch cfg 15: L R   LFE Ls Rs Lsd Rsd | Ls    Rs				*/
	DDPI_ENC_CHCFG16,         /*!< Ch cfg 16: L R   LFE Ls Rs Lrs Rrs | Ls    Rs				*/
	DDPI_ENC_CHCFG17,         /*!< Ch cfg 17: L R C LFE Ls Rs Lc  Rc  | L     R     C			*/
	DDPI_ENC_CHCFG18,         /*!< Ch cfg 18: L R C LFE Ls Rs Lw  Rw  | L     R					*/
	DDPI_ENC_CHCFG19,         /*!< Ch cfg 19: L R C LFE Ls Rs Lvh Rvh | L     R					*/
	DDPI_ENC_CHCFG20,         /*!< Ch cfg 20: L R C LFE Ls Rs Lsd Rsd | Ls    Rs				*/
	DDPI_ENC_CHCFG21,         /*!< Ch cfg 21: L R C LFE Ls Rs Lrs Rrs | Ls    Rs				*/
	DDPI_ENC_CHCFG22,         /*!< Ch cfg 22: L R C LFE Ls Rs Ts  Cvh | Ls    Rs    C			*/
	DDPI_ENC_CHCFG23,         /*!< Ch cfg 23: L R   LFE Ls Rs Cs      | Ls    Rs				*/
	DDPI_ENC_CHCFG24,         /*!< Ch cfg 24: L R C LFE Ls Rs Cs      | Ls    Rs				*/
	DDPI_ENC_CHCFG25,         /*!< Ch cfg 25: L R   LFE Ls Rs Cs  Ts  | Ls    Rs				*/
	DDPI_ENC_CHCFG26,         /*!< Ch cfg 26: L R C LFE Ls Rs Cs  Cvh | Ls    Rs    C			*/
	DDPI_ENC_CHCFG27          /*!< Ch cfg 27: L R C LFE Ls Rs Cs  Ts  | Ls    Rs				*/
} DDPI_ENC_ACMOD;

/*! \brief Autosetup profile options for DDPI_ENC_PARAMID_AUTOSETUP_PROF param */
typedef enum
{
	DDPI_ENC_AUTOSETUP_UNSUPP=-1,            /*!< Autosetup profile not supported in all modes  */
	DDPI_ENC_AUTOSETUP_PROF_DEF=0,           /*!< Default (wideband) profile                    */
	DDPI_ENC_AUTOSETUP_PROF_SPEECH           /*!< Speech/Commentary profile                     */
} DDPI_ENC_AUTOSETUP_PROF;

/*! \brief Dynamic range compression (DRC) profile options for DDPI_ENC_PARAMID_DYNRNGPROF
           DDPI_ENC_PARAMID_COMPRPROF, and DDPI_ENC_PARAMID_GBLDRCPROF params */
typedef enum
{
	DDPI_ENC_DRCPROF_NONE=0,                 /*!< DRC profile 0: none                           */
	DDPI_ENC_DRCPROF_FILMSTD,                /*!< DRC profile 1: film standard compression      */
	DDPI_ENC_DRCPROF_FILMLIGHT,              /*!< DRC profile 2: film light compression         */
	DDPI_ENC_DRCPROF_MUSICSTD,               /*!< DRC profile 3: music standard compression     */
	DDPI_ENC_DRCPROF_MUSICLIGHT,             /*!< DRC profile 4: music light compression        */
	DDPI_ENC_DRCPROF_SPEECH                  /*!< DRC profile 5: speech compression             */
} DDPI_ENC_DRCPROF;

/*! \brief Preferred stereo downmix mode options for DDPI_ENC_PARAMID_DMIXMOD param */
typedef enum
{
	DDPI_ENC_DMIXMOD_NI=0,                   /*!< Dmixmod 0: Not indicated                      */
	DDPI_ENC_DMIXMOD_PL,                     /*!< Dmixmod 1: Pro Logic downmix preferred        */
	DDPI_ENC_DMIXMOD_ST,                     /*!< Dmixmod 2: Stereo downmix preferred           */
	DDPI_ENC_DMIXMOD_PLII                    /*!< Dmixmod 3: Pro Logic II downmix preferred     */
} DDPI_ENC_DMIXMOD;

/*! \brief Center mix level code options for DDPI_ENC_PARAMID_CMIXLEV param */
typedef enum
{
	DDPI_ENC_CMIXLEV_M30=0,                  /*!< Mix level code 0: 0.707 (-3.0 dB)             */
	DDPI_ENC_CMIXLEV_M45,                    /*!< Mix level code 1: 0.595 (-4.5 dB)             */
	DDPI_ENC_CMIXLEV_M60                     /*!< Mix level code 2: 0.500 (-6.0 dB)             */
} DDPI_ENC_CMIXLEV;

/*! \brief Surround mix level code options for DDPI_ENC_PARAMID_SURMIXLEV param */
typedef enum
{
	DDPI_ENC_SURMIXLEV_M30=0,                /*!< Mix level code 0: 0.707 (-3.0 dB)             */
	DDPI_ENC_SURMIXLEV_M60,                  /*!< Mix level code 1: 0.500 (-6.0 dB)             */
	DDPI_ENC_SURMIXLEV_MINF                  /*!< Mix level code 2: 0.000 (-infinity)           */
} DDPI_ENC_SURMIXLEV;

/*! \brief Lt/Rt center mix level code options for DDPI_ENC_PARAMID_LTRTCMIXLEV */
typedef enum
{
	DDPI_ENC_LTRTCMIXLEV_30=0,               /*!< Mix level code 0: 1.414 (+3.0 dB)             */
	DDPI_ENC_LTRTCMIXLEV_15,                 /*!< Mix level code 1: 1.189 (+1.5 dB)             */
	DDPI_ENC_LTRTCMIXLEV_00,                 /*!< Mix level code 2: 1.000 (0.0 dB)              */
	DDPI_ENC_LTRTCMIXLEV_M15,                /*!< Mix level code 3: 0.841 (-1.5 dB)             */
	DDPI_ENC_LTRTCMIXLEV_M30,                /*!< Mix level code 4: 0.707 (-3.0 dB)             */
	DDPI_ENC_LTRTCMIXLEV_M45,                /*!< Mix level code 5: 0.595 (-4.5 dB)             */
	DDPI_ENC_LTRTCMIXLEV_M60,                /*!< Mix level code 6: 0.500 (-6.0 dB)             */
	DDPI_ENC_LTRTCMIXLEV_MINF                /*!< Mix level code 7: 0.000 (-inf dB)             */
} DDPI_ENC_LTRTCMIXLEV;

/*! \brief Lt/Rt surround mix level code options for DDPI_ENC_PARAMID_LTRTSURMIXLEV param */
typedef enum
{
	DDPI_ENC_LTRTSURMIXLEV_M15=3,            /*!< Mix level code 3: 0.841 (-1.5 dB)             */
	DDPI_ENC_LTRTSURMIXLEV_M30,              /*!< Mix level code 4: 0.707 (-3.0 dB)             */
	DDPI_ENC_LTRTSURMIXLEV_M45,              /*!< Mix level code 5: 0.595 (-4.5 dB)             */
	DDPI_ENC_LTRTSURMIXLEV_M60,              /*!< Mix level code 6: 0.500 (-6.0 dB)             */
	DDPI_ENC_LTRTSURMIXLEV_MINF              /*!< Mix level code 7: 0.000 (-inf dB)             */
} DDPI_ENC_LTRTSURMIXLEV;

/*! \brief Lo/Ro center mix level code options for DDPI_ENC_PARAMID_LOROCMIXLEV param */
typedef enum
{
	DDPI_ENC_LOROCMIXLEV_30=0,               /*!< Mix level code 0: 1.414 (+3.0 dB)             */
	DDPI_ENC_LOROCMIXLEV_15,                 /*!< Mix level code 1: 1.189 (+1.5 dB)             */
	DDPI_ENC_LOROCMIXLEV_00,                 /*!< Mix level code 2: 1.000 (0.0 dB)              */
	DDPI_ENC_LOROCMIXLEV_M15,                /*!< Mix level code 3: 0.841 (-1.5 dB)             */
	DDPI_ENC_LOROCMIXLEV_M30,                /*!< Mix level code 4: 0.707 (-3.0 dB)             */
	DDPI_ENC_LOROCMIXLEV_M45,                /*!< Mix level code 5: 0.595 (-4.5 dB)             */
	DDPI_ENC_LOROCMIXLEV_M60,                /*!< Mix level code 6: 0.500 (-6.0 dB)             */
	DDPI_ENC_LOROCMIXLEV_MINF                /*!< Mix level code 7: 0.000 (-inf dB)             */
} DDPI_ENC_LOROCMIXLEV;

/*! \brief Lo/Ro surround mix level code options for DDPI_ENC_PARAMID_LOROSURMIXLEV param */
typedef enum
{
	DDPI_ENC_LOROSURMIXLEV_M15=3,            /*!< Mix level code 3: 0.841 (-1.5 dB)             */
	DDPI_ENC_LOROSURMIXLEV_M30,              /*!< Mix level code 4: 0.707 (-3.0 dB)             */
	DDPI_ENC_LOROSURMIXLEV_M45,              /*!< Mix level code 5: 0.595 (-4.5 dB)             */
	DDPI_ENC_LOROSURMIXLEV_M60,              /*!< Mix level code 6: 0.500 (-6.0 dB)             */
	DDPI_ENC_LOROSURMIXLEV_MINF              /*!< Mix level code 7: 0.000 (-inf dB)             */
} DDPI_ENC_LOROSURMIXLEV;

/*! \brief Dolby Surround EX Mode options for DDPI_ENC_PARAMID_DSUREXMOD param */
typedef enum
{
	DDPI_ENC_DSUREXMOD_NI=0,                 /*!< Dolby Surround EX Mode 0: not indicated       */
	DDPI_ENC_DSUREXMOD_DISABL,               /*!< Dolby Surround EX Mode 1: disabled            */
	DDPI_ENC_DSUREXMOD_ENABLE                /*!< Dolby Surround EX Mode 2: enabled             */
} DDPI_ENC_DSUREXMOD;

/*! \brief Dolby Headphone Mode options for DDPI_ENC_PARAMID_DHEADPHONMOD param */
typedef enum
{
	DDPI_ENC_DHEADPHONMOD_NI=0,              /*!< Dolby Headphone Mode 0: not indicated         */
	DDPI_ENC_DHEADPHONMOD_DISABL,            /*!< Dolby Headphone Mode 1: disabled              */
	DDPI_ENC_DHEADPHONMOD_ENABLE             /*!< Dolby Headphone Mode 2: enabled               */
} DDPI_ENC_DHEADPHONMOD;

/*! \brief A/D Converter Type options for DDPI_ENC_PARAMID_ADCONVTYP param */
typedef enum
{
	DDPI_ENC_ADCONVTYP_STD=0,                /*!< A/D Converter Type 0: Standard                */
	DDPI_ENC_ADCONVTYP_HDCD                  /*!< A/D Converter Type 1: HDCD                    */
} DDPI_ENC_ADCONVTYP;

/*! \brief Copyright flag options for DDPI_ENC_PARAMID_COPYRIGHTB param */
typedef enum
{
	DDPI_ENC_COPYRIGHTB_NOCOPYRIGHT=0,       /*!< Copyright flag 0: no copyright                */
	DDPI_ENC_COPYRIGHTB_COPYRIGHT            /*!< Copyright flag 1: copyright                   */
} DDPI_ENC_COPYRIGHTB;

/*! \brief Original bitstream flag options for DDPI_ENC_PARAMID_ORIGBS param */
typedef enum
{
	DDPI_ENC_ORIGBS_COPY=0,                  /*!< Original bitstream flag 0: copied bitstream   */
	DDPI_ENC_ORIGBS_ORIG                     /*!< Original bitstream flag 1: original bitstream */
} DDPI_ENC_ORIGBS;

/*! \brief Bitstream mode options for DDPI_ENC_PARAMID_BSMOD param */
typedef enum
{
	DDPI_ENC_BSMOD_CM=0, /*!< Bitstream mode 0: Main audio service: complete main (CM)          */
	DDPI_ENC_BSMOD_ME,   /*!< Bitstream mode 1: Main audio service: music and effects (ME)      */
	DDPI_ENC_BSMOD_VI,   /*!< Bitstream mode 2: Associated audio service: visually impaired (VI)*/
	DDPI_ENC_BSMOD_HI,   /*!< Bitstream mode 3: Associated audio service: hearing impaired (HI) */
	DDPI_ENC_BSMOD_D,    /*!< Bitstream mode 4: Associated audio service: dialogue (D)          */
	DDPI_ENC_BSMOD_C,    /*!< Bitstream mode 5: Associated audio service: commentary (C)        */
	DDPI_ENC_BSMOD_E,    /*!< Bitstream mode 6: Associated audio service: emergency (E)         */
	DDPI_ENC_BSMOD_VO    /*!< Bitstream mode 7: Associated audio service: voice over (VO)       */
} DDPI_ENC_BSMOD;

/*! \brief Room type options for DDPI_ENC_PARAMID_ROOMTYP param */
typedef enum
{
	DDPI_ENC_ROOMTYP_NI=0,                   /*!< Room type 0: Not Indicated                    */
	DDPI_ENC_ROOMTYP_LG,                     /*!< Room type 1: Large Room, X curve monitor      */
	DDPI_ENC_ROOMTYP_SM                      /*!< Room type 2: Small room, flat monitor         */
} DDPI_ENC_ROOMTYP;

/*! \brief Dolby Surround Mode options for DDPI_ENC_PARAMID_DSURMOD param */
typedef enum
{
	DDPI_ENC_DSURMOD_NI=0,                   /*!< Dolby Surround Mode 0: not indicated          */
	DDPI_ENC_DSURMOD_DISABL,                 /*!< Dolby Surround Mode 1: disabled               */
	DDPI_ENC_DSURMOD_ENABLE                  /*!< Dolby Surround Mode 2: enabled                */
} DDPI_ENC_DSURMOD;

/*! \brief Dolby Lic Type options for DDPI_ENC_PARAMID_LICKEYTYPE param */
typedef enum
{
	DDPI_ENC_ENFLICTYPE_TEXT,
	DDPI_ENC_ENFLICTYPE_DMP_ILOK,
	DDPI_ENC_ENFLICTYPE_COUNT
} DDPI_ENC_ENFLICTYPE;

/*! \brief Program scale factor options for DDPI_ENC_PARAMID_PGMSCL and DDPI_ENC_PARAMID_EXTPGMSCL params */
typedef enum
{
	DDPI_ENC_SCLFACTOR_UNSUPP=-1,            /*!< Scale factors not supported in all modes      */
	DDPI_ENC_SCLFACTOR_MUTE=0,               /*!< Program scale factor 0: mute                  */
	DDPI_ENC_SCLFACTOR_M50=1,                /*!< Program scale factor 1: -50 dB                */
	DDPI_ENC_SCLFACTOR_0=51,                 /*!< Program scale factor 51:  0 dB                */
	DDPI_ENC_SCLFACTOR_12=63                 /*!< Program scale factor 63: 12 dB                */
} DDPI_ENC_SCLFACTOR;

/*! \brief Premix compression word selector options for DDPI_ENC_PARAMID_PREMIXCMPSEL param */
typedef enum
{
	DDPI_ENC_PREMIXCMPSEL_DYNRNG=0,          /*!< use dynrng for premix compression             */
	DDPI_ENC_PREMIXCMPSEL_COMPR              /*!< use compr for premix compression              */
} DDPI_ENC_PREMIXCMPSEL;

/*! \brief Dynamic range control word source selector options for DDPI_ENC_PARAMID_DRCSRC param */
typedef enum
{
	DDPI_ENC_DRCSRC_EXTERNAL=0,              /*!< use dynrng and compr from external prog       */
	DDPI_ENC_DRCSRC_CURRENT                  /*!< use dynrng and compr from current prog        */
} DDPI_ENC_DRCSRC;

/*! \brief Premix compression word scale factor options for DDPI_ENC_PARAMID_PREMIXCMPSCL param */
typedef enum
{
	DDPI_ENC_PREMIXCMPSCL_0=0,               /*!< 0% compression scaling                        */
	DDPI_ENC_PREMIXCMPSCL_14_3,              /*!< 14.3% compression scaling                     */
	DDPI_ENC_PREMIXCMPSCL_28_6,              /*!< 28.6% compression scaling                     */
	DDPI_ENC_PREMIXCMPSCL_42_9,              /*!< 42.9% compression scaling                     */
	DDPI_ENC_PREMIXCMPSCL_57_1,              /*!< 57.1% compression scaling                     */
	DDPI_ENC_PREMIXCMPSCL_71_4,              /*!< 71.4% compression scaling                     */
	DDPI_ENC_PREMIXCMPSCL_85_7,              /*!< 85.7% compression scaling                     */
	DDPI_ENC_PREMIXCMPSCL_100                /*!< 100% compression scaling                      */
} DDPI_ENC_PREMIXCMPSCL;

/*! \brief Mix data field definition options for DDPI_ENC_PARAMID_MIXDEF param */
typedef enum
{
	DDPI_ENC_MIXDEF0=0,                      /*!< mixing option 1, no additional bits           */
	DDPI_ENC_MIXDEF1,                        /*!< mixing option 2, 5 additional bits            */
	DDPI_ENC_MIXDEF2,                        /*!< mixing option 3, 12 additional bits           */
	DDPI_ENC_MIXDEF3                         /*!< mixing option 4, 16-264 additional bits       */
} DDPI_ENC_MIXDEF;

/*! \brief External program scale factors for DDPI_ENC_PARAMID_EXTPGMSCL_* params */
typedef enum
{
	DDPI_ENC_EXTPGMSCLFACTOR_UNDEFINED = -1, /*!< do not transmit                               */
	DDPI_ENC_EXTPGMSCLFACTOR_NEG_1DB   = 0,  /*!< -1dB                                          */
	DDPI_ENC_EXTPGMSCLFACTOR_NEG_2DB   = 1,  /*!< -2dB                                          */
	DDPI_ENC_EXTPGMSCLFACTOR_NEG_3DB   = 2,  /*!< -3dB                                          */
	DDPI_ENC_EXTPGMSCLFACTOR_NEG_4DB   = 3,  /*!< -4dB                                          */
	DDPI_ENC_EXTPGMSCLFACTOR_NEG_5DB   = 4,  /*!< -5dB                                          */
	DDPI_ENC_EXTPGMSCLFACTOR_NEG_6DB   = 5,  /*!< -6dB                                          */
	DDPI_ENC_EXTPGMSCLFACTOR_NEG_8DB   = 6,  /*!< -8dB                                          */
	DDPI_ENC_EXTPGMSCLFACTOR_NEG_10DB  = 7,  /*!< -10dB                                         */
	DDPI_ENC_EXTPGMSCLFACTOR_NEG_12DB  = 8,  /*!< -12dB                                         */
	DDPI_ENC_EXTPGMSCLFACTOR_NEG_14DB  = 9,  /*!< -14dB                                         */
	DDPI_ENC_EXTPGMSCLFACTOR_NEG_16DB  = 10, /*!< -16dB                                         */
	DDPI_ENC_EXTPGMSCLFACTOR_NEG_19DB  = 11, /*!< -19dB                                         */
	DDPI_ENC_EXTPGMSCLFACTOR_NEG_22DB  = 12, /*!< -22dB                                         */
	DDPI_ENC_EXTPGMSCLFACTOR_NEG_25DB  = 13, /*!< -25dB                                         */
	DDPI_ENC_EXTPGMSCLFACTOR_NEG_28DB  = 14, /*!< -28dB                                         */
	DDPI_ENC_EXTPGMSCLFACTOR_NEG_INF   = 15  /*!< -inf (mute)                                   */
} DDPI_ENC_EXTPGMSCLFACTOR;

/*! \brief Timecode frame rate code options for DDPI_ENC_PARAMID_TC_FRCODE param */
typedef enum
{
	DDPI_ENC_TC_FRMRATECODE_2398=1,          /*!< Timecode frame rate code: 1 = 23.98 fps       */
	DDPI_ENC_TC_FRMRATECODE_24,              /*!< Timecode frame rate code: 2 = 24 fps          */
	DDPI_ENC_TC_FRMRATECODE_25,              /*!< Timecode frame rate code: 3 = 25 fps          */
	DDPI_ENC_TC_FRMRATECODE_2997,            /*!< Timecode frame rate code: 4 = 29.97 fps       */
	DDPI_ENC_TC_FRMRATECODE_30               /*!< Timecode frame rate code: 5 = 30 fps          */
} DDPI_ENC_TC_FRMRATECODE;

#define ENC_DEFAULT_INPUTCFG	(-2)

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* ENC_API_PARAMS_H */
