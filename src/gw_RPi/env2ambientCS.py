# -*- coding: utf-8 -*-
# 環境センサーをスキャンし、見つけたら300秒ごとにconnectして
# 最新データー(latest data)を取得し、Ambientに送信する
# 1台(Single)のセンサー端末に対応

from bluepy.btle import Peripheral, DefaultDelegate, Scanner, BTLEException, UUID
import bluepy.btle
from threading import Thread, Timer
import time
import sys
import struct
import argparse
import ambient

channelID = 100
writeKey = 'writeKey'

def _OMRON_UUID(val):
    return UUID('%08X-7700-46F4-AA96-D5E974E32A54' % (0x0C4C0000 + val))

devs = {
    'omron': {'desc': 'Short Local Name', 'value': 'Env', 'uuid': _OMRON_UUID(0x3001)},
    'esp32': {'desc': 'Complete Local Name', 'value': 'AmbientEnv-01', 'uuid': 'b0c8c0fa-6d46-11e8-adc0-fa7ae01bbebc'}
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

def timeoutRetry(addr):
    MSG('timer expired (%s)' % addr)
    devThread = scannedDevs[addr]
    devThread.forceDisconnect()
    MSG('Thread disconnected (%s)' % addr)

def send2ambient(am, dataRow):
    if target == 'esp32':
        (seq, temp, humid, press) = struct.unpack('<Bhhh', dataRow)
        MSG(seq, temp / 100, humid / 100, press / 10)
        ret = am.send({'d1': temp / 100, 'd2': humid / 100, 'd3': press / 10})
        MSG('sent to Ambient (ret = %d)' % ret.status_code)
    else:
        (seq, temp, humid, light, uv, press, noise, discom, heat, batt) = struct.unpack('<BhhhhhhhhH', dataRow)
        MSG(seq, temp / 100, humid / 100, light, uv / 100, press / 10, noise / 100, discom / 100, heat / 100, batt / 1000)
        ret = am.send({'d1': temp / 100, 'd2': humid / 100, 'd3': press / 10, 'd4': batt / 1000, 'd5': light, 'd6': noise / 100})
        MSG('sent to Ambient (ret = %d)' % ret.status_code)

class EnvSensor(Thread, Peripheral):
    def __init__(self, dev):
        Peripheral.__init__(self)
        Thread.__init__(self)
        self.setDaemon(True)
        self.dev = dev
        self.isConnected = False
        self.am = ambient.Ambient(channelID, writeKey)

    def run(self):
        while True:
            t = Timer(30, timeoutRetry, [self.dev.addr])
            t.start()
            while self.isConnected == False:  # つながるまでconnectする
                try:
                    self.connect(self.dev)
                    self.isConnected = True
                except BTLEException as e:
                    pass
            MSG('connected to ', self.dev.addr)
            try:
                latestDataRow = self.getCharacteristics(uuid=devs[target]['uuid'])[0]
                dataRow = latestDataRow.read()
                send2ambient(self.am, dataRow)
                t.cancel()
                time.sleep(300.0)
            except BTLEException as e:
                MSG('BTLE Exception while getCharacteristics on ', self.dev.addr)
                MSG('type:' + str(type(e)))
                MSG('args:' + str(e.args))
                self.disconnect()
                self.isConnected = False
                t.cancel()

    def forceDisconnect(self):
        if self.isConnected:
            self.disconnect()
        self.isConnected = False

scannedDevs = {}

class ScanDelegate(DefaultDelegate):
    def __init__(self):
        DefaultDelegate.__init__(self)

    def handleDiscovery(self, dev, isNewDev, isNewData):
        if isNewDev:
            for (adtype, desc, value) in dev.getScanData():  # スキャンデーターを調べる
                if desc == devs[target]['desc'] and value == devs[target]['value']:  # 対象を見つけたら
                    if dev.addr in scannedDevs.keys():  # すでに見つけていたらスキップ
                        return
                    MSG('New %s %s' % (value, dev.addr))
                    devThread = EnvSensor(dev)  # EnvSensorクラスのインスタンスを生成
                    scannedDevs[dev.addr] = devThread
                    devThread.start()  # スレッドを起動

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('-d',action='store_true', help='debug msg on')
    parser.add_argument('-o',action='store_true', help='device is omron env sensor')

    args = parser.parse_args(sys.argv[1:])

    global Debugging
    Debugging = args.d
    bluepy.btle.Debugging = args.d

    global target
    if args.o:
        target = 'omron'

    scanner = Scanner().withDelegate(ScanDelegate())
    while True:
        try:
            scanner.scan(5.0) # スキャンする。デバイスを見つけた後の処理はScanDelegateに任せる
        except BTLEException:
            MSG('BTLE Exception while scannning.')

if __name__ == "__main__":
    main()
