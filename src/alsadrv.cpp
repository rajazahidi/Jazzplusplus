/*
**  The JAZZ++ Midi Sequencer
**
** Copyright (C) 1994-2000 Andreas Voss and Per Sigmond, all rights reserved.
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
**
** ALSA audio player for ALSA 0.5.x driver
** Copyright (c) 2000 Takashi Iwai <tiwai@suse.de>
**
** Mostly based on audiodrv.c.
**
** Aug. 11, 2000
**	Initial version: only playback is tested.
*/                                                                              

#include "wx.h"
#pragma hdrstop

#include "config.h"
#include "alsadrv.h"
#include "jazz.h"	// MIN and MAX
#include <unistd.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <errno.h>

#include "trackwin.h"

#if SND_LIB_MINOR >= 6
#define ALSA6
#endif

#ifdef ALSA6
#define snd_pcm_write(pcm,data,size) snd_pcm_writei(pcm,data,size)
#define snd_pcm_read(pcm,data,size) snd_pcm_readi(pcm,data,size)
#endif
#define MAX_FRAGS	16	// enough large?



class tAlsaAudioListener : public wxTimer
{
public:
	tAlsaAudioListener(tAlsaAudioPlayer *p, int key)
	{
		hard_exit = TRUE;
		player = p;
		player->listener = this;
		player->rec_info = 0;  // not recording!
		player->running_mode = 0;
		
		// SYNC seems not to work?? so add 8 more silent buffers
		// to hear the end of the sample too.
		player->OpenDsp(tAlsaAudioPlayer::PLAYBACK, 0);
		count = 8 + player->samples.PrepareListen(key);
		Start(20);
	}

	tAlsaAudioListener(tAlsaAudioPlayer *p, tSample &spl, long fr_smpl, long to_smpl) {
		hard_exit = TRUE;
		player = p;
		player->listener = this;
		player->rec_info = 0;  // not recording!
		player->running_mode = 0;

		player->OpenDsp(tAlsaAudioPlayer::PLAYBACK, 0);
		player->samples.ResetBufferSize(player->frag_byte_size[tAlsaAudioPlayer::PLAYBACK]);
		count = 8 + player->samples.PrepareListen(&spl, fr_smpl, to_smpl);
		Start(20);
	}

	~tAlsaAudioListener()
	{
		Stop();
		player->CloseDsp(hard_exit);
		player->listener = 0;
	}

	virtual void Notify()
	{
		count -= player->WriteSamples();
		count += player->samples.ContinueListen();
		if (count <= 0) {
			hard_exit = FALSE;
			delete this;
		}
	}

	long GetPlayPosition()
	{
		return player->GetCurrentPosition(tAlsaAudioPlayer::PLAYBACK);
	}

private:
	tAlsaAudioPlayer *player;
	int count;
	int hard_exit;
};


tAlsaAudioPlayer::tAlsaAudioPlayer(tSong *song)
	: tAlsaPlayer(song)
{
	AudioBuffer   = new tEventArray();
	installed     = 0;
	audio_enabled = 0;
	listener      = 0;
	can_duplex    = 0;    // no duplex yet.
	pcm[PLAYBACK] = NULL;
	pcm[CAPTURE] = NULL;

#ifdef ALSA6
	dev[PLAYBACK] = Config.StrValue(C_AlsaAudioOutputDevice);
	dev[CAPTURE] = Config.StrValue(C_AlsaAudioInputDevice);
	can_duplex = 1; /* FIXME */
	installed = 1;
	audio_enabled = 1;
#else
	card = Config(C_AlsaAudioCard);
	dev[PLAYBACK] = Config(C_AlsaAudioOutputDevice);
	dev[CAPTURE] = Config(C_AlsaAudioInputDevice);

	snd_ctl_t *handle;
	snd_pcm_info_t pcminfo;

	// check for device
	if (snd_ctl_open(&handle, card) < 0) {
		perror("snd_ctl_open");
		return;
	}
	memset(&pcminfo, 0, sizeof(pcminfo));
	if (snd_ctl_pcm_info(handle, dev[CAPTURE], &pcminfo) < 0) {
		perror("snd_ctl_pcm_info (input)");
		return;
	}
	if (snd_ctl_pcm_info(handle, dev[PLAYBACK], &pcminfo) < 0) {
		perror("snd_ctl_pcm_info (output)");
		return;
	}
	if (dev[PLAYBACK] == dev[CAPTURE]
	    && (pcminfo.flags & SND_PCM_INFO_DUPLEX))
		can_duplex = 1;	// good soundcard!
	snd_ctl_close(handle);
	installed = 1;
	audio_enabled = 1;
#endif
}


