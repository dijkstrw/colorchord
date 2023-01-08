/*
 * This is almost entirely DisplayFileWrite, adjusted by
 * Willem Dijkstra, so that the output is understood by the Orochi 5x5
 * keyboard.
 *
 * The really cool bits are all by Charles Lohr -- thanks mate! His
 * original copyright is below.
 */

//Copyright 2015 <>< Charles Lohr under the ColorChord License.

#include "outdrivers.h"
#include "notefinder.h"
#include <stdio.h>
#include "parameters.h"
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "color.h"
#include "CNFG.h"
#include <sys/mman.h>
#include <sys/stat.h>        /* For mode constants */
#include <fcntl.h>           /* For O_* constants */
#include <unistd.h>

extern struct NoteFinder * nf;

struct OrochiDriver
{
	int lights_file;
	int total_leds;
        int size;
	char pass_buffer[2 + (MAX_LEDS*6)];
	og_thread_t  rt_thread;
	og_sema_t rt_sema;
        char command[2];
};

static void * LightsWrite( void * v )
{
	struct OrochiDriver * d = (struct OrochiDriver *)v;
	while(1)
	{
		OGLockSema( d->rt_sema );
		if( d->lights_file > 0 )
		{
			int r = write( d->lights_file, d->pass_buffer, d->size );
			if( r != d->size ) goto fail_write;
		}
		else
		{
			const char * lightsfile = GetParameterS( "lightsfile", 0 );
			if( lightsfile )
			{
				d->lights_file = open( lightsfile, O_WRONLY  );
				if( d->lights_file <= 0 )
				{
					fprintf( stderr, "Error: Can't open \"%s\" (%d)\n", lightsfile, d->lights_file );
				}
				else
				{
					fprintf( stderr, "File %s opened OK\n", lightsfile );
				}
			}
		}
		continue;
	fail_write:
		fprintf( stderr, "File writing fault\n" );
		close( d->lights_file );
		d->lights_file = 0;
	}
	return 0;
}

static void OrochiUpdate(void * id, struct NoteFinder*nf)
{
	struct OrochiDriver * d = (struct OrochiDriver*)id;
	int i, position = 0;

	if( OGGetSema( d->rt_sema ) > 0 ) return;
        d->pass_buffer[position++] = d->command[0];
	for( i = 0; i < d->total_leds; i++ )
	{
                snprintf((char *)&d->pass_buffer[position], 1 + (3 * 2),
                                 "%02x%02x%02x",
                                 OutLEDs[i * 3 + 0],
                                 OutLEDs[i * 3 + 1],
                                 OutLEDs[i * 3 + 2]);
                position += 6;
	}
        d->pass_buffer[position++] = '\n';
        d->size = position;
        OGUnlockSema( d->rt_sema );
}

static void OrochiParams(void * id )
{
	struct OrochiDriver * d = (struct OrochiDriver*)id;

	d->total_leds = 25;	RegisterValue( "leds", PAINT, &d->total_leds, sizeof( d->total_leds ));
        d->command[0] = 'B';    RegisterValue( "command", PABUFFER, &d->command, sizeof( d->command ));
        d->size = 2;
}

static struct DriverInstances * DisplayOrochi(const char * parameters)
{
	struct DriverInstances * ret = malloc( sizeof( struct DriverInstances ) );
	struct OrochiDriver * d = ret->id = malloc( sizeof( struct OrochiDriver ) );
	memset( d, 0, sizeof( struct OrochiDriver ) );
	ret->Func = OrochiUpdate;
	ret->Params = OrochiParams;
	OrochiParams( d );
	printf( "Loaded DisplayOrochi\n" );
        d->rt_sema = OGCreateSema();
	d->rt_thread = OGCreateThread( LightsWrite, d );
	return ret;
}

REGISTER_OUT_DRIVER(DisplayOrochi);
