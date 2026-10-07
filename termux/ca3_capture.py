#!/usr/bin/env python3
"""
CA3 Capture - Capture USB traffic to file.
"""

import sys
import argparse
sys.path.insert(0, __file__.rsplit('/', 1)[0])

from ca3 import CA3, CA3Error

def main():
    parser = argparse.ArgumentParser(description="Capture CA3 USB traffic")
    parser.add_argument("--min-records", type=int, default=100, help="Minimum records to capture")
    parser.add_argument("--timeout", type=float, default=60.0, help="Maximum capture time (seconds)")
    parser.add_argument("--output", help="Output file prefix (default: auto)")
    args = parser.parse_args()
    
    try:
        with CA3() as ca3:
            # Wait for device
            print("Waiting for device...")
            info = ca3.wait_for_device(timeout=30)
            print(f"Device: {info.product_name} ({info.vendor_id}:{info.product_id})")
            
            # Start capture
            print("Starting capture...")
            if not ca3.capture_start():
                print("Capture already active")
            
            # Wait for records
            print(f"Capturing (target: {args.min_records} records, timeout: {args.timeout}s)...")
            result = ca3.wait_for_capture(args.min_records, args.timeout)
            
            print(f"Capture complete: {result.record_count} records")
            print(f"File: {result.file}")
            
    except CA3Error as e:
        print(f"Error: {e}", file=sys.stderr)
        sys.exit(1)
    except KeyboardInterrupt:
        print("\nInterrupted", file=sys.stderr)
        sys.exit(130)

if __name__ == "__main__":
    main()