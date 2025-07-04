#!/usr/bin/env bash
# Usage: rerun.sh [-dryrun] [-nostop] [-d sec] [-c vep-config.txt] [-xv vep ...] [-lv vep ... | -lp vep tile partition ...] [-sv vep ...] [-sp vep tile partition ...]
#
# -d schedule the TDM update after sec seconds (default 2)
#
# by default vep-config.txt is generated from all vep_*/vep-config.txt
# with -c use the specified vep-config.txt instead
# -c is required when using -lp/-sp, and is optional when using -lv/-sv
#
# -xv: exclude these veps (ignore these directories)
#
# default: no -lv no -lp: load all veps in the vep_* directories
# -lv: load veps
# -lp: load tile/partitions 
# -l* without arguments means load no vep/partition; this is useful in combination with -s*
# note that veps/partions that have been loaded previously remain in memory
# note that loading a partition includes clearing its shared region
# note that loading an entire vep additionally includes clearing its shared region
#
# default: no -s*: resume all suspended veps/partitions, keep running veps/partitions running
# -sv suspend all veps listed, and continue/resume all those that are not
# -sp suspend all partitions listed, and continue/resume all those that are not
# (-s* without arguments is thus the same as no -s*)
# -lv and -sv are converted to corresponding -lp and -sp
#
# default: no -nostop: stop partitions before any reloading and suspending/resuming
# note that stopping a partition does not affect its state (incl. shared region), only its timing
# with -nostop, do not stop partitions during reconfiguration
#
# dryrun is useful for debugging only; it generates all files required for running but doesn't run
#
### WARNING HARDCODED ADDRESSES ###
# these should be consistent with vep_0/libbsp/include/platform.h

##################################################
# -nostop -lv  -sv  effect (similar for -lp -sp)
##################################################
#                   DEFAULT: stop all running veps, load code of all veps in the config, schedule & start all
#              -sv  stop all veps, load code of all veps in the config, schedule all veps except those listed with -sp
#         -lv       stop all veps, load specified veps, schedule all veps
#         -lv  -sv  stop all veps, load specified veps, schedule all veps except those listed with -sv
# -nostop           load all veps [WARNING]
# -nostop -lv       load specified veps [WARNING]
# -nostop      -sv  load all veps [WARNING], schedule all veps except those listed with -sv
# -nostop -lv  -sv  load specified veps [WARNING], suspend (don't schedule) veps listed with -sv
#
# WARNING: loading a (new) vep over a running vep will likely lead to errors
# since its code is overwritten while it is executing; instead
# 1- either don't load running veps by excluding them with the -lv flag
# 2- or suspend veps (rerun -nostop -sv) before loading them
#
##################################################

TOOLS=${TOOLS:-./tools}
DYNLOAD=${DYNLOAD:-$TOOLS/dynload-0.06}
PLATFORMCONFIG=${PLATFORMCONFIG:-./platformconfig-v0.05}
# ensure ./ in case . isn't in PATH
PLATFORMMHZ=40000000 # used in calculation of delay
USAGE="Usage: $0 [-dryrun] [-nostop] [-d sec] [-c vep-config.txt] [-xv vep ...] [-lv vep ... | -lp tile partition ...] [-sv vep ...] [-sp tile partition ...]"

if [ ${USER} != "student" ] ; then
  echo "$0: this script must be run as student (not root)" 1>&2
  exit 1
fi

if [ ! -d $TOOLS ] ; then
  echo "$0: error: must run at top-level"
  exit 1
fi

if [ "$1" == "-dryrun" ] ; then
  DRYRUN=1
  shift
else
  DRYRUN=0
fi

# no command line arguments: stop all
if [ "$1" == "-nostop" ] ; then
  STOP=0
  shift
else
  STOP=1
fi

if [ "$1" == "-d" ] ; then
  shift
  if [ "$1" == "" ] ; then
    echo $USAGE 1>&2
    echo "$0: error: missing delay" 1>&2
    exit 1
  fi
  SEC=$1
  shift
  if [ $SEC == 0 ] ; then
    echo "$0: warning: reconfiguration on different cores may be unsynchronised" 1>&2
    DELAY=1 # one cycle
  else
    DELAY="math.ceil($SEC*$PLATFORMMHZ)"
  fi
else
  DELAY="math.ceil(2*$PLATFORMMHZ)"
fi

VEPCONFIG=
if [ "$1" == "-c" ] ; then
  if [ -f "$2" ] ; then
    VEPCONFIG=$2
    shift 2
  else
    echo $USAGE 1>&2
    echo "$0: error: cannot open file: $2" 1>&2
    exit 1
  fi
# else generate vep-config.txt from the veps's vep-config.txt
fi

if [ "$1" == "-xv" ] ; then
  # exclude these partitions 
  shift
  while true ; do
    case ${1} in
    [0-9]*)
      EXCLUDEVEP="$EXCLUDEVEP $1"
      shift 1;;
    *) break;;
    esac
  done
