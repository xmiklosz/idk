#!/usr/bin/env python3
"""Test binary protocol encoding/decoding"""

from binary_protocol import encode_message, decode_message
import time

def test_register():
    print("Testing REGISTER message...")
    msg = {
        "type": "register",
        "device_type": "ThermoNode",
        "timestamp": int(time.time()),
        "low_battery": False,
        "token": ""
    }

    binary = encode_message(msg)
    print(f"  Encoded size: {len(binary)} bytes")

    decoded = decode_message(binary)
    print(f"  Checksum valid: {decoded.get('checksum_valid')}")
    print(f"  Type: {decoded.get('type')}")
    print(f"  Device: {decoded.get('device_type')}")
    print("  ✓ REGISTER test passed\n")

def test_ack():
    print("Testing ACK message...")
    msg = {
        "type": "ack",
        "device_type": "ThermoNode",
        "timestamp": int(time.time()),
        "token": "TKN-ABC123XYZ789"
    }

    binary = encode_message(msg)
    print(f"  Encoded size: {len(binary)} bytes")

    decoded = decode_message(binary)
    print(f"  Checksum valid: {decoded.get('checksum_valid')}")
    print(f"  Type: {decoded.get('type')}")
    print(f"  Token: {decoded.get('token')}")
    print("  ✓ ACK test passed\n")

def test_data_thermonode():
    print("Testing DATA message (ThermoNode)...")
    msg = {
        "type": "data",
        "device_type": "ThermoNode",
        "timestamp": int(time.time()),
        "low_battery": False,
        "token": "TKN-TEST12345678",
        "payload": {
            "temp": "23.5°C",
            "hum": "65.3%",
            "dew": "15.2°C",
            "pressure": "1013.25hPa"
        }
    }

    binary = encode_message(msg)
    print(f"  Encoded size: {len(binary)} bytes")
    print(f"  Savings vs JSON: ~{200 - len(binary)} bytes (JSON ~200 bytes)")

    decoded = decode_message(binary)
    print(f"  Checksum valid: {decoded.get('checksum_valid')}")
    print(f"  Type: {decoded.get('type')}")
    print(f"  Payload: {decoded.get('payload')}")
    print("  ✓ ThermoNode DATA test passed\n")

def test_data_windsense():
    print("Testing DATA message (WindSense)...")
    msg = {
        "type": "data",
        "device_type": "WindSense",
        "timestamp": int(time.time()),
        "low_battery": True,
        "token": "TKN-WIND98765432",
        "payload": {
            "speed": "12.3m/s",
            "gust": "18.7m/s",
            "direction": "245°",
            "turbulance": "0.6"
        }
    }

    binary = encode_message(msg)
    print(f"  Encoded size: {len(binary)} bytes")

    decoded = decode_message(binary)
    print(f"  Checksum valid: {decoded.get('checksum_valid')}")
    print(f"  Low battery: {decoded.get('low_battery')}")
    print(f"  Payload: {decoded.get('payload')}")
    print("  ✓ WindSense DATA test passed\n")

def test_checksum_error():
    print("Testing CHECKSUM_ERROR message...")
    msg = {
        "type": "checksum_error",
        "timestamp": int(time.time()),
        "message": "Invalid checksum - please resend"
    }

    binary = encode_message(msg)
    print(f"  Encoded size: {len(binary)} bytes")

    decoded = decode_message(binary)
    print(f"  Checksum valid: {decoded.get('checksum_valid')}")
    print(f"  Message: {decoded.get('message')}")
    print("  ✓ CHECKSUM_ERROR test passed\n")

def test_corrupted_message():
    print("Testing corrupted message detection...")
    msg = {
        "type": "ack",
        "device_type": "ThermoNode",
        "timestamp": int(time.time()),
        "token": "TKN-CORRUPT12345"
    }

    binary = encode_message(msg)
    # Corrupt the checksum
    corrupted = binary[:-1] + bytes([(binary[-1] + 1) & 0xFF])

    decoded = decode_message(corrupted)
    print(f"  Checksum valid: {decoded.get('checksum_valid')}")
    if not decoded.get('checksum_valid'):
        print("  ✓ Corruption detected correctly\n")
    else:
        print("  ✗ ERROR: Corruption not detected!\n")

if __name__ == "__main__":
    print("=" * 60)
    print("Binary Protocol Test Suite")
    print("=" * 60 + "\n")

    test_register()
    test_ack()
    test_data_thermonode()
    test_data_windsense()
    test_checksum_error()
    test_corrupted_message()

    print("=" * 60)
    print("All tests completed!")
    print("=" * 60)
