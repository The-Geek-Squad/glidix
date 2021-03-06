/* $Id$ */

/* isblank( int )

   This file is part of the Public Domain C Library (PDCLib).
   Permission is granted to use, modify, and / or redistribute at will.
*/

#include <ctype.h>

#ifndef REGTEST

int isblank( int c )
{
    return (c == '\t') || (c == ' ');
}

#endif

#ifdef TEST
#include <_PDCLIB_test.h>

int main( void )
{
    TESTCASE( isblank( ' ' ) );
    TESTCASE( isblank( '\t' ) );
    TESTCASE( ! isblank( '\v' ) );
    TESTCASE( ! isblank( '\r' ) );
    TESTCASE( ! isblank( 'x' ) );
    TESTCASE( ! isblank( '@' ) );
    return TEST_RESULTS;
}

#endif