fi

declare -a LOADV
declare -a LOADT
declare -a LOADP
declare -A PARTITIONVEPMAP
LOADALL=1
rm -f $TOOLS/vep-config.txt
if [ "$VEPCONFIG" != "" ] ; then
  cp $VEPCONFIG $TOOLS/vep-config.txt
else
  for v in {1..30} ; do  # max 30 veps in this directory
    if [ -d vep_$v ] ; then
      SKIP=0
      for xv in $EXCLUDEVEP ; do [ "$xv" == "$v" ] && SKIP=1 && break ; done
      if [ $SKIP == 0 ] ; then
        sed --e "s+^+vep $v +g" vep_$v/vep-config.txt >> $TOOLS/vep-config.txt
      fi
    fi
  done
fi
if [ "$1" == "-lv" ] ; then
  # load vep, i.e. all its partitions
  LOADALL=0
  shift
  while true ; do
    # should check that $1 isn't in EXCLUDEVEP
    case ${1} in
    [0-9]*)
      LOADV+=($1);
      for i in vep_$1/partition_* ; do
        T=`echo $i | sed 's+vep_.*/partition_\([0-9][0-9]*\)_\([0-9][0-9]*\)+\1+'`
        LOADT+=("$T")
        P=`echo $i | sed 's+vep_.*/partition_\([0-9][0-9]*\)_\([0-9][0-9]*\)+\2+'`
        LOADP+=("$P")
        KEY="$T"_$P
        PARTITIONVEPMAP[$KEY]+=$1
      done
      shift 1;;
    *) break;;
    esac
  done
elif [ "$1" == "-lp" ] ; then
  if [ "$VEPCONFIG" == "" ] ; then
    echo $USAGE 1>&2
    echo "$0: error: -lp requires -c vep-config.txt" 1>&2
    exit 1
  fi
  LOADALL=0
  shift
  while true ; do
     # should check that $1 isn't in EXCLUDEVEP
    case ${1}pp${2} in
    [0-9]*pp[0-9]*)
      LOADT+=($2);
      LOADP+=($3);
      KEY="$2"_$3
      PARTITIONVEPMAP[$KEY]+=$1
      shift 3;;
    *) break;;
    esac
  done
else # LOADALL = 1
  for v in {1..20} ; do  # max 20 veps in this directory
    SKIP=0
    for xv in $EXCLUDEVEP ; do [ "$xv" == "$v" ] && SKIP=1 && break ; done
    if [ $SKIP == 0 -a -d vep_$v ] ; then
      LOADV+=("$v")
      for i in vep_$v/partition_* ; do
        T=`echo $i | sed 's+vep_.*/partition_\([0-9][0-9]*\)_\([0-9][0-9]*\)+\1+'`
        LOADT+=("$T")
        P=`echo $i | sed 's+vep_.*/partition_\([0-9][0-9]*\)_\([0-9][0-9]*\)+\2+'`
        LOADP+=("$P")
        KEY="$T"_$P
        PARTITIONVEPMAP[$KEY]+=$v
      done
    fi
  done
fi
# debug, print partition->vep map
#for key in "${!PARTITIONVEPMAP[@]}"; do echo "$key => ${PARTITIONVEPMAP[$key]}"; done

# suspend
# if -sv is used with -lp (rather than -lv) then we assume they are consistent
# (i.e. all partitions suspended with -sv are in vep-config.txt and loaded)
# similar for -sv with -c
if [ "$1" == "-sv" ] ; then
  shift
  SUSPEND="suspend"
  [ "$VEPCONFIG" != "" ] && echo "$0: warning: using -c or -lp with -sv; their consistency is not checked"
  while true ; do
    case $1 in
    [0-9]*)
      for i in vep_$1/partition_* ; do
        SUSPEND="$SUSPEND $1 `echo $i | sed 's+vep_.*/partition_\([0-9][0-9]*\)_\([0-9][0-9]*\)+\1 \2+'`"
      done
      shift 1;;
    *) break;;
    esac
  done
elif [ "$1" == "-sp" ] ; then
  shift
  SUSPEND="suspend"
  [ ${#LOADV[@]} -gt 0 ] && echo "$0: warning: using -lv with -sp; their consistency is not checked"
  while true ; do
    case $1 in
    [0-9]*) SUSPEND="$SUSPEND $1"; shift;;
    *) break;;
    esac
  done
else
  SUSPEND=
fi

if [ "$*" != "" ] ; then
  # error since usually due to missing -c
  echo $USAGE 1>&2
  echo "$0: error: trailing arguments: $*"
  exit 1
fi

