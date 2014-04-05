/*************************************************************************/
/*  audio_stream_resampled.cpp                                           */
/*************************************************************************/
/*                       This file is part of:                           */
/*                           GODOT ENGINE                                */
/*                    http://www.godotengine.org                         */
/*************************************************************************/
/* Copyright (c) 2007-2014 Juan Linietsky, Ariel Manzur.                 */
/*                                                                       */
/* Permission is hereby granted, free of charge, to any person obtaining */
/* a copy of this software and associated documentation files (the       */
/* "Software"), to deal in the Software without restriction, including   */
/* without limitation the rights to use, copy, modify, merge, publish,   */
/* distribute, sublicense, and/or sell copies of the Software, and to    */
/* permit persons to whom the Software is furnished to do so, subject to */
/* the following conditions:                                             */
/*                                                                       */
/* The above copyright notice and this permission notice shall be        */
/* included in all copies or substantial portions of the Software.       */
/*                                                                       */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,       */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF    */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.*/
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY  */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,  */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE     */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                */
/*************************************************************************/
#include "audio_stream_resampled.h"
#include "globals.h"
int AudioStreamResampled::get_channel_count() const {

	if (!rb)
		return 0;

	return channels;
}


template<int C>
void AudioStreamResampled::_resample(int32_t *p_dest,int p_todo,int32_t p_increment) {

	for (int i=0;i<p_todo;i++) {

		offset = (offset + p_increment)&(((1<<(rb_bits+MIX_FRAC_BITS))-1));
		uint32_t pos = offset >> MIX_FRAC_BITS;
		uint32_t frac = offset & MIX_FRAC_MASK;
#ifndef FAST_AUDIO
		ERR_FAIL_COND(pos>=rb_len);
#endif
		uint32_t pos_next = (pos+1)&rb_mask;
		//printf("rb pos %i\n",pos);

		// since this is a template with a known compile time value (C), conditionals go away when compiling.
		if (C==1) {

			int32_t v0 = rb[pos];
			int32_t v0n=rb[pos_next];
#ifndef FAST_AUDIO
			v0+=(v0n-v0)*(int32_t)frac >> MIX_FRAC_BITS;
#endif
			v0<<=16;
			p_dest[i]=v0;

		}
		if (C==2) {

			int32_t v0 = rb[(pos<<1)+0];
			int32_t v1 = rb[(pos<<1)+1];
			int32_t v0n=rb[(pos_next<<1)+0];
			int32_t v1n=rb[(pos_next<<1)+1];

#ifndef FAST_AUDIO
			v0+=(v0n-v0)*(int32_t)frac >> MIX_FRAC_BITS;
			v1+=(v1n-v1)*(int32_t)frac >> MIX_FRAC_BITS;
#endif
			v0<<=16;
			v1<<=16;
			p_dest[(i<<1)+0]=v0;
			p_dest[(i<<1)+1]=v1;

		}

		if (C==4) {

			int32_t v0 = rb[(pos<<2)+0];
			int32_t v1 = rb[(pos<<2)+1];
			int32_t v2 = rb[(pos<<2)+2];
			int32_t v3 = rb[(pos<<2)+3];
			int32_t v0n = rb[(pos_next<<2)+0];
			int32_t v1n=rb[(pos_next<<2)+1];
			int32_t v2n=rb[(pos_next<<2)+2];
			int32_t v3n=rb[(pos_next<<2)+3];

#ifndef FAST_AUDIO
			v0+=(v0n-v0)*(int32_t)frac >> MIX_FRAC_BITS;
			v1+=(v1n-v1)*(int32_t)frac >> MIX_FRAC_BITS;
			v2+=(v2n-v2)*(int32_t)frac >> MIX_FRAC_BITS;
			v3+=(v3n-v3)*(int32_t)frac >> MIX_FRAC_BITS;
#endif
			v0<<=16;
			v1<<=16;
			v2<<=16;
			v3<<=16;
			p_dest[(i<<2)+0]=v0;
			p_dest[(i<<2)+1]=v1;
			p_dest[(i<<2)+2]=v2;
			p_dest[(i<<2)+3]=v3;

		}

		if (C==6) {

			int32_t v0 = rb[(pos*6)+0];
			int32_t v1 = rb[(pos*6)+1];
			int32_t v2 = rb[(pos*6)+2];
			int32_t v3 = rb[(pos*6)+3];
			int32_t v4 = rb[(pos*6)+4];
			int32_t v5 = rb[(pos*6)+5];
			int32_t v0n = rb[(pos_next*6)+0];
			int32_t v1n=rb[(pos_next*6)+1];
			int32_t v2n=rb[(pos_next*6)+2];
			int32_t v3n=rb[(pos_next*6)+3];
			int32_t v4n=rb[(pos_next*6)+4];
			int32_t v5n=rb[(pos_next*6)+5];

#ifndef FAST_AUDIO
			v0+=(v0n-v0)*(int32_t)frac >> MIX_FRAC_BITS;
			v1+=(v1n-v1)*(int32_t)frac >> MIX_FRAC_BITS;
			v2+=(v2n-v2)*(int32_t)frac >> MIX_FRAC_BITS;
			v3+=(v3n-v3)*(int32_t)frac >> MIX_FRAC_BITS;
			v4+=(v4n-v4)*(int32_t)frac >> MIX_FRAC_BITS;
			v5+=(v5n-v5)*(int32_t)frac >> MIX_FRAC_BITS;
#endif
			v0<<=16;
			v1<<=16;
			v2<<=16;
			v3<<=16;
			v4<<=16;
			v5<<=16;
			p_dest[(i*6)+0]=v0;
			p_dest[(i*6)+1]=v1;
			p_dest[(i*6)+2]=v2;
			p_dest[(i*6)+3]=v3;
			p_dest[(i*6)+4]=v4;
			p_dest[(i*6)+5]=v5;

		}


	}


	rb_read_pos=offset>>MIX_FRAC_BITS;

}


