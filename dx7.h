typedef unsigned char Parm;		/* holds 0-127 value parameters */

struct dx7operator {
	Parm eg_rate[4];	/* Envelope generator rate 1-4		0-99 */
	Parm eg_level[4];	/* Envelope generator level 1-4		0-99 */
	Parm kls_break;		/* Keyboard level scale break point	0-99 */
	Parm kls_ldepth;	/* Keyboard level scale left depth	0-99 */
	Parm kls_rdepth;	/* Keyboard level scale right depth	0-99 */
	Parm kls_lcurve;	/* Keyboard level scale left curve	0-3 */
	Parm kls_rcurve;	/* Keyboard level scale right curve	0-3 */	
	Parm k_rate_scale;	/* Keyboard rate scaling		0-7 */
	Parm mod_sens;		/* Modulation sensitivity		0-3 */
	Parm vel_sens;		/* Key velocity sensitivity		0-7 */
	Parm out_level;		/* Output level				0-99 */
	Parm osc_mode;		/* Oscillator mode			0-1 */
	Parm freq_coarse;	/* Oscillator frequency coarse tune	0-31 */
	Parm freq_fine;		/* Oscillator frequency fine tune	0-99 */
	Parm detune;		/* Oscillator detune			0-14 */
};

struct dx7voice {
	struct dx7operator op[6]; /* Settings for operators 6..1 */
	Parm peg_rate[4];	/* Pitch envelope rates 1..4		0-99 */
	Parm peg_level[4];	/* Pitch envelope levels 1..4		0-99 */
	Parm algorithm;		/* Algorithm				0-31 */
	Parm feedback;		/* Feedback				0-7 */
	Parm osc_sync;		/* Oscillator synchronize		0-1 */
	Parm lfo_speed;		/* Low frequency oscillator speed	0-99 */
	Parm lfo_delay;		/* Low frequency oscillator delay	0-99 */
	Parm lfo_pmd;		/* LFO pitch modulation depth		0-99 */
	Parm lfo_amd;		/* LFO amplitude modulation depth	0-99 */
	Parm lfo_sync;		/* LFO synchronize			0-1 */
	Parm lfo_wave;		/* LFO wave (photocopy *says* 9-4!)	0-4 */
	Parm lfo_pms;		/* Pitch modulation sensitivity		0-7 */
	Parm transpose;		/* Transpose				0-48 */
	Parm name[10];		/* Voice name				ascii */
};

/* Common structure of 4 function paramater pairs */
struct dx7slider {
	Parm range;
	Parm assign;
};

/*
 *	Remember the state of all the global settings of the DX7.
 *	This is based on the Transmission Data section of the MIDI spec.
 */
struct dx7state {
	Parm key[128];		/* last velocity (including 0) received */
		/* control change parameters */
	Parm control[128];	/* control settings */
	Parm program;		/* last program change received */
	Parm after;		/* after-touch value */
	Parm pb_lsb, pb_msb;	/* pitch bend, 7 bits each */

	/* Function parameters g=2, parm 64..77 */
	Parm mono;		/* mono/polyphony mode */
	Parm pb_range;		/* pitch bend */
	Parm pb_step;
	Parm port_mode;		/* portamento */
	Parm port_glis;
	Parm port_time;
	struct dx7slider sl_mod, sl_foot, sl_breath, sl_after;
};
