# Made by @xdavidhu (github.com/xdavidhu, https://xdavidhu.me/)

import serial
import io
import os,sys
import subprocess
import signal
import time
import win32pipe, win32file
defBaud=250000 #921600
serialportInput=""
try:    
    serialportInput = input("[?] Select a serial port (default '/dev/ttyUSB0 or COM7'): ")
    if serialportInput == "":
        serialport = "/dev/ttyUSB0"        
        serialport = "COM7"
    else:
        serialport = serialportInput
except KeyboardInterrupt:
    print("\n[+] Exiting...")
    exit()

boardRateInput=""
try:
    canBreak = False
    while not canBreak:
        boardRateInput = input("[?] Select a baudrate (default '921600'): ")
        if boardRateInput == "":
            boardRate = defBaud #250000
            canBreak = True
        else:
            try:
                boardRate = int(boardRateInput)
            except KeyboardInterrupt:
                print("\n[+] Exiting...")
                exit()
            except Exception as e:
                print("[!] Please enter a number!")
                continue
            canBreak = True
except KeyboardInterrupt:
    print("\n[+] Exiting...")
    exit()

isWin=os.name == 'nt'

if not isWin: 
    filenameInput=""
    try:
        filenameInput = input("[?] Select a filename (default 'capture.pcap'): ")
        if filenameInput == "":
            filename = "capture.pcap"
        else:
            filename = filenameInput
    except KeyboardInterrupt:
        print("\n[+] Exiting...")
        exit()


canBreak = False

print("using "+serialport)
while not canBreak:
    try:
        ser = serial.Serial(serialport, boardRate)
        canBreak = True
    except KeyboardInterrupt:
        print("\n[+] Exiting...")
        exit()
    except Exception as e:
        print("[!] Serial connection failed... Retrying..."+str(e))
        time.sleep(2)
        continue

print("[+] Serial connected. Name: " + ser.name)


# check = 0
# while check == 0:
#     line = ser.readline()
#     if b"<<START>>" in line:
#         check = 1
#         print("[+] Stream started...")
#     #else: print '"'+line+'"'

# print("[+] Starting up wireshark...")


    
if not isWin:
    f = open(filename,'wb')
    cmd = "tail -f -c +0 " + filename + " | wireshark -k -i -"
    p = subprocess.Popen(cmd, stdout=subprocess.PIPE,shell=True, preexec_fn=os.setsid)

if isWin:
    cmd=['C:\Program Files\Wireshark\Wireshark.exe', r'-i\\.\pipe\wireshark','-k']
    # cmd=['C:\Program Files\Wireshark\Wireshark.exe', r'-i\\.\pipe\wireshark','-k','-wcapture.pcap','-b','files:2','-b','filesize:20']
    p=subprocess.Popen(cmd)

if isWin:
    pipe = win32pipe.CreateNamedPipe(
        r'\\.\pipe\wireshark',
        win32pipe.PIPE_ACCESS_OUTBOUND,
        win32pipe.PIPE_TYPE_MESSAGE | win32pipe.PIPE_WAIT,
        1, 65536, 65536,
        300,
        None)
    win32pipe.ConnectNamedPipe(pipe, None)
    print("Created Named Pipe")
    # ser.write(b"F2")
    # ser.write(b"OB")    
    # ser.write(b"S1")
try:
    while True:
        ch = ser.read()   
        #print(bytes(ch).hex())
        if isWin:
            win32file.WriteFile(pipe,ch)
        else:
            f.write(ch)
            f.flush()

except KeyboardInterrupt:
    print("[+] Stopping...")
    os.killpg(os.getpgid(p.pid), signal.SIGTERM)

if not isWin:
    f.close()
ser.close()
print("[+] Done.")
