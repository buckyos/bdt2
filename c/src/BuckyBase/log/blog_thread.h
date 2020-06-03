#pragma once
#include "./blog_file.h"

typedef struct _BLogThread _BLogThread;

_BLogThread* _newBLogThread(_BLogFile* fileTarget, int64_t flushInterval);

int _blogThreadOutput(_BLogThread* thread, const char* line, size_t len);