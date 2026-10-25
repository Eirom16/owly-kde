#!/usr/bin/env python3
"""Process-boundary fixture. Does not touch Wi-Fi, D-Bus or the network."""
import json
import os
import signal
import sys
import time


def emit(**event):
    print(json.dumps(event), flush=True)


def log(value):
    path = os.environ.get("OWLY_TEST_LOG")
    if path:
        with open(path, "a", encoding="utf-8") as stream:
            stream.write(json.dumps(value) + "\n")


log(sys.argv[1:])
command = sys.argv[1]
if command == "doctor":
    emit(hardware=True, missing=[], mode="shared", driver="fixture", issues=[])
elif command == "radio":
    def release(*_):
        log("released")
        sys.exit(0)
    signal.signal(signal.SIGTERM, release)
    emit(event="ready", interface="awdl0")
    while True:
        time.sleep(0.1)
elif command == "browse":
    # Split a JSON record across writes to exercise incremental decoding.
    sys.stdout.write('{"event":"device",')
    sys.stdout.flush()
    time.sleep(0.05)
    sys.stdout.write('"id":"aa:bb:cc:dd:ee:ff","name":"iPhone de prueba"}\n')
    sys.stdout.flush()
    emit(event="device", id="other-id", name="iPhone de prueba")
    emit(event="device", id="another-device", name="MacBook con un nombre largo para probar el diseño")
elif command == "receive":
    emit(event="ready")
    if os.environ.get("OWLY_TEST_INCOMING"):
        emit(event="request", id=7, sender="iPhone de prueba", items=[{"name": "foto de vacaciones.jpg", "size": 1234}])
    for line in sys.stdin:
        answer = json.loads(line)
        log(answer)
        if answer["accept"]:
            emit(event="received", id=answer["id"], paths=json.loads(os.environ.get("OWLY_TEST_RECEIVED", '["/tmp/foto de vacaciones.jpg"]')))
elif command == "send":
    if os.environ.get("OWLY_TEST_SEND_FAIL"):
        print("owly: no AWDL peer answered /Discover", file=sys.stderr, flush=True)
        sys.exit(2)
    if os.environ.get("OWLY_TEST_SLOW"):
        time.sleep(30)
    emit(event="accepted")
    emit(event="progress", sent=50, total=100)
    emit(event="delivered")
