// berkidectl — CLI management tool for BerkIDE server
// berkidectl — BerkIDE sunucusu icin CLI yonetim araci
//
// Usage: berkidectl [options] <command> [args...]
// Kullanim: berkidectl [seçenekler] <komut> [argümanlar...]

#define CPPHTTPLIB_NO_EXCEPTIONS
#include "http/httplib.h"
#include "nlohmann/json.hpp"

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <functional>
#include <algorithm>

using json = nlohmann::json;

// --- Connection config ---

struct Config {
    std::string host = "127.0.0.1";
    int port = 1881;
    std::string token;
    bool rawJson = false;   // Output raw JSON without formatting
    // JSON ciktisini bicimlendirmeden ver
};

// --- HTTP client helpers ---

// Make a GET request and return parsed JSON
// GET istegi yap ve ayrismis JSON dondur
json httpGet(const Config& cfg, const std::string& path) {
    httplib::Client cli(cfg.host, cfg.port);
    cli.set_connection_timeout(3);
    cli.set_read_timeout(10);

    httplib::Headers headers;
    if (!cfg.token.empty()) {
        headers.emplace("Authorization", "Bearer " + cfg.token);
    }

    auto res = cli.Get(path, headers);
    if (!res) {
        return json({{"error", "connection failed"}, {"detail", "Could not connect to " + cfg.host + ":" + std::to_string(cfg.port)}});
    }
    if (res->status == 401) {
        return json({{"error", "unauthorized"}, {"detail", "Bearer token required or invalid"}});
    }
    if (res->status != 200) {
        return json({{"error", "http_error"}, {"status", res->status}, {"body", res->body}});
    }

    auto parsed = json::parse(res->body, nullptr, false);
    if (parsed.is_discarded()) {
        return json({{"raw", res->body}});
    }
    return parsed;
}

// Make a POST request with JSON body and return parsed JSON
// JSON govdeli POST istegi yap ve ayrismis JSON dondur
json httpPost(const Config& cfg, const std::string& path, const json& body) {
    httplib::Client cli(cfg.host, cfg.port);
    cli.set_connection_timeout(3);
    cli.set_read_timeout(10);

    httplib::Headers headers;
    if (!cfg.token.empty()) {
        headers.emplace("Authorization", "Bearer " + cfg.token);
    }

    auto res = cli.Post(path, headers, body.dump(), "application/json");
    if (!res) {
        return json({{"error", "connection failed"}, {"detail", "Could not connect to " + cfg.host + ":" + std::to_string(cfg.port)}});
    }
    if (res->status == 401) {
        return json({{"error", "unauthorized"}, {"detail", "Bearer token required or invalid"}});
    }
    if (res->status != 200) {
        return json({{"error", "http_error"}, {"status", res->status}, {"body", res->body}});
    }

    auto parsed = json::parse(res->body, nullptr, false);
    if (parsed.is_discarded()) {
        return json({{"raw", res->body}});
    }
    return parsed;
}

// --- Output helpers ---

// Print JSON result (formatted or raw)
// JSON sonucunu yazdir (bicimlendirilmis veya ham)
void printJson(const json& j, bool raw) {
    if (raw) {
        std::cout << j.dump() << "\n";
    } else {
        std::cout << j.dump(2) << "\n";
    }
}

// Print a simple table from array of objects
// Nesne dizisinden basit tablo yazdir
void printTable(const json& arr, const std::vector<std::string>& cols) {
    if (!arr.is_array() || arr.empty()) {
        std::cout << "(empty)\n";
        return;
    }

    // Calculate column widths
    // Sutun genisliklerini hesapla
    std::map<std::string, size_t> widths;
    for (const auto& col : cols) widths[col] = col.size();

    for (const auto& row : arr) {
        for (const auto& col : cols) {
            std::string val = row.contains(col) ? row[col].dump() : "";
            if (row.contains(col) && row[col].is_string()) val = row[col].get<std::string>();
            widths[col] = std::max(widths[col], val.size());
        }
    }

    // Print header
    // Baslik yazdir
    for (const auto& col : cols) {
        std::string upper = col;
        std::transform(upper.begin(), upper.end(), upper.begin(), ::toupper);
        std::cout << upper;
        std::cout << std::string(widths[col] - upper.size() + 2, ' ');
    }
    std::cout << "\n";

    // Print separator
    // Ayirici yazdir
    for (const auto& col : cols) {
        std::cout << std::string(widths[col], '-') << "  ";
    }
    std::cout << "\n";

    // Print rows
    // Satirlari yazdir
    for (const auto& row : arr) {
        for (const auto& col : cols) {
            std::string val;
            if (row.contains(col)) {
                if (row[col].is_string()) val = row[col].get<std::string>();
                else val = row[col].dump();
            }
            std::cout << val;
            std::cout << std::string(widths[col] - val.size() + 2, ' ');
        }
        std::cout << "\n";
    }
}

