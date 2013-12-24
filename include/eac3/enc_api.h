/* $Header: //depot/licensing/ddplus/ddp_enc/dev/20130325_Zeus_v1.3/ddp_enc/include/enc_api.h#1 $ */

/***************************************************************************\
*
*	This program is protected under international and U.S. copyright laws
*	as an unpublished work.  Do not copy.
*
*	Copyright 2011 by Dolby Laboratories Inc. All rights reserved.
*
*	Module:		Dolby Digital Plus Encoder API Module
*
*	File:		enc_api.h
*
\***************************************************************************/

/**
 *  @file enc_api.h
 *
 *  @brief Encoder API params header file.
 *
 * Part of the Encoder API Module.
 */

#ifndef ENC_API_H
#define ENC_API_H

/*
 * remove dependency of 'dlb_types.h'
 * sizeof(long)==8 (64 bit) when compiled in 64 bit Linux and COFF of C64/C64+,
 * sizeof(long)==4 (32 bit) when complied in 32 bit Linux, ARM, EABI of C64/C64+ and Windows
 * sizeof(int) ==4 (32 bit) applies to ARM, TI DSP COFF/EABI, Linux 64/32,and Windows
 * Therefore we use 'int' as 'int32_t' and 'unsigned int' as 'uint32_t'
 */
typedef unsigned int uint32_t;
typedef int int32_t;

/**** API Dependencies ****/
#include "dlb_buffer.h"

/**** API Parameter Defines and Enumerations ****/
#include "enc_api_params.h"

/****  C++ Compatibility ****/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**** API Defines ****/

#define	DDPI_ENC_BLKSIZE                (1536)  /*!< Fixed input PCM block size                 */
#define DDPI_ENC_DEF_FRAMESPERFRAMESET  (1)     /*!< Default framesets have 1 frame with 6 blks */
#define DDPI_ENC_MAX_FRAMESPERFRAMESET  (6)     /*!< At most, framesets have 6 frames with 1 blk */

/*! \brief Values for ddpi_enc_param_desc.rangetype */
#define	DDPI_ENC_PARAMRANGE_MIXMAX      (1)     /*!< range of ddpi_enc_param_desc is in min_val and max_val */
#define	DDPI_ENC_PARAMRANGE_LIST        (2)     /*!< range of ddpi_enc_param_desc is in range_vals          */

/*! \brief API Error Codes */
#define DDPI_ENC_ERR_NO_ERROR                       0   /*!< No error */
#define DDPI_ENC_ERR_FATAL                          1   /*!< general fatal error */
#define DDPI_ENC_ERR_RATE                           2   /*!< data rate too low for specified mode */
#define DDPI_ENC_ERR_CHANS                          3   /*!< unsupported number of input channels */
#define DDPI_ENC_ERR_BITRATE                        4   /*!< unsupported bitstream data rate */
#define DDPI_ENC_ERR_SMPRATE                        5   /*!< unsupported sample rate */
#define DDPI_ENC_ERR_BUFWDTH                        6   /*!< unsupported aux buffer data width */
#define DDPI_ENC_ERR_SRMOD                          7   /*!< unsupported sample rate modifier */
#define DDPI_ENC_ERR_RSVSIP                         8   /*!< unsupported use of reserved SIP parameter */
#define DDPI_ENC_ERR_AUXDATL                        9   /*!< unsupported auxdata length */
#define DDPI_ENC_ERR_BWLOWCPL                       10  /*!< audio bandwidth lower than couple start freq */
#define DDPI_ENC_ERR_CHBWCOD                        11  /*!< chbwcod too high */
#define DDPI_ENC_ERR_PHSCORE                        12  /*!< cplcoe and phscore inconsistant */
#define DDPI_ENC_ERR_LFEHALF                        13  /*!< LFE not allowed in 1/2 and 1/4 rate modes.  LFE not coded. */
#define DDPI_ENC_ERR_BWSPX                          14  /*!< audio bandwidth too low for SPX - SPX turned off. */
#define DDPI_ENC_ERR_BWHIGH                         15  /*!< audbwcod too high */

