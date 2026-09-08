#include "vr_speedrun.h"
#include "pc/utils/json.hpp"
#include <chrono>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <filesystem>
#include <string>
#include <vector>

namespace {
struct Segment { std::string name; double best = -1; double actual = -1; };
std::vector<Segment> segments = {{"Finish", -1, -1}};
unsigned int completed = 0;
double start = 0, accumulated = 0;
bool started = false, running = false;
std::string status = "Basic timer ready";
std::string localSetup;
constexpr size_t maxFile = 4 * 1024 * 1024;
std::filesystem::path utf8_path(const std::string& path) {
#if defined(__cpp_char8_t)
    return std::filesystem::path(reinterpret_cast<const char8_t*>(path.c_str()));
#else
    return std::filesystem::u8path(path);
#endif
}
bool read_file(const char* path, std::string& data) {
    std::ifstream in(path, std::ios::binary | std::ios::ate);
    if (!in || in.tellg() < 0 || in.tellg() > static_cast<std::streamoff>(maxFile)) return false;
    data.resize(static_cast<size_t>(in.tellg()));
    in.seekg(0);
    return data.empty() || static_cast<bool>(in.read(&data[0], data.size()));
}
std::string clean_name(std::string name) {
    if (name.size() > 48) name.resize(48);
    // The native HUD font does not support arbitrary control/color codes.
    for (char& c : name) if (static_cast<unsigned char>(c) < 32 || c == '\\') c = ' ';
    return name.empty() ? "Split" : name;
}
void commit(std::vector<Segment>& candidate) {
    segments.swap(candidate);
    vr_speedrun_reset();
}
// Restricted XML text decoding: no DTDs, external entities or scripts.
std::string xml_text(std::string value) {
    const char* entities[] = {"&amp;", "&lt;", "&gt;", "&quot;", "&apos;"};
    const char replacements[] = {'&', '<', '>', '"', '\''};
    std::string out;
    for (size_t i = 0; i < value.size(); ++i) {
        bool found = false;
        if (value[i] == '&') for (unsigned int j = 0; j < 5; ++j) {
            const std::string entity(entities[j]);
            if (value.compare(i, entity.size(), entity) == 0) {
                out += replacements[j]; i += entity.size() - 1; found = true; break;
            }
        }
        if (!found) out += value[i];
    }
    return out;
}
double parse_time(const std::string& value) {
    // LiveSplit RealTime fields use hh:mm:ss[.fffffff] (optional day prefix).
    unsigned int hours = 0, minutes = 0, days = 0; double seconds = 0; int consumed = 0;
    if (std::sscanf(value.c_str(), "%u.%u:%u:%lf%n", &days, &hours, &minutes, &seconds, &consumed) != 4) {
        days = 0; consumed = 0;
        if (std::sscanf(value.c_str(), "%u:%u:%lf%n", &hours, &minutes, &seconds, &consumed) != 3) return -1;
    }
    if (consumed != static_cast<int>(value.size()) || minutes >= 60 ||
        !std::isfinite(seconds) || seconds < 0 || seconds >= 60 || days > 365) return -1;
    return days * 86400.0 + hours * 3600.0 + minutes * 60.0 + seconds;
}
}
double vr_speedrun_now(void) {
    return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();
}
void vr_speedrun_initialize(const char* setupFile, unsigned int count) {
    static bool initialized = false;
    if (initialized) return;
    initialized = true;
    vr_speedrun_basic(count);
    const auto localUtf8 = (utf8_path(setupFile).parent_path() / "local.json").u8string();
    localSetup.assign(localUtf8.begin(), localUtf8.end());
    std::ifstream local(utf8_path(localSetup));
    if (local.good()) {
        local.close();
        if (vr_speedrun_load(localSetup.c_str())) return;
    }
    std::ifstream existing(setupFile);
    if (existing.good()) { existing.close(); vr_speedrun_load(setupFile); }
}
double vr_speedrun_best(void) {
    return completed < segments.size() ? segments[completed].best : segments.back().best;
}
double vr_speedrun_elapsed(double now) {
    return accumulated + (running ? std::max(0.0, now - start) : 0.0);
}
void vr_speedrun_reset(void) {
    completed = 0; start = accumulated = 0; started = running = false;
    for (auto& segment : segments) segment.actual = -1;
    status = "Timer reset";
}
void vr_speedrun_split(double now) {
    if (!std::isfinite(now)) return;
    if (!started) { started = running = true; start = now; status = "Timer started"; return; }
    if (!running || completed >= segments.size()) return;
    segments[completed++].actual = vr_speedrun_elapsed(now);
    status = "Split recorded";
    if (completed == segments.size()) { accumulated = vr_speedrun_elapsed(now); running = false; }
    if (!running) status = "Run finished";
}
void vr_speedrun_pause(double now) {
    if (!started || completed == segments.size() || !std::isfinite(now)) return;
    if (running) accumulated = vr_speedrun_elapsed(now);
    else start = now;
    running = !running;
    status = running ? "Timer resumed" : "Timer paused";
}
void vr_speedrun_basic(unsigned int count) {
    count = std::max(1U, std::min(120U, count));
    std::vector<Segment> candidate;
    for (unsigned int i = 0; i < count; ++i) candidate.push_back({"Split " + std::to_string(i + 1), -1, -1});
    commit(candidate); status = "Basic setup created";
}
unsigned int vr_speedrun_count(void) { return completed; }
void vr_speedrun_configure(unsigned int count) {
    count = std::max(1U, std::min(120U, count));
    std::vector<Segment> candidate = segments;
    candidate.resize(count);
    for (unsigned int i = 0; i < count; ++i) {
        if (candidate[i].name.empty()) candidate[i].name = "Split " + std::to_string(i + 1);
    }
    commit(candidate); status = "Split count confirmed; timer reset; names kept";
}
bool vr_speedrun_set_name(unsigned int index, const char* name) {
    if (index >= segments.size() || !name) return false;
    segments[index].name = clean_name(name);
    status = "Split name updated";
    return true;
}
const char* vr_speedrun_segment_name(unsigned int index) {
    return index < segments.size() ? segments[index].name.c_str() : "";
}
double vr_speedrun_segment_time(unsigned int index) {
    return index < segments.size() ? segments[index].actual : -1;
}
unsigned int vr_speedrun_total(void) { return static_cast<unsigned int>(segments.size()); }
const char* vr_speedrun_name(void) {
    return completed < segments.size() ? segments[completed].name.c_str() : "Finished";
}
bool vr_speedrun_running(void) { return running; }
const char* vr_speedrun_status(void) { return status.c_str(); }
bool vr_speedrun_load(const char* filename) {
    try {
        std::string data;
        if (!read_file(filename, data)) throw std::runtime_error("read");
        const auto json = nlohmann::json::parse(data);
        if (json.at("version") != 1 || !json.at("segments").is_array()) throw std::runtime_error("format");
        const auto& list = json.at("segments");
        if (list.empty() || list.size() > 120) throw std::runtime_error("count");
        std::vector<Segment> candidate;
        for (const auto& item : list) {
            double best = item.value("best_seconds", -1.0);
            if (!std::isfinite(best) || best < -1) throw std::runtime_error("time");
            candidate.push_back({clean_name(item.at("name").get<std::string>()), best, -1});
        }
        commit(candidate); status = "Loaded setup.json"; return true;
    } catch (...) { status = "Cannot load setup.json; current run kept"; return false; }
}
bool vr_speedrun_save(const char* filename) {
    try {
        std::ifstream existing(filename);
        if (existing.good()) { status = "Export exists; rename it before exporting again"; return false; }
        existing.close();
        nlohmann::json json;
        json["version"] = 1;
        json["segments"] = nlohmann::json::array();
        for (const auto& segment : segments) json["segments"].push_back({
            {"name", segment.name}, {"best_seconds", segment.best}, {"last_seconds", segment.actual}});
        // Write a temporary sibling first. Never truncate a player's existing setup.
        std::string temp = std::string(filename) + ".tmp";
        std::ofstream out(temp, std::ios::binary | std::ios::trunc);
        out << json.dump(2) << '\n'; out.close();
        if (!out) throw std::runtime_error("write");
        // Export uses a separate name; refuse to overwrite it silently.
        if (std::rename(temp.c_str(), filename) != 0) throw std::runtime_error("rename");
        status = "Saved export.json"; return true;
    } catch (...) { status = "Could not save export.json"; return false; }
}
bool vr_speedrun_import_lss(const char* filename) {
    std::string data;
    if (!read_file(filename, data) || data.find("<!") != std::string::npos ||
        data.find("<Run") == std::string::npos) {
        status = "Cannot read run.lss (plain LiveSplit XML required)"; return false;
    }
    std::vector<Segment> candidate;
    const size_t segmentsStart = data.find("<Segments>");
    const size_t segmentsEnd = data.find("</Segments>", segmentsStart == std::string::npos ? 0 : segmentsStart);
    if (segmentsStart == std::string::npos || segmentsEnd == std::string::npos) {
        status = "run.lss has no segment list"; return false;
    }
    size_t cursor = segmentsStart;
    while ((cursor = data.find("<Segment>", cursor)) != std::string::npos && cursor < segmentsEnd) {
        size_t end = data.find("</Segment>", cursor);
        size_t name = data.find("<Name>", cursor);
        size_t nameEnd = name == std::string::npos ? name : data.find("</Name>", name);
        if (candidate.size() == 120 || end == std::string::npos || end > segmentsEnd ||
            name == std::string::npos || nameEnd == std::string::npos || nameEnd > end) {
            status = "Invalid run.lss; current run kept"; return false;
        }
        Segment segment{clean_name(xml_text(data.substr(name + 6, nameEnd - name - 6))), -1, -1};
        size_t pb = data.find("<SplitTime name=\"Personal Best\">", nameEnd);
        if (pb != std::string::npos && pb < end) {
            size_t pbEnd = data.find("</SplitTime>", pb);
            size_t time = data.find("<RealTime>", pb);
            size_t timeEnd = time == std::string::npos ? time : data.find("</RealTime>", time);
            if (pbEnd < end && time < pbEnd && timeEnd < pbEnd)
                segment.best = parse_time(data.substr(time + 10, timeEnd - time - 10));
        }
        candidate.push_back(segment); cursor = end + 10;
    }
    if (candidate.empty()) { status = "run.lss contains no splits"; return false; }
    commit(candidate); status = "Imported run.lss names and real-time PBs"; return true;
}

bool vr_speedrun_save_local(void) {
    if (localSetup.empty()) return false;
    try {
        nlohmann::json json;
        json["version"] = 1;
        json["segments"] = nlohmann::json::array();
        for (const auto& segment : segments) json["segments"].push_back({
            {"name", segment.name}, {"best_seconds", segment.best}});
        const auto path = utf8_path(localSetup);
        const auto temp = utf8_path(localSetup + ".tmp");
        std::ofstream out(temp, std::ios::binary | std::ios::trunc);
        out << json.dump(2) << '\n'; out.close();
        if (!out) throw std::runtime_error("write");
        // Only replace the application's local setup. Imported player files
        // and exported results are never overwritten by in-game editing.
        std::filesystem::rename(temp, path);
        return true;
    } catch (...) {
        status = "Setup active, but local.json could not be saved";
        return false;
    }
}
