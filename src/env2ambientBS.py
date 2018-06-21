# -*- coding: utf-8 -*-
# オムロン環境センサーをLimited Broadcasterモードにして
# 10秒アドバタイズ、290秒休み(開発中は50秒休み)に設定
# 常時スキャンし、データーを取得したらAmbientに送信する
# 1台(Single)のセンサー端末に対応

from bluepy.btle import Peripheral, DefaultDelegate, Scanner, BTLEException, UUID
import bluepy.btle
import sys
import struct
import argparse
import ambient

channelID = 100
writeKey = 'writeKey'
am = ambient.Ambient(channelID, writeKey)

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
    (temp, humid, light, uv, press, noise, accelX, accelY, accelZ, batt) = struct.unpack('<hhhhhhhhhB', bytes.fromhex(dataRow))
    MSG(temp / 100, humid / 100, light, uv / 100, press / 10, noise / 100, (batt + 100) / 100)
    ret = am.send({'d1': temp / 100, 'd2': humid / 100, 'd3': press / 10, 'd4': (batt + 100) / 100, 'd5': light, 'd6': noise / 100})
    MSG('sent to Ambient (ret = %d)' % ret.status_code)

class ScanDelegate(DefaultDelegate):
    def __init__(self):
        DefaultDelegate.__init__(self)
        self.lastseq = None

    def handleDiscovery(self, dev, isNewDev, isNewData):
            if isNewDev or isNewData:
                for (adtype, desc, value) in dev.getScanData():
                    if desc == 'Manufacturer' and value[0:4] == 'd502':
                        if value[4:6] != self.lastseq:
                            self.lastseq = value[4:6]
                            send2ambient(value[6:])

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('-d',action='store_true', help='debug msg on')

    args = parser.parse_args(sys.argv[1:])

    global Debugging
    Debugging = args.d
    bluepy.btle.Debugging = args.d

    scanner = Scanner().withDelegate(ScanDelegate())
    while True:
        try:
            scanner.scan(10.0) # スキャンする。デバイスを見つけた後の処理はScanDelegateに任せる
        except BTLEException:
            MSG('BTLE Exception while scannning.')

if __name__ == "__main__":
    main()