tAlsaAudioPlayer::~tAlsaAudioPlayer()
{
	delete listener;
	delete AudioBuffer;
	if (pcm[PLAYBACK]) {
		snd_pcm_close(pcm[PLAYBACK]);
		pcm[PLAYBACK] = NULL;
	}
	if (pcm[CAPTURE]) {
		snd_pcm_close(pcm[CAPTURE]);
		pcm[CAPTURE] = NULL;
	}
}


int tAlsaAudioPlayer::LoadSamples(const char *filename)
{
	return samples.Load(filename);
}

int tAlsaAudioPlayer::RecordMode() const
{
	return running_mode & (1 << CAPTURE);
}

int tAlsaAudioPlayer::PlayBackMode() const
{
	return running_mode & (1 << PLAYBACK);
}

void tAlsaAudioPlayer::StartPlay(long clock, long loopClock, int cont)
{
	delete listener;
	samples.StartPlay(clock);

	tAlsaPlayer::StartPlay(clock, loopClock, cont);
	if (!audio_enabled)
		return;

	long ticks_per_minute = Song->TicksPerQuarter * Song->Speed();
	samples.ResetBuffers(AudioBuffer, clock, ticks_per_minute);
	last_scount = 0;
	cur_pos = 0;
	audio_clock_offset = clock;
	midi_speed  = Song->Speed();
	curr_speed  = midi_speed;

	running_mode = 0;
	if (rec_info && rec_info->Track->GetAudioMode()) {
		OpenDsp(CAPTURE, 1);
		recbuffers.ResetBufferSize(frag_byte_size[CAPTURE]);
	}
	if (dev[CAPTURE] != dev[PLAYBACK]
	    || can_duplex || running_mode == 0) {
		OpenDsp(PLAYBACK, 1);
		samples.ResetBufferSize(frag_byte_size[PLAYBACK]);
		samples.FillBuffers(OutClock);
	}
	if (running_mode == 0) {
		audio_enabled = 0;
		return;
	}
	
	// ok, suspend the device until midi starts
	if (PlayBackMode())
		WriteSamples();
	compose_echo(clock, 1); // trigger echo
}


void tAlsaAudioPlayer::StartAudio()
{
#ifdef ALSA6
	if (pcm[PLAYBACK])
		snd_pcm_start(pcm[PLAYBACK]);
	if (pcm[CAPTURE])
		snd_pcm_start(pcm[CAPTURE]);
#else
	if (pcm[PLAYBACK])
		snd_pcm_playback_go(pcm[PLAYBACK]);
	if (pcm[CAPTURE])
		snd_pcm_capture_go(pcm[CAPTURE]);
#endif
}