#define DDPI_ENC_ERR_INITPARAMS                     16  /*!< error initializing encoder control parameters. */
#define DDPI_ENC_ERR_STFMEMINIT                     17
#define DDPI_ENC_ERR_STFPROCESS                     18
#define DDPI_ENC_ERR_CPL_ECPL_ENABL                 19  /*!< coupling and e-coupling cannot both be enabled simultaneously */

#define DDPI_ENC_ERR_PGM_NUM                        20
#define DDPI_ENC_ERR_UNSUPPORTED_PARAM              21
#define DDPI_ENC_ERR_READONLY_PARAM                 22
#define DDPI_ENC_ERR_BLOCKSIZE                      23
#define DDPI_ENC_ERR_UNSUPPORTED_MODE               24
#define DDPI_ENC_ERR_NULL_PARAMETER                 25
#define DDPI_ENC_ERR_MAX_PGMS                       26
#define DDPI_ENC_ERR_MAX_CHANS                      27
#define DDPI_ENC_ERR_OUTPUT_BUFFER_DESCRIPTOR       28
#define DDPI_ENC_ERR_INPUT_BUFFER_DESCRIPTOR        29

#define DDPI_ENC_ERR_INVALID_AUDIO_CODING_MODE      30
#define DDPI_ENC_ERR_INVALID_DIALNORM               31
#define DDPI_ENC_ERR_INVALID_LFEFILTERFLAG          32
#define DDPI_ENC_ERR_INVALID_DCFILTERFLAG           33
#define DDPI_ENC_ERR_INVALID_COMPR_PROFILE          34
#define DDPI_ENC_ERR_INVALID_COMPR2_PROFILE         35
#define DDPI_ENC_ERR_INVALID_DYNRNG_PROFILE         36
#define DDPI_ENC_ERR_INVALID_DYNRNG2_PROFILE        37
#define DDPI_ENC_ERR_INVALID_LFE_MODE               38
#define DDPI_ENC_ERR_INVALID_DIGDEEMPH_MODE         39
#define DDPI_ENC_ERR_INVALID_SAMPRATECODE           40
#define DDPI_ENC_ERR_INVALID_ENC51_ENCODING_MODE    41
#define DDPI_ENC_ERR_INVALID_BWFILTERFLAG           42
#define DDPI_ENC_ERR_INVALID_PH90FILTERFLAG         43
#define DDPI_ENC_ERR_INVALID_SURATTFLAG             44
#define DDPI_ENC_ERR_INVALID_DMIXMOD                45
#define DDPI_ENC_ERR_INVALID_LTRTCMIXLEV            46
#define DDPI_ENC_ERR_INVALID_LTRTSURMIXLEV          47
#define DDPI_ENC_ERR_INVALID_LOROCMIXLEV            48
#define DDPI_ENC_ERR_INVALID_LOROSURMIXLEV          49
#define DDPI_ENC_ERR_INVALID_DSUREXMOD              50
#define DDPI_ENC_ERR_INVALID_DHEADPHONMOD           51
#define DDPI_ENC_ERR_INVALID_ADCONVTYP              52
#define DDPI_ENC_ERR_INVALID_MIXLEVEL               53
#define DDPI_ENC_ERR_INVALID_COPYRIGHTB             54
#define DDPI_ENC_ERR_INVALID_ORIGBS                 55
#define DDPI_ENC_ERR_INVALID_BSMOD                  56
#define DDPI_ENC_ERR_INVALID_ROOMTYP                57
#define DDPI_ENC_ERR_INVALID_DSURMOD                58
#define DDPI_ENC_ERR_INVALID_DATARATE               59
#define DDPI_ENC_ERR_INVALID_AUTOSETUPPROFILE       60
#define DDPI_ENC_ERR_INVALID_SUBSTREAMID            61
#define DDPI_ENC_ERR_INVALID_BSTYPE                 62
#define DDPI_ENC_ERR_INVALID_PGMSCL                 63
#define DDPI_ENC_ERR_INVALID_EXTPGMSCL              64
#define DDPI_ENC_ERR_INVALID_AUTOCPL                65
#define DDPI_ENC_ERR_INVALID_CPLENABL               66
#define DDPI_ENC_ERR_INVALID_CPLBEGFREQCODE         67
#define DDPI_ENC_ERR_INVALID_AUTOBW                 68
#define DDPI_ENC_ERR_INVALID_AUDBWCODE              69
#define DDPI_ENC_ERR_INVALID_DIALNORM2_ACMOD_SET    71
#define DDPI_ENC_ERR_INVALID_DIALNORM2              72
#define DDPI_ENC_ERR_INVALID_AUTOECP                73
#define DDPI_ENC_ERR_INVALID_ECPLENABL              74
#define DDPI_ENC_ERR_INVALID_ECPLBEGFREQCODE        75
#define DDPI_ENC_ERR_INVALID_CONVFRMSIZCOD          76
#define DDPI_ENC_ERR_INVALID_AUTOTPNP               77
#define DDPI_ENC_ERR_INVALID_TPNPENABL              78
#define DDPI_ENC_ERR_INVALID_AUTOAHT                79
#define DDPI_ENC_ERR_INVALID_AHTENABL               80
#define DDPI_ENC_ERR_INVALID_BLKSPERFRAME           81
#define DDPI_ENC_ERR_INVALID_AUTOSPX                82
#define DDPI_ENC_ERR_INVALID_SPXENABL               83
#define DDPI_ENC_ERR_INVALID_SPXBEGFREQCODE         84
#define DDPI_ENC_ERR_INVALID_TCINUSE                85
#define DDPI_ENC_ERR_INVALID_TC_FRAMERATECODE       86
#define DDPI_ENC_ERR_INVALID_TCISDROPFRAME          87
#define DDPI_ENC_ERR_INVALID_TC_OFFSET              88
#define DDPI_ENC_ERR_INVALID_ENF_ENABL              89
#define DDPI_ENC_ERR_INVALID_ENF_LICDATALEN         90
#define DDPI_ENC_ERR_INVALID_ENF_LICDATATYPE        91
#define DDPI_ENC_ERR_ECPL_NOT_SUPPORTED             92
#define DDPI_ENC_ERR_TPNP_NOT_SUPPORTED             93
#define DDPI_ENC_ERR_AHT_NOT_SUPPORTED              94
#define DDPI_ENC_ERR_SPX_NOT_SUPPORTED              95
#define DDPI_ENC_ERR_CHANMAP_DEP_ONLY               96
#define DDPI_ENC_ERR_INVALID_CMIXLEV                97
#define DDPI_ENC_ERR_INVALID_SURMIXLEV              98
#define DDPI_ENC_ERR_UPMIXER_NOT_SUPPORTED          99
#define DDPI_ENC_ERR_INVALID_GLOBAL_COMP_PROF       100
#define DDPI_ENC_ERR_INVALID_AUDIOCFG            	101
#define DDPI_ENC_ERR_INVALID_CE_CMIXLEV             102
#define DDPI_ENC_ERR_INVALID_CE_SURMIXLEV           103
#define DDPI_ENC_ERR_INVALID_CE_OPTIM               104
#define DDPI_ENC_ERR_INVALID_CE_DRATE_AUDIOCFG      105
#define DDPI_ENC_ERR_INVALID_LOUDLIB_RT_STATE       106
#define DDPI_ENC_ERR_INVALID_LOUDLIB_RESET			107
#define DDPI_ENC_ERR_INVALID_CONTENT_TYPE           108
#define DDPI_ENC_ERR_DOWNMIXER_NOT_SUPPORTED        109
#define DDPI_ENC_ERR_CONFIG_ADVQUEUE                130