# (re)make state.json
(
  cd $TOOLS
  # vep-config.txt is the concatenation of all active VEPs
  rm -f state.json vep-config.cmd
  [ -x generate-json ] || make
  ./generate-json vep-config.txt -json $SUSPEND > vep-config.cmd &&
  cat vep-config.cmd | $PLATFORMCONFIG interactive > /dev/null
)
if [ $? != 0 ] ; then
  echo "$0: *** cannot load/start all VEPs/partitions" 1>&2
  echo "$0: *** this is probably due to conflicting memory regions or TDM slot allocations" 1>&2
  echo "$0: *** check for the error messages above the memory or TDM allocation overview" 1>&2
  echo "$0: *** either reallocate memory regions or TDM slots, or use -xv to not run some VEPs" 1>&2
  exit 1
fi

if [ $DRYRUN == 1 ] ; then
  echo "$0: info: dryrun"
  exit 0
fi

[ $STOP == 1 ] && echo "$0: info: stop all partitions"
[ "$NOLOADV" != "" ] && echo "$0: info: ignoring VEP directories $NOLOADV"
if [ $LOADALL == 1 ] ; then
  /bin/echo -n "$0: info: (re)loading all partitions (VEPs: ${LOADV[@]}, partitions: "
else
  /bin/echo -n "$0: info: (re)loading partitions (VEPs: ${LOADV[@]}, partitions: "
fi
for (( i=0; i < ${#LOADT[@]}; i++ )) ; do
  /bin/echo -n "${PARTITIONVEPMAP[${LOADT[$i]}_${LOADP[$i]}]}/${LOADT[$i]}_${LOADP[$i]} "
done
echo ")"
if [ "$SUSPEND" != "" ] ; then
  echo "$0: info: $SUSPEND"
fi

if [ $STOP == 1 ] ; then
  sudo $DYNLOAD 0 stop
  sudo $DYNLOAD 1 stop
  sudo $DYNLOAD 2 stop
fi

if [ $LOADALL == 1 ] ; then
  # ignores veps that are not in the json file
  sudo $DYNLOAD 0 json-load $TOOLS/state.json all
  sudo $DYNLOAD 1 json-load $TOOLS/state.json all
  sudo $DYNLOAD 2 json-load $TOOLS/state.json all
  # clear all partition & vep shared memories
  for v in {1..20} ; do  # max 20 veps in this directory
    if [ -d vep_$v ] ; then
    SKIP=0
      for xv in $EXCLUDEVEP ; do [ "$xv" == "$v" ] && SKIP=1 && break ; done
      [ $SKIP == 0 ] || continue;
      for F in vep_$v/*/dynload-clear.cmd ; do
        cat $F | while read -r line ; do
          sudo $DYNLOAD $line
        done
      done
    fi
  done
else
  for (( i=0; i < ${#LOADT[@]}; i++ )) ; do
    if [ $i -le ${#LOADP[@]} ] ; then
      tl="${LOADT[$i]}"
      pt="${LOADP[$i]}"
      tlpt="${tl}_${pt}"
      # only load specified partitions
      # note that previously loaded partitions that are not overwritten remain in memory and may be scheduled
      sudo $DYNLOAD $tl json-load $TOOLS/state.json $pt
      # clear private shared region for each tile+partition that we're loading
      # the shared memories are NOT cleared (since other partitions may be running)
      if [ $STOP == 0 ] ; then
        F=vep_${PARTITIONVEPMAP[${LOADT[$i]}_${LOADP[$i]}]}/partition_${tlpt}/dynload-clear.cmd
        if [ -f $F ] ; then
          cat $F | while read -r line ; do
            echo sudo $DYNLOAD $line
          done
        else
          echo "$0: error: cannot clear memory because $F does not exist; either use ./run.sh or make first"
	        exit 1
        fi
      fi
    fi
  done
fi

# upload the new TDM schedules
sudo $DYNLOAD 0 json-tdm $TOOLS/state.json
sudo $DYNLOAD 1 json-tdm $TOOLS/state.json
sudo $DYNLOAD 2 json-tdm $TOOLS/state.json

# program the time at which to switch to the new the TDM schedule
# here, it is at the first TDM table revolution two seconds from now
# WARNING HARDCODED ADDRESS
VAL=$(sudo $DYNLOAD 0 read 0x08FC0000 2)
NEXT1=$(python3 -c "import math; print(hex(0x$VAL+$DELAY))")
NEXT2=$(python3 -c "import math; print(0x$VAL+$DELAY,',',(0x$VAL+$DELAY)>>32,'/',(0x$VAL+$DELAY)&0xFFFFFFFF)")
echo "$0: reconfiguring TDM schedule at RISC-V clock cycle $NEXT1 ($NEXT2)"
sudo $DYNLOAD 0 schedule_switch_time ${NEXT1}
sudo $DYNLOAD 1 schedule_switch_time ${NEXT1}
sudo $DYNLOAD 2 schedule_switch_time ${NEXT1}

if [ `ps u| grep readout.sh | grep -v grep | wc -l` = "0" ] ; then
  echo "$0: warning: no readout.sh running"
fi

# leave for debugging
#rm -f $TOOLS/vep-config.{cmd,txt} $TOOLS/state.json