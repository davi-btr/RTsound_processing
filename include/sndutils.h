/*
 *sndutils.c
 *
 *library for sndprocess
 *
*/

#define MIN_KEY		0
#define MAX_KEY 	127

#ifndef BOOL_TYPE
#define BOOL_TYPE
typedef enum {FALSE = 0, TRUE} bool_t;
#endif

typedef struct note_struct {
  bool_t	on;
  int		key;
  int		vel;
} note_info_t;

#ifndef NOTE_STATS
#define NOTE_STATS
static float MIDI_freq[MAX_KEY - MIN_KEY + 1] = {8.1758, 8.6620, 9.1770, 9.7227, 10.3009, 10.9134, 11.5623, 12.2499, 12.9783, 13.7500, 14.5676, 15.4339, 16.3516, 17.3239, 18.3540, 19.4454, 20.6017, 21.8268, 23.1247, 24.4997, 25.9565, 27.5000, 29.1352, 30.8677, 32.7032, 34.6478, 36.7081, 38.8909, 41.2034, 43.6535, 46.2493, 48.9994, 51.9131, 55.0000, 58.2705, 61.7354, 65.4064, 69.2957, 73.4162, 77.7817, 82.4069, 87.3071, 92.4986, 97.9989, 103.8262, 110.0000, 116.5409, 123.4708, 130.8128, 138.5913, 146.8324, 155.5635, 164.8138, 174.6141, 184.9972, 195.9977, 207.6523, 220.0000, 233.0819, 246.9416, 261.6256, 277.1826, 293.6648, 311.1270, 329.6276, 349.2282, 369.9944, 391.9954, 415.3047, 440.0000, 466.1638, 493.8833, 523.2511, 554.3653, 587.3295, 622.2540, 659.2551, 698.4565, 739.9889, 783.9909, 830.6094, 880.0000, 932.3276, 987.7666, 1046.5022, 1108.7305, 1174.6590, 1244.5079, 1318.5103, 1396.9129, 1479.9777, 1567.9818, 1661.2188, 1760.0000, 1864.6549, 1975.5334, 2093.0045, 2217.4609, 2349.3183, 2489.0158, 2637.0203, 2793.8260, 2959.9554, 3135.9632, 3322.4377, 3520.0000, 3729.3098, 3951.0668, 4186.0089, 4434.9218, 4698.6366, 4978.0317, 5274.0406, 5587.6521, 5919.9109, 6271.9265, 6644.8755, 7040.0000, 7458.6214, 7902.1319, 8372.0178, 8869.8452, 9397.2715, 9956.0633, 10548.0829, 11175.3024, 11839.8217, 12543.8554};

static float octave_ratio[MAX_KEY - MIN_KEY + 1] = {0.8320, 0.5445, 0.4375, 1.0358, 1.8473, 1.7404, 1.0646, 0.3729, 0.6003, 0.4390, 0.4430, 0.9771, 0.8239, 0.5679, 0.5541, 0.6600, 0.6669, 0.4837, 0.5002, 0.2550, 0.4131, 0.3018, 0.4148, 0.4406, 0.4008, 0.2427, 0.2348, 0.3076, 0.2899, 0.1959, 0.0877, 0.2310, 0.2902, 0.5151, 0.2882, 6.0840, 6.4857, 9.9052, 3.3233, 24.2588, 25.5479, 51.3549, 221.5547, 65.8027, 69.7166, 19.3153, 118.6768, 0.5329, 0.6522, 0.5956, 0.7528, 0.7840, 0.7617, 0.7808, 0.7494, 9.5647, 15.6207, 12.3036, 15.6356, 2.1034, 2.3369, 2.3560, 2.3157, 2.6540, 2.4726, 2.5043, 2.6289, 25871.7402, 812.3201, 9371.0049, 564.9780, 197.5410, 273.3660, 189.7327, 216.9722, 1.1885, 1.2831, 1.3546, 1.4511, 8.0115, 8.2573, 8.6290, 8.6818, 8.5963, 9.2611, 175.4133, 133.8493, 182.0157, 155.8724, 140.5083, 180.3035, 56.0204, 73.1021, 101.3058, 91.4193, 1373.6012, 6365.3931, 4328.1685, 5417.5098, 7241.2695, 12583.2939, 18851.5469, 7884.6582, 87786.6875, 9609.6602, 3185.8020, 1096.9713, 474.6582, 494.5000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000};

static float harmonic_power_start[MAX_KEY - MIN_KEY + 1] = {2.8628, 2.6842, 2.5522, 2.2880, 2.2708, 2.1442, 2.0257, 1.9129, 1.7953, 1.6995, 1.6055, 1.5072, 1.4124, 1.3523, 1.2743, 1.1971, 1.1276, 1.0719, 1.0131, 0.9496, 0.8833, 0.8490, 0.7931, 0.7477, 0.7139, 0.6707, 0.6377, 0.5882, 0.5610, 0.5344, 0.5054, 0.4788, 0.4642, 0.4353, 0.4093, 0.4274, 0.4068, 0.3880, 0.4030, 0.4363, 0.4052, 0.4225, 0.4112, 0.3645, 0.4051, 0.3967, 0.3842, 0.3678, 0.3575, 0.3816, 0.3629, 0.4143, 0.4232, 0.4210, 0.4143, 0.3908, 0.3848, 0.3847, 0.3794, 0.3544, 0.3503, 0.3485, 0.3537, 0.3055, 0.3067, 0.3028, 0.3096, 0.3682, 0.3693, 0.3722, 0.3722, 0.3408, 0.3461, 0.3375, 0.3438, 0.1963, 0.1987, 0.2018, 0.1999, 0.3241, 0.3295, 0.3370, 0.3447, 0.3502, 0.3551, 0.4235, 0.4210, 0.4200, 0.4191, 0.4203, 0.4226, 0.3586, 0.3967, 0.4149, 0.3875, 0.3134, 0.3014, 0.2975, 0.2897, 0.2878, 0.1533, 0.1318, 0.1086, 0.0899, 0.0742, 0.0139, 0.0134, 0.0127, 0.0124, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000, 0.0000};
#endif

int get_index(float *, unsigned int, float *);

float harm_pwr_calc(float, float *, int, double);

int get_velocity(int, float, float);