#define DDPI_ENC_ERR_INVALID_LFE_ACMOD_SET          137
#define DDPI_ENC_ERR_INVALID_AUDPRODINFOFLAG        138
#define DDPI_ENC_ERR_INVALID_RESERVED_FIELD         139
#define DDPI_ENC_ERR_INVALID_CONTROL_STRUCTURE      140

#define DDPI_ENC_ERR_INVALID_PANMEAN                142
#define DDPI_ENC_ERR_INVALID_EXTPGMLSCL             143
#define DDPI_ENC_ERR_INVALID_EXTPGMCSCL             144
#define DDPI_ENC_ERR_INVALID_EXTPGMRSCL             145
#define DDPI_ENC_ERR_INVALID_EXTPGMLSSCL            146
#define DDPI_ENC_ERR_INVALID_EXTPGMRSSCL            147
#define DDPI_ENC_ERR_INVALID_EXTPGMLFESCL           148
#define DDPI_ENC_ERR_INVALID_EXTPGMAUX1SCL          149
#define DDPI_ENC_ERR_INVALID_EXTPGMAUX2SCL          150
#define DDPI_ENC_ERR_INVALID_DMIXSCL                151
#define DDPI_ENC_ERR_PANMEAN_REQ_ACMOD_1            152
#define DDPI_ENC_ERR_INVALID_MIXDEF                 153
#define DDPI_ENC_ERR_INVALID_PREMIXCMPSEL           154
#define DDPI_ENC_ERR_INVALID_DRCSRC                 155
#define DDPI_ENC_ERR_INVALID_PREMIXCMPSCL           156
#define DDPI_ENC_ERR_INVALID_DYNCPL                 157
#define DDPI_ENC_ERR_INVALID_DYNSPX                 158
#define DDPI_ENC_ERR_EXCEED_MAX_LOUDNESSLIBS		159
#define DDPI_ENC_ERR_UNEXPECTED_PARAMETER			160
#define DDPI_ENC_ERR_INVALID_DATA_COUNT				161
#define DDPI_ENC_ERR_INVALID_DATA_SIZE				162
#define DDPI_ENC_ERR_DATA_NOT_SET					163
#define DDPI_ENC_ERR_INVALID_ENF_USERDATA           164
#define DDPI_ENC_ERR_INVALID_ENF_DATE               165
#define DDPI_ENC_ERR_INVALID_ENF_LICTYPE            166
#define DDPI_ENC_ERR_INVALID_LICTYPE				167
#define DDPI_ENC_ERR_INVALID_LICDATA                168
#define DDPI_ENC_ERR_INVALID_ACMOD_INPUT_CFG		169
#define DDPI_ENC_ERR_UNSUPPORTED_ACMOD_INPUT_CFG	170
#define DDPI_UNHANDLED_ENCODER_MODE					171
#define DDPI_ENC_ERR_PARAMETER_ACCESS				172
#define DDPI_ENC_ERR_UNSUPPORTED_DOWNMIX_CFG		173
#define DDPI_ENC_UNSUPPORTED_PARAM					7979

