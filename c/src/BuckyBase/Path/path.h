#pragma once

#if defined(BFX_OS_WIN)
typedef wchar_t	BfxPathCharType;
#else // posix
typedef char	BfxPathCharType;
#endif // posix

#if defined(BFX_OS_WIN)
#define BFX_PATH_LITERAL(x)	    L ## x
#define BFX_PATH_FORMAT		    "ls"
#define BFX_PATH_LOG_FORMAT		"S"
#else // posix
#define BFX_PATH_LITERAL(x)	    x
#define BFX_PATH_FORMAT		    "s"
#define BFX_PATH_LOG_FORMAT		"s"
#endif // posix

// 路径分割符
#define BFX_PATH_POSIX_SEPARATOR			BFX_PATH_LITERAL('/')
#define BFX_PATH_WINDOW_NORMAL_SEPARATOR	BFX_PATH_LITERAL('\\')

// 扩展名分隔符
#define BFX_PATH_EXT_SEPARATOR				BFX_PATH_LITERAL('.')

// 特殊文件夹
#define BFX_PATH_CURRENTDIR					BFX_PATH_LITERAL(".")
#define BFX_PATH_PARENTDIR					BFX_PATH_LITERAL("..")

// 当前平台下的标准路径分隔符
#if defined(BFX_OS_WIN)
#define BFX_PATH_SEPARATOR	BFX_PATH_WINDOW_NORMAL_SEPARATOR
#else // posix
#define BFX_PATH_SEPARATOR	BFX_PATH_POSIX_SEPARATOR
#endif // posix


// 当前平台支持的路径字符串的最大长度
#if defined(BFX_OS_WIN)
#define BFX_PATH_MAX	MAX_PATH
#else // posix
#define BFX_PATH_MAX	PATH_MAX
#endif // posix

// 连接两个路径字符串，返回dest
BFX_API(BfxPathCharType*) BfxPathStringCat(BfxPathCharType* dest, size_t size, const BfxPathCharType* src);

// 比较两个路径 返回-1/0/1
BFX_API(int) BfxPathCompare(const BfxPathCharType* left, const BfxPathCharType* right);

BFX_API(size_t) BfxPathLength(const BfxPathCharType* path);

// 判断否是相对路径
BFX_API(bool) BfxPathIsRelative(const BfxPathCharType *path);

// 判断目标文件是否存在
BFX_API(bool) BfxPathFileExists(const BfxPathCharType* path);

// 判断一个路径是不是相对路径
BFX_API(bool) BfxPathIsRelative(const BfxPathCharType* path);

// 在路径后添加标准路径分隔符，如果path长度允许的话
// C: -> C:\; C:\test -> C:\test\;
BFX_API(BfxPathCharType*) BfxPathAddBackslash(BfxPathCharType* path, size_t size);

// 判断一个路径是不是有效的UNC路径
// 比如下述路径都是UNC路径： \\path\test; \\path; \\?\C:\
// 需要注意该函数不对指向本机磁盘的unc做额外解析
BFX_API(bool) BfxPathIsUNC(const BfxPathCharType* path);

// 判断是不是诸如 \\server\share形式的路径
BFX_API(bool) BfxPathIsUNCServerShare(const BfxPathCharType* path);

// 去除路径里面多余的. ..等符号
BFX_API(bool) BfxPathCanonicalize(BfxPathCharType* dest, const BfxPathCharType* path);

// 拼接dirname和filename到dest，返回dest
BFX_API(BfxPathCharType*) BfxPathJoin(BfxPathCharType* dest, const BfxPathCharType* dirname, const BfxPathCharType* filename);

/*
C:\foo      -> C:\
C:\foo\bar  -> C:\foo
C:\foo\     -> C:\foo
C:\         -> C:\
\\x\y\x     -> \\x\y
\\x\y       -> \\x
\\x         -> \\ (Just the double slash!)
\foo        -> \  (Just the slash!)
*/
BFX_API(bool) BfxPathRemoveFileSpec(BfxPathCharType* path);


BFX_API(bool) BfxPathIsDirectory(const BfxPathCharType* path);

BFX_API(const BfxPathCharType*) BfxPathFindFileName(const BfxPathCharType* path);

BFX_API(const BfxPathCharType*) BfxPathFindExtension(const BfxPathCharType* path);