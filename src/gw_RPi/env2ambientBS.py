# -*- coding: utf-8 -*-
# 環境センサーをLimited Broadcasterモードにして
# 10秒アドバタイズ、290秒休み(開発中は50秒休み)に設定
# 常時スキャンし、データーを取得したらAmbientに送信する
# 1台(Single)のセンサー端末に対応

from bluepy.btle import Peripheral, DefaultDelegate, Scanner, BTLEException, UUID
import bluepy.btle
import sys
import struct
from datetime import datetime
import argparse
import ambient

channelID = 100
writeKey = 'writeKey'
am = ambient.Ambient(channelID, writeKey)
companyID = 'ffff'

Debugging = False
def DBG(*args):
    if Debugging:
        msg = " ".join([str(a) for a in args])
        print(msg)
        sys.stdout.flush()

Verbose = True
def MSG(*args):
    if Verbose:
        msg = " ".join([str(a) for a in args])
        print(msg)
        sys.stdout.flush()

def send2ambient(dataRow):
    if companyID == 'ffff':
        (temp, humid, press) = struct.unpack('<hhh', bytes.fromhex(dataRow))
        MSG(temp / 100, humid / 100, press / 10)
        ret = am.send({'d1': temp / 100, 'd2': humid / 100, 'd3': press / 10})
        MSG('sent to Ambient (ret = %d)' % ret.status_code)
    else:
        (temp, humid, light, uv, press, noise, accelX, accelY, accelZ, batt) = struct.unpack('<hhhhhhhhhB', bytes.fromhex(dataRow))
        MSG(temp / 100, humid / 100, light, uv / 100, press / 10, noise / 100, (batt + 100) / 100)
        ret = am.send({'d1': temp / 100, 'd2': humid / 100, 'd3': press / 10, 'd4': (batt + 100) / 100, 'd5': light, 'd6': noise / 100})
        MSG('sent to Ambient (ret = %d)' % ret.status_code)

class ScanDelegate(DefaultDelegate):
    def __init__(self):
        DefaultDelegate.__init__(self)
        self.lastseq = None
        self.lasttime = datetime.fromtimestamp(0)

    def handleDiscovery(self, dev, isNewDev, isNewData):
        if isNewDev or isNewData:
            for (adtype, desc, value) in dev.getScanData():
                if desc == 'Manufacturer' and value[0:4] == companyID:
                    delta = datetime.now() - self.lasttime
                    if value[4:6] != self.lastseq and delta.total_seconds() > 11: # アドバタイズする10秒の間に測定が実行されseqが加算されたものは捨てる
                        self.lastseq = value[4:6]
                        self.lasttime = datetime.now()
                        send2ambient(value[6:])

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('-d',action='store_true', help='debug msg on')
    parser.add_argument('-o',action='store_true', help='device is omron env sensor')

    args = parser.parse_args(sys.argv[1:])

    global Debugging
    Debugging = args.d
    bluepy.btle.Debugging = args.d

    global companyID
    if args.o:
        companyID = 'd502'

    scanner = Scanner().withDelegate(ScanDelegate())
    while True:
        try:
            scanner.scan(5.0) # スキャンする。デバイスを見つけた後の処理はScanDelegateに任せる
        except BTLEException:
            MSG('BTLE Exception while scannning.')

if __name__ == "__main__":
    main()
