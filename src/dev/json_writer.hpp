#pragma once

// ============================================================================
// JsonWriter — a streaming JSON emitter, and nothing more
// ============================================================================
// Writing JSON is trivial; parsing it is not. The harness only ever writes
// (telemetry.json, run.json), so this is ~80 lines instead of a fourth
// FetchContent dependency. Call pattern mirrors the document: beginObject,
// key, value, ..., endObject. The writer tracks commas and indentation; the
// caller is trusted to balance begin/end and to put a key before every value
// inside an object.
//
// Floats print with %.9g - enough digits to round-trip a float exactly, so
// two runs that agree in memory agree on disk byte for byte.
// ============================================================================

#include <cstdio>
#include <string>
#include <vector>

class JsonWriter {
public:
  void beginObject() { open('{'); }
  void endObject() { close('}'); }
  // compact = true keeps the array on one line: "[1, 2]" for coordinates.
  void beginArray(bool compact = false) { open('[', compact); }
  void endArray() { close(']'); }

  void key(const char *name) {
    separate();
    quoted(name);
    m_out += ": ";
    m_afterKey = true;
  }

  void value(int v) { scalar(std::to_string(v)); }
  void value(unsigned int v) { scalar(std::to_string(v)); }
  void value(bool v) { scalar(v ? "true" : "false"); }
  void value(float v) {
    char buf[32];
    std::snprintf(buf, sizeof buf, "%.9g", (double)v);
    scalar(buf);
  }
  void value(const char *v) {
    separate();
    quoted(v);
  }
  void value(const std::string &v) { value(v.c_str()); }

  // Convenience for the common "key": scalar pair.
  template <typename T> void field(const char *name, const T &v) {
    key(name);
    value(v);
  }

  const std::string &str() const { return m_out; }

private:
  struct Container {
    bool first;   // nothing written yet?
    bool compact; // one line, ", " separated
  };
  std::string m_out;
  std::vector<Container> m_open;
  bool m_afterKey = false;

  void indent() { m_out.append(m_open.size() * 2, ' '); }

  // Comma-and-newline before anything that is not the first entry of its
  // container and does not directly follow a key.
  void separate() {
    if (m_afterKey) {
      m_afterKey = false;
      return;
    }
    if (m_open.empty())
      return;
    Container &c = m_open.back();
    if (!c.first)
      m_out += c.compact ? ", " : ",";
    if (!c.compact) {
      m_out += '\n';
      indent();
    }
    c.first = false;
  }

  void open(char c, bool compact = false) {
    separate();
    m_out += c;
    m_open.push_back({true, compact});
  }

  void close(char c) {
    Container closing = m_open.back();
    m_open.pop_back();
    if (!closing.first && !closing.compact) {
      m_out += '\n';
      indent();
    }
    m_out += c;
    if (m_open.empty())
      m_out += '\n';
  }

  void scalar(const std::string &text) {
    separate();
    m_out += text;
  }

  void quoted(const char *s) {
    m_out += '"';
    for (; *s; ++s) {
      switch (*s) {
      case '"': m_out += "\\\""; break;
      case '\\': m_out += "\\\\"; break;
      case '\n': m_out += "\\n"; break;
      case '\t': m_out += "\\t"; break;
      default:
        if ((unsigned char)*s < 0x20) {
          char buf[8];
          std::snprintf(buf, sizeof buf, "\\u%04x", *s);
          m_out += buf;
        } else {
          m_out += *s;
        }
      }
    }
    m_out += '"';
  }
};
