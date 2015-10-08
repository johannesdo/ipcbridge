from sys import stdin
import time
import signal
import threading

import ipcbridge

loop = 1

#can't still be shut down properly, because interpreter blocks in C on ipcbridge.read()
#maybe implement read with timeout?


def handler(signum, frame):
    global loop
    loop = 0
    exit(0)

signal.signal(signal.SIGINT, handler)

print("Recv")
class RecvThread(threading.Thread):
    def run(self):
        while True: 
            msg = ipcbridge.read()
            print(msg)


RecvT = RecvThread()
RecvT.daemon = True
RecvT.start()


print("Send")
class SendThread(threading.Thread):
    def run(self):
        while True:
            line = stdin.readline() 
            ipcbridge.send(line) 

SendT = SendThread()
SendT.daemon = True
SendT.start()

print("startup done")
while loop and threading.active_count() > 0:
    time.sleep(0.1)


