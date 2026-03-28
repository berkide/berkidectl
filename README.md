# berkidectl

**CLI management tool** for BerkIDE Core.

A command-line interface that talks to a running [berkide-core](https://github.com/berkide/berkide-core) server via HTTP. Manage buffers, execute commands, inspect state, and control the editor from your terminal.

Like `kubectl` for Kubernetes — `berkidectl` for BerkIDE.

## Quick Start

### Build

```bash
git clone https://github.com/berkide/berkidectl.git
cd berkidectl
./build.sh
```

### Usage

```bash
berkidectl ping                                    # Health check
berkidectl status                                  # Server info
berkidectl endpoints                               # List all API endpoints
berkidectl commands                                # List all 267 commands
berkidectl exec theme.active                       # Execute a command
berkidectl exec theme.set '{"name":"berkide-light"}'  # With arguments
berkidectl state                                   # Editor state
berkidectl buffers                                 # Open buffer list
berkidectl open /path/to/file.cpp                  # Open file
berkidectl save                                    # Save active buffer
```

## Options

| Option | Default | Description |
|--------|---------|-------------|
| `--host <addr>` | `127.0.0.1` | Server address |
| `--port <port>` | `1881` | Server HTTP port |
| `--token <token>` | — | Bearer auth token |
| `--json` | — | Raw JSON output |

### Examples

```bash
# Connect to remote server
berkidectl --host 192.168.1.100 --port 2000 status

# With authentication
berkidectl --token mysecrettoken commands

# Script-friendly JSON output
berkidectl --json exec theme.active | jq '.result.name'
```

## Commands

| Command | Description |
|---------|-------------|
| `ping` | Health check — test connectivity |
| `status` | Server info (version, ports, TLS, auth) |
| `endpoints` | List all API endpoints with metadata |
| `commands` | List all registered editor commands |
| `exec <cmd> [args]` | Execute any command with optional JSON args |
| `state` | Full editor state |
| `buffers` | List open buffers |
| `buffer` | Show active buffer content |
| `cursor` | Cursor position |
| `open <path>` | Open file |
| `save` | Save active buffer |
| `close` | Close active buffer |
| `help [topic]` | Help topics |

## Related Projects

| Project | Description |
|---------|-------------|
| [berkide](https://github.com/berkide/berkide) | Umbrella project |
| [berkide-core](https://github.com/berkide/berkide-core) | Headless editor server (required) |
| [berkide-tui](https://github.com/berkide/berkide-tui) | Terminal UI client |
| [berkide-plugins](https://github.com/berkide/berkide-plugins) | Official plugin collection |

## Branch Strategy

- `main` — Stable releases only. Protected.
- `dev` — Active development. All PRs target `dev`.
- Feature branches from `dev`: `feature/xxx`, `fix/xxx`

**Never push directly to `main`.** All changes go through `dev` first.

## License

MIT