void tAlsaAudioPlayer::OpenDsp(int mode, int sync_mode)
{
	int tmp;

	if (!audio_enabled)
		return;

#ifdef ALSA6
	unsigned int channels;
	snd_pcm_format_t format;
	int buffer_size, period_size;

	frame_shift[mode] = 0;
	if (samples.BitsPerSample() == 8)
		format = SND_PCM_FORMAT_U8;
	else {
		format = SND_PCM_FORMAT_S16_LE;
		frame_shift[mode]++;
	}
	channels =  samples.GetChannels();
	if (channels > 1)
		frame_shift[mode]++;

	snd_pcm_stream_t stream = (mode == PLAYBACK) ?
		SND_PCM_STREAM_PLAYBACK : SND_PCM_STREAM_CAPTURE;
	if (snd_pcm_open(&pcm[mode], dev[mode], stream, SND_PCM_NONBLOCK) < 0) { 
		perror("snd_pcm_open");
		audio_enabled = 0;
		return;
	}

	snd_pcm_hw_params_t *hw;
	snd_pcm_hw_params_alloca(&hw);
	snd_pcm_hw_params_any(pcm[mode], hw);
	if (snd_pcm_hw_params_set_access(pcm[mode], hw, SND_PCM_ACCESS_RW_INTERLEAVED) < 0) {
		perror("cannot set interleaved access");
		goto __error;
	}
	if (snd_pcm_hw_params_set_format(pcm[mode], hw, format) < 0) {
		perror("cannot set audio format");
		goto __error;
	}
	if (snd_pcm_hw_params_set_channels(pcm[mode], hw, channels) < 0) {
		perror("cannot set audio channels");
		goto __error;
	}
	if (snd_pcm_hw_params_set_rate(pcm[mode], hw, samples.GetSpeed(), 0) < 0) {
		cerr  << "cannot set audio rate: " << samples.GetSpeed() << endl;
		goto __error;
	}

	period_size = FRAGBYTES >> frame_shift[mode];
	if ((period_size = snd_pcm_hw_params_set_period_size_near(pcm[mode], hw, period_size, 0)) < 0) {
		perror("cannot set audio period");
		goto __error;
	}
	buffer_size = period_size * MAX_FRAGS;
	if ((buffer_size = snd_pcm_hw_params_set_buffer_size_near(pcm[mode], hw, buffer_size)) < 0) {
		perror("cannot set audio buffer");
		goto __error;
	}
	if (snd_pcm_hw_params(pcm[mode], hw) < 0) {
		perror("snd_pcm_hw_params");
		goto __error;
	}

	frag_size[mode] = period_size; /* in frames */
	frag_byte_size[mode] = period_size << frame_shift[mode];
	frame_boundary[mode] = 0x7fffffff;

	snd_pcm_sw_params_t *sw;

	snd_pcm_sw_params_alloca(&sw);
	snd_pcm_sw_params_current(pcm[mode], sw);
	if (sync_mode)
		snd_pcm_sw_params_set_start_threshold(pcm[mode], sw, 0x7fffffff); /* FIXME */
	else
		snd_pcm_sw_params_set_start_threshold(pcm[mode], sw, 1);
	if (snd_pcm_sw_params(pcm[mode], sw) < 0) {
		perror("snd_pcm_sw_params");
		goto __error;
	}

#else
	unsigned int format, channels;

	frame_shift[mode] = 0;
	if (samples.BitsPerSample() == 8)
		format = SND_PCM_SFMT_U8;
	else {
		format = SND_PCM_SFMT_S16_LE;
		frame_shift[mode]++;
	}
	channels =  samples.GetChannels();
	if (channels > 1)
		frame_shift[mode]++;

	int direction = (mode == PLAYBACK) ?
		SND_PCM_OPEN_PLAYBACK : SND_PCM_OPEN_CAPTURE;
	direction |= SND_PCM_OPEN_NONBLOCK;
	if (snd_pcm_open(&pcm[mode], card, dev[mode], direction) < 0) {
		perror("snd_pcm_open");
		audio_enabled = 0;
		return;
	}

	snd_pcm_channel_params_t params;
	memset(&params, 0, sizeof(params));
	params.mode = SND_PCM_MODE_BLOCK;
	params.channel = mode;
	params.format.interleave = 1;
	params.format.format = format;
	params.format.voices =  channels;
	params.format.rate = samples.GetSpeed();

	// start/stop mode
	if (sync_mode)
		params.start_mode = SND_PCM_START_GO;
	else
		params.start_mode = SND_PCM_START_DATA;
	params.stop_mode = SND_PCM_STOP_STOP;

	params.buf.block.frag_size = FRAGBYTES;
	params.buf.block.frags_min = 1;
	params.buf.block.frags_max = MAX_FRAGS; // -1 ??

	if (snd_pcm_channel_params(pcm[mode], &params) < 0) {
		perror("snd_pcm_params");
		goto __error;
	}

	if (snd_pcm_channel_prepare(pcm[mode], mode) < 0) {
		perror("snd_pcm_prepare");
		goto __error;
	}

	// check if fragsize was ok
	snd_pcm_channel_setup_t setup;
	memset(&setup, 0, sizeof(setup));
	setup.mode = SND_PCM_MODE_BLOCK;
	setup.channel = mode;
	if (snd_pcm_channel_setup(pcm[mode], &setup)< 0) {
		perror("snd_pcm_setup");
		goto __error;
	}
	frag_size[mode] = setup.buf.block.frag_size; // in byte size
	frag_byte_size[mode] = frag_size[mode];
	frame_boundary[mode] = SND_PCM_BOUNDARY >> frame_shift[mode];
#endif

	running_mode |= (1 << mode);

	return;

__error:
	snd_pcm_close(pcm[mode]);
	pcm[mode] = NULL;
	audio_enabled = 0;
	return;
}


