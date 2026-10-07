#!/usr/bin/env python3
"""
CA3 Bridge Termux Client
Communicates with CA3 Bridge Android APK via localhost JSON API.
"""

import json
import socket
import sys
import time
from typing import Any, Dict, Optional, List
from dataclasses import dataclass


DEFAULT_HOST = "127.0.0.1"
DEFAULT_PORT = 18765
DEFAULT_TIMEOUT = 10.0


class CA3Error(Exception):
    """CA3 Bridge client error."""
    pass


class CA3ConnectionError(CA3Error):
    """Connection-related error."""
    pass


class CA3ProtocolError(CA3Error):
    """Protocol-related error."""
    pass


@dataclass
class DeviceInfo:
    connected: bool
    has_permission: bool
    vendor_id: str
    product_id: str
    product_name: str
    interfaces: int
    state: str


@dataclass
class DeviceStatus:
    connected: bool
    has_permission: bool
    vendor_id: int
    product_id: int
    product_name: str
    interface_number: int
    state: str
    capture_active: bool
    active_mode: bool


@dataclass
class DescriptorInfo:
    vendor_id: int
    product_id: int
    device_class: int
    device_subclass: int
    device_protocol: int
    manufacturer_name: Optional[str]
    product_name: Optional[str]
    serial_number: Optional[str]
    interface_number: int
    bulk_in: Optional[Dict]
    bulk_out: Optional[Dict]
    interrupt_in: Optional[Dict]
    interrupt_out: Optional[Dict]


@dataclass
class CaptureStatus:
    active: bool
    record_count: int


@dataclass
class CaptureStopResult:
    file: str
    record_count: int


@dataclass
class Ca3Status:
    interface_claimed: bool
    bulk_in_available: bool
    bulk_out_available: bool
    interrupt_in_available: bool
    interrupt_out_available: bool


