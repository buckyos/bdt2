
#ifndef __BUCKYBASE_RESULT_H__
#define __BUCKYBASE_RESULT_H__

// 错误码定义，以宏的形式提供，需要注意下面几点：
// 1. 0 代表成功，不可使用0值表示出错值
// 2. 所有宏定义都以BFX_RESULT_XXX形式定义，名称可以长些，避免重复和冲突
// 3. [1-100)之间的错误码，是全局通用的错误码，用以表示一些通用的出错提示，比如参数错误，内存不够等
// 4. 错误码定义必须遵循值从小到大的原则来定义，避免重复并且方便查找
// 5. 独立功能/模块的应该定义一组相关的错误码，并分配一个合适的区间(最好加以注明）


// 错误码分成三部分
// 0-16位的16个bit，表示错误码内容
// 17-24位的8个bit，表示错误码类型
// 25-32位的8个bit，暂时预留

// 错误码类型定义
#define BFX_RESULTTYPE_DEFAULT  0   // 默认类型
#define BFX_RESULTTYPE_SYSTEM   1   // 系统错误码

// 构造一个错误码
#define BFX_MAKE_RESULT(type, code) (((type & 0x000000FF) << 16) | (code & 0x0000FFFF))

// 获取错误码类型
#define BFX_GET_RESULTTYPE(result) ((result & 0x00FF0000) >> 16)

// 获取错误码的内容
#define BFX_GET_RESULTCODE(result) (result & 0x0000FFFF)

// 对于默认错误码，错误码值和对应错误码内容是相等的，也即高16bit为全0；
// 对于非默认错误码，需要使用BFX_GET_RESULTCODE来获取错误码的真正内容



// 0表示成功，所有返回错误码的接口都需遵循该原则！！！
#define BFX_RESULT_SUCCESS					0

/************************************************************************/
/*					 [1-99] 用以表示全局通用的错误码               */
/************************************************************************/

// 调用失败了
#define BFX_RESULT_FAILED					1

// 参数错误
#define BFX_RESULT_INVALIDPARAM				2
#define BFX_RESULT_INVALID_PARAM            BFX_RESULT_INVALIDPARAM

// 内存不够了
#define BFX_RESULT_OUTOFMEMORY				3

// 对应的接口或者功能没有实现
#define BFX_RESULT_NOT_IMPL					4

// 非法的调用线程
#define BFX_RESULT_INVALID_CALLTHREAD		5

// 异步操作提交成功
#define BFX_RESULT_PENDING					6

// 缓冲区不够
#define BFX_RESULT_NOT_ENOUGH_BUFFER		7

// 缓冲区过小
#define BFX_RESULT_INSUFFICIENT_BUFFER	    BFX_RESULT_NOT_ENOUGH_BUFFER

// 无效的缓冲区
#define BFX_RESULT_INVALID_BUFFER			8

// 指定的内容已经存在
#define BFX_RESULT_ALREADY_EXISTS           9

// 指定的内容不存在
#define BFX_RESULT_NOT_FOUND                10

// 错误的状态机状态
#define BFX_RESULT_INVALID_STATE			11

// 不支持的接口或者实现
#define BFX_RESULT_NOT_SUPPORT              12

// 无效的数字签名
#define BFX_RESULT_INVALID_SIGNATURE        13

// 无效的长度
#define BFX_RESULT_INVALID_LENGTH           14

// 无效的JSON字符串
#define BFX_RESULT_INVALID_JSON             15

// 无效的令牌
#define BFX_RESULT_INVALID_TOKEN            16

// 无效的指针
#define BFX_RESULT_INVALID_POINTER          17

// 非必须的选项，被忽略，语义算成功
#define BFX_RESULT_OPTION_IGNORE            18

// 不可识别的类型
#define BFX_RESULT_UNKNOWN_TYPE             19

// 操作不可无阻塞完成了
#define BFX_RESULT_EAGAIN                   20
#define BFX_RESULT_EWOULDBLOCK              BFX_RESULT_EAGAIN

// 关闭
#define BFX_RESULT_CLOSED                   21

// 解析失败
#define BFX_RESULT_PARSE_FAILED             22

// 无效的格式
#define BFX_RESULT_INVALID_FORMAT           23

// 无效的数量
#define BFX_RESULT_INVALID_COUNT            24

// 无效的状态
#define BFX_RESULT_INVALID_STATUS           25

// 无效的类型
#define BFX_RESULT_INVALID_TYPE             26

// 不可预期的、意外的结果
#define BFX_RESULT_UNEXPECTED               27

// 被取消了
#define BFX_RESULT_CANCELED                 28

// 超时了
#define BFX_RESULT_TIMEOUT                  29

// 没有更多数据了
#define BFX_RESULT_NO_MORE_DATA             30

// 被终止了
#define BFX_RESULT_STOP                     31

// 无效的sockaddr
#define BFX_RESULT_INVALID_SOCKADDR         32

// 不匹配
#define BFX_RESULT_UNMATCH                  33

// 未知结果
#define BFX_RESULT_UNKNOWN                  34

// 溢出错误
#define BFX_RESULT_OVERFLOW                 35

// 连接断开
#define BFX_RESULT_DISCONNECT				36

// 流式解析需要更多数据
#define BFX_RESULT_NEED_MORE_DATA		    37

// 权限不足
#define BFX_RESULT_ACCESS                   38

// 缓冲区满
#define BFX_RESULT_BUFFER_FULL		        39

// 没有改变
#define BFX_RESULT_NOT_CHANGED              40

// 已经做过该操作
#define BFX_RESULT_ALREADY_OPERATION        41

// 系统繁忙
#define BFX_RESULT_BUSY						42

// 太短
#define BFX_RESULT_TOO_SMALL			    43

// 太长
#define BFX_RESULT_TOO_LARGE			    44

// 无效flags
#define BFX_RESULT_INVALID_FLAGS                        100

// 无效字符
#define BFX_RESULT_INVALID_CHARS                        101

// 不完整字符
#define BFX_RESULT_INCOMPLETE_CHARS                     102

// 尚未支持的转换
#define BFX_RESULT_INVALID_TRANSLATE                    103

// 无效的编码输入
#define BFX_RESULT_INVALID_ENCODE                       104

#define BFX_RESULT_INVALID_PEERID	                    200

#endif //__BUCKYBASE_RESULT_H__