bool AudioStreamResampled::mix(int32_t *p_dest, int p_frames) {


	if (!rb || !_can_mix())
		return false;

	int write_pos_cache=rb_write_pos;

	int32_t increment=(mix_rate*MIX_FRAC_LEN)/get_mix_rate();

	int rb_todo;

	if (write_pos_cache==rb_read_pos) {
		return false; //out of buffer

	} else if (rb_read_pos<write_pos_cache) {

		rb_todo=write_pos_cache-rb_read_pos-1;
	} else {

		rb_todo=(rb_len-rb_read_pos)+write_pos_cache-1;
	}

	int todo = MIN( ((int64_t(rb_todo)<<MIX_FRAC_BITS)/increment)+1, p_frames );
#if 0
	if (int(mix_rate)==get_mix_rate()) {


		if (channels==6) {

			for(int i=0;i<p_frames;i++) {

				int from = ((rb_read_pos+i)&rb_mask)*6;
				int to = i*6;

				p_dest[from+0]=int32_t(rb[to+0])<<16;
				p_dest[from+1]=int32_t(rb[to+1])<<16;
				p_dest[from+2]=int32_t(rb[to+2])<<16;
				p_dest[from+3]=int32_t(rb[to+3])<<16;
				p_dest[from+4]=int32_t(rb[to+4])<<16;
				p_dest[from+5]=int32_t(rb[to+5])<<16;
			}

		} else {
			int len=p_frames*channels;
			int from=rb_read_pos*channels;
			int mask=0;
			switch(channels) {
				case 1: mask=rb_len-1; break;
				case 2: mask=(rb_len*2)-1; break;
				case 4: mask=(rb_len*4)-1; break;
			}

			for(int i=0;i<len;i++) {

				p_dest[i]=int32_t(rb[(from+i)&mask])<<16;
			}
		}

		rb_read_pos = (rb_read_pos+p_frames)&rb_mask;
	} else
#endif
	{

		switch(channels) {
			case 1: _resample<1>(p_dest,todo,increment); break;
			case 2: _resample<2>(p_dest,todo,increment); break;
			case 4: _resample<4>(p_dest,todo,increment); break;
			case 6: _resample<6>(p_dest,todo,increment); break;
		}

	}

	return true;
}


Error AudioStreamResampled::_setup(int p_channels,int p_mix_rate,int p_minbuff_needed) {

	ERR_FAIL_COND_V(p_channels!=1 && p_channels!=2 && p_channels!=4 && p_channels!=6,ERR_INVALID_PARAMETER);


	float buffering_sec = int(GLOBAL_DEF("audio/stream_buffering_ms",500))/1000.0;
	int desired_rb_bits =nearest_shift(MAX(buffering_sec*p_mix_rate,p_minbuff_needed));

	bool recreate=!rb;

	if (rb && (uint32_t(desired_rb_bits)!=rb_bits || channels!=uint32_t(p_channels))) {
		//recreate

		memdelete_arr(rb);
		memdelete_arr(read_buf);
		recreate=true;

	}

	if (recreate) {

		channels=p_channels;
		rb_bits=desired_rb_bits;
		rb_len=(1<<rb_bits);
		rb_mask=rb_len-1;
		rb = memnew_arr( int16_t, rb_len * p_channels );
		read_buf = memnew_arr( int16_t, rb_len * p_channels );

	}

	mix_rate=p_mix_rate;
	offset=0;
	rb_read_pos=0;
	rb_write_pos=0;

	//avoid maybe strange noises upon load
	for (int i=0;i<(rb_len*channels);i++) {

		rb[i]=0;
		read_buf[i]=0;
	}

	return OK;

}

void AudioStreamResampled::_clear() {

	if (!rb)
		return;

	AudioServer::get_singleton()->lock();
	//should be stopped at this point but just in case
	if (rb) {
		memdelete_arr(rb);
		memdelete_arr(read_buf);
	}
	rb=NULL;
	offset=0;
	rb_read_pos=0;
	rb_write_pos=0;
	read_buf=NULL;
	AudioServer::get_singleton()->unlock();

}

AudioStreamResampled::AudioStreamResampled() {

	rb=NULL;
	offset=0;
	read_buf=NULL;
}

AudioStreamResampled::~AudioStreamResampled() {

	if (rb) {
		memdelete_arr(rb);
		memdelete_arr(read_buf);
	}

}