class CA3:
    """CA3 Bridge client for Termux."""
    
    def __init__(self, host: str = DEFAULT_HOST, port: int = DEFAULT_PORT, timeout: float = DEFAULT_TIMEOUT):
        self.host = host
        self.port = port
        self.timeout = timeout
        self._request_id = 0
        self._socket: Optional[socket.socket] = None
        self._reader = None
    
    def connect(self) -> None:
        """Connect to CA3 Bridge."""
        try:
            self._socket = socket.create_connection((self.host, self.port), timeout=self.timeout)
            self._socket.settimeout(self.timeout)
            self._reader = self._socket.makefile('r', encoding='utf-8')
        except socket.timeout:
            raise CA3ConnectionError(f"Connection timeout to {self.host}:{self.port}")
        except ConnectionRefusedError:
            raise CA3ConnectionError(f"Connection refused - is CA3 Bridge running on {self.host}:{self.port}?")
        except Exception as e:
            raise CA3ConnectionError(f"Failed to connect: {e}")
    
    def disconnect(self) -> None:
        """Disconnect from CA3 Bridge."""
        if self._reader:
            try:
                self._reader.close()
            except:
                pass
        if self._socket:
            try:
                self._socket.close()
            except:
                pass
        self._reader = None
        self._socket = None
    
    def __enter__(self) -> 'CA3':
        self.connect()
        return self
    
    def __exit__(self, exc_type, exc_val, exc_tb) -> None:
        self.disconnect()
    
    def _send_request(self, method: str, **params) -> Dict[str, Any]:
        """Send a JSON request and parse response."""
        if not self._socket or not self._reader:
            raise CA3ConnectionError("Not connected")
        
        self._request_id += 1
        request = {"id": self._request_id, "method": method, **params}
        request_json = json.dumps(request) + "\n"
        
        try:
            self._socket.sendall(request_json.encode('utf-8'))
            response_line = self._reader.readline()
            if not response_line:
                raise CA3ProtocolError("Empty response from server")
            
            response = json.loads(response_line.strip())
            
            if not response.get("ok", False):
                error = response.get("error", "UNKNOWN_ERROR")
                raise CA3ProtocolError(f"Server error: {error}")
            
            result_str = response.get("result", "{}")
            return json.loads(result_str) if isinstance(result_str, str) else result_str
            
        except json.JSONDecodeError as e:
            raise CA3ProtocolError(f"Invalid JSON response: {e}")
        except socket.timeout:
            raise CA3ConnectionError("Request timeout")
        except Exception as e:
            raise CA3ProtocolError(f"Request failed: {e}")
    
    # --- API Methods ---
    
    def info(self) -> DeviceInfo:
        """Get basic device information."""
        result = self._send_request("device.info")
        return DeviceInfo(**result)
    
    def status(self) -> DeviceStatus:
        """Get detailed device status."""
        result = self._send_request("device.status")
        return DeviceStatus(**result)
    
    def descriptor(self) -> DescriptorInfo:
        """Get USB descriptor information."""
        result = self._send_request("usb.descriptor")
        return DescriptorInfo(**result)
    
    def capture_start(self) -> bool:
        """Start USB traffic capture."""
        result = self._send_request("capture.start")
        return result.get("started", False)
    
    def capture_stop(self) -> CaptureStopResult:
        """Stop USB traffic capture."""
        result = self._send_request("capture.stop")
        return CaptureStopResult(**result)
    
    def capture_status(self) -> CaptureStatus:
        """Get capture status."""
        result = self._send_request("capture.status")
        return CaptureStatus(**result)
    
    def ca3_status(self) -> Ca3Status:
        """Get CA3-specific status."""
        result = self._send_request("ca3.status")
        return Ca3Status(**result)
    
    # --- Convenience Methods ---
    
    def wait_for_device(self, timeout: float = 30.0, poll_interval: float = 1.0) -> DeviceInfo:
        """Wait until a device is connected and has permission."""
        start = time.time()
        while time.time() - start < timeout:
            try:
                info = self.info()
                if info.connected and info.has_permission:
                    return info
            except CA3Error:
                pass
            time.sleep(poll_interval)
        raise CA3ConnectionError("Timeout waiting for device")
    
    def wait_for_capture(self, min_records: int = 1, timeout: float = 60.0, poll_interval: float = 0.5) -> CaptureStopResult:
        """Start capture, wait for records, then stop."""
        if not self.capture_start():
            raise CA3ProtocolError("Failed to start capture (already capturing?)")
        
        start = time.time()
        while time.time() - start < timeout:
            status = self.capture_status()
            if status.record_count >= min_records:
                return self.capture_stop()
            time.sleep(poll_interval)
        
        # Timeout - stop anyway
        return self.capture_stop()
    
    def print_device_info(self) -> None:
        """Print formatted device information."""
        info = self.info()
        print(f"Connected:      {info.connected}")
        print(f"Permission:     {info.has_permission}")
        print(f"VID:            {info.vendor_id}")
        print(f"PID:            {info.product_id}")
        print(f"Product:        {info.product_name}")
        print(f"Interfaces:     {info.interfaces}")
        print(f"State:          {info.state}")
    
    def print_status(self) -> None:
        """Print formatted status."""
        status = self.status()
        print(f"Connected:      {status.connected}")
        print(f"Permission:     {status.has_permission}")
        print(f"VID:            0x{status.vendor_id:04X}")
        print(f"PID:            0x{status.product_id:04X}")
        print(f"Product:        {status.product_name}")
        print(f"Interface:      {status.interface_number}")
        print(f"State:          {status.state}")
        print(f"Capturing:      {status.capture_active}")
        print(f"Active Mode:    {status.active_mode}")
    
    def print_descriptor(self) -> None:
        """Print formatted USB descriptor."""
        desc = self.descriptor()
        print(f"VID:                    0x{desc.vendor_id:04X}")
        print(f"PID:                    0x{desc.product_id:04X}")
        print(f"Device Class:           0x{desc.device_class:02X}")
        print(f"Device Subclass:        0x{desc.device_subclass:02X}")
        print(f"Device Protocol:        0x{desc.device_protocol:02X}")
        print(f"Manufacturer:           {desc.manufacturer_name or 'N/A'}")
        print(f"Product:                {desc.product_name or 'N/A'}")
        print(f"Serial:                 {desc.serial_number or 'N/A'}")
        print(f"Selected Interface:     {desc.interface_number}")
        
        for name, ep in [("BULK IN", desc.bulk_in), ("BULK OUT", desc.bulk_out),
                         ("INT IN", desc.interrupt_in), ("INT OUT", desc.interrupt_out)]:
            if ep:
                print(f"{name}:                 EP=0x{ep.get('endpointNumber', 0):02X} MAX={ep.get('maxPacketSize', 0)}")


