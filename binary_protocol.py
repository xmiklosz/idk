import struct
import crc

# Message types (3 bit) - 8 possible values
MSG_REGISTER = 0
MSG_ACK = 1
MSG_DATA = 2
MSG_PING = 3
MSG_PING_RESPONSE = 4
MSG_CHECKSUM_ERROR = 5
MSG_INVALID_TOKEN = 6
MSG_ERROR = 7

# Device types (2 bit) - 4 possible values
DEV_THERMONODE = 0
DEV_WINDSENSE = 1
DEV_RAINDETECT = 2
DEV_AIRQUALITYBOX = 3

DEVICE_MAP = {
    "ThermoNode": DEV_THERMONODE,
    "WindSense": DEV_WINDSENSE,
    "RainDetect": DEV_RAINDETECT,
    "AirQualityBox": DEV_AIRQUALITYBOX,
}

DEVICE_REVERSE = {v: k for k, v in DEVICE_MAP.items()}


def calculate_checksum_binary(data):
    """Calculate CRC32 checksum on binary data"""
    crc32_func = crc.Calculator(crc.Crc32.CRC32)
    return crc32_func.checksum(data)


def pack_header(msg_type, device_type, low_battery=False):
    """
    Pack header byte:
    - bits 0-2: msg_type (3 bits)
    - bits 3-4: device_type (2 bits)
    - bit 5: low_battery (1 bit)
    - bits 6-7: reserved (2 bits)
    """
    header = (msg_type & 0x07) | ((device_type & 0x03) << 3) | ((1 if low_battery else 0) << 5)
    return header


def unpack_header(header_byte):
    """Unpack header byte"""
    msg_type = header_byte & 0x07
    device_type = (header_byte >> 3) & 0x03
    low_battery = bool((header_byte >> 5) & 0x01)
    return msg_type, device_type, low_battery


def encode_token(token_str):
    """Encode token string to 16 bytes (fixed size)"""
    if not token_str:
        return b'\x00' * 16
    return token_str.encode('ascii').ljust(16, b'\x00')[:16]


def decode_token(token_bytes):
    """Decode token from 16 bytes to string"""
    return token_bytes.rstrip(b'\x00').decode('ascii')


def encode_string(s, length):
    """Encode string to fixed length bytes"""
    return s.encode('utf-8').ljust(length, b'\x00')[:length]


def decode_string(b):
    """Decode string from bytes"""
    return b.rstrip(b'\x00').decode('utf-8')


# ============================================================================
# Payload encoders/decoders for each device type
# ============================================================================

def encode_thermonode_payload(payload):
    """
    Encode ThermoNode payload (8 bytes total):
    - temp: -50.0 to 60.0°C (signed 16 bit, scaled by 10) = 2 bytes
    - hum: 0.0 to 100.0% (unsigned 16 bit, scaled by 10) = 2 bytes
    - dew: -50.0 to 60.0°C (signed 16 bit, scaled by 10) = 2 bytes
    - pressure: 800.0 to 1100.0 hPa (unsigned 16 bit, scaled by 100, offset by 800.0) = 2 bytes
    """
    # Parse values from strings like "23.4°C"
    temp = float(payload['temp'].rstrip('°C'))
    hum = float(payload['hum'].rstrip('%'))
    dew = float(payload['dew'].rstrip('°C'))
    pressure = float(payload['pressure'].rstrip('hPa'))

    # Scale and convert to integers
    temp_int = int(round(temp * 10))
    hum_int = int(round(hum * 10))
    dew_int = int(round(dew * 10))
    pressure_int = int(round((pressure - 800.0) * 100))

    # Pack as: signed16, unsigned16, signed16, unsigned16 (big-endian)
    return struct.pack('>hHhH', temp_int, hum_int, dew_int, pressure_int)


def decode_thermonode_payload(data):
    """Decode ThermoNode payload"""
    temp_int, hum_int, dew_int, pressure_int = struct.unpack('>hHhH', data)

    temp = temp_int / 10.0
    hum = hum_int / 10.0
    dew = dew_int / 10.0
    pressure = (pressure_int / 100.0) + 800.0

    return {
        "temp": f"{temp}°C",
        "hum": f"{hum}%",
        "dew": f"{dew}°C",
        "pressure": f"{round(pressure, 2)}hPa"
    }


