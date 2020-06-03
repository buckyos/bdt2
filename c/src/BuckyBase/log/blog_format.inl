
// 换行符的定义
#if defined(BFX_OS_WIN)
const char* _blogLineBreak = "\r\n";
#elif defined(BFX_OS_MACOSX)
const char* _blogLineBreak = "\r";
#elif defined(BFX_OS_POSIX)
const char* _blogLineBreak = "\n";
#else // others
#error("unsupported platform!!!")
#endif // others

//#if defined(BFX_COMPILER_GCC)

static BFX_THREAD_LOCAL char* _prettyFunctionFuncName;

// 判断是不是有效的函数名，包括带::作用域限定符号  xxx::xxx
static bool _prettyFunctionIsValidFuncChar(char ch) {
    return ((ch >= 'a' && ch <= 'z')
        || (ch >= 'A' && ch <= 'Z')
        || (ch >= '0' && ch <= '9')
        || ch == '_'
        || ch == ':');
}

static const char* _prettyFunctionSetFuncName(const char* name, size_t len) {
    const size_t maxLen = 128;
    if (_prettyFunctionFuncName == NULL) {
        _prettyFunctionFuncName = (char*)BfxMalloc(maxLen);

        // FIXME 线程粒度的释放
    }

    size_t realLen = min(len, maxLen - 1);
    strncpy(_prettyFunctionFuncName, name, realLen);
    _prettyFunctionFuncName[realLen] = '\0';

    return _prettyFunctionFuncName;
}

static const char* _prettyFunctionExtractor(const char* prettyFunction, const char* function) {

    // 编译器处理后的函数名包含下面几种情况：
    // 如果函数没返回值，那么会以name(...)形式
    // 如果返回非函数，那么会以type name(...)形式
    // 如果返回函数，那么会以type (*name(...))(...)
    // 如果返回嵌套函数，那么会以type (*(*name(...))(...))(...)形式

    const char* bracketPtr = NULL;
    bool bracket = false;
    for (const char* it = prettyFunction; *it; ++it)
    {
        if (*it == '(')
        {
            bracketPtr = it;
            bracket = true;
        }
        else if (*it == '*')
        {
            bracketPtr = NULL;
            bracket = false;
        }
        else if (*it == ' ')
        {
            // do nothing
        }
        else
        {
            // 在vs下还需排除__cdecl __stdcall __fastcall __thiscall 等调用规范限定符，gcc编译器先不考虑
            if (bracket)
            {
                break;
            }
        }
    }

    const char* dest = function;

    if (bracket && bracketPtr > prettyFunction) {
        const char* begin, * end = bracketPtr - 1;
        while (end > prettyFunction && !_prettyFunctionIsValidFuncChar(*end))
        {
            --end;
        }

        begin = end;
        while (begin >= prettyFunction && _prettyFunctionIsValidFuncChar(*begin))
        {
            --begin;
        }

        ++begin;

        if (end >= begin)
        {
            dest = _prettyFunctionSetFuncName(begin, end - begin);
        }
    }

    return dest;
}

//#endif // BFX_COMPILER_GCC

#if defined(BFX_COMPILER_MSVC)
#define BLOG_FUNCTION __FUNCTION__
#else // GNUC
#define BLOG_FUNCTION char BFX_MACRO_CONCAT( ((const char*)_prettyFunctionExtractor(__PRETTY_FUNCTION__, __FUNCTION__))
#endif // GNUC