void tAlsaAudioPlayer::CloseDsp(int reset)
{
	if (pcm) {
#ifdef ALSA6
		if (reset) {
			if (pcm[PLAYBACK]) {
				if (snd_pcm_drop(pcm[PLAYBACK]) < 0)
					perror("playback drop");
			}
		} else {
			if (pcm[PLAYBACK]) {
				if (snd_pcm_drain(pcm[PLAYBACK]) < 0 )
					perror("playback drain");
			}
			if (pcm[CAPTURE]) {
				if (snd_pcm_drain(pcm[CAPTURE]) < 0 )
					perror("capture drain");
			}
		}
#else
		if (reset) {
			if (pcm[PLAYBACK]) {
				if (snd_pcm_playback_drain(pcm[PLAYBACK]) < 0)
					perror("playback_drain");
			}
		} else {
			if (pcm[PLAYBACK]) {
				if (snd_pcm_channel_flush(pcm[PLAYBACK], SND_PCM_CHANNEL_PLAYBACK) < 0 )
					perror("playback_flush");
			}
			if (pcm[CAPTURE]) {
				if (snd_pcm_channel_flush(pcm[CAPTURE], SND_PCM_CHANNEL_CAPTURE) < 0 )
					perror("capture_flush");
			}
		}
#endif
		if (pcm[PLAYBACK]) {
			snd_pcm_close(pcm[PLAYBACK]);
			pcm[PLAYBACK] = NULL;
		} 
		if (pcm[CAPTURE]) {
			snd_pcm_close(pcm[CAPTURE]);
			pcm[CAPTURE] = NULL;
		} 
	}
}

void tAlsaAudioPlayer::Notify()
{
	if (audio_enabled) {
		if (pcm[PLAYBACK]) {
			WriteSamples();
			// here it may hang when swapping in pages
			samples.FillBuffers(OutClock);
			WriteSamples();
		}
		if (pcm[CAPTURE])
			ReadSamples();

		if (pcm[PLAYBACK] && samples.softsync)
			MidiSync();
	}
	tAlsaPlayer::Notify();
}

// number of frames (or bytes) free
int tAlsaAudioPlayer::GetFreeSpace(int mode)
{
#ifdef ALSA6
	snd_pcm_status_t *info;
	snd_pcm_status_alloca(&info);
	if (snd_pcm_status(pcm[mode], info) < 0) {
		perror("snd_pcm_status");
		return 0;
	}
	return snd_pcm_status_get_avail(info); /* in frames */
#else
	snd_pcm_channel_status_t info;
	memset(&info, 0, sizeof(info));
	info.mode = SND_PCM_MODE_BLOCK;
	info.channel = mode == PLAYBACK ? SND_PCM_CHANNEL_PLAYBACK :
		SND_PCM_CHANNEL_CAPTURE;
	if (snd_pcm_channel_status(pcm[mode], &info) < 0) {
		perror("snd_pcm_status");
		return 0;
	}
	return info.free; /* in bytes */
#endif
}


int tAlsaAudioPlayer::WriteSamples()
{
	if (!audio_enabled || pcm[PLAYBACK] == NULL)
		return 0;

	int blocks_written = 0;
	int room;

	room = GetFreeSpace(PLAYBACK);

	for (; room > frag_size[PLAYBACK]; room -= frag_size[PLAYBACK]) {
		tAudioBuffer *buf = samples.full_buffers.Get();
		if (buf == 0)
			break;
		ssize_t written = snd_pcm_writei(pcm[PLAYBACK], buf->Data(), frag_size[PLAYBACK]);
		if (written < 0) {
			if (written == -EPIPE) {
				cerr << "xrun!!" << endl;
#ifdef ALSA6
				snd_pcm_prepare(pcm[PLAYBACK]);
#else
				snd_pcm_channel_prepare(pcm[PLAYBACK], PLAYBACK);
#endif
			} else {
				perror("audio write");
			}
		}
		if (written > 0)
			cur_scount += written;
		blocks_written++;
		samples.free_buffers.Put(buf);
	}

	return blocks_written;
}


void tAlsaAudioPlayer::ReadSamples()
{
	if (!audio_enabled || pcm[CAPTURE] == NULL)
		return;

	int room = GetFreeSpace(CAPTURE);

	for (; room > frag_size[CAPTURE]; room -= frag_size[CAPTURE]) {
		short *b = recbuffers.RequestBuffer()->data;
		if (snd_pcm_read(pcm[CAPTURE], b, frag_size[CAPTURE]) !=
		    frag_size[CAPTURE]) {
			recbuffers.UndoRequest();
			break;
		}
	}
}


void tAlsaAudioPlayer::ResetPlay(long clock)
{
	tAlsaPlayer::ResetPlay(clock);
	if (pcm[PLAYBACK]) {
#ifdef ALSA6
		snd_pcm_drop(pcm[PLAYBACK]);
#else
		snd_pcm_playback_drain(pcm[PLAYBACK]);
#endif
		long ticks_per_minute = Song->TicksPerQuarter * Song->Speed();
		//samples.ResetBuffers(AudioBuffer, clock, ticks_per_minute);
	}
	audio_clock_offset = clock;
	cur_pos = 0;
}