def encode_windsense_payload(payload):
    """
    Encode WindSense payload (6 bytes total):
    - speed: 0.0 to 50.0 m/s (unsigned 16 bit, scaled by 10) = 2 bytes
    - gust: 0.0 to 70.0 m/s (unsigned 16 bit, scaled by 10) = 2 bytes
    - direction: 0 to 359° (unsigned 16 bit) = 2 bytes
    - turbulance: 0.0 to 1.0 (unsigned 8 bit, scaled by 10) = 1 byte
    - padding: 1 byte
    """
    speed = float(payload['speed'].rstrip('m/s'))
    gust = float(payload['gust'].rstrip('m/s'))
    direction = int(payload['direction'].rstrip('°'))
    turbulance = float(payload['turbulance'])

    speed_int = int(round(speed * 10))
    gust_int = int(round(gust * 10))
    turbulance_int = int(round(turbulance * 10))

    # Pack as: unsigned16, unsigned16, unsigned16, unsigned8, padding
    return struct.pack('>HHHBx', speed_int, gust_int, direction, turbulance_int)


def decode_windsense_payload(data):
    """Decode WindSense payload"""
    speed_int, gust_int, direction, turbulance_int = struct.unpack('>HHHBx', data)

    speed = speed_int / 10.0
    gust = gust_int / 10.0
    turbulance = turbulance_int / 10.0

    return {
        "speed": f"{speed}m/s",
        "gust": f"{gust}m/s",
        "direction": f"{direction}°",
        "turbulance": f"{turbulance}"
    }


def encode_raindetect_payload(payload):
    """
    Encode RainDetect payload (6 bytes total):
    - rainfall: 0.0 to 500.0 mm (unsigned 16 bit, scaled by 10) = 2 bytes
    - soil: 0.0 to 100.0% (unsigned 16 bit, scaled by 10) = 2 bytes
    - flood: 0 to 3 (unsigned 8 bit) = 1 byte
    - duration: 0 to 60 s (unsigned 8 bit) = 1 byte
    """
    rainfall = float(payload['rainfall'].rstrip('mm'))
    soil = float(payload['soil'].rstrip('%'))
    flood = int(payload['flood'])
    duration = int(payload['duration'])

    rainfall_int = int(round(rainfall * 10))
    soil_int = int(round(soil * 10))

    # Pack as: unsigned16, unsigned16, unsigned8, unsigned8
    return struct.pack('>HHBB', rainfall_int, soil_int, flood, duration)


def decode_raindetect_payload(data):
    """Decode RainDetect payload"""
    rainfall_int, soil_int, flood, duration = struct.unpack('>HHBB', data)

    rainfall = rainfall_int / 10.0
    soil = soil_int / 10.0

    return {
        "rainfall": f"{rainfall}mm",
        "soil": f"{soil}%",
        "flood": str(flood),
        "duration": str(duration)
    }


def encode_airqualitybox_payload(payload):
    """
    Encode AirQualityBox payload (6 bytes total):
    - CO2: 300 to 5000 ppm (unsigned 16 bit, offset by 300) = 2 bytes
    - ozone: 0.0 to 500.0 µg/m³ (unsigned 16 bit, scaled by 10) = 2 bytes
    - quality: 0 to 500 AQI (unsigned 16 bit) = 2 bytes
    """
    CO2 = int(payload['CO2'].rstrip('ppm'))
    ozone = float(payload['ozone'].rstrip('µg/m³'))
    quality = int(payload['quality'].rstrip('AQI'))

    CO2_int = CO2 - 300
    ozone_int = int(round(ozone * 10))

    # Pack as: unsigned16, unsigned16, unsigned16
    return struct.pack('>HHH', CO2_int, ozone_int, quality)


def decode_airqualitybox_payload(data):
    """Decode AirQualityBox payload"""
    CO2_int, ozone_int, quality = struct.unpack('>HHH', data)

    CO2 = CO2_int + 300
    ozone = ozone_int / 10.0

    return {
        "CO2": f"{CO2}ppm",
        "ozone": f"{ozone}µg/m³",
        "quality": f"{quality}AQI"
    }


PAYLOAD_ENCODERS = {
    DEV_THERMONODE: encode_thermonode_payload,
    DEV_WINDSENSE: encode_windsense_payload,
    DEV_RAINDETECT: encode_raindetect_payload,
    DEV_AIRQUALITYBOX: encode_airqualitybox_payload,
}

PAYLOAD_DECODERS = {
    DEV_THERMONODE: decode_thermonode_payload,
    DEV_WINDSENSE: decode_windsense_payload,
    DEV_RAINDETECT: decode_raindetect_payload,
    DEV_AIRQUALITYBOX: decode_airqualitybox_payload,
}


# ============================================================================
# Message encoders/decoders
# ============================================================================

