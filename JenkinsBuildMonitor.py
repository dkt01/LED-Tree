import jenkinsapi
from jenkinsapi.jenkins import Jenkins
import time
import datetime
import socket

PATTERN_DONE      = 3 # Spin
PATTERN_RUN       = 1 # Pulse
PATTERN_PROGRESS  = 2 # Progress

COLOR_GOOD = 0x00FF00
COLOR_BAD  = 0xFF0000

passing = True
curRed = 0x00
curGreen = 0x00
curBlue = 0x00
curPattern = 0x03
curParam = 0x00

# Set URL and job name for server
URL     = "http://localhost:8080"
JOBNAME = "TEST"
REFRESHINTERVAL = 10 # seconds

TREEIP = "192.168.1.253"
TREEPORT = 8733

def get_server_instance():
    server = None
    try:
        server = Jenkins(URL)
    except Exception as e:
        print e
        server = None
    return server

def sendControl(red=0x00, green=0x00, blue=0x00, patttern=0x03, param=0x00):
    try:
        message = chr(red) + chr(green) + chr(blue) + chr(patttern) + chr(param)
        treeSock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        treeSock.setblocking(0)
        treeSock.sendto(message, (TREEIP,TREEPORT))
    except Exception as e:
        print e

while(True):
    server = get_server_instance()
    if (server != None and server.has_job(JOBNAME)):
        job = server.get_job(JOBNAME)
        lastBuild = job.get_last_build_or_none()
        if(lastBuild != None):
            passing = job.get_last_completed_build().get_status()

            # Set color
            color = COLOR_BAD
            if(passing == jenkinsapi.constants.STATUS_SUCCESS):
                color = COLOR_GOOD
            curRed   = ((color >> 16) & 0xFF)
            curGreen = ((color >> 8 ) & 0xFF)
            curBlue  = (color & 0xFF)

            curParam = 100
            if(lastBuild.is_running()):
                completeBuild = job.get_last_completed_build()
                startTime = lastBuild._data['timestamp']/1000 # Unix timestamp (in milliseconds)
                startTime = datetime.datetime.utcfromtimestamp(startTime) # Convert to datetime
                curTime = datetime.datetime.utcnow()
                curDuration = (curTime - startTime).total_seconds()
                prevDuration = lastBuild._data['estimatedDuration']/1000 # Milliseconds
                progress = float(curDuration)/float(prevDuration)
                if(progress > 1.0):
                    curPattern = PATTERN_RUN
                else:
                    curPattern = PATTERN_PROGRESS
                    curParam = int(progress*100)
            else:
                curPattern = PATTERN_DONE
        else:
            print "Error retrieving job"
    print "Red:",curRed,"Green:",curGreen,"Blue:",curBlue,"Pattern:",curPattern,"Param:",curParam
    sendControl(curRed, curGreen, curBlue, curPattern, curParam)

    time.sleep(REFRESHINTERVAL);
