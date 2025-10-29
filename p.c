#if defined( DEVELOPMENT_ )

#pragma comment( lib , "kernel32.lib"  )
#pragma comment( lib , "libucrt.lib"   )
#pragma comment( lib , "vcruntime.lib" )

#include <assert.h>
#include <memory.h>

#include <immintrin.h>

#include <Windows.h>

#endif

// storage types
typedef void                                   V ; //   0-bit
typedef unsigned char                          P ; //   1-bit
typedef unsigned char                          B ; //   8-bit
typedef unsigned short                         H ; //  16-bit
typedef unsigned int                           W ; //  32-bit
typedef unsigned long long                     L ; //  64-bit
typedef __attribute__(( vector_size( 32 ) )) B Y ; // 256-bit

// encoded types
typedef char ASCII ; // ASCII character
typedef L    A     ; // address

// address operations
#define A_(   value   ) ( ( A            ) & ( value   ) )
#define ARB_( address ) ( ( B * restrict )   ( address ) )
#define ARH_( address ) ( ( H * restrict )   ( address ) )
#define ARW_( address ) ( ( W * restrict )   ( address ) )
#define ARL_( address ) ( ( L * restrict )   ( address ) )
#define ARY_( address ) ( ( Y * restrict )   ( address ) )

// `CLALIGNED_` : an attribute that aligns a declaration to the size of a cache line.
#define CLALIGNED_ __attribute__(( aligned( 64 ) ))

#undef NULL
#undef FALSE
#undef TRUE

#define NULL  ( 0      )
#define FALSE ( 0 != 0 )
#define TRUE  ( !FALSE )

#define CHARACTER_SPACE  ( ' '  )
#define CHARACTER_NULL   ( '\0' )
#define CHARACTER_ESCAPE ( '\\' )
#define CHARACTER_MARK   ( '"'  )

#define COMMAND_SIZE_MAX ( 1 << 13 ) // 8192 byte

#define OPTION_INDEX_MAX_LOG2  ( 13 )
#define OPTION_TYPE_COUNT_LOG2 (  3 )

#define OPTION_NAME_SIZE_MAX ( 1 << 5                      ) //  32 byte
#define OPTION_COUNT_MAX     ( 1 << 8                      ) // 256 option
#define OPTION_TYPE_COUNT    ( 1 << OPTION_TYPE_COUNT_LOG2 ) //   4 type

#define OPTION_TYPE_U ( 0 ) // 64-bit unsigned integer
#define OPTION_TYPE_S ( 1 ) // 64-bit signed integer
#define OPTION_TYPE_F ( 2 ) // 64-bit floating-point number
#define OPTION_TYPE_A ( 3 ) // 64-bit address (into the original command container)

#define TERMINATE( status )  \
	__asm__(                   \
		"mov rax, 0x2C       \n" \
		"mov rcx, -1         \n" \
		"mov rdx, " #status "\n" \
		"syscall             \n" \
	)

static inline W WALIGNR( W value , W alignment )
{
	register W misalignment ;
	register P do_align     ;

#if defined( DEVELOPMENT_ )
	assert( alignment % 2 == 0 ) ;
#endif

	misalignment = value & ( alignment - 1 ) ;
	do_align     = misalignment == 0         ;
	alignment   -= misalignment              ;
	value       += do_align ? 0 : alignment  ;

	return value ;
}

static inline Y YCMPEQB( Y left , Y right )
{
	register Y result ;

	__asm__(
		"vpcmpeqb %0, %1, %2"
		: "=x" ( result )
		:  "x" ( left   )
		,  "x" ( right  )
	) ;

	return result ;
}

static inline W YMOVMSKB( Y value )
{
	W result ;

	__asm__(
		"vpmovmskb %0, %1"
		: "=r"(result)
		:  "x"(value)
	);

	return result ;
}

static inline W WT0CNT( W value )
{
	register W result ;

	__asm__(
		"tzcnt %0, %1"
		: "=r" ( result )
		:  "r" ( value  )
	) ;

	return result ;
}

static inline W WT1CNT( W value )
{
	register W result ;

	__asm__(
		"popcnt %0, %1"
	 	: "=r" ( result )
	  :  "r" ( value  )
	);

	return result ;
}

static inline Y YZERO( )
{
	register Y result ;

	__asm__(
		"vpxor %0, %1, %1"
		: "=x" ( result )
		:  "x" ( result )
	);

	return result ;
}


static inline V BCOPY( A destination , A source , W count )
{
#if defined( DEVELOPMENT_ )
	( V ) memcpy( ARB_( destination ) , ARB_( source ) , count ) ;
#endif
}

static inline V YCOPY( A destination , A source , W count )
{
#if defined( DEVELOPMENT_ )
	BCOPY( destination, source, count * sizeof( Y ) ) ;
#endif
}

struct
{
	B command[ COMMAND_SIZE_MAX ] CLALIGNED_ ;

	B option_name_list[ OPTION_COUNT_MAX ][ OPTION_NAME_SIZE_MAX ] CLALIGNED_ ;
}
global ;

struct
{
	B command[ COMMAND_SIZE_MAX ] CLALIGNED_ ;

	B command_option_index_list[ COMMAND_SIZE_MAX / 8 ] CLALIGNED_ ;

	B option_name_list[ OPTION_COUNT_MAX ][ OPTION_NAME_SIZE_MAX ] CLALIGNED_ ;

	B option_name_size_list[ ( OPTION_COUNT_MAX + 8 - 1 ) / 8] CLALIGNED_ ;
}
local;