def main():
    """CLI entry point."""
    import argparse
    
    parser = argparse.ArgumentParser(description="CA3 Bridge Termux Client")
    parser.add_argument("--host", default=DEFAULT_HOST, help="Bridge host (default: 127.0.0.1)")
    parser.add_argument("--port", type=int, default=DEFAULT_PORT, help="Bridge port (default: 18765)")
    parser.add_argument("--timeout", type=float, default=DEFAULT_TIMEOUT, help="Timeout in seconds")
    
    subparsers = parser.add_subparsers(dest="command", required=True)
    
    subparsers.add_parser("info", help="Get basic device info")
    subparsers.add_parser("status", help="Get detailed device status")
    subparsers.add_parser("descriptor", help="Get USB descriptor")
    subparsers.add_parser("capture-start", help="Start USB capture")
    subparsers.add_parser("capture-stop", help="Stop USB capture")
    subparsers.add_parser("capture-status", help="Get capture status")
    subparsers.add_parser("ca3-status", help="Get CA3 status")
    
    capture_wait = subparsers.add_parser("capture-wait", help="Capture until records collected")
    capture_wait.add_argument("--min-records", type=int, default=100, help="Minimum records to collect")
    capture_wait.add_argument("--timeout", type=float, default=60.0, help="Max wait time")
    
    wait_device = subparsers.add_parser("wait-device", help="Wait for device connection")
    wait_device.add_argument("--timeout", type=float, default=30.0, help="Max wait time")
    
    args = parser.parse_args()
    
    try:
        with CA3(args.host, args.port, args.timeout) as ca3:
            if args.command == "info":
                ca3.print_device_info()
            elif args.command == "status":
                ca3.print_status()
            elif args.command == "descriptor":
                ca3.print_descriptor()
            elif args.command == "capture-start":
                if ca3.capture_start():
                    print("Capture started")
                else:
                    print("Capture already active")
            elif args.command == "capture-stop":
                result = ca3.capture_stop()
                print(f"Capture stopped: {result.record_count} records saved to {result.file}")
            elif args.command == "capture-status":
                status = ca3.capture_status()
                print(f"Active: {status.active}, Records: {status.record_count}")
            elif args.command == "ca3-status":
                status = ca3.ca3_status()
                print(f"Interface Claimed: {status.interface_claimed}")
                print(f"BULK IN: {status.bulk_in_available}")
                print(f"BULK OUT: {status.bulk_out_available}")
                print(f"INT IN: {status.interrupt_in_available}")
                print(f"INT OUT: {status.interrupt_out_available}")
            elif args.command == "capture-wait":
                result = ca3.wait_for_capture(args.min_records, args.timeout)
                print(f"Capture stopped: {result.record_count} records saved to {result.file}")
            elif args.command == "wait-device":
                info = ca3.wait_for_device(args.timeout)
                print("Device ready:")
                ca3.print_device_info()
    except CA3Error as e:
        print(f"Error: {e}", file=sys.stderr)
        sys.exit(1)
    except KeyboardInterrupt:
        print("\nInterrupted", file=sys.stderr)
        sys.exit(130)


if __name__ == "__main__":
    main()