def encode_message(msg_dict):
    """
    Encode message dictionary to binary format.
    Format: [header(1)] [timestamp(4)] [token(16)] [payload(variable)] [checksum(4)]
    """
    msg_type_str = msg_dict.get("type", "")
    device_type_str = msg_dict.get("device_type", "")
    low_battery = msg_dict.get("low_battery", False)
    timestamp = msg_dict.get("timestamp", 0)
    token = msg_dict.get("token", "")

    # Map message type string to number
    msg_type_map = {
        "register": MSG_REGISTER,
        "ack": MSG_ACK,
        "data": MSG_DATA,
        "ping": MSG_PING,
        "ping_response": MSG_PING_RESPONSE,
        "checksum_error": MSG_CHECKSUM_ERROR,
        "invalid_token": MSG_INVALID_TOKEN,
        "error": MSG_ERROR,
    }
    msg_type = msg_type_map.get(msg_type_str, MSG_ERROR)

    # Map device type
    device_type = DEVICE_MAP.get(device_type_str, 0)

    # Build message without checksum first
    header = pack_header(msg_type, device_type, low_battery)
    data = struct.pack('>BI', header, timestamp)

    # Add token (16 bytes) for most message types
    if msg_type in (MSG_ACK, MSG_DATA, MSG_PING, MSG_PING_RESPONSE, MSG_INVALID_TOKEN):
        data += encode_token(token)

    # Add payload for data messages
    if msg_type == MSG_DATA:
        payload = msg_dict.get("payload", {})
        encoder = PAYLOAD_ENCODERS.get(device_type)
        if encoder:
            data += encoder(payload)

    # Add message field for error messages
    if msg_type in (MSG_CHECKSUM_ERROR, MSG_ERROR):
        message = msg_dict.get("message", "")
        data += encode_string(message, 64)

    if msg_type == MSG_INVALID_TOKEN:
        reason = msg_dict.get("reason", "")
        data += encode_string(reason, 64)

    # Calculate and append checksum
    checksum = calculate_checksum_binary(data)
    data += struct.pack('>I', checksum)

    return data


def decode_message(data):
    """
    Decode binary message to dictionary.
    """
    if len(data) < 9:  # Minimum: header(1) + timestamp(4) + checksum(4)
        raise ValueError("Message too short")

    # Extract and verify checksum
    received_checksum = struct.unpack('>I', data[-4:])[0]
    data_without_checksum = data[:-4]
    expected_checksum = calculate_checksum_binary(data_without_checksum)

    if received_checksum != expected_checksum:
        # Return a special dict indicating checksum error
        return {"checksum_valid": False}

    # Parse header and timestamp
    header, timestamp = struct.unpack('>BI', data[:5])
    msg_type, device_type, low_battery = unpack_header(header)

    # Map message type number to string
    msg_type_reverse = {
        MSG_REGISTER: "register",
        MSG_ACK: "ack",
        MSG_DATA: "data",
        MSG_PING: "ping",
        MSG_PING_RESPONSE: "ping_response",
        MSG_CHECKSUM_ERROR: "checksum_error",
        MSG_INVALID_TOKEN: "invalid_token",
        MSG_ERROR: "error",
    }

    msg_dict = {
        "checksum_valid": True,
        "type": msg_type_reverse.get(msg_type, "error"),
        "device_type": DEVICE_REVERSE.get(device_type, "UNKNOWN"),
        "timestamp": timestamp,
        "low_battery": low_battery,
    }

    offset = 5

    # Extract token for relevant message types
    if msg_type in (MSG_ACK, MSG_DATA, MSG_PING, MSG_PING_RESPONSE, MSG_INVALID_TOKEN):
        if len(data) >= offset + 16:
            token_bytes = data[offset:offset+16]
            msg_dict["token"] = decode_token(token_bytes)
            offset += 16

    # Extract payload for data messages
    if msg_type == MSG_DATA:
        decoder = PAYLOAD_DECODERS.get(device_type)
        if decoder:
            # Determine payload size
            payload_sizes = {
                DEV_THERMONODE: 8,
                DEV_WINDSENSE: 8,
                DEV_RAINDETECT: 6,
                DEV_AIRQUALITYBOX: 6,
            }
            payload_size = payload_sizes.get(device_type, 0)
            if len(data) >= offset + payload_size:
                payload_bytes = data[offset:offset+payload_size]
                msg_dict["payload"] = decoder(payload_bytes)
                offset += payload_size

    # Extract message for error messages
    if msg_type in (MSG_CHECKSUM_ERROR, MSG_ERROR):
        if len(data) >= offset + 64:
            message_bytes = data[offset:offset+64]
            msg_dict["message"] = decode_string(message_bytes)
            offset += 64

    if msg_type == MSG_INVALID_TOKEN:
        if len(data) >= offset + 64:
            reason_bytes = data[offset:offset+64]
            msg_dict["reason"] = decode_string(reason_bytes)
            offset += 64

    return msg_dict


def verify_checksum_binary(data):
    """
    Verify that binary message has valid checksum.
    Returns True if valid, False otherwise.
    """
    if len(data) < 5:
        return False

    received_checksum = struct.unpack('>I', data[-4:])[0]
    data_without_checksum = data[:-4]
    expected_checksum = calculate_checksum_binary(data_without_checksum)

    return received_checksum == expected_checksum
