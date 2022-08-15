#include <stdlib.h>
#include <alsa/asoundlib.h>

#include "audiostream.h"

#define IF_ERR(op, msg, extra) if((err = op) < 0) {fprintf(stderr, msg, snd_strerror(err)); extra;} 
#define IF_ERR_EXIT(op, msg) if ((err = op) < 0) {fprintf(stderr, msg, snd_strerror(err)); exit(1);} 

#define DEBUG

#ifdef DEBUG
#define dbg_printf printf
#else
#define dbg_printf(args...)
#endif

void pcm_init(pcm_stream_t *settings)
{
	int err;
	snd_pcm_hw_params_t *hw_params;
	snd_pcm_access_t access;

	dbg_printf("init pcm\n");
	if (settings->audiodev == NULL) {
		char str[8] = "default";

		settings->audiodev = str;
	}

	dbg_printf("%s %d %d %d %d\n", settings->audiodev, settings->direction, settings->samplerate, settings->frames, settings->channels);

	IF_ERR_EXIT(snd_pcm_open(&(settings->handle), settings->audiodev, settings->direction, 0), "cannot open audio device (%s)\n")

	IF_ERR_EXIT((snd_pcm_hw_params_malloc(&hw_params)), "cannot allocate hardware parameter structure (%s)\n")

	IF_ERR_EXIT((snd_pcm_hw_params_any(settings->handle, hw_params)), "cannot initialize hardware parameter structure (%s)\n")

	if (settings->interleaved)
		access = SND_PCM_ACCESS_RW_INTERLEAVED;
	else
		access = SND_PCM_ACCESS_RW_NONINTERLEAVED;
	IF_ERR_EXIT((snd_pcm_hw_params_set_access(settings->handle, hw_params, access)), "cannot set acces type (%s)\n")


	IF_ERR_EXIT((snd_pcm_hw_params_set_format(settings->handle, hw_params, SND_PCM_FORMAT_FLOAT_LE)), "cannot set sample format (%s)\n")

	IF_ERR_EXIT((snd_pcm_hw_params_set_rate_near(settings->handle, hw_params, &(settings->samplerate), 0)), "cannot set sample rate (%s)\n")

	dbg_printf("Got actual rate: %u\n", settings->samplerate);

	IF_ERR_EXIT((snd_pcm_hw_params_set_channels(settings->handle, hw_params, settings->channels)), "cannot set channel count (%s)\n")

	//set_buffer_size_near() FRAMES

	IF_ERR_EXIT((snd_pcm_hw_params(settings->handle, hw_params)), "cannot set parameters (%s)\n")

	snd_pcm_hw_params_free(hw_params);

	IF_ERR_EXIT(snd_pcm_prepare(settings->handle), "cannot prepare audio interface (%s)\n")

	dbg_printf("pcm set!\n");
}

void pcm_capture(pcm_stream_t *stream)
{
	int err;

	if (stream->interleaved) {
		err = snd_pcm_readi(stream->handle, stream->buffer, stream->frames);
	} else {
		err = snd_pcm_readn(stream->handle, (void**)&(stream->buffer), stream->frames);
	}

	if (err != (stream->frames)) {
		fprintf(stderr, "read from audio interface failed (%s)\n", snd_strerror(err));
		exit(1);
	}
}

void pcm_play(pcm_stream_t *stream)
{
	int err;
	
	if (stream->interleaved) {
		IF_ERR((snd_pcm_writei(stream->handle, stream->buffer, stream->frames)), "write to audio interface failed (%s)\n", snd_pcm_prepare(stream->handle))
	} else {
		IF_ERR((snd_pcm_writen(stream->handle, (void**)&(stream->buffer), stream->frames)), "write to audio interface failed (%s)\n", snd_pcm_prepare(stream->handle))
	}
}

void pcm_end(pcm_stream_t *stream)
{
	int err;

	IF_ERR((snd_pcm_drain(stream->handle)), "retention of residual data failed (%s)\n", snd_pcm_drop(stream->handle))

	snd_pcm_close(stream->handle);
}

