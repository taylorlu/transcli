import os
import os.path
import glob
import re
import sys
import math
from time import sleep
import urllib.request
from ftplib import FTP
import json
import urllib

dlurl = "http://api.cloudplay.pptv.com/fsvc/private/1/file/1186916/download_url?download_inner=0&download_expire=864000"
pfid = "1186916"
rootdir = "E:\\ppc\\"
#taskListFile = open(rootdir + "tasks1.txt", "rt")
taskListFile = open(rootdir + "taskids.txt", "rt")

if(taskListFile):
    print("Open file success.")

while True:
    line = taskListFile.readline()
    if not line:
        break
    line = line.strip()
    #words = line.split("|")
    if(len(line) > 0):
        #dlurl = dlurl.replace(pfid, words[3].strip())
        dlurl = dlurl.replace(pfid, line)
        
        #print(words[6])
        #print(dlurl)
        response = urllib.request.urlopen(dlurl).read()
        #print(response)
        obj = json.loads(response.decode('gbk'))
        #print(obj["data"])
        #videodir = rootdir + words[6].strip()
        #if not os.path.exists(videodir):
            #os.makedirs(videodir)
        #cmd = ("curl.exe -o %s \"%s\"" % ("%s%s\\%s.ppc" % (rootdir, words[6].strip(), words[3].strip()), obj['data']))
        cmd = ("curl.exe -o %s \"%s\"" % ("%s\\%s.ppc" % (rootdir, line), obj['data']))
        print(cmd)
        os.popen(cmd).read()
        dlurl = "http://api.cloudplay.pptv.com/fsvc/private/1/file/1186916/download_url?download_inner=0&download_expire=864000"
taskListFile.close()
	