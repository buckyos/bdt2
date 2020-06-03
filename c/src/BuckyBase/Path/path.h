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

// ·���ָ��
#define BFX_PATH_POSIX_SEPARATOR			BFX_PATH_LITERAL('/')
#define BFX_PATH_WINDOW_NORMAL_SEPARATOR	BFX_PATH_LITERAL('\\')

// ��չ���ָ���
#define BFX_PATH_EXT_SEPARATOR				BFX_PATH_LITERAL('.')

// �����ļ���
#define BFX_PATH_CURRENTDIR					BFX_PATH_LITERAL(".")
#define BFX_PATH_PARENTDIR					BFX_PATH_LITERAL("..")

// ��ǰƽ̨�µı�׼·���ָ���
#if defined(BFX_OS_WIN)
#define BFX_PATH_SEPARATOR	BFX_PATH_WINDOW_NORMAL_SEPARATOR
#else // posix
#define BFX_PATH_SEPARATOR	BFX_PATH_POSIX_SEPARATOR
#endif // posix


// ��ǰƽ̨֧�ֵ�·���ַ�������󳤶�
#if defined(BFX_OS_WIN)
#define BFX_PATH_MAX	MAX_PATH
#else // posix
#define BFX_PATH_MAX	PATH_MAX
#endif // posix

// ��������·���ַ���������dest
BFX_API(BfxPathCharType*) BfxPathStringCat(BfxPathCharType* dest, size_t size, const BfxPathCharType* src);

// �Ƚ�����·�� ����-1/0/1
BFX_API(int) BfxPathCompare(const BfxPathCharType* left, const BfxPathCharType* right);

BFX_API(size_t) BfxPathLength(const BfxPathCharType* path);

// �жϷ������·��
BFX_API(bool) BfxPathIsRelative(const BfxPathCharType *path);

// �ж�Ŀ���ļ��Ƿ����
BFX_API(bool) BfxPathFileExists(const BfxPathCharType* path);

// �ж�һ��·���ǲ������·��
BFX_API(bool) BfxPathIsRelative(const BfxPathCharType* path);

// ��·������ӱ�׼·���ָ��������path��������Ļ�
// C: -> C:\; C:\test -> C:\test\;
BFX_API(BfxPathCharType*) BfxPathAddBackslash(BfxPathCharType* path, size_t size);

// �ж�һ��·���ǲ�����Ч��UNC·��
// ��������·������UNC·���� \\path\test; \\path; \\?\C:\
// ��Ҫע��ú�������ָ�򱾻����̵�unc���������
BFX_API(bool) BfxPathIsUNC(const BfxPathCharType* path);

// �ж��ǲ������� \\server\share��ʽ��·��
BFX_API(bool) BfxPathIsUNCServerShare(const BfxPathCharType* path);

// ȥ��·����������. ..�ȷ���
BFX_API(bool) BfxPathCanonicalize(BfxPathCharType* dest, const BfxPathCharType* path);

// ƴ��dirname��filename��dest������dest
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