/**** Encoder API structures ****/

/*! \brief Encoder instance handle */
typedef struct enc_instance_s enc_instance;

/*! \brief Encoder parameter descriptor */
typedef struct ddpi_enc_param_s 
{
    int   id;      /*!< parameter ID    */
    int   val;     /*!< parameter value */
} ddpi_enc_param;

typedef struct ddpi_enc_param_desc_s
{
	int                   id;                /*!< parameter ID */
	int                   readonly;          /*!< if nonzero, parameter is readonly */
	int                   errcode;           /*!< error code if this parameter is out of range */
	int                   default_val;       /*!< default value */
	int                   range_type;        /*!< range type flag */
	int                   min_val;           /*!< valid if range_type==DDPI_ENC_PARAMRANGE_MIXMAX) */
	int                   max_val;           /*!< valid if range_type==DDPI_ENC_PARAMRANGE_MIXMAX) */
	int                   num_range_vals;    /*!< valid if range_type==DDPI_ENC_PARAMRANGE_LIST) */
	const int            *p_range_vals;      /*!< valid if range_type==DDPI_ENC_PARAMRANGE_LIST) */
} ddpi_enc_param_desc;

/*! \brief Parameter descriptor table entry */
typedef struct ddpi_enc_param_desc_tab_entry_s
{
	int         nparams;                     /*!< number of parameters  */
	const ddpi_enc_param_desc *p_params;     /*!< parameter descriptors */
} ddpi_enc_param_desc_tab_entry;

/*! \brief Query output parameters */
typedef struct ddpi_enc_query_op_s
{
	int         num_pools;                   /*!< Number of memory pools                        */	
	int         max_pgms;                    /*!< Maximum supported independent programs        */
	int         version_major;               /*!< Major version number                          */
	int         version_minor;               /*!< Minor version number                          */
	int         version_update;              /*!< Update version number                         */	
	const char *p_copyright;                 /*!< Copyright string                              */
	int         configflags;                 /*!< Configuration bit field                       */
	int         defaultmode;                 /*!< Default encoder mode                          */
	int         p_modes[DDPI_ENC_MODE_COUNT];/*!< Valid encoder modes                           */
#ifdef DDP_VALIDATE_LICENSE
    int         license_required;            /*!< Set to 1 if the encoder needs a license in order to run */
#endif /* DDP_VALIDATE_LICENSE */
	const ddpi_enc_param_desc_tab_entry *p_paramtab[DDPI_ENC_MODE_COUNT]; /*!< Parameter descriptor table */
} ddpi_enc_query_op;

