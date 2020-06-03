#pragma once

typedef struct _BLogFile _BLogFile;

_BLogFile* _newBLogFile(const BfxPathCharType* filepath, size_t maxCount, size_t maxSize);

int _blogFileOutput(_BLogFile* file, const char* line, size_t len);

void _blogFileFlush(_BLogFile* file);