// --- Command implementations ---

// berkidectl ping
int cmdPing(const Config& cfg, const std::vector<std::string>&) {
    httplib::Client cli(cfg.host, cfg.port);
    cli.set_connection_timeout(3);
    auto res = cli.Get("/ping");
    if (!res) {
        std::cerr << "FAIL: Cannot connect to " << cfg.host << ":" << cfg.port << "\n";
        return 1;
    }
    std::cout << res->body << " (" << cfg.host << ":" << cfg.port << ")\n";
    return 0;
}

// berkidectl status
int cmdStatus(const Config& cfg, const std::vector<std::string>&) {
    auto j = httpGet(cfg, "/api/server");
    if (j.contains("error")) { printJson(j, false); return 1; }
    if (cfg.rawJson) { printJson(j, true); return 0; }

    std::cout << j.value("name", "?") << " v" << j.value("version", "?") << "\n";
    std::cout << "Status:    " << j.value("status", "?") << "\n";
    if (j.contains("http") && j["http"].is_object()) {
        std::cout << "HTTP:      " << j["http"].value("bind", "?") << ":" << j["http"].value("port", 0) << "\n";
    }
    if (j.contains("ws") && j["ws"].is_object()) {
        std::cout << "WebSocket: " << j["ws"].value("port", 0) << "\n";
    }
    std::cout << "TLS:       " << (j.value("tls", false) ? "enabled" : "disabled") << "\n";
    std::cout << "Auth:      " << (j.value("auth", false) ? "required" : "none") << "\n";
    std::cout << "Endpoints: " << j.value("endpoints", 0) << "\n";
    return 0;
}

// berkidectl endpoints
int cmdEndpoints(const Config& cfg, const std::vector<std::string>&) {
    auto j = httpGet(cfg, "/api/endpoints");
    if (j.contains("error")) { printJson(j, false); return 1; }
    if (cfg.rawJson) { printJson(j, true); return 0; }
    printTable(j, {"method", "path", "description", "auth"});
    return 0;
}

// berkidectl commands
int cmdCommands(const Config& cfg, const std::vector<std::string>&) {
    auto j = httpGet(cfg, "/api/commands");
    if (j.contains("error")) { printJson(j, false); return 1; }
    if (cfg.rawJson) { printJson(j, true); return 0; }

    if (j.contains("mutations") && j["mutations"].is_array()) {
        std::cout << "Mutations (" << j["mutations"].size() << "):\n";
        for (const auto& cmd : j["mutations"]) {
            std::cout << "  " << cmd.get<std::string>() << "\n";
        }
    }
    if (j.contains("queries") && j["queries"].is_array()) {
        std::cout << "\nQueries (" << j["queries"].size() << "):\n";
        for (const auto& cmd : j["queries"]) {
            std::cout << "  " << cmd.get<std::string>() << "\n";
        }
    }
    if (j.is_array()) {
        for (const auto& cmd : j) std::cout << "  " << cmd.dump() << "\n";
    }
    return 0;
}

// berkidectl exec <cmd> [args_json]
int cmdExec(const Config& cfg, const std::vector<std::string>& args) {
    if (args.empty()) {
        std::cerr << "Usage: berkidectl exec <command_name> [args_json]\n";
        std::cerr << "Example: berkidectl exec theme.active\n";
        std::cerr << "         berkidectl exec theme.set '{\"name\":\"berkide-light\"}'\n";
        return 1;
    }

    json body = {{"cmd", args[0]}};
    if (args.size() > 1) {
        auto parsed = json::parse(args[1], nullptr, false);
        if (parsed.is_discarded()) {
            std::cerr << "Error: Invalid JSON arguments: " << args[1] << "\n";
            return 1;
        }
        body["args"] = parsed;
    } else {
        body["args"] = json::object();
    }

    auto j = httpPost(cfg, "/api/command", body);
    printJson(j, cfg.rawJson);
    return j.contains("error") ? 1 : 0;
}

