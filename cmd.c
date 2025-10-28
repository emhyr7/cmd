#define DEV_

#define LINUX_

////////////////////////////////////////////////////////////////////////////////

#include <immintrin.h>

#if defined( WIN32_ )

#include <Windows.h>

#elif defined( LINUX_ )

#include <fcntl.h>
#endif

////////////////////////////////////////////////////////////////////////////////

#if defined( DEV_ )

#include <assert.h>
#include <string.h>

#pragma comment( lib, "libucrt.lib" )

#endif

////////////////////////////////////////////////////////////////////////////////

#undef NULL
#undef FALSE
#undef TRUE

// constants

#define NULL  ( 0       )
#define FALSE ( 0 != 0  )
#define TRUE  ( !FALSE  )
#define KB    ( 1 << 10 )
#define MB    ( 1 << 20 )
#define GB    ( 1 << 30 )

////////////////////////////////////////////////////////////////////////////////

// keywords

#define STDCALL_               __stdcall
#define ASSERT_(  expression ) _Static_assert( (expression), "" )
#define ALIGN_(   alignment  ) __attribute__(( aligned( alignment ) ))
#define INLINE_                inline __attribute__(( always_inline ))
#define ASM_                   __asm__
#define COUNTOF_( array      ) ( sizeof( array ) / sizeof( (array)[ 0 ] ) )

////////////////////////////////////////////////////////////////////////////////

// storage types

typedef void                                   V; //   0-bit
typedef unsigned char                          P; //   1-bit
typedef unsigned char                          B; //   8-bit
typedef unsigned short                         H; //  16-bit
typedef unsigned int                           W; //  32-bit
typedef unsigned long long                     L; //  64-bit
typedef B __attribute__(( vector_size( 16 ) )) X; // 128-bit
typedef B __attribute__(( vector_size( 32 ) )) Y; // 256-bit
typedef B __attribute__(( vector_size( 64 ) )) Z; // 512-bit

// numeric types

typedef signed             int SW; // 32-bit signed
typedef signed long long   int SL; // 64-bit signed
typedef unsigned           int UW; // 32-bit unsigned
typedef unsigned long long int UL; // 64-bit unsigned
typedef float                  FW; // 32-bit floating-point
typedef double                 FL; // 64-bit floating-point
typedef SW                     S ;
typedef UW                     U ;
typedef FW                     F ;
typedef UL                     A ; // address

/// encoded types

typedef char ASCII; // ASCII character
typedef char UTF8 ; // UTF-8 character

////////////////////////////////////////////////////////////////////////////////

#define A_(   value         ) ( ( L             ) &( value   ))
#define ARB_( address       ) ( ( B    *restrict)  ( address ))
#define ARH_( address       ) ( ( H    *restrict)  ( address ))
#define ARW_( address       ) ( ( W    *restrict)  ( address ))
#define ARL_( address       ) ( ( L    *restrict)  ( address ))
#define ARX_( address       ) ( ( X    *restrict)  ( address ))
#define ARY_( address       ) ( ( Y    *restrict)  ( address ))
#define ARZ_( address       ) ( ( Z    *restrict)  ( address ))
#define ART_( address, type ) ( ( type *restrict)  ( address ))

static inline Y YLD(A source)
{
	register Y value;

	ASM_(
		"vmovdqa %0, %1"
		: "=x" ( value  )
		:  "m" ( source )
	);

	return value;
}

static inline V YST( A destination, Y value )
{
	ASM_(
		"vmovdqa %0, %1"
		: "=m" ( destination )
		:  "x" ( value       )
	);
}

static inline Y YXOR( Y left, Y right )
{
	register Y result;

	ASM_(
		"vpxor %0,%1,%2"
		: "=x" ( result )
		:  "x" ( left   )
		,  "x" ( right  )
	);

	return result;
}

static inline V BCOPY( A destination, A source, W count )
{
	register W i, block;

	for( i = 0; i < count; i += 1 )
	{
		block = ARB_( source )[ i ];
		ARB_( destination )[ i ] = block;
	}
}

static inline V YCOPY( A destination, A source, W count )
{
	register W i;
	register Y block;

	for( i = 0; i < count; i += 1 )
	{
		block = ARY_( source )[ i ];
		ARY_( destination )[ i ] = block;
	}
}

static inline W WALIGNR( W value, W alignment )
{
	register W misalignment;
	register P aligned;

#if defined( DEV_ )
	assert( alignment % 2 == 0 );
#endif

	misalignment = value & ( alignment - 1 );
	aligned      = misalignment == 0;
	alignment   -= misalignment;
	value       += aligned ? 0 : alignment;

	return value;
}

////////////////////////////////////////////////////////////////////////////////

#define CHARACTER_SPACE  (' ' )
#define CHARACTER_NULL   ('\0')
#define CHARACTER_ESCAPE ('\\')
#define CHARACTER_MARK   ('"' )

#define COMMAND_SIZE_MAX     ( 1 << 13 ) // 8192 byte
#define COMMAND_PADDING_SIZE ( 8 )

