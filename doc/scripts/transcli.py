# -*- coding: cp936 -*-
# !/usr/bin/python
# batch transcoding for the videos in the given dir using specified preset xml
# <jianfengzheng@pptv.com>


def get_muxfmt_from_xml( xml ):
    import re
    fmtExp = r"<mux format=\"(?P<muxfmt>(mp4|flv))\".*/>"
    fmtObj = re.compile( fmtExp )
    with open( xml, 'r' ) as f:
        for line in f:
            fmtMatch = fmtObj.search( line )
            if fmtMatch:
                return fmtMatch.groupdict()["muxfmt"]
    return "mp4"

def get_br_from_xml( xml ):
    import re
    vsObj = re.compile( r"<video>" )
    veObj = re.compile( r"</video>" )
    brExp = r"<codec.*bitrate=\"(?P<br>\d+[bkmBKM])\".*/>"
    brObj = re.compile( brExp )
    with open( xml, 'r' ) as f:
        video_node_start = 0
        video_node_end = 0
        for line in f:
            if not video_node_start and vsObj.search( line ):
                video_node_start = 1
                continue
            if video_node_start and not video_node_end and veObj.search( line ):
                video_node_end = 1
                continue
            if video_node_start and not video_node_end:
                brMatch = brObj.search( line )
                if brMatch:
                    return "_br_" + brMatch.groupdict()["br"] + "bs"
    return "_br_-"


def get_fps_from_log( log ):
    import re
    fpsObj = re.compile( r".*MP4Box.*-fps (?P<fps>\d+).*" )
    with open( log, 'r' ) as f:
        for line in f:
            fpsMatch = fpsObj.search( line )
            if fpsMatch:
                return "_fps_" + fpsMatch.groupdict()["fps"]
    return "_br_-"

def get_wh_from_log( log ):
    import re
    whObj = re.compile( r".*ffmpeg.*scale=(?P<w>\d+):(?P<h>\d+).*" )
    with open( log, 'r' ) as f:
        for line in f:
            whMatch = whObj.search( line )
            if whMatch:
                whdict = whMatch.groupdict()
                return "_{:s}x{:s}".format(whdict["w"], whdict["h"])
    return "_br_-"

def set_windows_env():
    print("work in windows")
    global work_dir, yuv_path
    work_dir = r"D:/work/mpeg4_skip_test/"
    yuv_path = r"yuv"
    import os
    os.chdir(work_dir)
    return

def set_linux_env():
    print("work in linux")
    return

def set_env():
    import platform
    sys = platform.system()
    if sys == "Windows":
        set_windows_env()
    elif sys == "Linux":
        set_linux_env()
    else:
        print("not support system " + sys)
    return

def open_dbg_log( opt ):
    import os,time
    import platform
    buf = "\n\n"
    buf += "time = " + time.strftime( '%z: %Y-%m-%d %X', time.localtime( time.time() ) ) + "\n"
    buf += "system = " + platform.system() + "\n"
    buf += "batch transcoding using python script\n"
    #
    if not os.path.exists( opt.dbglog ):
        with open( opt.dbglog , 'w' ) as dlog:
            dlog.write( buf )
    else:
        with open( opt.dbglog , 'a' ) as dlog:
            dlog.write( buf )
    return

def batch_test( opt ):
    import os
    open_dbg_log( opt )
    for video in os.listdir( opt.idir ):
        if video[-4:] != '.log':
            ivname = os.path.basename(video)
            ivpath = os.path.abspath( os.path.join(opt.idir, video) )
            logfile= os.path.abspath( os.path.join(opt.ldir, ivname) ) + ".log"
            ovpath = os.path.abspath( os.path.join(opt.odir, ivname) ) + ".tmp"
            cmdl = opt.bin + " -p " + opt.xml + " -i " + ivpath + " -o " + ovpath + " -l " + logfile
            print "[execute]", cmdl, "\n"
            if opt.simulate:
                r = 0
            else:
                r = os.system(cmdl)
            if r:
                print "[error] transcli failed!!!"
                return r
            else:
                ovdst = os.path.abspath( os.path.join(opt.odir, ivname) )
                ovdst += get_wh_from_log( logfile )
                #ovdst += get_fps_from_log( logfile )
                ovdst += get_br_from_xml( opt.xml )
                ovdst += "." + get_muxfmt_from_xml( opt.xml )
                print "[rename]", ovpath, "->", ovdst, "\n"
                if os.path.isfile(ovdst) and not opt.overwrite:
                    print ovdst," already exist"
                elif not opt.simulate:
                    os.rename(ovpath, ovdst)
    return 0
    #end for file
#end func. batch_test()


def opt_check(opt):
    if opt.bin and opt.xml and opt.idir and opt.odir and opt.ldir:
        pass
    else:
        print "something un-specified"
        return 1
    opt.bin = os.path.abspath(opt.bin)
    opt.xml = os.path.abspath(opt.xml)
    if not os.path.isfile(opt.bin):
        print "invalid transcli binary"
        return 1
    if not os.path.isfile(opt.xml):
        print "invalid preset xml"
        return 1
    if not os.path.isdir(opt.idir):
        print "invalid input dir"
        return 1
    if not os.path.isdir(opt.odir):
        print "mkdir output dir '{:s}'".format(opt.odir)
        os.makedirs(opt.odir)
    if not os.path.isdir(opt.ldir):
        print "mkdir log idr '{:s}'".format(opt.ldir)
        os.makedirs(opt.ldir)
    return 0


if __name__ == "__main__":
    import os
    from optparse import OptionParser
    parser = OptionParser()
    parser.add_option("-T", "--tcli", dest="bin",
                      default=r"./transcli/transcli.exe",
                      help=r"transcli binary [%default]")
    parser.add_option("-P", "--preset", dest="xml",
                      help="preset xml")
    parser.add_option("-I", "--idir", dest="idir",
                      help="input dir")
    parser.add_option("-O", "--odir", dest="odir",
                      default=r"out",
                      help="output dir [%default]")
    parser.add_option("-L", "--ldir", dest="ldir",
                      default=r"log",
                      help="log dir [%default]")
    parser.add_option("-d", "--dbglog", dest="dbglog",
                      default=r".dbglog",
                      help="script debugging log [%default]")
    parser.add_option("-w", "--overwrite", dest="overwrite",
                      action="store_true", default=False,
                      help="force overwrite output if already exist [%default]")
    parser.add_option("-s", "--simulate", dest="simulate",
                      action="store_true", default=False,
                      help="just print cmdl, not really execute [%default]")
    (opt, args) = parser.parse_args()
    print "option value:\n", opt
    if opt_check(opt):
        exit(1)
    print "==========================================\n"
    batch_test( opt )