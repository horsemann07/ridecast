import sys                  ;
import os                   ;
import subprocess           ;
import re                   ;
from collections import namedtuple ;

import struct
import time
import datetime
import random
import subprocess
from timeit import Timer
import cProfile
import pstats
#import StringIO
import pdb
import numpy as np

CONTAINER_FW    = sys.argv[1]
CONTAINER_RAM_LOADER = sys.argv[2]
CC33XX_CONF_FILE = sys.argv[3]
OUTPATH         = sys.argv[4]

def main_function():
    print(CONTAINER_FW)
    print(CONTAINER_RAM_LOADER)
    print(CC33XX_CONF_FILE)
    dtype = np.dtype('B')

    #read the fw container
    with open(CONTAINER_FW, "rb") as container_fw:
        container_fw_array = np.fromfile(container_fw,dtype)

    #read the loader container
    with open(CONTAINER_RAM_LOADER, "rb") as container_ram_loader:
        container_loader_array = np.fromfile(container_ram_loader,dtype)

    #read cc33xx conf data
    with open(CC33XX_CONF_FILE, "rb") as cc33xx_conf_file:
        cc33xx_conf_array = np.fromfile(cc33xx_conf_file,dtype)

    text = "    char gFWbuffer[] = {"
    j = 0;
    for i in container_fw_array:	
        text = text + ("0x%x," % (container_fw_array[j]))
        j = j+1
    text = text[0:(len(text) - 1)] + ",0x0,0x0,0x0,0x0};";

    text2 = "    char gRAMbootBuffer[] = {"
    j = 0;
    for i in container_loader_array:	
        text2 = text2 + ("0x%x," % (container_loader_array[j]))
        j = j+1
    text2 = text2[0:(len(text2) - 1)] + "};";

    text3 = "    char gCC33xxConfBuffer[] = {"
    j = 0;
    for i in cc33xx_conf_array:	
        text3 = text3 + ("0x%x," % (cc33xx_conf_array[j]))
        j = j+1
    text3 = text3[0:(len(text3) - 1)] + "};";


    print(OUTPATH)
    
    NEWOUTPATH = OUTPATH + "containers.h";

    hfile = open(NEWOUTPATH,"w");
    hfile.write("#ifndef CONTAINERS_H\r\n");
    hfile.write("#define CONTAINERS_H\r\n");
    hfile.write("\r\n\r\n" + text);
    hfile.write("\r\n");
    hfile.write("\r\n\r\n" + text2);
    hfile.write("\r\n");
    hfile.write("\r\n\r\n" + text3);
    hfile.write("\r\n");
    hfile.write("#endif\r\n");
    hfile.close()




if __name__ == "__main__":
    main_function()




