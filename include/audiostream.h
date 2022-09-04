#include <alsa/asoundlib.h>

typedef snd_pcm_t pcm_t;
typedef snd_seq_t midi_t;

typedef enum {PLAYBACK, CAPTURE} pcm_dir_t;
typedef enum {OUTPUT = 1, INPUT, DUPLEX} midi_dir_t;

#ifndef BOOL_TYPE
#define BOOL_TYPE

typedef enum {FALSE = 0, TRUE} bool_t;

#endif

typedef struct pcm_struct {
	pcm_t*		handle;
	char*		audiodev;
	float*		buffer;		//type should be based on data format
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

int pcm_init(pcm_stream_t *);

int pcm_capture(pcm_stream_t *);

int pcm_play(pcm_stream_t *);

int pcm_end(pcm_stream_t *);

int midi_init(midi_stream_t *);

int midi_noteon(midi_stream_t *, int, int, int);

int midi_noteoff(midi_stream_t *, int, int);

int midi_end(midi_stream_t *);

