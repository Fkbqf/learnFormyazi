#include "Logger.h"
#include <bits/types/time_t.h>
#include <cstdio>
#include <errno.h>
#include <stdarg.h>
#include <stdexcept>
#include <string.h>
#include <time.h>

using namespace Lyz::utility;

const char *Logger::s_level[LEVEL_COUNT] = {"DEBUG", "INFO", "WARN", "ERROR",
                                            "FATAL"};

Logger *Logger::m_instance = nullptr;

Logger::Logger() : m_max(0), m_fout(0), m_level(level::DEBUG) {}
Logger::~Logger() { close(); }

Logger *Logger::instance() {
  if (m_instance == nullptr)
    m_instance = new Logger();
  return m_instance;
}

void Logger::open(const string &filename) {
  m_filname = filename;
  m_fout.open(filename.c_str(), ios::app);
  if (m_fout.fail()) {
    throw std::logic_error("open log file failed: " + filename);
  }
  m_fout.seekp(0, ios::end);
  m_len = m_fout.tellp();
}

void Logger::close() { m_fout.close(); }

void Logger::log(level level, const char *file, int line, const char *format,
                 ...) {
  if (m_level > level)
    return;

  if (m_fout.fail()) {
    throw std::logic_error("open log file failed: " + m_filname);
  }

  time_t ticks = time(NULL);
  struct tm *ptm = localtime(&ticks);
  char timestamp[32];
  memset(timestamp, 0, 32);
  strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", ptm);

  int len = 0;
  const char *fmt = "%s %s %s:%d";
  len = snprintf(NULL, 0, fmt, timestamp, s_level[level], file, line);

  if (len > 0) {
    char *buffer = new char[len + 1];
    snprintf(buffer, len + 1, fmt, timestamp, s_level[level], file, line);
    buffer[len] = '\0';
    m_fout << buffer;
    delete[] buffer;
    m_len += len;
  }

  va_list arg_ptr;
  va_start(arg_ptr, format);
  len = vsnprintf(NULL, 0, format, arg_ptr);
  va_end(arg_ptr);
  if (len > 0) {
    char *content = new char[len + 1];
    va_start(arg_ptr, format);
    vsnprintf(content, len + 1, format, arg_ptr);
    va_end(arg_ptr);
    content[len] = 0;
    m_fout << content;
    delete[] content;
    m_len += len;
  }
  m_fout << "\n";
  m_fout.flush();
  if (m_max > 0 && m_len >= m_max) {
    rotate();
  }
}

void Logger::max(int bytes) { m_max = bytes; }

void Logger::setlevel(int level) { m_level = level; }

void Logger::rotate() {
  close();
  time_t ticks = time(NULL);
  struct tm *ptm = localtime(&ticks);
  char timestamp[32];
  memset(timestamp, 0, sizeof(timestamp));
  strftime(timestamp, sizeof(timestamp), ".%Y-%m-%d_%H-%M-%S", ptm);
  string filename = m_filname + timestamp;
  if (rename(m_filname.c_str(), filename.c_str()) != 0) {
    throw std::logic_error("rename log file failed: " +
                           string(strerror(errno)));
  }
  open(m_filname);
}
