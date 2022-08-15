//#include <stdlib.h>
#include <alsa/asoundlib.h>

//#include <sys/types.h>
//#include <sys/stat.h>
//#include <fcntl.h>

typedef snd_pcm_t pcm_t;
typedef snd_seq_t midi_t;

typedef enum {PLAYBACK, CAPTURE} pcm_dir_t;
typedef enum {OUTPUT = 1, INPUT, DUPLEX} midi_dir_t;
typedef enum {FALSE = 0, TRUE} bool_t;

typedef struct pcm_struct {
	pcm_t*		handle;
	char*		audiodev;
	float*		buffer;	//type should be based on data format
	unsigned int	frames;
	unsigned int	samplerate;
	unsigned int	channels;
	pcm_dir_t	direction;
	bool_t		interleaved;
	//sample format and endianess to be handled
	//default float 32b (-1.0:1.0), little endian
	//fields to be added to introduce new customization options
} pcm_stream_t;
typedef struct midi_struct {
	midi_t*		handle;
	char*		mididev;
	char*		clientname;
	char*		portname;
	int		portid;
	midi_dir_t	direction;
	bool_t		nonblocking;
	//client name, queue tempo, etc... to be handled
} midi_stream_t;

void pcm_init(pcm_stream_t*);

void pcm_capture(pcm_stream_t*);

void pcm_play(pcm_stream_t*);

void pcm_end(pcm_stream_t*);

void midi_init(midi_stream_t*);

void midi_noteon(midi_stream_t*, int, int, int);

void midi_noteoff(midi_stream_t*, int, int);

void midi_end(midi_stream_t*);