#define OPTION_INDEX_MAX_LOG2  ( 13 )
#define OPTION_TYPE_COUNT_LOG2 (  3 )
#define OPTION_NAME_SIZE_MAX   ( 32 )
#define OPTION_COUNT_MAX       ( 1 << 6                      ) //   64 option
#define OPTION_STRING_SIZE_MAX ( 1 << 6                      ) //   64 byte
#define OPTION_INDEX_MAX       ( 1 << OPTION_INDEX_MAX_LOG2  ) // 8192 bit : 1024 byte
#define OPTION_TYPE_COUNT      ( 1 << OPTION_TYPE_COUNT_LOG2 ) //    4 variant

#define OPTION_TYPE_U ( 0 ) // 64-bit unsigned integer
#define OPTION_TYPE_S ( 1 ) // 64-bit signed integer
#define OPTION_TYPE_F ( 2 ) // 64-bit floating-point number
#define OPTION_TYPE_A ( 3 ) // 64-bit address (into the original command container)

ASSERT_( OPTION_NAME_SIZE_MAX == sizeof( Y ) );

static struct
{
	ALIGN_( 64 ) B command[ COMMAND_SIZE_MAX ];
	ALIGN_( 64 ) B options[ OPTION_COUNT_MAX ][ OPTION_NAME_SIZE_MAX ];
}
l2;

static struct
{
	ALIGN_( 64 ) B command[ COMMAND_SIZE_MAX + COMMAND_PADDING_SIZE ];

	// `indices` is an array of bits.
	// each bit corresponds to an index.
	// if the value of a bit is 0, then the corresponding index is invalid.
	ALIGN_( 64 ) B indices[ COMMAND_SIZE_MAX / 8 ];

	ALIGN_( 64 ) B options[ OPTION_COUNT_MAX ][ OPTION_NAME_SIZE_MAX ];
}
l1;

// GOAL: ingest a table of mappings between options and bit indices, then vomit a corresponding data structure.

V THREAD0( )
{

	return;
#if defined( DEV_ )
#if defined( WIN32_ )
  // emulate usage.
  A command = ( A )GetCommandLineA(  );
  BCOPY( A_( l2.command ), command, strlen(  ( char * ) command ) );
#elif defined( LINUX_ )
  
#endif
#endif

	register W i, j, k;

	// move the command into L1d$
	YCOPY( A_( l1.command ), A_( l2.command ), COMMAND_SIZE_MAX / sizeof( Y ) );

	// erase quotes within command
	{
		register A command;
		register W character;
		register P is_mark, is_valid_mark, do_mark, do_escape, do_erase;

		command   = A_( l1.command );
		do_mark   = FALSE;
		do_escape = FALSE;

		for( i = 0; i < COMMAND_SIZE_MAX; i += 1 )
		{
			character = ARB_( command )[ i ];

			// the mark is valid only if not escaped.
			is_mark       = character == CHARACTER_MARK;
			is_valid_mark = is_mark && !do_escape;

			// every other valid mark ends the quote.
			do_mark ^= is_valid_mark;

			// check if the next character is escaped.
			do_escape = character == CHARACTER_ESCAPE;

			// the `|| is_valid_mark` ensures the mark is erased too.
			do_erase  = do_mark || is_valid_mark;
			character = do_erase ? CHARACTER_SPACE : character;

			ARB_( command )[ i ] = character;
		}
	}

	// find options
	{
		register A command, indices;
		register W character_0, character_1, bit, bits;
		register P is_space_0, is_space_1, do_option;

		command = A_( l1.command );
		indices = A_( l1.indices );

		for( i = 0; i < COMMAND_SIZE_MAX; i += 1 )
		{
			character_0 = ARB_( command )[ i + 0 ];
			character_1 = ARB_( command )[ i + 1 ];

			// a space followed by a non-space implies the start of an option.
			is_space_0 = character_0 == ' ';
			is_space_1 = character_1 == ' ';
			do_option  = is_space_0 && !is_space_1;

			// shift `do_option` to the proper bit.
			bit = do_option << ( i % 8 );

			// insert `bit`.
			bits = ARB_( indices )[ i / 8 ];
			bits = bits | bit;

			ARB_( indices )[ i / 8 ] = bits;
		}
	}

	// move options from L2d$ to L1d$.
	YCOPY( A_( l1.options ), A_( l2.options ), OPTION_COUNT_MAX * OPTION_NAME_SIZE_MAX );
	
	// check options
	#if 0
	{
		register Y null_name, name_a, name_b, y0;
		register A command, indices, options, bits, bit;
		register P do_option;

		command = A_( l1.command );
		indices = A_( l1.indices );
		options = A_( l1.options );

		for( i = 0; i < COMMAND_SIZE_MAX; i += 1 )
		{
			bits = ARB_( indices )[ i / 8 ];
			bit  = bits & ( 1 << ( i % 8 ) );

			// the option is valid if `bit` is 1.
			do_option = bit == 1;

			// 
			
			null_name = YXOR( name_a, name_a );
			name_a    = ARY_( command )[ i + 1 ];
			name_a    = do_option ? null_name : name_a;

			for( j = 0; j < OPTION_COUNT_MAX; i += 1 )
			{
				name_b = ARY_( options )[ j ];

				y0 = YCMPEQB( name_a, name_b );
			}
		}
	}
	#endif
}
