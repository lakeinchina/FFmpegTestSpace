package me.lake.decoder_any2pcm;

import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioTrack;
import android.util.Log;

/**
 * Created by lake on 29/11/16.
 * Decoder_any2pcm project.
 */
public class PcmPlayer {
    private AudioTrack trackplayer;

    PcmPlayer() {

    }

    public void prepare() {
        int bufsize = AudioTrack.getMinBufferSize(44100, AudioFormat.CHANNEL_OUT_MONO,
                AudioFormat.ENCODING_PCM_16BIT);

        trackplayer = new AudioTrack(AudioManager.STREAM_MUSIC,
                44100,
                AudioFormat.CHANNEL_OUT_MONO,
                AudioFormat.ENCODING_PCM_16BIT,
                bufsize,
                AudioTrack.MODE_STREAM);
    }

    public void write(byte[] data) {
        trackplayer.write(data, 0, data.length);
    }

    public void start() {
        trackplayer.play();
    }

    public void stop() {
        trackplayer.stop();
    }

    public void destroy() {
        trackplayer.release();
    }
}
