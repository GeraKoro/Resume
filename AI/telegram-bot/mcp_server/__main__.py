"""Запуск MCP-сервера по stdio: python -m mcp_server"""

from __future__ import annotations

import logging
import os
import sys

from dotenv import load_dotenv

from mcp_server.server import mcp


def main() -> None:
    logging.basicConfig(level=logging.INFO, stream=sys.stderr)
    load_dotenv(os.environ.get("ENV_FILE", ".env"), interpolate=False)
    mcp.run(transport="stdio")


if __name__ == "__main__":
    main()