// berkidectl state
int cmdState(const Config& cfg, const std::vector<std::string>&) {
    auto j = httpGet(cfg, "/api/state");
    if (j.contains("error")) { printJson(j, false); return 1; }
    if (cfg.rawJson) { printJson(j, true); return 0; }

    if (j.contains("cursor") && j["cursor"].is_object()) {
        std::cout << "Cursor: line=" << j["cursor"].value("line", 0)
                  << " col=" << j["cursor"].value("col", 0) << "\n";
    }
    if (j.contains("buffer") && j["buffer"].is_object()) {
        auto& buf = j["buffer"];
        std::cout << "Buffer: " << buf.value("filePath", "?")
                  << " (" << buf.value("lineCount", 0) << " lines"
                  << (buf.value("modified", false) ? ", modified" : "") << ")\n";
    }
    if (j.contains("mode")) {
        std::cout << "Mode: " << j["mode"].dump() << "\n";
    }
    if (j.contains("buffers") && j["buffers"].is_array()) {
        std::cout << "Open buffers: " << j["buffers"].size() << "\n";
        for (const auto& b : j["buffers"]) {
            std::cout << "  [" << b.value("index", 0) << "] "
                      << b.value("title", "?")
                      << (b.value("active", false) ? " *" : "") << "\n";
        }
    }
    return 0;
}

// berkidectl buffers
int cmdBuffers(const Config& cfg, const std::vector<std::string>&) {
    auto j = httpGet(cfg, "/api/buffers");
    if (j.contains("error")) { printJson(j, false); return 1; }
    if (cfg.rawJson) { printJson(j, true); return 0; }
    printTable(j, {"index", "title", "active"});
    return 0;
}

// berkidectl buffer
int cmdBuffer(const Config& cfg, const std::vector<std::string>&) {
    auto j = httpGet(cfg, "/api/buffer");
    if (j.contains("error")) { printJson(j, false); return 1; }
    if (cfg.rawJson) { printJson(j, true); return 0; }

    if (j.contains("lines") && j["lines"].is_array()) {
        int n = 1;
        for (const auto& line : j["lines"]) {
            std::cout << n++ << "\t" << line.get<std::string>() << "\n";
        }
    } else {
        printJson(j, false);
    }
    return 0;
}

// berkidectl cursor
int cmdCursor(const Config& cfg, const std::vector<std::string>&) {
    auto j = httpGet(cfg, "/api/cursor");
    if (j.contains("error")) { printJson(j, false); return 1; }
    if (cfg.rawJson) { printJson(j, true); return 0; }
    std::cout << "line=" << j.value("line", 0) << " col=" << j.value("col", 0) << "\n";
    return 0;
}

// berkidectl open <path>
int cmdOpen(const Config& cfg, const std::vector<std::string>& args) {
    if (args.empty()) {
        std::cerr << "Usage: berkidectl open <file_path>\n";
        return 1;
    }
    auto j = httpPost(cfg, "/api/buffer/open", {{"path", args[0]}});
    printJson(j, cfg.rawJson);
    return j.contains("error") ? 1 : 0;
}

// berkidectl save
int cmdSave(const Config& cfg, const std::vector<std::string>&) {
    auto j = httpPost(cfg, "/api/buffer/save", json::object());
    printJson(j, cfg.rawJson);
    return j.contains("error") ? 1 : 0;
}

// berkidectl close
int cmdClose(const Config& cfg, const std::vector<std::string>&) {
    auto j = httpPost(cfg, "/api/buffer/close", json::object());
    printJson(j, cfg.rawJson);
    return j.contains("error") ? 1 : 0;
}

// berkidectl help [topic]
int cmdHelp(const Config& cfg, const std::vector<std::string>& args) {
    if (args.empty()) {
        auto j = httpGet(cfg, "/api/help");
        if (j.contains("error")) { printJson(j, false); return 1; }
        if (cfg.rawJson) { printJson(j, true); return 0; }
        printTable(j, {"id", "title", "tags"});
        return 0;
    }

    auto j = httpGet(cfg, "/api/help/" + args[0]);
    if (j.contains("error")) { printJson(j, false); return 1; }
    if (cfg.rawJson) { printJson(j, true); return 0; }

    std::cout << "# " << j.value("title", "?") << "\n\n";
    std::cout << j.value("content", "") << "\n";
    return 0;
}