long tAlsaAudioPlayer::GetCurrentPosition(int mode)
{
#ifdef ALSA6
	return cur_scount;
#else
	snd_pcm_channel_status_t info;
	memset(&info, 0, sizeof(info));
	info.mode = SND_PCM_MODE_BLOCK;
	info.channel = mode == PLAYBACK ? SND_PCM_CHANNEL_PLAYBACK :
		SND_PCM_CHANNEL_CAPTURE;
	if (snd_pcm_channel_status(pcm[mode], &info) < 0) {
		perror("snd_pcm_status");
		return -1L;
	}
	return info.scount >> frame_shift[mode];
#endif
}

void tAlsaAudioPlayer::MidiSync()
{
	if (!audio_enabled)
		return;

	int mode;
	if (pcm[PLAYBACK])
		mode = PLAYBACK;
	else if (pcm[CAPTURE])
		mode = CAPTURE;
	else
		return; // disabled

	long scount = GetCurrentPosition(mode);

	// get realtime info for audio/midi sync
	if (scount != last_scount) {
		unsigned int qtick;
#ifdef ALSA6
		snd_seq_queue_status_t *status;
		snd_seq_queue_status_alloca(&status);
		if (snd_seq_get_queue_status(handle, queue, status) < 0) {
			perror("snd_seq_get_queue_status");
			return;
		}
		qtick = snd_seq_queue_status_get_tick_time(status);
#else
		snd_seq_queue_status_t status;
		if (snd_seq_get_queue_status(handle, queue, &status) < 0) {
			perror("snd_seq_get_queue_status");
			return;
		}
		qtick = status.tick;
#endif
		int samplediff;
		if (scount < last_scount)
			samplediff = frame_boundary[mode] - (last_scount - scount);
		else
			samplediff = scount - last_scount;
		last_scount = scount;
		cur_pos += samplediff;
		long audio_clock = (long)samples.Samples2Ticks(cur_pos) + audio_clock_offset;
		int delta_clock = audio_clock - qtick;
		int new_speed = midi_speed + delta_clock;
		// limit speed changes to some reasonable values
		const int limit = 1;
		new_speed = MIN(new_speed, midi_speed + limit);
		new_speed = MAX(new_speed, midi_speed - limit);

		if (new_speed != curr_speed) {
			snd_seq_event_t ev;
			memset(&ev, 0, sizeof(ev));
			snd_seq_ev_set_source(&ev, self.port);
			snd_seq_ev_set_subs(&ev);
			snd_seq_ev_set_direct(&ev);
			snd_seq_ev_set_fixed(&ev);
			int us  = (int)( 60.0E6 / (double)new_speed );
			snd_seq_ev_set_queue_tempo(&ev, queue, us);
			write(&ev, 1);
			curr_speed = new_speed;
			// xview has reentrancy problems!!
			// TrackWin->DrawSpeed(curr_speed);
		}
	}
}

void tAlsaAudioPlayer::StopPlay()
{
	samples.StopPlay();
	tAlsaPlayer::StopPlay();
	if (!audio_enabled)
		return;

	CloseDsp(TRUE);
	if (RecordMode()) {
		long frc = rec_info->FromClock;
		if (frc < audio_clock_offset)
			frc = audio_clock_offset;
		long toc = rec_info->ToClock;
		if (toc > recd_clock)
			toc = recd_clock;
		samples.SaveRecordingDlg(frc, toc, recbuffers);
	}
	recbuffers.Clear();
	// xview has reentrancy problems!!
	// TrackWin->DrawSpeed(midi_speed);
}



void tAlsaAudioPlayer::ListenAudio(int key, int start_stop_mode)
{
	if (!audio_enabled)
		return;

	// when already listening then stop listening
	if (listener) {
		delete listener;
		listener = 0;
		if (start_stop_mode)
			return;
	}
	if (key < 0)
		return;

	if (pcm[PLAYBACK])  // device busy (playing)
		return;
	listener = new tAlsaAudioListener(this, key);
}

void tAlsaAudioPlayer::ListenAudio(tSample &spl, long fr_smpl, long to_smpl)
{
	if (!audio_enabled)
		return;

	// when already listening then stop listening
	if (listener) {
		delete listener;
		listener = 0;
	}
	if (pcm[PLAYBACK])  // device busy (playing)
		return;
	listener = new tAlsaAudioListener(this, spl, fr_smpl, to_smpl);
}

long tAlsaAudioPlayer::GetListenerPlayPosition()
{
	if (!listener)
		return -1L;
	return listener->GetPlayPosition();
}
