
// ���з��Ķ���
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

// �ж��ǲ�����Ч�ĺ�������������::�������޶�����  xxx::xxx
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

        // FIXME �߳����ȵ��ͷ�
    }

    size_t realLen = min(len, maxLen - 1);
    strncpy(_prettyFunctionFuncName, name, realLen);
    _prettyFunctionFuncName[realLen] = '\0';

    return _prettyFunctionFuncName;
}

static const char* _prettyFunctionExtractor(const char* prettyFunction, const char* function) {

    // �����������ĺ������������漸�������
    // �������û����ֵ����ô����name(...)��ʽ
    // ������طǺ�������ô����type name(...)��ʽ
    // ������غ�������ô����type (*name(...))(...)
    // �������Ƕ�׺�������ô����type (*(*name(...))(...))(...)��ʽ

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
            // ��vs�»����ų�__cdecl __stdcall __fastcall __thiscall �ȵ��ù淶�޶�����gcc�������Ȳ�����
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