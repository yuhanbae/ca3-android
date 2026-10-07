#!/usr/bin/env python3
"""
CA3 Info - Quick device information tool.
"""

import sys
sys.path.insert(0, __file__.rsplit('/', 1)[0])

from ca3 import CA3, CA3Error

def main():
    try:
        with CA3() as ca3:
            info = ca3.info()
            print(json.dumps({
                "connected": info.connected,
                "has_permission": info.has_permission,
                "vendor_id": info.vendor_id,
                "product_id": info.product_id,
                "product_name": info.product_name,
                "interfaces": info.interfaces,
                "state": info.state
            }, indent=2))
    except CA3Error as e:
        print(json.dumps({"error": str(e)}), file=sys.stderr)
        sys.exit(1)

if __name__ == "__main__":
    import json
    main()