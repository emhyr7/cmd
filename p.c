#define EXIT( ) __asm__("movl $60,%eax\nmovl $0,%ebx\nint $0x80")
#define OPEN(path, flags, mode) __asm__("movl $5,%eax\nmovll $0,%ebx\n")
#define READ( )

#define CHARACTER_SPACE  (' ' )
#define CHARACTER_NULL   ('\0')
#define CHARACTER_ESCAPE ('\\')
#define CHARACTER_MARK   ('"' )

#define COMMAND_SIZE_MAX     (1 << 13) // 8192 byte
#define COMMAND_PADDING_SIZE (1 <<  6) //   64 byte

#define OPTION_INDEX_MAX_LOG2  (13)
#define OPTION_TYPE_COUNT_LOG2 ( 3)

#define OPTION_NAME_SIZE_MAX   (1 << 6                     ) //   64 byte
#define OPTION_COUNT_MAX       (1 << 6                     ) //   64 option
#define OPTION_STRING_SIZE_MAX (1 << 6                     ) //   64 byte
#define OPTION_INDEX_MAX       (1 << OPTION_INDEX_MAX_LOG2 ) // 8192 bit    | 1024 byte
#define OPTION_TYPE_COUNT      (1 << OPTION_TYPE_COUNT_LOG2) //    4 variant

#define OPTION_TYPE_U (0) // 64-bit unsigned integer
#define OPTION_TYPE_S (1) // 64-bit signed integer
#define OPTION_TYPE_F (2) // 64-bit floating-point number
#define OPTION_TYPE_A (3) // 64-bit address (into the original command container)

THREAD0()
{
	/* retrieve the command */
	{
		W command_file_handle;

		command_file_handle = OPEN("/proc/self/");
	}

	/* NOTE: replace all NULL with SPACE */

	EXIT();
}