V THREAD0( V )
{
	// iteration counters.

	register W i , j ;

#if defined( DEVELOPMENT_ )
	{
		register A command;

		command = ( A )GetCommandLineA( ) ;

		BCOPY( A_( global.command ) , command , strlen( ( char * ) command ) ) ;

#define F_( index , name ) \
	BCOPY( A_( global.option_name_list[ index ] ) , A_( name ) , sizeof( name ) - 1 );
		F_( 0 , "a"    )
		F_( 1 , "bb"   )
		F_( 2 , "ccc"  )
		F_( 3 , "dddd" )
#undef F_
	}
#endif

	// load command from global into local.

	BCOPY( A_( local.command ) , A_( global.command ) , COMMAND_SIZE_MAX ) ;

	// clean command.
	// "clean" implies spaces and quotes are transformed into NULL.

	{
		register A command_address ;
		register W character ;
		register P is_space , is_mark , is_valid_mark , do_mark , do_escape , do_erase ;

		command_address = A_( local.command ) ;
		do_mark         = FALSE               ;
		do_escape       = FALSE               ;

		for( i = 0 ; i < COMMAND_SIZE_MAX ; i += 1 )
		{
			character = ARB_( command_address )[ i ] ;

			// check spaces too for erasure.

			is_space = character == CHARACTER_SPACE ;

			// the mark is valid only if not escaped.

			is_mark       = character == CHARACTER_MARK ;
			is_valid_mark = is_mark && !do_escape       ;

			// every other valid mark ends the quote.

			do_mark ^= is_valid_mark ;

			// the next character is escaped if the current character is an escape.

			do_escape = character == CHARACTER_ESCAPE ;

			// erase the character if applicable.
			// the `|| is_valid_mark` ensures the current mark is erased too.

			do_erase  = is_space || do_mark || is_valid_mark  ;
			character = do_erase ? CHARACTER_NULL : character ;

			// store.

			ARB_( command_address )[ i ] = character ;
		}
	}

	// find options.
	// 

	{
		register A input_address , output_address , store_address ;
		register W character_0, character_1, store ;
		register P is_null_0 , is_null_1 , do_option ;

		input_address  = A_( local.command                   ) ;
		output_address = A_( local.command_option_index_list ) ;

		for( i = 1 ; i < COMMAND_SIZE_MAX ; i += 1 )
		{
			character_0 = ARB_( input_address )[ i - 1 ] ;
			character_1 = ARB_( input_address )[ i - 0 ] ;

			// check if we're at the onset of an option.
			// NULL followed by a non-NULL implies the start of an option.

			is_null_0 = character_0 == CHARACTER_NULL ;
			is_null_1 = character_1 == CHARACTER_NULL ;
			do_option = is_null_0 && !is_null_1       ;

			// ...

			store_address = output_address + i / 8     ;
			store         = ARB_( store_address )[ 0 ] ;

			// insert the `do_option`.

			store |= do_option << ( i % 8 ) ; 

			// store.

			ARB_( store_address )[ 0 ] = store ;
		}
	}

	// load options from global into local.

	YCOPY( A_( local.option_name_list ) , A_( global.option_name_list ) , OPTION_COUNT_MAX ) ;

	// derive the size of each option

	{
		register A input_address , output_address , store_address  ;
		register Y load , null_vector , mask_vector ;
		register W mask , size , store , l , h ;

		input_address  = A_( local.option_name_list      ) ;
		output_address = A_( local.option_name_size_list ) ;

		null_vector = YZERO( ) ;

		for( i = 0 ; i < OPTION_COUNT_MAX ; i += 1 )
		{
			// load the name a 32-byte vector
			// because a name's max size is 32 bytes.

			load = ARY_( input_address )[ i ] ;

			// derive the size of option's name.
			// comparing the name with `null_vector` flags the name's bytes as 0,
			// thereby counting the trailing 0s equates to the name's size.

			mask_vector = YCMPEQB ( load , null_vector ) ;
			mask        = YMOVMSKB( mask_vector        ) ;
			size        = WT0CNT  ( mask               ) ;

			// get address of store at 8-bit granularity.

			store_address = output_address + i * 5 / 8 ;
			store         = ARB_( store_address )[ 0 ] ;

			// insert the size of the option's name.
			// we're using 2*8 bits (16 bits) of `store` for 2 contiguous bytes
			// because a name's max size is unaligned between 2 bytes.

			store |= size << ( i * 5 % 8 ) ;
			
			// store 16-bit `load` at 8-bit granularity.

			ARH_( store_address )[ 0 ] = store;
		}
	}

#if 0

	// check options

	{
		register Y null_y_vector, name_a , name_b , cmp_mask_vector;
		register A command , indices , options;
		register W bits , bit , name_b_size , cmp_mask , cmp_size , option_index;
		register P do_option , is_equal;

		null_y_vector = YZERO( );

		command = A_( local.command );
		indices = A_( local.indices );
		options = A_( local.options );

		for( i = 0; i < COMMAND_SIZE_MAX; i += 1 )
		{
			bits = ARB_( indices )[ i / 8 ];
			bit  = bits & ( 1 << ( i % 8 ) );

			// the option is valid if `bit` is 1.

			do_option = bit == 1;

			// load the name of the option from `command`.

			name_a      = ARY_( command )[ i ];
			name_a_size = ARb_(  );

			// ... 

			name_a = do_option ? null_y_vector : name_a;

			option_index = -1;

			for( j = 0; j < OPTION_COUNT_MAX; j += 1 )
			{
				name_b = ARY_( options )[ j ];

				// guage the size of `name_b`

				// check the option

				cmp_mask_vector = YCMPEQB ( name_a, name_b  );
				cmp_mask       = YMOVMSKB( cmp_mask_vector  );
				cmp_size       = WT1CNT  ( cmp_mask         );

				is_equal     = name_b_size == cmp_size;
				option_index = is_equal ? j : option_index;
			}
		}
	}

#endif

	TERMINATE( 0 );
}
