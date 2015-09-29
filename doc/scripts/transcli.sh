# !/bin/bash
# batch transcoding
# <jianfengzheng@pptv.com>
# @see print_help() for usage

#set -x

TRANSCLI='transcli/transcli'
PRESET=
IDIR=
ODIR=out
LDIR=log
OVERWRITE=0
SIMULATE=0
ARGS=`getopt -o hHT:P:I:O:L:ws --long help,tcli:,preset:,idir:,odir:,ldir:,overwrite,simulate -n transh -- "$@"`

# bad args
if [ $? -ne 0 ]; then 
	echo "bad arguments"; 
	exit 1; 
fi

# reset the args
eval set -- $ARGS

print_help() {
	echo '
	-T | --tcli ) 	transcli binary	[./transcli/transcli]
	-P | --preset ) preset xml
	-I | --idir ) 	input dir
	-O | --odir ) 	output dir		[./out]
	-L | --ldir ) 	log dir			[./log]
	-H | --help )   help
	-w | --overwrite )  force overwrite output if already exist [false]
	-s | --simulate )   just print cmdl, not really excute [false]
	'
}

# parse the args
while true; do
	case $1 in
	-T | --tcli )   	TRANSCLI="$2"; 	shift 2;;
	-P | --preset ) 	PRESET="$2"; 	shift 2;;
	-I | --idir ) 		IDIR="$2"; 		shift 2;;
	-O | --odir ) 		ODIR="$2"; 		shift 2;;
	-L | --ldir ) 		LDIR="$2"; 		shift 2;;
	-h | -H | --help ) 	print_help;		shift;		exit 1;;
	-w | --overwrite ) 	OVERWRITE=1;	shift;;
	-s | --simulate ) 	SIMULATE=1;		shift;;
	-- ) 					            shift; 		break;;
	*  )	echo "unrecognized option '$1'";		exit 1;;
	esac
done

if [ ! -f ${TRANSCLI} ]; then 
	echo "executable '${TRANSCLI}' not exist"; 
	exit 1;
fi

if [ ! -f ${PRESET} ]; then 
	echo "preset xml '${PRESET}' not exist"; 
	exit 1;
fi

if [ ! -d ${IDIR} ]; then 
	echo "input dir '${IDIR}' not exist or not a directory"; 
	exit 1;
fi

# <! create dir($1) if not exist
assert_dir () {
	# param $1	dir path
	# param $2	dir type/comment
	if [ -z $1 ]; then 
		echo "$2 dir must be specified"; 
		exit 1;
	elif [ ! -d $1 ]; then 
		echo "$2 dir '$1' not exist, try to make";
		if ! mkdir -p $1; then
			echo "create $2 dir '$1' failed"
			exit 1
		fi 
	fi
}

assert_dir ${ODIR} "output"
assert_dir ${LDIR} "logfile"

# <! expand $1 to full path
fullpath() {
	# param $1 any path
	echo $(cd `dirname $1`;pwd)/$(basename $1)
}
PRESET=$(fullpath ${PRESET})

echo "--preset='${PRESET}' "
echo "--idir='${IDIR}' "
echo "--odir='${ODIR}' "
echo "--ldir='${LDIR}' "

get_muxfmt_from_xml() {
	# param $1 xml file
	awk '{ if (match($0, /<mux format="(mp4|flv)"/, arr)) {
		printf ".%s", arr[1];
	}}' "$1"
}

get_br_from_xml() {
	# param $1 xml file
	awk '{ if (match($0, /<video>/, ch)) {
		while(getline nl) {
			if (match(nl, /<codec.*bitrate="([0-9]+[bkmBKM])".*>/, arr)) {
				printf "_%s_%s", "br", arr[1];
				exit;
			}
			if (match(nl, /<video\/>/)) {
				exit;
			}
		}
	}}' "$1"
}


get_fps_from_log() {
	# param $1 log file
	awk '/\[INFO\]/ { if (match($0, /.*MP4Box.*-fps ([0-9]+).*/, field)) {
		printf "_[%s.%s]", "fps", field[1];
		exit 0;
	}}' "$1"
}


get_wh_from_log() {
	# param $1 log file
	awk '/\[INFO\]/ { if (match($0, /.*ffmpeg.*scale=([0-9]+):([0-9]+).*/, wh)) {
		printf "_[%s.%sx%s]", "wxh", wh[1], wh[2];
		exit 0;
	}}' "$1"
}

for video in ${IDIR}/*; do
	ivname=$(basename ${video})
	ivext=${ivname##*.}
	if [ ${ivext} == log ]; then
		echo "jump logfile '${ivname}'"
		continue
	fi
	ovpath=${ODIR}/${ivname%.*}.tmp
	logfile=${LDIR}/${ivname}.log
	logfile=$(fullpath ${logfile})
	ivpath=$(fullpath ${video})
	ovpath=$(fullpath ${ovpath})
	cmdl="${TRANSCLI} -p ${PRESET} -i ${ivpath} -o ${ovpath} -l ${logfile} 2>&1"
	echo ${cmdl}; 
	if [ $SIMULATE -eq 0 ]; then ${cmdl}; fi

	if [ $? -ne 0 ]; then
		echo "transcli failed!!!"
	else
		ovdst=${ovpath%.tmp}
		ovdst+=`get_br_from_xml ${PRESET}`
		#ovdst+=`get_wh_from_log ${logfile}`
		#ovdst+=`get_fps_from_log ${logfile}`
		ovdst+=`get_muxfmt_from_xml ${PRESET}`
		cmdl="mv -v ${ovpath} ${ovdst}"
		echo ${cmdl};
		if [ $SIMULATE -eq 0 ]; then ${cmdl}; fi
	fi
done