/*! \brief Delay and flushing descriptor */
typedef struct ddpi_enc_delay_s 
{
	uint32_t latencysamps;         /*!< Encoder latency, in samples (not including decode) */
	uint32_t flushsamps;           /*!< Samples to completely flush encoder buffers        */
} ddpi_enc_delay;

/*! \brief Memory pool descriptor */
typedef struct ddpi_enc_memory_pool_s
{
    size_t       size;                       /*!< size requirement (bytes) for pool             */
    void        *p_mem;                      /*!< pointer to pool memory                        */
} ddpi_enc_memory_pool;

/*! \brief Querymem output parameters */
typedef struct ddpi_enc_querymem_op_s
{
	ddpi_enc_memory_pool  *p_pool_reqs;      /*!< Array of pools to receive memory requirements */
	size_t                 maxoutbufsize;    /*!< Bitstream output buffer requirements (bytes)  */
	ddpi_enc_delay         delay;            /*!< Delay descriptor structure                    */
} ddpi_enc_querymem_op;

/*! \brief Per-program static configuration parameters */
typedef struct ddpi_enc_pgm_cfg_s
{
	int         init_mode;                   /*!< Initial encoder mode for this indep pgm       */
	int         max_chans;                   /*!< Maximum number of channels for this indep pgm */
	int         max_wordsperframeset;        /*!< Maximum words per frameset for this indep pgm */
} ddpi_enc_pgm_cfg;

/*! \brief Per-instance static configuration parameters */
typedef struct ddpi_enc_static_params_s
{
	int         p_modes[DDPI_ENC_MODE_COUNT];/*!< Flags to enable encoder modes                 */
	int         max_pgms;                    /*!< Maximum number of independent programs        */
	ddpi_enc_pgm_cfg *p_pgm_cfgs;            /*!< Config for each independent program stream    */
} ddpi_enc_static_params;

/*! \brief #ddpi_enc_process input parameters */
typedef struct ddpi_enc_process_ip_s
{
	dlb_buffer       *p_pgm_pcm_buffers;      /*!< PCM buffer descriptors for each indep program */
	int               npgms;                  /*!< Number of indep programs to encode            */
	int               nsamples;               /*!< Number of samples in each channel's buffer    */
} ddpi_enc_process_ip;

/*! \brief per-program output status */
typedef struct ddpi_enc_pgm_status_s
{
	short framesperframeset;                 /*!< Frames/frameset for this indep program stream */
	short outchans;                          /*!< Number of chans for this indep program stream */
	short bsid;                              /*!< Bitstream ID for this indep program stream    */
	short *p_peak;                           /*!< Array of peak absolute signal level values
		NOTE: The caller must assign this pointer to an array prior to calling ddpi_enc_process().
		After ddpi_enc_process() returns, that array will contain peak absolute signal level 
		values for each input PCM channel passed to ddpi_enc_process(), in the same order as 
		the input PCM data, defined in DDPI_ENC_CHANS											*/
	short complev;                           /*!< Compressor input level value                  */
	short linecomp;                          /*!< Line out mode dynamic range compression words */
	short rfcomp;                            /*!< RF mode dynamic range compression words       */
	short smr;                               /*!< Smoothed signal-to-mask ratio indicator       */
} ddpi_enc_pgm_status;

/*! \brief #ddpi_enc_process output parameters */
typedef struct ddpi_enc_process_op_s
{
	dlb_buffer           bitstream;          /*!< Single multiplexed output bitstream           */
	int                  noctets;            /*!< Size of output data, in octets, in dlb_buffer */
	ddpi_enc_pgm_status *p_status;           /*!< If non-zero, receives per-pgm output status   */
} ddpi_enc_process_op;