void midi_init(midi_stream_t* seq)
{//snd_seq_t **seq_handle, char* dev, char* port_name, int *port_id
	int err;
  
	dbg_printf("init seq\n");
	if (seq->mididev == NULL) {
		char str[8] = "default";
    
		seq->mididev = str;
	}
	dbg_printf("seq init with %s\n", seq->mididev);
	IF_ERR_EXIT((snd_seq_open(&(seq->handle), seq->mididev, seq->direction, seq->nonblocking)), "cannot open audio device (%s)\n")
	dbg_printf("sequencer opened\n");
	/*
	if (seq->clientname == NULL) {
		char str[17] = "test-MIDI-client";

		seq->portname = str;
	}
	snd_seq_set_client_name(seq->handle, seq->clientname);
	dbg_printf("clientname (%s)\n", seq->clientname);
	*/
	if (seq->portname == NULL) {
		char str[15] = "test-MIDI-port";

		seq->portname = str;
	}
	dbg_printf("port name (%s)\n", seq->portname);
	//port type also SND_SEQ_PORT_TYPE_APPLICATION
	IF_ERR_EXIT((snd_seq_create_simple_port(seq->handle, seq->portname, seq->direction, SND_SEQ_PORT_TYPE_MIDI_GENERIC)), "cannot open audio device (%s)\n")

	seq->portid = err;
	dbg_printf("port %d\n", seq->portid);
	//IF_ERR_EXIT(((err = snd_seq_connect_to(*seq_handle, *port_id, SND_SEQ_CLIENT_SYSTEM, SND_SEQ_PORT_SYSTEM_ANNOUNCE)) < 0), (stderr, "snd_seq_subscribe_to() failed: %s\n", snd_strerror(err)))
	//IF_ERR((snd_seq_connect_to(seq->handle, seq->portid, SND_SEQ_CLIENT_SYSTEM, SND_SEQ_PORT_SYSTEM_ANNOUNCE)), "snd_seq_subscribe_to() failed: %s\n", (dbg_printf("")))
  
}

void midi_noteon(midi_stream_t* seq, int chan, int key, int vel)
{
	snd_seq_event_t ev;
	
	snd_seq_ev_clear(&ev);
	snd_seq_ev_set_source(&ev, seq->portid);
	snd_seq_ev_set_subs(&ev);
	snd_seq_ev_set_direct(&ev);
	ev.type = SND_SEQ_EVENT_NOTEON;
	ev.data.note.channel = chan;
	ev.data.note.note = key;
	ev.data.note.velocity = vel;

	snd_seq_event_output_direct(seq->handle, &ev);
}

void midi_noteoff(midi_stream_t* seq, int chan, int key)
{
	snd_seq_event_t ev;
	
	snd_seq_ev_clear(&ev);
	snd_seq_ev_set_source(&ev, seq->portid);
	snd_seq_ev_set_subs(&ev);
	snd_seq_ev_set_direct(&ev);
	ev.type = SND_SEQ_EVENT_NOTEOFF;

	snd_seq_event_output_direct(seq->handle, &ev);
}

void midi_end(midi_stream_t *seq)
{
	snd_seq_close(seq->handle);
}


/*
int main (int argc, char *argv[])
{
  int err;
  pcm_t *pcm_stream;
  //seq_t *seq_stream;
  unsigned int samplerate = 48000, frames = 1024, channels = 2;
  char *audio_in = NULL;
  float *buf, dft_on_note[10], *x;
  //note_info_t table[MAX_KEY - MIN_KEY + 1];
  //snd_seq_event_t ev;
  
  pcm_stream_t sett = {NULL, audio_in, buf, frames, samplerate, channels, CAPTURE, TRUE};
  
  init_pcm(&sett);

  //init_seq(&seq_stream, midi_out, port_name, &port_id);
  pcm_capture(&sett);
  
  //snd_seq_close(seq_stream);
  snd_pcm_close(pcm_stream);
}
*/
