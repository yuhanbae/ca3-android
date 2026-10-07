#!/usr/bin/env python3
"""
CA3 Shell - Interactive REPL for CA3 Bridge.
"""

import sys
import cmd
import json
import shlex
sys.path.insert(0, __file__.rsplit('/', 1)[0])

from ca3 import CA3, CA3Error, CA3ConnectionError, CA3ProtocolError


class CA3Shell(cmd.Cmd):
    intro = "CA3 Bridge Interactive Shell\nType 'help' or '?' for commands.\n"
    prompt = "ca3> "
    
    def __init__(self):
        super().__init__()
        self.ca3: CA3 = None
        self.connected = False
    
    def do_connect(self, arg):
        """Connect to CA3 Bridge: connect [host] [port]"""
        args = shlex.split(arg)
        host = args[0] if args else "127.0.0.1"
        port = int(args[1]) if len(args) > 1 else 18765
        
        try:
            self.ca3 = CA3(host, port)
            self.ca3.connect()
            self.connected = True
            print(f"Connected to {host}:{port}")
        except CA3Error as e:
            print(f"Connection failed: {e}")
    
    def do_disconnect(self, arg):
        """Disconnect from CA3 Bridge"""
        if self.ca3:
            self.ca3.disconnect()
            self.connected = False
            print("Disconnected")
    
    def _check_connected(self):
        if not self.connected or not self.ca3:
            print("Not connected. Use 'connect' first.")
            return False
        return True
    
    def do_info(self, arg):
        """Get basic device info"""
        if not self._check_connected(): return
        try:
            info = self.ca3.info()
            print(json.dumps(info.__dict__, indent=2))
        except CA3Error as e:
            print(f"Error: {e}")
    
    def do_status(self, arg):
        """Get detailed device status"""
        if not self._check_connected(): return
        try:
            status = self.ca3.status()
            print(json.dumps(status.__dict__, indent=2))
        except CA3Error as e:
            print(f"Error: {e}")
    
    def do_descriptor(self, arg):
        """Get USB descriptor"""
        if not self._check_connected(): return
        try:
            desc = self.ca3.descriptor()
            print(json.dumps(desc.__dict__, indent=2))
        except CA3Error as e:
            print(f"Error: {e}")
    
    def do_capture_start(self, arg):
        """Start USB capture"""
        if not self._check_connected(): return
        try:
            if self.ca3.capture_start():
                print("Capture started")
            else:
                print("Capture already active")
        except CA3Error as e:
            print(f"Error: {e}")
    
    def do_capture_stop(self, arg):
        """Stop USB capture"""
        if not self._check_connected(): return
        try:
            result = self.ca3.capture_stop()
            print(json.dumps(result.__dict__, indent=2))
        except CA3Error as e:
            print(f"Error: {e}")
    
    def do_capture_status(self, arg):
        """Get capture status"""
        if not self._check_connected(): return
        try:
            status = self.ca3.capture_status()
            print(json.dumps(status.__dict__, indent=2))
        except CA3Error as e:
            print(f"Error: {e}")
    
    def do_capture_wait(self, arg):
        """Capture until min records reached: capture_wait [min_records] [timeout]"""
        if not self._check_connected(): return
        args = shlex.split(arg)
        min_records = int(args[0]) if args else 100
        timeout = float(args[1]) if len(args) > 1 else 60.0
        
        try:
            print(f"Capturing (target: {min_records} records)...")
            result = self.ca3.wait_for_capture(min_records, timeout)
            print(f"Capture complete: {result.record_count} records")
            print(f"File: {result.file}")
        except CA3Error as e:
            print(f"Error: {e}")
    
    def do_ca3_status(self, arg):
        """Get CA3-specific status"""
        if not self._check_connected(): return
        try:
            status = self.ca3.ca3_status()
            print(json.dumps(status.__dict__, indent=2))
        except CA3Error as e:
            print(f"Error: {e}")
    
    def do_wait_device(self, arg):
        """Wait for device connection: wait_device [timeout]"""
        if not self._check_connected(): return
        args = shlex.split(arg)
        timeout = float(args[0]) if args else 30.0
        
        try:
            print("Waiting for device...")
            info = self.ca3.wait_for_device(timeout)
            print("Device ready:")
            print(json.dumps(info.__dict__, indent=2))
        except CA3Error as e:
            print(f"Error: {e}")
    
    def do_raw(self, arg):
        """Send raw JSON request: raw <method> [json_params]"""
        if not self._check_connected(): return
        args = shlex.split(arg)
        if not args:
            print("Usage: raw <method> [json_params]")
            return
        
        method = args[0]
        params = {}
        if len(args) > 1:
            try:
                params = json.loads(" ".join(args[1:]))
            except json.JSONDecodeError as e:
                print(f"Invalid JSON: {e}")
                return
        
        try:
            result = self.ca3._send_request(method, **params)
            print(json.dumps(result, indent=2))
        except CA3Error as e:
            print(f"Error: {e}")
    
    def do_quit(self, arg):
        """Exit the shell"""
        if self.connected:
            self.do_disconnect("")
        return True
    
    def do_EOF(self, arg):
        """Exit on Ctrl+D"""
        print()
        return self.do_quit(arg)
    
    def emptyline(self):
        pass


def main():
    shell = CA3Shell()
    
    # Auto-connect if args provided
    if len(sys.argv) > 1:
        host = sys.argv[1]
        port = int(sys.argv[2]) if len(sys.argv) > 2 else 18765
        try:
            shell.ca3 = CA3(host, port)
            shell.ca3.connect()
            shell.connected = True
            print(f"Auto-connected to {host}:{port}")
        except CA3Error as e:
            print(f"Auto-connect failed: {e}")
    
    try:
        shell.cmdloop()
    except KeyboardInterrupt:
        print("\nExiting...")
        if shell.connected:
            shell.do_disconnect("")


if __name__ == "__main__":
    main()