/*! Error handling callback function */
#define DDPI_MAX_ERROR_CB_STRLEN (2048)      /*!< max size of p_errmsg string in error_cb       */
typedef void (*error_cb) 
	(int         iserror                     /*!< true for errors, false for warnings           */
	,const char *p_errmsg                    /*!< error or warning message string               */
	,const char *p_file                      /*!< file where error or warning occurred          */
	,int         line                        /*!< line where error or warning occurred          */
	,void       *p_cbdata                    /*!< callback data passed to #ddpi_enc_open        */
	);

/**** DDPlus Encoder Interface Functions (API) ****/

/*!
******************************************************************************
*
*   This function returns static configuration information about the 
*   subroutine library, including the version number, Dolby copyright string,
*   number of memory pools, and mode-specific configuration information.  
*   It is called once at system startup.
*
*	\return
*	-#	DDPI_ENC_ERR_NO_ERROR if no errors occurred
*
******************************************************************************/
int 
ddpi_enc_query
	(ddpi_enc_query_op *p_query_op                   /*!< \out: Query output */
	);

/*!
******************************************************************************
*
*   This function returns the subroutine memory requirements, given a set of
*   static configuration parameters.  It can be called as many times as 
*   necessary, to determine the memory requirements for different
*   configurations.  It is called at least once at system startup.
*
*	\return
*	-#	DDPI_ENC_ERR_NO_ERROR if no errors occurred
*
******************************************************************************/
int 
ddpi_enc_querymem
	(const ddpi_enc_static_params *p_sp              /*!< \in: Static params */
	,      ddpi_enc_querymem_op   *p_querymem_op     /*!< \out: Query output */
	);

/*!
******************************************************************************
*
*	This function initializes the encoder subroutine memory, using the 
*   externally allocated memory pools.  The static parameters passed to
*   this function must have the same values as the call to #ddpi_enc_querymem 
*   to ensure that the allocated memory is sufficient for the requested
*   configuration.	It is called once at system startup.
*
*	\return
*	-#	DDPI_ENC_ERR_NO_ERROR if no errors occurred
*
******************************************************************************/
int 
ddpi_enc_open
	(      enc_instance           **pp_enc      /*!< \out: API instance           */      
	,const ddpi_enc_static_params  *p_sp        /*!< \in:  Static params          */
	,const ddpi_enc_memory_pool    *p_pools     /*!< \in:  allocated memory       */
	,const error_cb                 errhandler  /*!< \in:  error callback         */
	,      void                    *p_errcbdata /*!< \in:  error_cb instance data */
	);

/*!
******************************************************************************
*
*	This function sets the specified number of encoder parameters, for a 
*   specific independent program.  The change does not take effect until the 
*   next call to #ddpi_enc_process.  
*
*	The parameters in the list are validated in linear order - first to last. 
*   If any of the parameter IDs or values are invalid, an error is returned 
*   and none of the parameters in the list are applied.
*
*	\return
*	-#	DDPI_ENC_ERR_NO_ERROR if no errors occurred
*	-#	DDPI_ENC_ERR_PGM_NUM  if specified program number is not valid
*	-#	DDPI_ENC_ERR_UNSUPPORTED_PARAM if parameter is not supported in current mode
*	-#	DDPI_ENC_ERR_UNSUPPORTED_MODE if an unsupported mode is specified
*	-#	DDPI_ENC_ERR_READONLY_PARAM if a read-only parameter is specified
*
******************************************************************************/
int 
ddpi_enc_setparams
	(    enc_instance				*p_enc     /*!< \mod: API instance                 */      
		,int						pgm       /*!< \in:  independent program number   */
		,int					    nparams   /*!< \in:  num of elements in p_params  */
		,const ddpi_enc_param      *p_params  /*!< \in:  pointer to parameter list    */
	);

/*!
******************************************************************************
*
*	This function gets the current values of the requested encoder parameters, 
*   for a specific independent program.
*
*	\return
*	-#	DDPI_ENC_ERR_NO_ERROR if no errors occurred
*	-#	DDPI_ENC_ERR_PGM_NUM  if specified program number is not valid
*	-#	DDPI_ENC_ERR_UNSUPPORTED_PARAM if parameter is not supported in current mode
*
******************************************************************************/
int 
ddpi_enc_getparams
	(const enc_instance   *p_enc     /*!< \in:  API instance                 */      
	,      int             pgm       /*!< \in:  independent program number   */
	,      int             nparams   /*!< \in:  num of elements in p_params  */
	,      ddpi_enc_param *p_params  /*!< \out: pointer to parameter list    */
	);

