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
import requests
import time

channelID = 100
writeKey = 'writeKey'
am = ambient.Ambient(channelID, writeKey)

devs = {
    'omron': {'companyID': 'd502'},
    'esp32': {'companyID': 'ffff'}
}
target = 'esp32'

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

def sendWithRetry(data):
    for retry in range(6):  # 10秒間隔で6回リトライして、ダメならこの回は送信しない
        try:
            ret = am.send(data)
            MSG('sent to Ambient (ret = %d)' % ret.status_code)
            break
        except requests.exceptions.RequestException as e:
            MSG('request failed.')
            time.sleep(10)

def send2ambient(dataRow):
    if companyID == 'ffff':
        (temp, humid, press) = struct.unpack('<hhh', bytes.fromhex(dataRow))
        MSG(temp / 100, humid / 100, press / 10)
        sendWithRetry({'d1': temp / 100, 'd2': humid / 100, 'd3': press / 10})
    else:
        (temp, humid, light, uv, press, noise, accelX, accelY, accelZ, batt) = struct.unpack('<hhhhhhhhhB', bytes.fromhex(dataRow))
        MSG(temp / 100, humid / 100, light, uv / 100, press / 10, noise / 100, (batt + 100) / 100)
        sendWithRetry({'d1': temp / 100, 'd2': humid / 100, 'd3': press / 10, 'd4': (batt + 100) / 100, 'd5': light, 'd6': noise / 100})

class ScanDelegate(DefaultDelegate):
    def __init__(self):
        DefaultDelegate.__init__(self)
        self.lastseq = None
        self.lasttime = datetime.fromtimestamp(0)

    def handleDiscovery(self, dev, isNewDev, isNewData):
        if isNewDev or isNewData:
            for (adtype, desc, value) in dev.getScanData():
                # print(adtype, desc, value)
                if target in ('omron', 'esp32'):
                    if desc == 'Manufacturer' and value[0:4] == devs[target]['companyID']:
                        delta = datetime.now() - self.lasttime
                        if value[4:6] != self.lastseq and delta.total_seconds() > 11: # アドバタイズする10秒の間に測定が実行されseqが加算されたものは捨てる
                            self.lastseq = value[4:6]
                            self.lasttime = datetime.now()
                            send2ambient(value[6:])
                elif target in ('microbit', 'microbit+BME280'):
                    if desc == '16b Service Data' and value[0:4] == 'aafe' and value[8:28] == '000000000000616d6269':
                        print(value)
                        seq = (int(value[32:33], 16) & 0xC) >> 2
                        print(seq)
                        if seq != self.lastseq:
                            self.lastseq = seq
                            if target == 'microbit':
                                temp = int(value[38:40], 16)
                                print('Micro:bit %d' % temp)
                                sendWithRetry({'d1': temp})
                            elif target == 'microbit+BME280':
                                temp =  (int(value[37:40], 16) & 0x3FF) / 10
                                humid = ((int(value[35:38], 16) & 0xFFC) >> 2) / 10
                                press = (int(value[32:35], 16) & 0x3FF) + 400
                                print('Micro:bit t: %f, h: %f, p: %f' % (temp, humid, press))
                                sendWithRetry({'d1': temp, 'd2': humid, 'd3': press})

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('-d',action='store_true', help='debug msg on')
    parser.add_argument('-o',action='store_true', help='device is omron env sensor')
    parser.add_argument('-b',action='store_true', help='device is BBC micro:bit')
    parser.add_argument('-bb',action='store_true', help='device is BBC micro:bit + BME280')

    args = parser.parse_args(sys.argv[1:])

    global Debugging
    Debugging = args.d
    bluepy.btle.Debugging = args.d

    global target
    if args.o:
        target = 'omron'
    elif args.b:
        target = 'microbit'
    elif args.bb:
        target = 'microbit+BME280'
    print(target)

    scanner = Scanner().withDelegate(ScanDelegate())
    while True:
        try:
            scanner.scan(5.0) # スキャンする。デバイスを見つけた後の処理はScanDelegateに任せる
        except BTLEException:
            MSG('BTLE Exception while scannning.')

if __name__ == "__main__":
    main()
