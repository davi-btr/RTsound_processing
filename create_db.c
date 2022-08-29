#include <stdlib.h>
#include <unistd.h>
#include <fluidsynth.h>

#define MIN_NOTE 64
#define MAX_NOTE 64+12

int main(int argc, char **argv)
{
  char *sfname = "/usr/share/sounds/sf2/FluidR3_GM.sf2";
  char fname[13] = "noteXXX.dat";

  for (int key = MIN_NOTE; key <= MAX_NOTE; key++) {
    fluid_settings_t *settings;
    fluid_synth_t *synth;
    fluid_audio_driver_t *adriver;
    int sfont_id;

    fname[4] = (char)(key / 100 + 0x30);
    fname[5] = (char)(key / 10 + 0x30);
    fname[6] = (char)(key % 10 + 0x30);

    printf("file %s\n", fname);
    //Create and change the settings
    settings = new_fluid_settings();
    fluid_settings_setstr(settings, "audio.driver", "file");
    fluid_settings_setstr(settings, "audio.file.format", "float");
    fluid_settings_setstr(settings, "audio.file.type", "raw");
    fluid_settings_setstr(settings, "audio.file.name", fname);
    fluid_settings_setint(settings, "audio.period-size", 1024);
    fluid_settings_setstr(settings, "audio.sample-format", "float");
    fluid_settings_setnum(settings, "synth.gain", 7.0);
    fluid_settings_setnum(settings, "synth.sample-rate", 48000.0);
    //Create the synthesizer
    synth = new_fluid_synth(settings);
    //Create the audio driver (starts the synthesizer)
    adriver = new_fluid_audio_driver(settings, synth);
    //Load a SoundFont and reset presets
    sfont_id = fluid_synth_sfload(synth, sfname, 1);
    if(sfont_id == FLUID_FAILED)
    {
        fprintf(stderr, "Loading the SoundFont failed!\n");

        goto err;
    }
    //Write note on output file
    fluid_synth_noteon(synth, 0, key, 127);
    printf("nota %d\n", key);
    //Sleep for 2 second
    sleep(2);
    //Stop the note
    fluid_synth_noteoff(synth, 0, key);
err:
    //Clean up
    delete_fluid_audio_driver(adriver);
    delete_fluid_synth(synth);
    delete_fluid_settings(settings);
  }

  return 0;
}