/*!
******************************************************************************
*
*	This function sets encoder data, for a specific independent program. 
*   The format of the data is determined by the id parameter and the size
*   and count parameters are used to validate the data being passed in
*   p_val has the expected size.
*
*   The data values passed in do not take effect until the next call to 
*   #ddpi_enc_process.
*
*	\return
*	-#	DDPI_ENC_ERR_NO_ERROR if no errors occurred
*	-#	DDPI_ENC_ERR_PGM_NUM  if specified program number is not valid
*	-#	DDPI_ENC_ERR_UNSUPPORTED_PARAM if parameter is not supported in current mode
*
******************************************************************************/
int 
ddpi_enc_setdata
(      enc_instance   *p_enc         /*!< \mod: API instance                 */      
,      int             pgm           /*!< \in:  independent program number   */
,      int             id            /*!< \in:  data ID                      */
,      int             size          /*!< \in:  size of data type in bytes   */
,      int             count         /*!< \in:  number of data elements      */
,const void           *p_val         /*!< \in:  pointer to data memory       */
);

/*!
******************************************************************************
*
*	This function gets encoder data, for a specific independent program. 
*   The format of the data is determined by the id parameter and the size
*   and count parameters are used to validate the data being returned in
*   p_val has the expected size.
*
*	\return
*	-#	DDPI_ENC_ERR_NO_ERROR if no errors occurred
*	-#	DDPI_ENC_ERR_PGM_NUM  if specified program number is not valid
*	-#	DDPI_ENC_ERR_UNSUPPORTED_PARAM if parameter is not supported in current mode
*
******************************************************************************/
int 
ddpi_enc_getdata
(const enc_instance   *p_enc         /*!< \in:  API instance                 */      
,      int             pgm           /*!< \in:  independent program number   */
,      int             id            /*!< \in:  data ID                      */
,      int             size          /*!< \in:  size of data type in bytes   */
,      int             count         /*!< \in:  number of data elements      */
,      void           *p_val         /*!< \out: pointer to data memory       */
);

/*!
******************************************************************************
*
*	This function encodes a complete bitstream access unit, which contains
*   a collection of Dolby Digital and/or Dolby Digital Plus frames that 
*   represents 6 blocks of audio 
*   data from the same point in time across multiple programs.  An access unit 
*   always represents 1536 PCM samples worth of audio data from all channels
*   in all substreams.  Dolby Digital frames will always contain 6 blocks 
*   of audio data, whereas Dolby Digital Plus will contain 1, 2, 3, or 6
*   audio blocks.  The number of Dolby Digital Plus frames needed to 
*   represent 1536 PCM samples is referred to as a frameset.
*
*	\return
*	-#	DDPI_ENC_ERR_NO_ERROR if no errors occurred
*
******************************************************************************/
int 
ddpi_enc_process
	(enc_instance        *p_enc       /*!< \mod: API instance                */      
	,ddpi_enc_process_ip *p_inparams  /*!< \in:  input PCM buffers           */
	,ddpi_enc_process_op *p_outparams /*!< \out: output status and bitstream */
	);

/*!
******************************************************************************
*
*	This function performs all clean up necessary to close the encoder.
*
*	\return
*	-#	DDPI_ENC_ERR_NO_ERROR if no errors occurred
*
******************************************************************************/
int 
ddpi_enc_close
	(enc_instance        *p_enc                      /*!< \mod: API instance */      
	);

#ifdef DDP_VALIDATE_LICENSE
/*!
******************************************************************************
*
*	This function is used to setup license file and read license data
*
*	\return
*	-#	DDPI_ENC_ERR_NO_ERROR if no errors occurred
*
******************************************************************************/
int ddpi_enc_set_license(
            enc_instance        *p_enc,		    /*!< \mod: API instance */
            const void          *p_license,			/*!< \in: license data pointer */
            const size_t        size,				/*!< \in: license data size	*/
            const unsigned long manufacturer_id);	/*!< \in: manufacturer id	*/
#endif /* DDP_VALIDATE_LICENSE */

/* C++ Compatibility */
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* ENC_API_H */