// --- Command registry ---

struct Command {
    std::string name;
    std::string description;
    std::function<int(const Config&, const std::vector<std::string>&)> handler;
};

// All available commands in one place
// Tum mevcut komutlar tek bir yerde
static const std::vector<Command> commands = {
    {"ping",      "Health check — test server connectivity",            cmdPing},
    {"status",    "Server status, version, ports, TLS, auth",          cmdStatus},
    {"endpoints", "List all API endpoints with metadata",              cmdEndpoints},
    {"commands",  "List all registered editor commands",               cmdCommands},
    {"exec",      "Execute a command: exec <cmd> [args_json]",         cmdExec},
    {"state",     "Full editor state (cursor, buffer, mode)",          cmdState},
    {"buffers",   "List all open buffers",                             cmdBuffers},
    {"buffer",    "Show active buffer content",                        cmdBuffer},
    {"cursor",    "Show cursor position",                              cmdCursor},
    {"open",      "Open a file: open <path>",                          cmdOpen},
    {"save",      "Save active buffer to disk",                        cmdSave},
    {"close",     "Close active buffer",                               cmdClose},
    {"help",      "Help topics: help [topic_id]",                      cmdHelp},
};

// --- Usage ---

void printUsage() {
    std::cout << "berkidectl — BerkIDE server management CLI\n\n";
    std::cout << "Usage: berkidectl [options] <command> [args...]\n\n";
    std::cout << "Options:\n";
    std::cout << "  --host <addr>    Server address (default: 127.0.0.1)\n";
    std::cout << "  --port <port>    Server port (default: 1881)\n";
    std::cout << "  --token <token>  Bearer auth token\n";
    std::cout << "  --json           Raw JSON output (no formatting)\n";
    std::cout << "  --help           Show this help message\n";
    std::cout << "  --version        Show version\n\n";
    std::cout << "Commands:\n";
    for (const auto& cmd : commands) {
        std::cout << "  " << cmd.name;
        std::cout << std::string(12 - cmd.name.size(), ' ');
        std::cout << cmd.description << "\n";
    }
    std::cout << "\nExamples:\n";
    std::cout << "  berkidectl ping\n";
    std::cout << "  berkidectl status\n";
    std::cout << "  berkidectl endpoints\n";
    std::cout << "  berkidectl exec theme.active\n";
    std::cout << "  berkidectl exec theme.set '{\"name\":\"berkide-light\"}'\n";
    std::cout << "  berkidectl exec autopairs.listPairs\n";
    std::cout << "  berkidectl open /path/to/file.cpp\n";
    std::cout << "  berkidectl --port 2000 status\n";
    std::cout << "  berkidectl --token mytoken123 commands\n";
}

// --- Main ---

int main(int argc, char* argv[]) {
    Config cfg;
    std::string cmdName;
    std::vector<std::string> cmdArgs;

    // Parse arguments
    // Argumanlari ayristir
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];

        if (arg == "--host" && i + 1 < argc) {
            cfg.host = argv[++i];
        } else if (arg == "--port" && i + 1 < argc) {
            cfg.port = std::stoi(argv[++i]);
        } else if (arg == "--token" && i + 1 < argc) {
            cfg.token = argv[++i];
        } else if (arg == "--json") {
            cfg.rawJson = true;
        } else if (arg == "--help" || arg == "-h") {
            printUsage();
            return 0;
        } else if (arg == "--version" || arg == "-v") {
            std::cout << "berkidectl 0.1.0\n";
            return 0;
        } else if (arg[0] == '-') {
            std::cerr << "Unknown option: " << arg << "\n";
            std::cerr << "Use --help for usage information.\n";
            return 1;
        } else if (cmdName.empty()) {
            cmdName = arg;
        } else {
            cmdArgs.push_back(arg);
        }
    }

    if (cmdName.empty()) {
        printUsage();
        return 0;
    }

    // Find and run the command
    // Komutu bul ve calistir
    for (const auto& cmd : commands) {
        if (cmd.name == cmdName) {
            return cmd.handler(cfg, cmdArgs);
        }
    }

    std::cerr << "Unknown command: " << cmdName << "\n";
    std::cerr << "Use --help for available commands.\n";
    return 1;
}
